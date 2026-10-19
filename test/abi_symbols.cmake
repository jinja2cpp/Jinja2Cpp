# Fails when the library defines a jinja2 symbol outside the inline ABI namespace (docs/tasks/0076).
#   cmake -DNM=<nm> -DLIBRARY=<libjinja2cpp.a|.so|.dylib> -P test/abi_symbols.cmake
# Every mangled jinja2:: name must start jinja2::v2:: (`6jinja22v2`), and the vendored
# polymorphic/indirect types must not be left in a global `xyz`, where a user's own copy of
# the upstream headers would collide with them. scripts/abi_namespace.py keeps the sources
# in that shape; this checks what the compiler actually emitted.
foreach (_var NM LIBRARY)
    if (NOT ${_var})
        message(FATAL_ERROR "${_var} is required")
    endif ()
endforeach ()
# Defined symbols only: a static library repeats every reference in each object
execute_process(COMMAND "${NM}" --defined-only "${LIBRARY}" OUTPUT_VARIABLE _symbols ERROR_VARIABLE _err RESULT_VARIABLE _rc)
if (NOT _rc EQUAL 0)
    execute_process(COMMAND "${NM}" "${LIBRARY}" OUTPUT_VARIABLE _symbols ERROR_VARIABLE _err RESULT_VARIABLE _rc)
endif ()
if (NOT _rc EQUAL 0)
    message(FATAL_ERROR "${NM} ${LIBRARY} failed (${_rc}): ${_err}")
endif ()
# Mach-O prefixes every symbol with an extra underscore; the match starts after it
string(REGEX MATCHALL "_Z[^ \n]*" _syms "${_symbols}")
set(_tagged ${_syms})
list(FILTER _tagged INCLUDE REGEX "(^|[^0-9])6jinja22v2")
list(LENGTH _tagged _tagged)
set(_rest ${_syms})
list(TRANSFORM _rest REPLACE "(^|[^0-9])6jinja22v2" "\\1")
list(FILTER _rest INCLUDE REGEX "(^|[^0-9])6jinja2|^_ZN?K?3xyz")
# report the symbols as nm printed them
set(_bad "")
if (_rest)
    foreach (_sym IN LISTS _syms)
        string(REGEX REPLACE "(^|[^0-9])6jinja22v2" "\\1" _stripped "${_sym}")
        if (_stripped MATCHES "(^|[^0-9])6jinja2|^_ZN?K?3xyz")
            list(APPEND _bad "${_sym}")
        endif ()
    endforeach ()
endif ()
list(REMOVE_DUPLICATES _bad)
list(LENGTH _bad _nbad)
if (_nbad GREATER 0)
    list(SUBLIST _bad 0 20 _shown)
    string(REPLACE ";" "\n  " _shown "${_shown}")
    message(FATAL_ERROR "${_nbad} symbols outside jinja2::v2 in ${LIBRARY}, run scripts/abi_namespace.py:\n  ${_shown}")
endif ()
if (_tagged EQUAL 0)
    message(FATAL_ERROR "no jinja2::v2 symbols in ${LIBRARY}: is ${NM} reading the right file?")
endif ()
message(STATUS "${_tagged} jinja2::v2 symbols, none outside it")
