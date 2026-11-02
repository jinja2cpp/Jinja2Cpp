# Compare the parity suite with a Python Jinja2 other than the bundled one (docs/tasks/0186,
# test/parity/README.md "Other oracles"). Included from the test block of CMakeLists.txt.
#
#   bundled (default): the checked-in test/parity/expected/; nothing here runs and no Python is needed
#   python:  test/parity/generate.py runs under JINJA2CPP_TEST_ORACLE_PYTHON (default: the
#            Python3 interpreter CMake finds) with whatever jinja2 and markupsafe it has
#   venv:    a venv in the build directory gets JINJA2CPP_TEST_ORACLE_REQUIREMENTS from pip
#
# The other oracle's expectations are generated at test time, by the fixture
# jinja2cpp_parity_oracle_generate, so configure needs no network and a failing pip is a failing
# test. jinja2cpp_parity_oracle then runs the narrow suite against them and writes
# parity-oracle/report.md in the build directory.
set(JINJA2CPP_TEST_ORACLE bundled CACHE STRING "Python Jinja2 the parity suite compares against: bundled | python | venv")
set_property(CACHE JINJA2CPP_TEST_ORACLE PROPERTY STRINGS bundled python venv)
set(JINJA2CPP_TEST_ORACLE_PYTHON "" CACHE FILEPATH "Interpreter for JINJA2CPP_TEST_ORACLE=python, and the base of the venv (default: find_package(Python3))")
set(JINJA2CPP_TEST_ORACLE_REQUIREMENTS "" CACHE STRING
    "pip requirements for JINJA2CPP_TEST_ORACLE=venv, ';'-separated, e.g. 'jinja2==3.1.2;markupsafe==2.1.5' or 'jinja2 @ git+https://github.com/pallets/jinja@main'")

if (JINJA2CPP_TEST_ORACLE STREQUAL "bundled")
    return()
endif ()
if (NOT JINJA2CPP_TEST_ORACLE MATCHES "^(python|venv)$")
    message(FATAL_ERROR "JINJA2CPP_TEST_ORACLE must be bundled, python or venv, not '${JINJA2CPP_TEST_ORACLE}'")
endif ()
if (JINJA2CPP_TEST_ORACLE STREQUAL "venv" AND JINJA2CPP_TEST_ORACLE_REQUIREMENTS STREQUAL "")
    message(FATAL_ERROR "JINJA2CPP_TEST_ORACLE=venv needs JINJA2CPP_TEST_ORACLE_REQUIREMENTS, e.g. 'jinja2==3.1.2'")
endif ()

if (JINJA2CPP_TEST_ORACLE_PYTHON)
    set(_oracle_python "${JINJA2CPP_TEST_ORACLE_PYTHON}")
else ()
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    set(_oracle_python "${Python3_EXECUTABLE}")
endif ()
execute_process(COMMAND "${_oracle_python}" -c "import sys; print('%d.%d' % sys.version_info[:2])"
    OUTPUT_VARIABLE _oracle_python_version OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE _oracle_python_result)
if (NOT _oracle_python_result EQUAL 0)
    message(FATAL_ERROR "JINJA2CPP_TEST_ORACLE: cannot run ${_oracle_python}")
endif ()
# The Unicode tables in src/ come from the pinned Python, so Unicode cases may differ for a
# reason that is not a Jinja2C++ bug
if (NOT _oracle_python_version VERSION_EQUAL JINJA2CPP_ORACLE_PYTHON)
    message(WARNING "JINJA2CPP_TEST_ORACLE: the oracle runs Python ${_oracle_python_version}, but Jinja2C++ follows Python "
        "${JINJA2CPP_ORACLE_PYTHON} (.python-version); cases that depend on the Python version (Unicode data, textwrap, "
        "\\N{} names) can show up in the report as deltas that are not Jinja2 changes")
endif ()
message(STATUS "Parity oracle: ${JINJA2CPP_TEST_ORACLE}, Python ${_oracle_python_version} (${_oracle_python})")

set(_oracle_dir ${CMAKE_CURRENT_BINARY_DIR}/parity-oracle)
set(_oracle_requirements "")
if (JINJA2CPP_TEST_ORACLE STREQUAL "venv")
    # A file, so the list reaches pip without quoting trouble; the generate step reinstalls when it changes
    set(_oracle_requirements ${_oracle_dir}/requirements.txt)
    string(REPLACE ";" "\n" _oracle_requirement_lines "${JINJA2CPP_TEST_ORACLE_REQUIREMENTS}")
    file(WRITE ${_oracle_requirements}.in "${_oracle_requirement_lines}\n")
    configure_file(${_oracle_requirements}.in ${_oracle_requirements} COPYONLY)
endif ()

add_test(NAME jinja2cpp_parity_oracle_generate
    COMMAND ${CMAKE_COMMAND}
        -DPYTHON=${_oracle_python}
        -DREQUIREMENTS=${_oracle_requirements}
        -DGENERATE=${CMAKE_CURRENT_SOURCE_DIR}/test/parity/generate.py
        -DOUT=${_oracle_dir}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/parity_oracle_generate.cmake)
set_tests_properties(jinja2cpp_parity_oracle_generate PROPERTIES FIXTURES_SETUP jinja2cpp_parity_oracle)

# The narrow suite and the registry checks; the wide suite is held to the narrow result, not
# to an oracle, so it has nothing to add
add_test(NAME jinja2cpp_parity_oracle COMMAND jinja2cpp_tests --gtest_filter=Parity/*:ParityRegistry.*)
set_tests_properties(jinja2cpp_parity_oracle PROPERTIES
    FIXTURES_REQUIRED jinja2cpp_parity_oracle
    ENVIRONMENT "JINJA2CPP_TEST_ORACLE_DIR=${_oracle_dir}")
