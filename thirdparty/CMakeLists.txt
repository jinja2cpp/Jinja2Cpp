cmake_minimum_required(VERSION 3.0)

project (thirdparty)
list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

include (build_thirdparty)

BuildThirdparty(gtest ${CMAKE_CURRENT_SOURCE_DIR}/gtest include/gmock/gmock.h )
