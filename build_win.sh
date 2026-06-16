#!/bin/bash

set -e

SOURCE_DIR=$PWD
BUILD_TYPE="Debug"
RUN_TESTS=0

for arg in "$@"; do
    case $arg in
        Debug|Release) BUILD_TYPE=$arg ;;
        --tests)       RUN_TESTS=1 ;;
        *) echo "Unknown argument: $arg"; echo "Usage: $0 [Debug|Release] [--tests]"; exit 1 ;;
    esac
done

case $BUILD_TYPE in
    Debug)   BUILD_DIR="Build-debug" ;;
    Release) BUILD_DIR="Build-release" ;;
esac

echo === Building for ${BUILD_TYPE} ===

# Locate VS 2022 (version range [17.0,18.0) excludes VS 2026 Preview).
# -products '*' includes both the full IDE and standalone Build Tools.
VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
if [ ! -f "$VSWHERE" ]; then
    echo "ERROR: vswhere not found at $VSWHERE"
    exit 1
fi
VS_PATH=$("$VSWHERE" -products '*' -version "[17.0,18.0)" -latest \
    -property installationPath 2>/dev/null | head -1 | tr -d '\r')
if [ -z "$VS_PATH" ]; then
    echo "ERROR: Visual Studio 2022 not found. Please install it via the VS Installer."
    exit 1
fi
echo "Using VS 2022 at: $VS_PATH"

# Find the latest MSVC toolset and add it to PATH so ml64.exe, cl.exe etc.
# are resolvable by name in cmake compiler detection for all package builds.
MSVC_TOOLS=$(ls -d "$VS_PATH/VC/Tools/MSVC/"* 2>/dev/null | sort -V | tail -1)
if [ -z "$MSVC_TOOLS" ]; then
    echo "ERROR: No MSVC toolset found under $VS_PATH"
    exit 1
fi
echo "Using MSVC toolset: $MSVC_TOOLS"
export PATH="$MSVC_TOOLS/bin/HostX64/x64:$PATH"

set -x

  if [ $BUILD_TYPE == Release ]; then
    rm -rf $BUILD_DIR
  fi

  ## For OpenColorIO V2.4.2
  conan export conan-recipes/opencolorio

  mkdir -p $BUILD_DIR
  conan install . \
    --output-folder=$BUILD_DIR \
    --build=missing \
    --settings:host build_type=$BUILD_TYPE \
    --profile:host=default \
    --profile:build=default \
    -c tools.cmake.cmaketoolchain:generator="Visual Studio 17 2022" \
    -c 'tools.build:compiler_executables={"asm": "ml64", "c": "cl", "cpp": "cl"}'

python "licenses.py" $BUILD_DIR

  cmake \
    -B $BUILD_DIR \
    -S $SOURCE_DIR \
    -G "Visual Studio 17 2022" \
    -DCMAKE_GENERATOR_INSTANCE="$VS_PATH" \
    -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/conan_toolchain.cmake" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

  cmake --build $BUILD_DIR --target Filmvert --config $BUILD_TYPE -j8 || exit 255

if [ $RUN_TESTS -eq 1 ]; then
  cmake --build $BUILD_DIR --target filmvert_tests --config $BUILD_TYPE -j8
  ctest --test-dir $BUILD_DIR --output-on-failure
fi
