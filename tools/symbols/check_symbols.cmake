cmake_minimum_required(VERSION 3.15)

# Checks that the library exports exactly the surface reliable.h declares.
#
# The set of names reliable.h marks RELIABLE_EXPORT is the contract. Every global symbol the
# compiled library defines must be one of them: anything else is an internal helper that
# escaped, which is how a caller ends up depending on a function that is free to change.
#
# cmake -DRELIABLE_LIBRARY=<path> -DRELIABLE_HEADER=<path> -DRELIABLE_NM=<nm> -P check_symbols.cmake

if(NOT RELIABLE_LIBRARY OR NOT RELIABLE_HEADER OR NOT RELIABLE_NM)
    message(FATAL_ERROR "RELIABLE_LIBRARY, RELIABLE_HEADER and RELIABLE_NM are all required")
endif()

# the declared surface

file(READ "${RELIABLE_HEADER}" header)
string(REGEX MATCHALL "RELIABLE_EXPORT[^;]*;" declarations "${header}")

set(declared "")
foreach(declaration IN LISTS declarations)
    if(declaration MATCHES "(reliable_[A-Za-z0-9_]+)[ \t\r\n]*\\(")
        list(APPEND declared "${CMAKE_MATCH_1}")
    elseif(declaration MATCHES "\\(\\*(reliable_[A-Za-z0-9_]+)\\)")
        list(APPEND declared "${CMAKE_MATCH_1}")
    endif()
endforeach()
list(REMOVE_DUPLICATES declared)
list(SORT declared)
list(LENGTH declared num_declared)

# the defined surface

execute_process(COMMAND "${RELIABLE_NM}" -g "${RELIABLE_LIBRARY}"
                OUTPUT_VARIABLE nm_output RESULT_VARIABLE nm_result ERROR_VARIABLE nm_error)
if(NOT nm_result EQUAL 0)
    message(FATAL_ERROR "nm failed on ${RELIABLE_LIBRARY}: ${nm_error}")
endif()

string(REPLACE "\n" ";" nm_lines "${nm_output}")
set(defined "")
foreach(line IN LISTS nm_lines)
    # "<address> <type> <name>", or "<type> <name>" for an undefined symbol
    if(line MATCHES "^[0-9a-fA-F]* *([A-Za-z]) _?(reliable_[A-Za-z0-9_]+)$")
        if(NOT CMAKE_MATCH_1 STREQUAL "U")
            list(APPEND defined "${CMAKE_MATCH_2}")
        endif()
    endif()
endforeach()
list(REMOVE_DUPLICATES defined)
list(SORT defined)
list(LENGTH defined num_defined)

# every global the library defines has to be one the header declares

set(unexpected "")
foreach(symbol IN LISTS defined)
    if(NOT symbol IN_LIST declared)
        list(APPEND unexpected "${symbol}")
    endif()
endforeach()

set(missing "")
foreach(symbol IN LISTS declared)
    if(NOT symbol IN_LIST defined)
        list(APPEND missing "${symbol}")
    endif()
endforeach()

message(STATUS "reliable.h declares ${num_declared} exported names, the library defines ${num_defined}")

if(unexpected)
    message(FATAL_ERROR "the library exports symbols reliable.h does not declare: ${unexpected}")
endif()

if(missing)
    message(FATAL_ERROR "reliable.h declares symbols the library does not define: ${missing}")
endif()

message(STATUS "the exported surface matches reliable.h exactly")
