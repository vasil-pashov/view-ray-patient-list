#include "client.h"
#include "nlohmann/json.hpp"
#include "websocket.h"

namespace ViewRay {

	class PatientDataConn : public WebsocketConnectionMetadata<WSConnectionManager::Client> {
	public:
		PatientDataConn(int id, websocketpp::connection_hdl hdl, std::string uri) :
			WebsocketConnectionMetadata<WSConnectionManager::Client>(id, hdl, uri),
			patientsToExpand(0),
			patientList(std::make_shared<PatientList>()) {
		}
		using PatientList = std::unordered_map<std::string, Patient>;
		using PatientListPtr = std::shared_ptr<std::unordered_map<std::string, Patient>>;

		void onMessage(
			ClientT* client,
			websocketpp::connection_hdl hdl,
			typename ClientT::message_ptr msg
		) override {
			// This callback parses public:patients URI and recursively requests each
			// patient URI.
			nlohmann::json json = nlohmann::json::parse(msg->get_payload())["updateSubscriptions"];
			auto dataIt = json.find("public:patients");
			if (dataIt != json.end()) {
				// Parses patient list response to:
				// {updateSubscriptions: {"public:patients": "request"}}
				assert((*dataIt)["type"] == std::string("PatientList"));
				const size_t patientsCount = dataIt.value()["value"].size();
				for (const nlohmann::json& json : dataIt.value()["value"]) {
					const std::string& uri = json["uri"];
					patientList->emplace(uri, Patient(json));
				}
				patientsToExpand = patientList->size();
				// For some reason sending more than one URI in the same request e.g.
				// {"setSubscriptions": {"public:patients/1_2897763/root": "request",
				// "public:patients/0_1930886/root":"request"}} does not work and returns info only
				// for the first entry. Thus we send them one by one.				
				for (const nlohmann::json& json : dataIt.value()["value"]) {
					websocketpp::lib::error_code ec;
					const std::string& uri = json["uri"];
					const nlohmann::json request = {{"setSubscriptions", {{uri, "request"}}}};
					client->send(hdl, request.dump(), websocketpp::frame::opcode::text, ec);
				}
			} else {
				// Parses patient URI response to:
				// {setSubscriptions: {<patient_uri>: "request"}}
				for (auto it = json.begin(); it != json.end(); ++it) {
					assert((*it)["type"] == std::string("Patient"));
					const std::string& patientUri = it.key();
					const auto patientIt = patientList->find(patientUri);
					if (patientIt != patientList->end()) {
						patientIt->second.diagnosesFromJson((*it)["diagnoses"]);
					}
					patientsToExpand--;
				}

				if (patientsToExpand == 0) {
					client->close(hdl, websocketpp::close::status::normal, "");
					promise.set_value(WSAsyncResult<PatientListPtr>(patientList));
				}
			}
		}

		std::future<WSAsyncResult<PatientListPtr>> getFuture() {
			return std::move(promise.get_future());
		}

	private:
		std::promise<WSAsyncResult<PatientListPtr>> promise;
		PatientListPtr patientList;
		/// Counts how much responds to {"setSubscriptions": {<patient_uri>: "request"}}
		/// we have received. We will set one per customer in patient:list. This is initialized
		/// in the PatientDataConn::onMessage function and decremented again in it. onMessage is
		/// called on the same thread (different than the main) which handles connections. We do
		/// not need atomic in such case.
		int patientsToExpand;
	};

	ViewRayClient::ViewRayClient(std::string address) :
		address(std::move(address)) {
	}

	std::future<WSAsyncResult<ViewRayClient::PatientListPtr>> ViewRayClient::getPatientList() {
		// TODO this opens a new connection for each request (and closes it when the list is retrieved)
		// Not sure if this is the best approach. Maybe it's better to keep the connection open
		// but then we have a problem with multiple calls to getPatientList.
		std::future<WSAsyncResult<int>> connFuture = endpoint.connect<PatientDataConn>(address);
		WSAsyncResult<int> connID = connFuture.get();
		if (connID.hasError()) {
			std::promise<WSAsyncResult<PatientListPtr>> p;
			p.set_value(WSAsyncResult<PatientListPtr>(connID.getError()));
			return p.get_future();
		} else {
			const nlohmann::json request({{"setSubscriptions", {{"public:patients", "request"}}}});
			endpoint.send(connID.getData(), request.dump());
			WSConnectionManager::Metadata::Ptr metadata = endpoint.getMetadata(connID.getData());
			return static_cast<PatientDataConn*>(metadata.get())->getFuture();
		}
	}
}  // namespace ViewRay