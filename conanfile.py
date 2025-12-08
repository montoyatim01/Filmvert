import os
import platform
from types import NoneType

from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain
from conan.tools.files import copy

if platform.system() == "Windows":
    spdlog_options = {
        "spdlog*:header_only": True,
        "spdlog*:wchar_filenames": True,
    }
else:
    spdlog_options = {"spdlog*:header_only": True}

openimageio_options = {
    "openimageio*:with_opencv": False,
    "openimageio*:with_ffmpeg": False,
    "openimageio*:with_libheif": False,
    "openimageio*:with_hdf5": False,
    "openimageio*:with_tbb": False,
    "openimageio*:with_ptex": False,
    "openimageio*:shared": False,
    "openimageio*:with_libjpeg": "libjpeg",
    "openimageio*:with_raw": True,
}
minizip_options = {"minizip-ng*:with_libcomp": False}
libraw_options = {"libraw*:build_thread_safe": True}


class FilmvertConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    default_options = {
        **spdlog_options,
        **openimageio_options,
        **minizip_options,
        **libraw_options,
    }

    def requirements(self):
        self.requires("libpng/1.6.52")
        self.requires("libjpeg/9f", override=True)
        self.requires("openexr/3.4.4")
        self.requires("jasper/4.0.0")
        self.requires("lcms/2.16")
        self.requires("fmt/12.0.0", override=True)
        self.requires("imgui/1.92.5")
        self.requires("spdlog/1.16.0")
        self.requires("nlohmann_json/3.12.0")
        self.requires("libraw/0.21.2")
        self.requires("opencolorio/2.4.2")
        self.requires("openimageio/3.1.8.0")
        self.requires("minizip-ng/4.0.7")
        self.requires("exiv2/0.28.3")
        self.requires("glfw/3.4")
        self.requires("glew/2.2.0")

    def generate(self):
        output_dir = self.folders.generators
        imgui_dep = self.dependencies.get("imgui")
        if imgui_dep:
            # Copy ImGui bindings to a "bindings" subfolder in the output directory
            bindings_dir = os.path.join(output_dir, "bindings")

            copy(
                self,
                "imgui_impl_glfw.h",
                src=os.path.join(imgui_dep.package_folder, "res", "bindings"),
                dst=bindings_dir,
                keep_path=False,
            )
            copy(
                self,
                "imgui_impl_glfw.cpp",
                src=os.path.join(imgui_dep.package_folder, "res", "bindings"),
                dst=bindings_dir,
                keep_path=False,
            )
            copy(
                self,
                "imgui_impl_opengl3.h",
                src=os.path.join(imgui_dep.package_folder, "res", "bindings"),
                dst=bindings_dir,
                keep_path=False,
            )
            copy(
                self,
                "imgui_impl_opengl3.cpp",
                src=os.path.join(imgui_dep.package_folder, "res", "bindings"),
                dst=bindings_dir,
                keep_path=False,
            )
            copy(
                self,
                "imgui_impl_opengl3_loader.h",
                src=os.path.join(imgui_dep.package_folder, "res", "bindings"),
                dst=bindings_dir,
                keep_path=False,
            )
            copy(
                self,
                "imgui_impl_opengl3_loader.cpp",
                src=os.path.join(imgui_dep.package_folder, "res", "bindings"),
                dst=bindings_dir,
                keep_path=False,
            )

            # Copy ImGui source files
            copy(
                self,
                "imgui_demo.cpp",
                src=os.path.join(imgui_dep.package_folder, "res", "src"),
                dst=bindings_dir,
                keep_path=False,
            )
            copy(
                self,
                "imgui_widgets.cpp",
                src=os.path.join(imgui_dep.package_folder, "res", "src"),
                dst=bindings_dir,
                keep_path=False,
            )

        # Copy licenses from all dependencies
        licenses_dir = os.path.join(output_dir, "licenses")
        for dep in self.dependencies.values():
            if dep.package_folder != None:
                copy(
                    self,
                    "license*",
                    src=dep.package_folder,
                    dst=os.path.join(licenses_dir, dep.ref.name),
                    keep_path=True,
                    excludes=["*.pyc", "*.pyo"],
                )
