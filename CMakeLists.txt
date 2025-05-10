cmake_minimum_required(VERSION 3.18)

set(LANGUAGES C CXX)

if (NOT APPLE)
    #disable cmake architectures as we pass nvcc flags on our own below
    set(CMAKE_CUDA_ARCHITECTURES OFF)
    set(LANGUAGES ${LANGUAGES} CUDA)
else()
    set(LANGUAGES ${LANGUAGES} OBJC)
endif()

project(tgrain ${LANGUAGES})

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_OSX_DEPLOYMENT_TARGET "13.0")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT APPLE)
    find_package(CUDA 11.0 REQUIRED)
    message(STATUS "Found CUDA ${CUDA_VERSION_STRING} at ${CUDA_TOOLKIT_ROOT_DIR}")

    find_package(OpenCL)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    find_package(OpenSSL REQUIRED)
endif()

find_package(libraw REQUIRED)
find_package(OpenImageIO REQUIRED)
find_package(OpenColorIO REQUIRED)
find_package(exiv2 REQUIRED)
find_package(fmt REQUIRED)
target_compile_definitions( fmt::fmt INTERFACE FMT_HEADER_ONLY)
find_package(spdlog REQUIRED)
find_package(SDL2 REQUIRED CONFIG)
find_package(imgui REQUIRED)
find_package(nlohmann_json REQUIRED)

add_definitions(-DUSE_OPENIMAGEIO_RAW=1)

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
include_directories(src/image)
include_directories(src/window)
include_directories("${CMAKE_BINARY_DIR}/bindings")
#INCLUDE_DIRECTORIES("/Users/timothymontoya/Downloads/exiv2-0.28.5-Darwin-arm64/include")
#add_library(LibRaw STATIC IMPORTED)
#set_target_properties(LibRaw PROPERTIES
#    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/LibRaw/lib/libraw_r.dylib"
#    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/LibRaw"
#)

if (APPLE)
    include_directories("/usr/local/include/metal-cpp")
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeRC.cmake)

## BUILD INFO ##
# Get the current date and time
string(TIMESTAMP BUILD_DATE "%Y-%m-%d")

# Execute git command to get the commit hash
execute_process(COMMAND git rev-parse HEAD
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_COMMIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE)

list(APPEND CUDA_NVCC_FLAGS
    -lineinfo
    -gencode=arch=compute_50,code=sm_50
    -gencode=arch=compute_52,code=sm_52
    -gencode=arch=compute_60,code=sm_60
    -gencode=arch=compute_61,code=sm_61
    -gencode=arch=compute_62,code=sm_62
    -gencode=arch=compute_70,code=sm_70
    -gencode=arch=compute_72,code=sm_72
    -gencode=arch=compute_75,code=sm_75
    -gencode=arch=compute_80,code=sm_80
    -gencode=arch=compute_80,code=compute_80
)

if (APPLE)
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} \"-Wno-deprecated\"")
    set(ARCHI MacOS)
elseif (WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    list(APPEND CUDA_NVCC_FLAGS_DEBUG --std=c++17 -D_DEBUG)
    list(APPEND CUDA_NVCC_FLAGS_RELEASE --std=c++17)

    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} \"-D_CRT_SECURE_NO_WARNINGS\" \"-DCURL_STATICLIB\"")
    set(ARCHI Win64)
elseif (UNIX)
    list(APPEND CUDA_NVCC_FLAGS --std=c++17 --compiler-options="-fPIC" --compiler-options="-Wno-deprecated-declarations" --compiler-options="-Wno-deprecated")
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} \"-Wno-deprecated-declarations\"")
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} \"-Wno-deprecated\"")
    set(ARCHI Linux-x86-64)
endif()



set(IMGUI_SRCS
    ${CMAKE_BINARY_DIR}/bindings/imgui_impl_sdl2.h
    ${CMAKE_BINARY_DIR}/bindings/imgui_impl_sdl2.cpp
    ${CMAKE_BINARY_DIR}/bindings/imgui_impl_sdlrenderer2.h
    ${CMAKE_BINARY_DIR}/bindings/imgui_impl_sdlrenderer2.cpp
)

if (APPLE)
    add_custom_target(make-metallib DEPENDS default.metallib)

    add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/Filmvert.air"
        COMMAND xcrun -sdk macosx metal -std=macos-metal2.4 -mmacos-version-min=12.0 -ffast-math -DMETAL=1 -c ${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/Filmvert.metal -o "${CMAKE_BINARY_DIR}/Filmvert.air"
        DEPENDS src/gpu/Filmvert.metal src/renderParams.h
    )
    add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/default.metallib"
        COMMAND xcrun -sdk macosx metallib "${CMAKE_BINARY_DIR}/Filmvert.air" -o "${CMAKE_BINARY_DIR}/default.metallib"
        DEPENDS "${CMAKE_BINARY_DIR}/Filmvert.air"
    )
    # Custom target to copy the metallib
    add_custom_target(copy-metallib ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/default.metallib"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/default.metallib"
    DEPENDS "${CMAKE_BINARY_DIR}/default.metallib"
)
    set(LICENSEWINDOW_SRC
    ${CMAKE_BINARY_DIR}/bindings/imgui_demo.cpp
    ${CMAKE_BINARY_DIR}/bindings/imgui_widgets.cpp)
elseif (WIN32)
    set(LICENSEWINDOW_SRC
    ${CMAKE_BINARY_DIR}/bindings/imgui_demo.cpp
    ${CMAKE_BINARY_DIR}/bindings/imgui_widgets.cpp)
elseif (UNIX)
    set(LICENSEWINDOW_SRC
    ${CMAKE_BINARY_DIR}/bindings/imgui_demo.cpp
    ${CMAKE_BINARY_DIR}/bindings/imgui_widgets.cpp)

endif()

cmrc_add_resource_library(resources
ALIAS assets::rc
NAMESPACE assets
assets/fonts/Roboto-Black.ttf
assets/fonts/Roboto-Bold.ttf
assets/fonts/Roboto-Regular.ttf
assets/default.metallib
assets/studio-config-v2.2.0_aces-v1.3_ocio-v2.3.ocio
)
set_property(TARGET resources PROPERTY POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE IMAGE_SRCS src/image/*.cpp)
file(GLOB_RECURSE WINDOW_SRCS src/window/*.cpp)


set(TGRAIN_SRCS
    src/main.cpp
    src/logger.cpp
    ${WINDOW_SRCS}
    ${IMAGE_SRCS}
    src/roll.cpp
    src/utils.cpp
    src/grainParams.cpp
    src/metalGPU.cpp
    src/ocioProcessor.cpp
    src/threadPool.cpp
    src/threadPool.h
    src/macOSFile.mm
    ${LICENSEWINDOW_SRC}
    ${IMGUI_SRCS}
)
set(CMAKE_MACOSX_RPATH ON)
add_executable(tGrain MACOSX_BUNDLE ${TGRAIN_SRCS})
set_target_properties(tGrain PROPERTIES
    BUNDLE True
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.timothymontoya.tGrain"
    MACOSX_BUNDLE_BUNDLE_NAME "tGrain"
    MACOSX_BUNDLE_BUNDLE_VERSION "3.1.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "3.1.0"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist.in
)

if (APPLE)
    #set_source_files_properties(HaloLab/HaloLabPlugin.cpp
    #                           COMPILE_FLAGS "-x objective-c++")

    #target_link_directories(tGrain PUBLIC "/Users/timothymontoya/Downloads/exiv2-0.28.5/build/lib")
    target_link_libraries(tGrain
                  "-framework MetalKit"
                  "-framework IOKit"
                  "-framework CoreVideo"
                  "-framework OpenGL"
                  "-framework GLUT"
                  "-framework Cocoa"
                  "-framework QuartzCore"
                  "-framework Foundation"
                  "-framework Metal"
      SDL2::SDL2-static
                  imgui::imgui
                  spdlog::spdlog_header_only
                  assets::rc
                  nlohmann_json::nlohmann_json
                  libraw::libraw
                  OpenImageIO::OpenImageIO
                  OpenColorIO::OpenColorIO
                  exiv2::exiv2
                  #LibRaw
                  #-lomp
    )
elseif (WIN32)

    target_link_libraries(tGrain wsock32 ws2_32 GDI32 USER32 gdiplus Crypt32 Wldap32
                          SDL2::SDL2-static
                          imgui::imgui
                          spdlog::spdlog_header_only
                          assets::rc
                          nlohmann_json::nlohmann_json
                          OpenImageIO::OpenImageIO
                          cuda)
elseif (UNIX)
    target_link_libraries(tGrain -lOpenCL -lcurl -lcrypto -lz)
    target_link_libraries(tGrain
                          SDL2::SDL2-static
                          imgui::imgui
                          spdlog::spdlog_header_only
                          assets::rc
                          nlohmann_json::nlohmann_json
                          OpenImageIO::OpenImageIO
                          cuda)
    include_directories("/usr/include/gtk-3.0"
         "/usr/include/glib-2.0"
         "/usr/include/pango-1.0"
                        "/usr/include/harfbuzz"
         "/usr/include/cairo"
         "/usr/include/gdk-pixbuf-2.0"
         "/usr/include/atk-1.0"
              "/usr/lib64/glib-2.0/include"
              "/usr/lib/x86_64-linux-gnu/glib-2.0/include")
endif()

target_compile_definitions(tGrain PRIVATE
    TG_PROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"
    BUILD_DATE="${BUILD_DATE}"
    GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
)


if (APPLE)
    add_dependencies(tGrain make-metallib)

    #add_custom_command(TARGET tGrain POST_BUILD
    #  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #          "${CMAKE_SOURCE_DIR}/LibRaw/lib/libraw_r.24.dylib"
    #          "$<TARGET_BUNDLE_DIR:tGrain>/Contents/Frameworks/libraw_r.24.dylib"
    #  COMMAND install_name_tool -id "@rpath/libraw_r.24.dylib"
    #          "$<TARGET_BUNDLE_DIR:tGrain>/Contents/Frameworks/libraw_r.24.dylib"
    #  COMMAND install_name_tool -add_rpath "@executable_path/../Frameworks"
    #          $<TARGET_FILE:tGrain>
    #  COMMAND install_name_tool -change "/usr/local/lib/libraw_r.24.dylib"
    #          "@rpath/libraw_r.24.dylib"
    #          $<TARGET_FILE:tGrain>)

endif()
