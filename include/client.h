#pragma once
#include "patient_data.h"
#include "websocket.h"
#include <unordered_map>

namespace ViewRay {

	/// @brief Class used to retrieve data from ViewRay server
	class ViewRayClient {
	public:
		using PatientListPtr = std::shared_ptr<std::unordered_map<std::string, Patient>>;
		using PatientList = std::unordered_map<std::string, Patient>;

		/// @brief Initialize the client without establishing a connection
		/// Call ViewRayClient::init to establish a connection. It must be called
		/// before any requests are made.
		/// @param[in] address The address of the server
		ViewRayClient(std::string address);

		EC::ErrorCode init() {
			endpoint.init();
			return EC::ErrorCode();
		}

		/// @brief Async call to retrieve a patient list
		/// @return Future which will contain the patient list
		std::future<WSAsyncResult<PatientListPtr>> getPatientList();

	private:
		/// Address of the server
		std::string address;
		/// Websocket manager which manages the connection to the server
		WSConnectionManager endpoint;
	};
};	// namespace ViewRay