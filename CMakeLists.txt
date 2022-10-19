cmake_minimum_required(VERSION 3.24.0)
project(patient_list LANGUAGES CXX)
include(FetchContent)

FetchContent_Declare(
	json
	GIT_REPOSITORY "https://github.com/nlohmann/json"
	GIT_TAG "v3.11.2"
)

FetchContent_Declare(
	websocketpp
	GIT_REPOSITORY "https://github.com/zaphoyd/websocketpp"
	GIT_TAG "0.8.2"
)

FetchContent_Declare(
	errorcode
	GIT_REPOSITORY "https://github.com/vasil-pashov/error-code"
	GIT_TAG "v0.1.1"
)

FetchContent_MakeAvailable(
	json
	websocketpp
	errorcode
)

set(CPP
	cpp/main.cpp
	cpp/websocket.cpp
	cpp/patient_data.cpp
	cpp/client.cpp
)

set(HEADERS
	include/websocket.h
	include/patient_data.h
	include/client.h
)

add_executable(${PROJECT_NAME} ${CPP} ${HEADERS})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(
	${PROJECT_NAME}
		PRIVATE
			include
			${websocketpp_SOURCE_DIR}
			vendor/asio/include
)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
		ASIO_STANDALONE
		_WEBSOCKETPP_CPP11_TYPE_TRAITS_
		_WEBSOCKETPP_CPP11_RANDOM_DEVICE_
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		nlohmann_json
		error_code
)

if(UNIX)
    target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
            pthread
    )
endIf()
