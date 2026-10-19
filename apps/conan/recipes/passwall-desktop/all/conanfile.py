from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class PassWallDesktop(ConanFile):
    name = "passwall-desktop"
    version = "0.0.1"

    settings = "os", "compiler", "build_type", "arch"

    requires = (
        "drogon/1.9.13",
        "jwt-cpp/0.7.2"
    )

    generators = (
        "CMakeDeps",
        "CMakeToolchain",
    )

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
