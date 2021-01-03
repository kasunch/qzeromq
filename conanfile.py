import re
from conans import ConanFile, CMake, tools


class QZeroMQConan(ConanFile):
    name = "qzeromq"
    description = "Qt library for ZMQ"
    url = "https://github.com/kasunch/qzeromq"
    homepage = "https://github.com/kasunch/qzeromq"
    license = "Apache-2.0"  # SPDX Identifiers https://spdx.org/licenses/
    exports_sources = ["src/*", "perf/*", "QZeroMQConfig.cmake.in", "CMakeLists.txt"]
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake", "cmake_find_package"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    _git_is_dirty = False
    _git_commit = "unknown"
    _cmake = None

    def set_version(self):
        git = tools.Git(folder=self.recipe_folder)
        if not self.version:
            output = git.run("describe --all").splitlines()[0].strip()
            self.version = re.sub("^.*/v?|^v?", "", output)
        output = git.run("diff --stat").splitlines()
        self._git_is_dirty = True if output else False
        self._git_commit = git.run("rev-parse HEAD").splitlines()[0].strip()

        self.output.info("Version: %s, Commit: %s, Is_dirty: %s" %
                         (self.version, self._git_commit, self._git_is_dirty))

    def requirements(self):
        self.requires("qt/5.14.2@bincrafters/stable")
        self.requires("zeromq/4.3.3")

    def _configure_cmake(self):
        if not self._cmake:
            self._cmake = CMake(self)
            if self.options.shared:
                self._cmake.definitions["BUILD_SHARED"] = "ON"
                self._cmake.definitions["BUILD_STATIC"] = "OFF"
            else:
                self._cmake.definitions["BUILD_SHARED"] = "OFF"
                self._cmake.definitions["BUILD_STATIC"] = "ON"
            if self.options["zeromq"].shared:
                self._cmake.definitions["ZMQ_SHARED"] = "ON"
            else:
                self._cmake.definitions["ZMQ_SHARED"] = "OFF"
            self._cmake.definitions["WITH_PERF_TOOL"] = "ON"    
            self._cmake.definitions["USE_CONAN_BUILD_INFO"] = "ON"
            self._cmake.definitions["SOURCE_VERSION"] = self.version
            self._cmake.definitions["SOURCE_COMMIT"] = self._git_commit
            self._cmake.definitions["SOURCE_DIRTY"] = self._git_is_dirty
            del self._cmake.definitions["CMAKE_EXPORT_NO_PACKAGE_REGISTRY"]
            self._cmake.configure()
        return self._cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        pass
