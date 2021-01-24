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
    options = {
        "shared": [True, False],
        "workspaceBuild" : [True, False],
        "opengl_extension_wrapper": ["glew", "glad"],
        "with_opencl": [True, False],
    }

    default_options = {
        "shared" :True,
        "workspaceBuild" : False,
        "opengl_extension_wrapper": "glad",
        "with_opencl": True,
        }

    # all sources are deployed with the package
    exports_sources = "cmake/*", "doc/*", "src/*", "tests/*", "CMakeLists.txt", "utvisionConfig.cmake"


    def configure(self):
        if self.options.shared:
            self.options['opencv'].shared = True
            self.options['ubitrack_core'].shared = True
        if self.settings.os == "Linux":
            self.options["opencv"].with_gtk = True

    def requirements(self):
        userChannel = "ubitrack/stable"
        if self.options.workspaceBuild:
            userChannel = "local/dev"
        self.requires("ubitrack_core/%s@%s" % (self.version, userChannel))
        self.requires("opencv/[>=3.2.0 < 4.0.0]@camposs/stable" )
        self.requires("zstd/1.4.3")

        if self.options.opengl_extension_wrapper == 'glad':
            self.requires("glad/[>=0.1.27]@camposs/stable")
            if self.options.shared:
                self.options['glad'].shared = True
        elif self.options.opengl_extension_wrapper == 'glew':
            self.requires("glew/2.1.0@camposs/stable")
            self.requires("glext/1.3@camposs/stable")
            if self.options.shared:
                self.options['glew'].shared = True



    # add linux requirement: ubuntu "ocl-icd-opencl-dev"

    # def imports(self):
    #     self.copy(pattern="*.dll", dst="bin", src="bin") # From bin to bin
    #     self.copy(pattern="*.dylib*", dst="lib", src="lib") 
       
    def build(self):
        cmake = CMake(self)
        cmake.definitions['BUILD_SHARED_LIBS'] = self.options.shared
        cmake.definitions['ENABLE_UNITTESTS'] = not self.options['ubitrack_core'].without_tests
        cmake.definitions['HAVE_GLAD'] = self.options.opengl_extension_wrapper == 'glad'
        cmake.definitions['HAVE_GLEW'] = self.options.opengl_extension_wrapper == 'glew'
        cmake.definitions['WITH_OPENCL'] = self.options.with_opencl
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
