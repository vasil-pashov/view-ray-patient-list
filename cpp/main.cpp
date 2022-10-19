#include "error_code.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <string>
#include "websocket.h"
#include "client.h"

int main() {
	// Init the client
	ViewRay::ViewRayClient wsClient("ws://apply.viewray.com:4645");
	EC::ErrorCode err = wsClient.init();
	if (err.hasError()) {
		std::cout << err.getMessage() << '\n';
		return err.getStatus();
	}
	// Async Request the patient list
	auto promise = wsClient.getPatientList();
	ViewRay::WSAsyncResult<ViewRay::ViewRayClient::PatientListPtr> wsResult = promise.get();
	if (wsResult.hasError()) {
		std::cout << wsResult.getError().getMessage() << '\n';
		return wsResult.getError().getStatus();
	}
	// Wait for the patient list to be retrieved
	ViewRay::ViewRayClient::PatientListPtr patients = wsResult.getData();

	// Print the list
	for (auto patientsIt = patients->begin(); patientsIt != patients->end(); ++patientsIt) {
		std::cout << patientsIt->second;
		std::cout << "\n============================================\n";
	}
	return 0;
}