cmake_minimum_required(VERSION 3.20)
project(passwall_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Drogon CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_path(SODIUM_INCLUDE_DIR NAMES sodium.h REQUIRED)
find_library(SODIUM_LIBRARY NAMES sodium libsodium REQUIRED)

# Separate build targets enforce the direction of layer dependencies.
add_library(passwall_contracts INTERFACE)
target_compile_features(passwall_contracts INTERFACE cxx_std_17)
target_include_directories(passwall_contracts INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(passwall_contracts INTERFACE Jsoncpp_lib)

add_library(passwall_repositories STATIC
    src/services/repositories/PostgresPasswordWalletRepository.cpp)
target_link_libraries(passwall_repositories PUBLIC passwall_contracts Drogon::Drogon)

add_library(passwall_services STATIC
    src/services/AsyncExecutor.cpp
    src/services/HealthService.cpp
    src/services/AuthenticationService.cpp
    src/services/VaultService.cpp
    src/services/security/SecurityService.cpp
    src/services/security/SessionAuthenticator.cpp)
target_include_directories(passwall_services PRIVATE "${SODIUM_INCLUDE_DIR}")
target_link_libraries(passwall_services
    PUBLIC passwall_contracts
    PRIVATE jwt-cpp::jwt-cpp Threads::Threads "${SODIUM_LIBRARY}")

add_library(passwall_controllers STATIC
    src/controllers/HttpSupport.cpp
    src/controllers/HealthController.cpp
    src/controllers/AuthenticationController.cpp
    src/controllers/VaultController.cpp)
target_link_libraries(passwall_controllers PUBLIC passwall_services Drogon::Drogon)

add_executable(passwall_server main.cpp)
target_link_libraries(passwall_server PRIVATE passwall_controllers passwall_repositories)
