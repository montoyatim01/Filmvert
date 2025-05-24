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

for arch in arm64
do

  if [ $BUILD_TYPE == Release ]; then
    rm -rf $BUILD_DIR/$arch
  fi

  mkdir -p $BUILD_DIR/$arch
  CONAN_CMAKE_SYSTEM_PROCESSOR=$arch conan install -if $BUILD_DIR/$arch \
    -pr:b default \
    -pr:h darwin-$arch \
    --build=missing \
    -s build_type=$BUILD_TYPE \
    -b missing .

  #Build metal libarary and copy to assets
  CONAN_CMAKE_SYSTEM_PROCESSOR=$arch cmake \
    -B $BUILD_DIR/$arch -S $SOURCE_DIR \
    -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/$arch/conan_toolchain.cmake" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  cmake --build $BUILD_DIR/$arch --target make-metallib --config $BUILD_TYPE || exit 255
  cp $BUILD_DIR/$arch/default.metallib assets/default.metallib


  CONAN_CMAKE_SYSTEM_PROCESSOR=$arch cmake \
    -B $BUILD_DIR/$arch -S $SOURCE_DIR \
    -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/$arch/conan_toolchain.cmake" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  cmake --build $BUILD_DIR/$arch --target Filmvert --config $BUILD_TYPE -j8 || exit 255

done
