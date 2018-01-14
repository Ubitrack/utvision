from conans import ConanFile, CMake


class UbitrackCoreConan(ConanFile):
    name = "ubitrack_vision"
    version = "1.3.0"

    description = "Ubitrack Vision Library"
    url = "https://github.com/Ubitrack/utvision.git"
    license = "GPL"

    short_paths = True
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    options = {"shared": [True, False]}
    requires = (
        "opencv/[>=3.2.0]@camposs/stable", 
        "ubitrack_core/%s@ubitrack/stable" % version,
        )

    default_options = (
        "shared=True",
        )

    # all sources are deployed with the package
    exports_sources = "doc/*", "src/*", "tests/*", "CMakeLists.txt"

    def configure(self):
        if self.options.shared:
            self.options['opencv'].shared = True
            self.options['ubitrack_core'].shared = True

    def imports(self):
        self.copy(pattern="*.dll", dst="bin", src="bin") # From bin to bin
        self.copy(pattern="*.dylib*", dst="lib", src="lib") 
       
    def build(self):
        cmake = CMake(self)
        cmake.definitions['BUILD_SHARED_LIBS'] = self.options.shared
        cmake.configure()
        cmake.build()
        cmake.install()

    def package(self):
        # self.copy("*.h", dst="include", src="src")
        # self.copy("*.lib", dst="lib", keep_path=False)
        # self.copy("*.dll", dst="bin", keep_path=False)
        # self.copy("*.dylib*", dst="lib", keep_path=False)
        # self.copy("*.so", dst="lib", keep_path=False)
        # self.copy("*.a", dst="lib", keep_path=False)
        # self.copy("*", dst="bin", src="bin", keep_path=False)
        pass

    def package_info(self):
        suffix = ""
        if self.settings.os == "Windows":
            suffix += self.version.replace(".", "")
            if self.settings.build_type == "Debug":
                suffix += "d"
        self.cpp_info.libs.append("utvision%s" % (suffix))
