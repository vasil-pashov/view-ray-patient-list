#include "patient_data.h"
#include <nlohmann/json.hpp>

namespace ViewRay {

	Plan::Plan(const nlohmann::json& plan) :
		label(plan["label"]) {
		assert(plan["type"] == std::string("Plan"));
	}

	std::ostream& operator<<(std::ostream& os, const Plan& plan) {
		os << plan.label << ' ';
		return os;
	}

	Prescription::Prescription(const nlohmann::json& prescriptiopn) :
		description(prescriptiopn["description"]),
		label(prescriptiopn["label"]),
		numFractions(prescriptiopn["num_fractions"]) {
		assert(prescriptiopn["type"] == std::string("Prescription"));
		plans.reserve(prescriptiopn["plans"].size());
		for (const nlohmann::json& json : prescriptiopn["plans"]) {
			plans.emplace_back(json);
		}
	}

	std::ostream& operator<<(std::ostream& os, const Prescription& prescription) {
		os << "Description: " << prescription.description << ", ";
		os << "Label: " << prescription.label << ", ";
		os << "Num Fractions: " << prescription.numFractions << ", ";
		os << "Plans:[";
		for (const Plan& plan : prescription.plans) {
			os << plan;
		}
		os << "]\n";
		return os;
	}

	Diagnose::Diagnose(const nlohmann::json& diagnose) :
		description(diagnose["description"]),
		label(diagnose["label"]) {
		assert(diagnose["type"] == std::string("Diagnosis"));
		prescriptions.reserve(diagnose["prescriptions"].size());
		for (const nlohmann::json& json : diagnose["prescriptions"]) {
			prescriptions.emplace_back(json);
		}
	}

	std::ostream& operator<<(std::ostream& os, const Diagnose& diagnose) {
		os << "Label: " << diagnose.label << ", ";
		os << "Description: " << diagnose.description << ", ";
		os << "Prescriptions:[";
		for (const Prescription& p : diagnose.prescriptions) {
			os << p;
		}
		os << "]";
		return os;
	}

	Patient::Patient(const nlohmann::json& data) {
		id = data["id"];
		mrn = data["mrn"];
		dateOfBirth = data["date_of_birth"];
		firstName = data["first_name"];
		middleName = data["middle_name"];
		lastName = data["last_name"];
		sex = data["sex"] == "M" ? Sex::Male : Sex::Female;
		fractionsTotal = data["fractions_total"];
		fractionsCompleted = data["fractions_completed"];
		weigthKg = data["weight_kg"];
		readyForTreatment = data["ready_for_treatment"];
		registrationTime = data["registration_time"];
	}

	void Patient::diagnosesFromJson(const nlohmann::json& diagnosesJson) {
		diagnoses.clear();
		diagnoses.reserve(diagnosesJson.size());
		for (const auto& diagnose : diagnosesJson) {
			diagnoses.emplace_back(diagnose);
		}
	}
 
	std::ostream& operator<<(std::ostream& os, const Patient& patient) {
		os << "Patient ID: " << patient.id << '\n';
		os << "MRN: " << patient.mrn << '\n';
		os << "Date of birth: " << patient.dateOfBirth << '\n';
		os << "First Name: " << patient.firstName << '\n';
		os << "Middle Name: " << patient.middleName << '\n';
		os << "Last Name: " << patient.lastName << '\n';
		os << "Sex: " << (patient.sex == Patient::Sex::Male ? "Male" : "Female") << '\n';
		os << "Fractions Total: " << patient.fractionsTotal << '\n';
		os << "Fractions Completed: " << patient.fractionsCompleted << '\n';
		os << "Weight: " << patient.weigthKg << '\n';
		os << "Ready for treatment: " << (patient.readyForTreatment ? "True" : "False") << '\n';
		os << "Registration Time: " << patient.registrationTime << '\n';
		os << "Diagnoses:[";
		for (const Diagnose& d : patient.diagnoses) {
			os << d << '\n';
		}
		os << "]\n";
		return os;
	}
}  // namespace ViewRay