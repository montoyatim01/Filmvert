cmake_minimum_required(VERSION 3.18)

set(LANGUAGES C CXX)

if (NOT APPLE)
else()
    set(LANGUAGES ${LANGUAGES} OBJC)
endif()

project(filmvert ${LANGUAGES})

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_OSX_DEPLOYMENT_TARGET "13.0")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


find_package(libraw REQUIRED)
find_package(OpenImageIO REQUIRED)
find_package(OpenColorIO REQUIRED)
find_package(exiv2 REQUIRED)
find_package(fmt REQUIRED)
target_compile_definitions( fmt::fmt INTERFACE FMT_HEADER_ONLY)
find_package(spdlog REQUIRED)
find_package(imgui REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -DDEBUG_BUILD")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -DDEBUG_BUILD")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

include_directories(src)
include_directories(src/gpu)
include_directories(src/image)
include_directories(src/misc)
include_directories(src/ocio)
include_directories(src/roll)
include_directories(src/state)
include_directories(src/window)
include_directories("${CMAKE_BINARY_DIR}/bindings")



include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeRC.cmake)

## BUILD INFO ##
# Get the current date and time
string(TIMESTAMP BUILD_DATE "%Y-%m-%d")

# Execute git command to get the commit hash
execute_process(COMMAND git rev-parse HEAD
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_COMMIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE)


if (APPLE)
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} \"-Wno-deprecated\"")
    set(ARCHI MacOS)
elseif (WIN32)
    add_compile_definitions(NOMINMAX)
    set(ARCHI Win64)
elseif (UNIX)
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} \"-Wno-deprecated-declarations\"")
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} \"-Wno-deprecated\"")
    set(ARCHI Linux-x86-64)
endif()


cmrc_add_resource_library(resources
ALIAS assets::rc
NAMESPACE assets
assets/fonts/Roboto-Black.ttf
assets/fonts/Roboto-Bold.ttf
assets/fonts/Roboto-Regular.ttf
assets/fonts/JetBrainsMono-Regular.ttf
assets/studio-config-v2.2.0_aces-v1.3_ocio-v2.3.ocio
assets/studio-config-v3.0.0_aces-v2.0_ocio-v2.4.ocio
assets/logo.png
assets/licenses.txt
)
set_property(TARGET resources PROPERTY POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE IMGUI_SRCS ${CMAKE_BINARY_DIR}/bindings/*.cpp)
file(GLOB_RECURSE GPU_SRCS src/gpu/*.cpp)
file(GLOB_RECURSE IMAGE_SRCS src/image/*.cpp)
file(GLOB_RECURSE MISC_SRCS src/misc/*.cpp)
file(GLOB_RECURSE OCIO_SRCS src/ocio/*.cpp)
file(GLOB_RECURSE ROLL_SRCS src/roll/*.cpp)
file(GLOB_RECURSE STATE_SRCS src/state/*.cpp)
file(GLOB_RECURSE WINDOW_SRCS src/window/*.cpp)




set(FILMVERT_SRCS
    ${IMGUI_SRCS}
    src/main.cpp
    ${WINDOW_SRCS}
    ${IMAGE_SRCS}
    ${MISC_SRCS}
    ${ROLL_SRCS}
    ${STATE_SRCS}
    ${OCIO_SRCS}
    ${GPU_SRCS}
)


if (APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    add_executable(Filmvert MACOSX_BUNDLE ${FILMVERT_SRCS} src/misc/macOSFile.mm)
    set_target_properties(Filmvert PROPERTIES
    BUNDLE True
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.timothymontoya.Filmvert"
    MACOSX_BUNDLE_BUNDLE_NAME "Filmvert"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.2.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.2.0"
    MACOSX_BUNDLE_ICON_FILE "AppIcon"
)

    # Add the icon file to the bundle resources
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/Icon/AppIcon.icns PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
)
    target_sources(Filmvert PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Icon/AppIcon.icns)

    target_link_libraries(Filmvert
                  "-framework MetalKit"
                  "-framework IOKit"
                  "-framework CoreVideo"
                  "-framework OpenGL"
                  "-framework GLUT"
                  "-framework Cocoa"
                  "-framework QuartzCore"
                  "-framework Foundation"
                  "-framework Metal"
                  glfw
                  GLEW::GLEW
                  imgui::imgui
                  spdlog::spdlog_header_only
                  assets::rc
                  nlohmann_json::nlohmann_json
                  libraw::libraw
                  OpenImageIO::OpenImageIO
                  OpenColorIO::OpenColorIO
                  exiv2::exiv2
                  openssl::openssl
                  CURL::libcurl
                  ZLIB::ZLIB)
elseif (WIN32)
    add_executable(Filmvert WIN32 ${FILMVERT_SRCS} Icon/resources.rc)
    target_link_libraries(Filmvert wsock32 ws2_32 GDI32 USER32 gdiplus Crypt32 Wldap32
        glfw
        GLEW::GLEW
        imgui::imgui
        spdlog::spdlog_header_only
        assets::rc
        nlohmann_json::nlohmann_json
        libraw::libraw
        OpenImageIO::OpenImageIO
        OpenColorIO::OpenColorIO
        exiv2::exiv2
        openssl::openssl
        CURL::libcurl
        ZLIB::ZLIB)
elseif (UNIX)
    add_executable(Filmvert ${FILMVERT_SRCS})
    target_link_libraries(Filmvert
        glfw
        GLEW::GLEW
        imgui::imgui
        spdlog::spdlog_header_only
        assets::rc
        nlohmann_json::nlohmann_json
        libraw::libraw
        OpenImageIO::OpenImageIO
        OpenColorIO::OpenColorIO
        exiv2::exiv2
        openssl::openssl
        CURL::libcurl
        ZLIB::ZLIB)
endif()

target_compile_definitions(Filmvert PRIVATE
    TG_PROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"
    BUILD_DATE="${BUILD_DATE}"
    GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
)

# ---- Tests ----
# On macOS we build arm64 and x86_64 separately then lipo them together.
# Tests can only run on the arch that matches the host machine, so disable
# them automatically for cross-arch (e.g. x86_64 on Apple Silicon) builds.
if(APPLE AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL CMAKE_HOST_SYSTEM_PROCESSOR)
    set(_build_tests_default OFF)
    message(STATUS "Tests disabled: cross-arch build "
                   "(host=${CMAKE_HOST_SYSTEM_PROCESSOR}, "
                   "target=${CMAKE_SYSTEM_PROCESSOR})")
else()
    set(_build_tests_default ON)
endif()
option(BUILD_TESTS "Build the test suite" ${_build_tests_default})
if(BUILD_TESTS)
    enable_testing()
    find_package(Catch2 REQUIRED)

    # Core sources that can be compiled without GPU or GUI dependencies
    set(TEST_CORE_SRCS
        ${MISC_SRCS}
        ${IMAGE_SRCS}
        ${OCIO_SRCS}
    )

    file(GLOB TEST_SRCS tests/*.cpp)

    add_executable(filmvert_tests
        ${TEST_SRCS}
        ${TEST_CORE_SRCS}
    )

    target_include_directories(filmvert_tests PRIVATE
        src
        src/image
        src/roll
        src/state
        src/ocio
        "${CMAKE_BINARY_DIR}/bindings"
    )

    target_link_libraries(filmvert_tests PRIVATE
        Catch2::Catch2WithMain
        spdlog::spdlog_header_only
        nlohmann_json::nlohmann_json
        OpenImageIO::OpenImageIO
        exiv2::exiv2
        openssl::openssl
        CURL::libcurl
        ZLIB::ZLIB
    )

    target_compile_definitions(filmvert_tests PRIVATE
        TG_PROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"
    )

    include(CTest)
    include(Catch)
    catch_discover_tests(filmvert_tests)
endif()
