#pragma once
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace ViewRay {
	class Plan {
	public:
		Plan() = default;
		explicit Plan(const nlohmann::json& plan);
		friend std::ostream& operator<<(std::ostream& os, const Plan& plan);

	private:
		std::string label;
	};

	class Prescription {
	public:
		Prescription() = default;
		explicit Prescription(const nlohmann::json& data);
		friend std::ostream& operator<<(std::ostream& os, const Prescription& prescription);

	private:
		std::string description;
		std::string label;
		int numFractions;
		std::vector<Plan> plans;
	};

	class Diagnose {
	public:
		Diagnose() = default;
		explicit Diagnose(const nlohmann::json& data);

		friend std::ostream& operator<<(std::ostream& os, const Diagnose& diagnose);

	private:
		std::string description;
		std::string label;
		std::vector<Prescription> prescriptions;
	};

	class Patient {
	public:
		enum class Sex {
			Male,
			Female
		};
		Patient() = default;
		explicit Patient(const nlohmann::json& data);

		/// @brief Parses a JSON and stores the diagnoses for a patient
		/// @param diagnoses JSON representing the diagnoses
		void diagnosesFromJson(const nlohmann::json& diagnoses);

		Patient(const Patient&) = delete;
		Patient& operator=(const Patient&) = delete;

		Patient(Patient&&) = default;
		Patient& operator=(Patient&&) = default;

		/// Can be used to print a patient info on the console
		/// @todo Improve on the readability of the output. Needs a way to indent nested objects.
		friend std::ostream& operator<<(std::ostream& os, const Patient& dt);

	private:
		std::string id;
		std::string mrn;
		std::string dateOfBirth;
		std::string firstName;
		std::string middleName;
		std::string lastName;
		Sex sex;
		int fractionsTotal;
		int fractionsCompleted;
		int weigthKg;
		int registrationTime;
		bool readyForTreatment;
		std::vector<Diagnose> diagnoses;
	};
}  // namespace ViewRay