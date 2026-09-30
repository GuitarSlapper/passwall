cmake_minimum_required(VERSION 3.16)

project(password-wallet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(password-wallet main.cpp)

include(GNUInstallDirs)
install(TARGETS password-wallet
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
