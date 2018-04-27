macro (BuildThirdparty TargetName ThirdpartySrcPath ThirdpartyOutFile)

    set (BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/${TargetName}/build)
    set (INST_DIR ${CMAKE_CURRENT_BINARY_DIR}/${TargetName}/install)

    make_directory (${BUILD_DIR})
    message (STATUS '${ThirdpartySrcPath}')
    message (STATUS ">>> Thirdparty build type: ${CMAKE_BUILD_TYPE}")
    message (STATUS ">>> Thirdparty build type: ${CMAKE_CXX_FLAGS}")

    add_custom_command (
        OUTPUT ${BUILD_DIR}/CMakeCache.txt
        COMMAND ${CMAKE_COMMAND} ARGS -G "${CMAKE_GENERATOR}" -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX="${INST_DIR}" -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} "${ThirdpartySrcPath}"
        WORKING_DIRECTORY ${BUILD_DIR}
        COMMENT "Prebuild ${TargetName} library"
    )

    add_custom_command (
        OUTPUT ${INST_DIR}/${ThirdpartyOutFile}
        COMMAND ${CMAKE_COMMAND} ARGS --build . --target install -- VERBOSE=1
        WORKING_DIRECTORY ${BUILD_DIR}
        COMMENT "Build ${TargetName} library"
        DEPENDS ${BUILD_DIR}/CMakeCache.txt
        )

    add_custom_command (
        OUTPUT ${INST_DIR}/.build-${TargetName}
        COMMAND ${CMAKE_COMMAND} ARGS -E touch ./.build-${TargetName}
        WORKING_DIRECTORY ${INST_DIR}
        COMMENT "Finishing ${TargetName} library building"
        DEPENDS ${INST_DIR}/${ThirdpartyOutFile}
        )

    add_custom_target (
            ${TargetName} ALL
            COMMENT "Build ${TargetName} libraries"
            DEPENDS ${INST_DIR}/.build-${TargetName}
    )
endmacro ()
