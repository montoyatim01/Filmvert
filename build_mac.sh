#!/bin/bash

# Build debug version for Mac and install locally

set -e
HOST_ARCH=$(uname -m)
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

set -x

#rm -r $HOME/.conan/data/imgui

for arch in arm64 x86_64
do

  if [ "$arch" == "arm64" ]; then
    conan_arch="armv8"
  else
    conan_arch="$arch"
  fi

  if [ $BUILD_TYPE == Release ]; then
    rm -rf $BUILD_DIR/$arch
  fi

  ## For OpenColorIO V2.4.2
  conan export conan-recipes/opencolorio

  mkdir -p $BUILD_DIR/$arch
  # Build conan packages
  conan install . \
    --output-folder=$BUILD_DIR/$arch \
    --build=missing \
    --settings:host build_type=$BUILD_TYPE \
    --settings:host arch=$conan_arch \
    --profile:host=darwin-$arch \
    --profile:build=default
# Collect the licenses for all the dependencies
python3 "licenses.py" $BUILD_DIR/$arch

  # Build main program
  CONAN_CMAKE_SYSTEM_PROCESSOR=$arch
  cmake \
    -B $BUILD_DIR/$arch \
    -S $SOURCE_DIR \
    -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/$arch/conan_toolchain.cmake" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

  cmake --build $BUILD_DIR/$arch --target Filmvert --config $BUILD_TYPE -j12 || exit 255
done

if [ $RUN_TESTS -eq 1 ]; then
  cmake --build $BUILD_DIR/$HOST_ARCH --target filmvert_tests --config $BUILD_TYPE -j12
  ctest --test-dir $BUILD_DIR/$HOST_ARCH --output-on-failure
fi

## Stitch binaries together for universal build
cp -r $BUILD_DIR/arm64/bin/Filmvert.app $BUILD_DIR/
lipo $BUILD_DIR/arm64/bin/Filmvert.app/Contents/MacOS/Filmvert $BUILD_DIR/x86_64/bin/Filmvert.app/Contents/MacOS/Filmvert -create -output $BUILD_DIR/Filmvert.app/Contents/MacOS/Filmvert
