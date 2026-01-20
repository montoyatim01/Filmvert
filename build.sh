#!/bin/bash

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
    --settings:host arch=x86_64 \
    --profile:host=default \
    --profile:build=default \
    -c tools.system.package_manager:mode=skip

python "licenses.py" $BUILD_DIR

  cmake \
    -B $BUILD_DIR \
    -S $SOURCE_DIR \
    -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/conan_toolchain.cmake" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

  cmake --build $BUILD_DIR --target Filmvert --config $BUILD_TYPE -j8 || exit 255
