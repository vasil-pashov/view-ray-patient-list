#include "websocket.h"

namespace ViewRay {
	WSConnectionManager::WSConnectionManager() :
		nextMetadataID(0) {
	}

	void WSConnectionManager::init() {
		endpoint.clear_access_channels(websocketpp::log::alevel::all);
		endpoint.clear_error_channels(websocketpp::log::elevel::all);

		endpoint.init_asio();
		endpoint.start_perpetual();

		thread.reset(new websocketpp::lib::thread(&Client::run, &endpoint));
	}

	WSConnectionManager ::~WSConnectionManager() {
		websocketpp::lib::error_code ec;
		for (auto it : metadata) {
			if (it.second->getStatus() == Metadata::Status::Opened) {
				endpoint.close(
					it.second->getHandle(), websocketpp::close::status::going_away, "", ec
				);
			}
		}
		endpoint.stop_perpetual();
		if (thread->joinable()) {
			thread->join();
		}
	}

	EC::ErrorCode WSConnectionManager::close(
		int id,
		websocketpp::close::status::value code,
		const std::string& reason
	) {
		auto metadataIt = metadata.find(id);
		if (metadataIt == metadata.end()) {
			return EC::ErrorCode(ConnectionNotFound, "No connection found with id: %d", id);
		}

		websocketpp::lib::error_code ec;
		endpoint.close(metadataIt->second->getHandle(), code, reason, ec);
		if (ec) {
			return EC::ErrorCode(CannotCloseConnection, "Error initiating close: %s", ec.message());
		}

		return EC::ErrorCode();
	}

	EC::ErrorCode WSConnectionManager::close(
		websocketpp::connection_hdl hdl,
		websocketpp::close::status::value code,
		const std::string& reason
	) {
		websocketpp::lib::error_code ec;
		endpoint.close(hdl, code, reason, ec);
		if (ec) {
			return EC::ErrorCode(CannotCloseConnection, "Error initiating close: %s", ec.message());
		}

		return EC::ErrorCode();
	}

	EC::ErrorCode WSConnectionManager::send(int id, const std::string& message) {
		websocketpp::lib::error_code ec;

		auto metadataIt = metadata.find(id);
		if (metadataIt == metadata.end()) {
			return EC::ErrorCode(ConnectionNotFound, "No connection found with id: %d", id);
		}

		endpoint.send(
			metadataIt->second->getHandle(), message, websocketpp::frame::opcode::text, ec
		);
		if (ec) {
			return EC::ErrorCode(
				CannotSendMessage, "Error sending message: %s", ec.message().c_str()
			);
		}

		return EC::ErrorCode();
	}

	EC::ErrorCode WSConnectionManager::send(
		websocketpp::connection_hdl handle,
		const std::string& message
	) {
		websocketpp::lib::error_code ec;
		endpoint.send(handle, message, websocketpp::frame::opcode::text, ec);
		if (ec) {
			return EC::ErrorCode(
				CannotSendMessage, "Error sending message: %s", ec.message().c_str()
			);
		}

		return EC::ErrorCode();
	}

	WSConnectionManager::Metadata::Ptr WSConnectionManager::getMetadata(int id) {
		return metadata[id];
	}
}  // namespace ViewRay