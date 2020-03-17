import os.path
from conans import ConanFile, CMake


class LibMbusTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = ["cmake_find_package", "cmake", "cmake_paths"]

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        bin_path = os.path.join("bin", "libmbus-app")
        self.run(bin_path, run_environment=True)
