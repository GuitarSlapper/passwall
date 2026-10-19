# MIT License
#
# Copyright (c) 2026 GuitarSlapper
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.20)
project(passwall_desktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Network Concurrent)
find_path(SODIUM_INCLUDE_DIR NAMES sodium.h REQUIRED)
find_library(SODIUM_LIBRARY NAMES sodium libsodium REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.8)
    qt_policy(SET QTP0004 NEW)
endif()

add_library(passwall_desktop_api STATIC
    src/api/ApiClient.cpp
    src/api/ApiClient.h
)
target_include_directories(passwall_desktop_api PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(passwall_desktop_api PUBLIC Qt6::Core Qt6::Network)

add_library(passwall_desktop_crypto STATIC
    src/crypto/CryptoService.cpp
)
target_include_directories(passwall_desktop_crypto
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PRIVATE "${SODIUM_INCLUDE_DIR}"
)
target_link_libraries(passwall_desktop_crypto PUBLIC Qt6::Core PRIVATE "${SODIUM_LIBRARY}")

add_library(passwall_desktop_auth STATIC
    src/auth/AuthManager.cpp
    src/auth/AuthManager.h
    src/auth/KeyStore.cpp
)
target_link_libraries(passwall_desktop_auth PUBLIC passwall_desktop_api passwall_desktop_crypto Qt6::Concurrent)

add_library(passwall_desktop_vault STATIC
    src/vault/VaultManager.cpp
    src/vault/VaultManager.h
)
target_link_libraries(passwall_desktop_vault PUBLIC passwall_desktop_auth)

qt_add_executable(passwall_desktop src/main.cpp)
qt_add_qml_module(passwall_desktop
    URI Passwall
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/LoginPage.qml
        src/qml/RegisterPage.qml
        src/qml/VaultPage.qml
)
target_link_libraries(passwall_desktop
    PRIVATE passwall_desktop_vault Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2
)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(passwall_desktop_tests tests/ClientTests.cpp)
    target_link_libraries(passwall_desktop_tests PRIVATE passwall_desktop_vault Qt6::Test)
    add_test(NAME passwall_desktop_tests COMMAND passwall_desktop_tests)
    add_test(NAME passwall_desktop_qml_smoke COMMAND passwall_desktop -platform offscreen --smoke-test)
    set_tests_properties(passwall_desktop_qml_smoke PROPERTIES ENVIRONMENT "QML_DISABLE_DISK_CACHE=1")
endif()
