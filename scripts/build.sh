#!/usr/bin/env bash

set -ex

export BUILD_TARGET="all"
export CMAKE_OPTS="-DCMAKE_VERBOSE_MAKEFILE=OFF"
if [[ "${COMPILER}" != "" ]]; then export CXX=${COMPILER}; fi
if [[ "${BUILD_CONFIG}" == "" ]]; then export BUILD_CONFIG="Release"; fi
if [[ "${COLLECT_COVERAGE}" != "" ]]; then export BUILD_CONFIG="Debug" && export CMAKE_OPTS="${CMAKE_OPTS} -DJINJA2CPP_WITH_COVERAGE=ON"; fi
if [[ "${SANITIZE_BUILD}" != "" ]]; then export BUILD_CONFIG="RelWithDebInfo" && export CMAKE_OPTS="${CMAKE_OPTS} -DJINJA2CPP_WITH_SANITIZERS=${SANITIZE_BUILD}"; fi
$CXX --version
mkdir -p build && cd build
cmake $CMAKE_OPTS -DCMAKE_BUILD_TYPE=$BUILD_CONFIG -DCMAKE_CXX_FLAGS=$CMAKE_CXX_FLAGS -DJINJA2CPP_DEPS_MODE=internal $EXTRA_FLAGS .. && cmake --build . --config $BUILD_CONFIG --target all -- -j4
ctest -C $BUILD_CONFIG -V
