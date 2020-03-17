from conans import ConanFile, CMake


class LibMbus(ConanFile):
    name = "libmbus"
    version = "0.9.0"
    license = "BSD 3-Clause"
    url = "https://gitlab.com/gocarlos/libmbus"
    description = "M-bus Library from Raditex Control"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = ["cmake", "cmake_find_package", "cmake_paths"]
    exports_sources = "CMakeLists.txt", "cmake/*", "src/*", "mbus/*"

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["libmbus"]
        if self.settings.os == "Linux":
            self.cpp_info.system_libs = ["m", "pthread"]
