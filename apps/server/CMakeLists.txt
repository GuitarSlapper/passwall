cmake_minimum_required(VERSION 3.20)
project(passwall_server VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Drogon CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(jsoncpp CONFIG QUIET)
find_path(SODIUM_INCLUDE_DIR NAMES sodium.h REQUIRED)
find_library(SODIUM_LIBRARY NAMES sodium libsodium REQUIRED)

foreach(candidate JsonCpp::JsonCpp Jsoncpp_lib jsoncpp_lib jsoncpp_static jsoncpp)
    if(TARGET ${candidate})
        set(PASSWALL_JSONCPP_TARGET ${candidate})
        break()
    endif()
endforeach()
if(NOT PASSWALL_JSONCPP_TARGET)
    message(FATAL_ERROR "JsonCpp must provide an imported CMake target")
endif()

add_library(passwall_server_contracts INTERFACE)
target_include_directories(passwall_server_contracts INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(passwall_server_contracts INTERFACE ${PASSWALL_JSONCPP_TARGET})

add_library(passwall_server_repositories STATIC
    src/services/repositories/PostgresPasswordWalletRepository.cpp
)
target_link_libraries(passwall_server_repositories PUBLIC passwall_server_contracts Drogon::Drogon)

add_library(passwall_server_services STATIC
    src/services/AsyncExecutor.cpp
    src/services/HealthService.cpp
    src/services/AuthenticationService.cpp
    src/services/VaultService.cpp
    src/services/security/SecurityService.cpp
    src/services/security/SessionAuthenticator.cpp
)
target_include_directories(passwall_server_services PRIVATE "${SODIUM_INCLUDE_DIR}")
target_link_libraries(passwall_server_services
    PUBLIC passwall_server_contracts
    PRIVATE jwt-cpp::jwt-cpp Threads::Threads "${SODIUM_LIBRARY}"
)

add_library(passwall_server_controllers STATIC
    src/controllers/HttpSupport.cpp
    src/controllers/HealthController.cpp
    src/controllers/AuthenticationController.cpp
    src/controllers/VaultController.cpp
)
target_link_libraries(passwall_server_controllers PUBLIC passwall_server_services Drogon::Drogon)

add_executable(passwall_server src/main.cpp)
target_link_libraries(passwall_server PRIVATE passwall_server_controllers passwall_server_repositories)

include(CTest)
option(PASSWALL_ENABLE_INTEGRATION_TESTS "Test a running server with a disposable database" OFF)
if(BUILD_TESTING AND PASSWALL_ENABLE_INTEGRATION_TESTS)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME passwall_server_integration
        COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_integration.py"
    )
    set_tests_properties(passwall_server_integration PROPERTIES LABELS integration TIMEOUT 120)
endif()
