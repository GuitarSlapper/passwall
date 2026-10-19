class TestPassWallServer(ConanFile):
    version = "0.0.1"

    settings = "os", "compiler", "build_type", "arch"

    generators = (
        "CMakeDeps",
        "CMakeToolchain",
    )

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requirements(self.tested_reference_str)

    def build(self):

