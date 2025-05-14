import os
import platform
from conan.tools.cmake import CMakeToolchain
from conan import ConanFile

if platform.system() == "Windows":
    spdlog_options = {
        "spdlog*:header_only": True,
        "spdlog*:wchar_support": True,
        "spdlog*:wchar_filenames": True
    }
else:
     spdlog_options = {
        "spdlog*:header_only": True
    }
if platform.system() == "Linux":
    openimageio_options = {
    "openimageio*:with_opencv": False,
    "openimageio*:with_ffmpeg": False,
    "openimageio*:with_libheif": False,
    "openimageio*:with_hdf5": False,
    "openimageio*:with_tbb": False,
    "openimageio*:with_ptex": False,
    "openimageio*:shared": True,
    "openimageio*:with_libjpeg" : "libjpeg",
    "openimageio*:with_openjpeg" : False,
    "openimageio*:with_raw" : True
    }
else:
    openimageio_options = {
    "openimageio*:with_opencv": False,
    "openimageio*:with_ffmpeg": False,
    "openimageio*:with_libheif": False,
    "openimageio*:with_hdf5": False,
    "openimageio*:with_tbb": False,
    "openimageio*:with_ptex": False,
    "openimageio*:shared": False,
    "openimageio*:with_libjpeg" : "libjpeg",
    "openimageio*:with_openjpeg" : False,
    "openimageio*:with_raw" : True
    }
minizip_options = {
    "minizip-ng*:with_libcomp": False
}
libraw_options = {
    "libraw*:build_thread_safe" : True
}

class FilmvertConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = ["CMakeDeps", "CMakeToolchain", "cmake_find_package",
                  "VCVars", "virtualenv", "scons", "txt"]
    default_options = {**spdlog_options, **openimageio_options, **minizip_options, **libraw_options}

    def requirements(self):
        self.requires("libpng/1.6.44")
        self.requires("fmt/10.2.1")
        self.requires("imgui/1.91.4")
        self.requires("sdl/2.30.6")
        self.requires("spdlog/1.14.1")
        self.requires("nlohmann_json/3.11.3")
        self.requires("libraw/0.21.2")
        self.requires("openimageio/2.5.16.0")
        self.requires("opencolorio/2.4.0")
        self.requires("minizip-ng/4.0.6")
        self.requires("exiv2/0.28.2")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def imports(self):
        self.copy("imgui_impl_sdlrenderer2.h", dst="bindings", src="res/bindings")
        self.copy("imgui_impl_sdlrenderer2.cpp", dst="bindings", src="res/bindings")
        self.copy("imgui_impl_sdl2.cpp", dst="bindings", src="res/bindings")
        self.copy("imgui_impl_sdl2.h", dst="bindings", src="res/bindings")
        self.copy("imgui_demo.cpp", dst="bindings", src="res/src")
        self.copy("imgui_widgets.cpp", dst="bindings", src="res/src")
