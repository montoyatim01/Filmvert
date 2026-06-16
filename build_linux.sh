#!/bin/bash

set -e

SOURCE_DIR=$PWD
BUILD_TYPE="Debug"
RUN_TESTS=0
INSTALL_PACKAGES=0
## libglvnd-devel, libX11, libX11-common, libX11-xcb,
## libX11-devel, libXau-devel, libglvnd-core-devel,
## libglvnd-opengl, libxcb-devel,xorg-x11-proto-devel
## xkeyboard-config-devel, amongst potential others

for arg in "$@"; do
    case $arg in
        Debug|Release) BUILD_TYPE=$arg ;;
        --tests)       RUN_TESTS=1 ;;
        --ins_preq)     INSTALL_PACKAGES=1 ;;
        *) echo "Unknown argument: $arg"; echo "Usage: $0 [Debug|Release] [--tests] [--ins_preq]"; exit 1 ;;
    esac
done

case $BUILD_TYPE in
    Debug)   BUILD_DIR="Build-debug" ;;
    Release) BUILD_DIR="Build-release" ;;
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
    $([ $INSTALL_PACKAGES -eq 1 ] && echo "-c tools.system.package_manager:mode=install -c tools.system.package_manager:sudo=True")

python3 "licenses.py" $BUILD_DIR

  cmake \
    -B $BUILD_DIR \
    -S $SOURCE_DIR \
    -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/conan_toolchain.cmake" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

  cmake --build $BUILD_DIR --target Filmvert --config $BUILD_TYPE -j8 || exit 255

if [ $RUN_TESTS -eq 1 ]; then
  cmake --build $BUILD_DIR --target filmvert_tests --config $BUILD_TYPE -j8
  ctest --test-dir $BUILD_DIR --output-on-failure
fi
