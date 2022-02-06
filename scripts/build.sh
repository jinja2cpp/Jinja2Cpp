#!/usr/bin/env bash

set -ex

export BUILD_TARGET="all"
export CMAKE_OPTS="-DCMAKE_VERBOSE_MAKEFILE=OFF"
[[ "${COMPILER}" != "" ]] && export CXX=${COMPILER}
[[ "${BUILD_CONFIG}" == "" ]] && export BUILD_CONFIG="Release"
[[ "${COLLECT_COVERAGE}" != "" ]] && export BUILD_CONFIG="Debug" CMAKE_OPTS="${CMAKE_OPTS} -DJINJA2CPP_WITH_COVERAGE=ON"
[[ "${SANITIZE_BUILD}" != "" ]] && export BUILD_CONFIG="RelWithDebInfo" CMAKE_OPTS="${CMAKE_OPTS} -DJINJA2CPP_WITH_SANITIZERS=${SANITIZE_BUILD}"
$CXX --version
mkdir -p build && cd build
cmake $CMAKE_OPTS -DCMAKE_BUILD_TYPE=$BUILD_CONFIG -DCMAKE_CXX_FLAGS=$CMAKE_CXX_FLAGS -DJINJA2CPP_DEPS_MODE=internal $EXTRA_FLAGS .. && cmake --build . --config $BUILD_CONFIG --target all -- -j4
ctest -C $BUILD_CONFIG -V
