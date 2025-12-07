#!/bin/bash

# Build debug version for Mac and install locally

set -e

BUILD_TYPE=${1:-Debug}
SOURCE_DIR=$PWD
INSTALL_FLAG=0
case $BUILD_TYPE in
    Debug)
        BUILD_DIR="Build-debug"
        ;;
    Release)
        BUILD_DIR="Build-release"
        ;;
    *)
        echo "Invalid build type \"$BUILD_TYPE\"; must be Release or Debug"
        exit 1
        ;;

esac

echo === Building for ${BUILD_TYPE} ===

set -x

#rm -r $HOME/.conan/data/imgui

for arch in arm64 x86_64
do

  if [ $BUILD_TYPE == Release ]; then
    rm -rf $BUILD_DIR/$arch
  fi

  ## For OpenColorIO V2.4.2
  conan export conan-recipes/opencolorio

  mkdir -p $BUILD_DIR/$arch
  # Build conan packages
  CONAN_CMAKE_SYSTEM_PROCESSOR=$arch conan install -if $BUILD_DIR/$arch \
    -pr:b default \
    -pr:h darwin-$arch \
    --build=missing \
    -s build_type=$BUILD_TYPE \
    -b missing .
# Collect the licenses for all the dependencies
python3 "licenses.py" $BUILD_DIR/$arch

  # Build main program
  CONAN_CMAKE_SYSTEM_PROCESSOR=$arch cmake \
    -B $BUILD_DIR/$arch -S $SOURCE_DIR \
    -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/$arch/conan_toolchain.cmake" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  cmake --build $BUILD_DIR/$arch --target Filmvert --config $BUILD_TYPE -j8 || exit 255

done

## Stitch binaries together for universal build
cp -r $BUILD_DIR/arm64/bin/Filmvert.app $BUILD_DIR/
lipo $BUILD_DIR/arm64/bin/Filmvert.app/Contents/MacOS/Filmvert $BUILD_DIR/x86_64/bin/Filmvert.app/Contents/MacOS/Filmvert -create -output $BUILD_DIR/Filmvert.app/Contents/MacOS/Filmvert
