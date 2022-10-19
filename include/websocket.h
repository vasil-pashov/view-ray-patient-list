#pragma once
#include "error_code.h"
#include <websocketpp/client.hpp>
#include <websocketpp/common/memory.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <future>
#include <unordered_map>
#include <variant>

namespace ViewRay {
	/// @brief Wraps a result from async request.
	///
	/// This is supposed to be used as the value type of future/promise.
	/// It can hold one of two things an error or the value of type T
	/// @tparam T The type of the value which this can hold
	template <typename T>
	class WSAsyncResult {
	public:
		WSAsyncResult() = default;
		/// @brief Initialize with value
		/// @param[t] t The value to be stored
		explicit WSAsyncResult(T t) :
			data(std::move(t)) {
		}
		/// @brief Initialize with error
		/// @param[in] err The error to store
		explicit WSAsyncResult(EC::ErrorCode err) :
			data(std::move(err)) {
		}

		/// @brief Check if there is an error.
		/// @return true if this holds an error
		bool hasError() const {
			return std::holds_alternative<EC::ErrorCode>(data);
		}
		/// @brief Retrieve the data.
		///
		/// This is safe to be called only if WSAsyncResult::hasError returns false
		/// @return The data
		T getData() const {
			return std::get<T>(data);
		}

		/// @brief Retrieve the error
		///
		/// This is safe to be called only if WSAsyncResult::hasError returns true
		/// @return The error
		const EC::ErrorCode& getError() const {
			return std::get<EC::ErrorCode>(data);
		}

	private:
		std::variant<T, EC::ErrorCode> data;
	};

	/// @brief Represents a websocket connection managed by WebsocketEndpoint
	///
	/// This class wraps websocketpp (https://github.com/zaphoyd/websocketpp) functionality
	/// The implementation follows closely this tutorial:
	/// https://docs.websocketpp.org/md_tutorials_utility_client_utility_client.html
	/// @tparam Client The type of the WS client used by WebsocketEndpoint
	///
	/// @todo Maybe it will be cleaner if all on* functions are pure virtual. Currently
	/// only onMessage can vary based on the purpose of the connection
	template <typename Client>
	class WebsocketConnectionMetadata {
	public:
		friend class WebsocketEndpoint;
		using Ptr = websocketpp::lib::shared_ptr<WebsocketConnectionMetadata<Client>>;
		using ClientT = Client;

		enum class Status {
			Connecting,
			Opened,
			Closed,
			Failed
		};

		/// @brief Store metadata (state) for the connection represented by hdl param
		/// @param id The id inside the WebsocketEndpoint which created this connection
		/// @param hdl Handle to the actual connection
		/// @param uri Address of the other end of the connection
		WebsocketConnectionMetadata(int id, websocketpp::connection_hdl hdl, std::string uri) :
			id(id),
			server("N/A"),
			uri(uri),
			connectionHandle(hdl),
			status(Status::Connecting) {
		}

		/// @brief Retrieve a string representation of the last error which happed with the
		/// connection
		const std::string& getError() const {
			return error;
		}

		/// @brief Name of the server to which the connection was made
		const std::string& getServer() const {
			return server;
		}

		/// @brief Get the other end of the connection
		const std::string& getUri() const {
			return uri;
		}

		Status getStatus() const {
			return status;
		}

		/// @brief Retrieve the id of the connection inside WebsocketEndpoint which created it
		int getID() const {
			return id;
		}

		/// @brief Get a handle representing the connection
		websocketpp::connection_hdl getHandle() const {
			return connectionHandle;
		}

		/// @brief Callback called when a connection is opened
		/// @param[in] client The websocket client used by WebsocketEndpoint which spawned the
		/// connection
		/// @param[in] promise A Promise passed by the one created the connection. This is used
		/// because
		///		connections can be created in async fashion. The one who created the connection must
		///		be able to know when it was opened. The value of the promise will be set with the id
		///		of the connection inside the WebsocketEndpoint which created it
		/// @param[in] hdl Handle representing the connection
		void onOpen(
			Client* client,
			std::shared_ptr<std::promise<WSAsyncResult<int>>> promise,
			websocketpp::connection_hdl hdl
		) {
			status = Status::Opened;

			typename Client::connection_ptr con = client->get_con_from_hdl(hdl);
			server = con->get_response_header("Server");
			std::cout << "Connection to: " << server << " opened\n";
			promise->set_value(WSAsyncResult<int>(id));
		}

		/// @brief Callback called when a connection fails to open.
		/// @param[in] client The websocket client used by WebsocketEndpoint which spawned the
		/// connection
		/// @param[in] promise A Promise passed by the one created the connection. This is used
		/// because
		///		connections can be created in async fashion. The one who created the connection must
		///		be able to know when it was opened. The value of the promise will be set with the
		/// error 		which has occurred.
		/// @param[in] hdl Handle representing the connection
		void onFail(
			Client* client,
			std::shared_ptr<std::promise<WSAsyncResult<int>>> promise,
			websocketpp::connection_hdl hdl
		) {
			status = Status::Failed;

			typename Client::connection_ptr connection = client->get_con_from_hdl(hdl);
			server = connection->get_response_header("Server");
			error = connection->get_ec().message();
			promise->set_value(WSAsyncResult<int>(EC::ErrorCode("%s", error.c_str())));
		}

		/// @brief Callback called when a connection is closed
		/// @param[in] client The websocket client used by WebsocketEndpoint which spawned the
		/// connection
		/// @param[in] hdl Handle representing the connection
		void onClose(Client* client, websocketpp::connection_hdl hdl) {
			status = Status::Closed;

			typename Client::connection_ptr connection = client->get_con_from_hdl(hdl);
			std::stringstream s;
			s << "close code: " << connection->get_remote_close_code() << " ("
			  << websocketpp::close::status::get_string(connection->get_remote_close_code())
			  << "), close reason: " << connection->get_remote_close_reason();
			error = s.str();
			std::cout << "Connection to: " << server << " closed\n";
		}

		virtual void onMessage(
			Client* client,
			websocketpp::connection_hdl hdl,
			typename Client::message_ptr msg
		) = 0;

	private:
		std::string error;
		std::string server;
		std::string uri;
		websocketpp::connection_hdl connectionHandle;
		Status status;
		int id;
	};

	/// @brief Class to manage the lifetime of connections to a specific server
	///
	/// Follows closely websocketpp tutorial:
	/// https://docs.websocketpp.org/md_tutorials_utility_client_utility_client.html
	/// This class handles connections (create/send/close) in async fashion.
	class WSConnectionManager {
	public:
		enum ErrorCode {
			Success = 0,
			CannotConnect,
			ConnectionNotFound,
			CannotCloseConnection,
			CannotSendMessage
		};

		using Client = websocketpp::client<websocketpp::config::asio_client>;
		using Metadata = WebsocketConnectionMetadata<Client>;

		WSConnectionManager();

		~WSConnectionManager();
		/// @brief Setup the main loop which waits for new commands
		/// This must be called before any other function
		void init();

		/// @brief (Async) Connect to a specific URI
		/// @tparam The metadata which will handle this connection. Its onMessage function will be
		///		called when messages arrive
		/// @param[in] uri Where to connect to
		/// @param[in] onMessage Function to be called each time we receive data on this connection
		/// @return Future containing the connection ID.
		template <typename MetadataT>
		std::future<WSAsyncResult<int>> connect(std::string const& uri) {
			websocketpp::lib::error_code ec;

			// Create the connection object. This does not initiate connection, yet.
			Client::connection_ptr connection = endpoint.get_connection(uri, ec);

			// Check for errors
			if (ec) {
				std::promise<WSAsyncResult<int>> promise;
				promise.set_value(WSAsyncResult<int>(
					EC::ErrorCode(ErrorCode::CannotConnect, "%s\n", ec.message())
				));
				return promise.get_future();
			}

			// We always create and save the metadata for the connection. This way
			// we keep failed connections too. That's not too bad for small apps and
			// simplifies the logic a bit.
			int newID = nextMetadataID++;
			typename MetadataT::Ptr metadataPtr(new MetadataT(newID, connection->get_handle(), uri));
			metadata[newID] = metadataPtr;

			// std::function cannot have non-copyable objects as params
			std::shared_ptr<std::promise<WSAsyncResult<int>>>
				promisePtr(new std::promise<WSAsyncResult<int>>);

			// Setup handlers
			connection->set_open_handler(websocketpp::lib::bind(
				&MetadataT::onOpen,
				metadataPtr,
				&endpoint,
				promisePtr,
				websocketpp::lib::placeholders::_1
			));

			connection->set_fail_handler(websocketpp::lib::bind(
				&MetadataT::onFail,
				metadataPtr,
				&endpoint,
				promisePtr,
				websocketpp::lib::placeholders::_1
			));

			connection->set_close_handler(websocketpp::lib::bind(
				&MetadataT::onClose, metadataPtr, &endpoint, websocketpp::lib::placeholders::_1
			));

			connection->set_message_handler(websocketpp::lib::bind(
				&MetadataT::onMessage,
				static_cast<MetadataT*>(metadataPtr.get()),
				&endpoint,
				websocketpp::lib::placeholders::_1,
				websocketpp::lib::placeholders::_2
			));

			endpoint.connect(connection);
			return promisePtr->get_future();
		}

		/// @brief Close the connection
		/// @param[in] id ID of the connection inside this manager
		/// @param[in] code Status code of the close in form of int
		/// @param[in] reason Additional description
		EC::ErrorCode close(
			int id,
			websocketpp::close::status::value code,
			const std::string& reason
		);

		/// @brief Close the connection
		/// @param[in] handle A handle representing the connection. It must be
		///		create by this manager.
		/// @param[in] code Status code of the close in form of int
		/// @param[in] reason Additional description
		EC::ErrorCode close(
			websocketpp::connection_hdl handle,
			websocketpp::close::status::value code,
			const std::string& reason
		);

		/// @brief Send data via this websocket
		/// @param[in] id ID of the connection inside this manager
		/// @param[in] message Data to send
		EC::ErrorCode send(int id, const std::string& message);

		/// @brief Send data via this websocket
		/// @param[in] handle A handle representing the connection. It must be
		///		create by this manager.
		/// @param[in] message Data to send
		EC::ErrorCode send(websocketpp::connection_hdl, const std::string& message);

		typename Metadata::Ptr getMetadata(int id);

	private:
		std::unordered_map<int, typename Metadata::Ptr> metadata;
		websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread;
		Client endpoint;
		int nextMetadataID;
	};
}  // namespace ViewRay
