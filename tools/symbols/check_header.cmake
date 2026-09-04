# Checks the promises reliable.h makes that no compiler can check for us.
#
# The two array getters must hand back a const view, and the header must state the lifetime
# of every pointer that reaches a callback. Both are contract, and both are the kind of thing
# that quietly rots, so they are pinned here.
#
# cmake -DRELIABLE_HEADER=<path> -P check_header.cmake

if(NOT RELIABLE_HEADER)
    message(FATAL_ERROR "RELIABLE_HEADER is required")
endif()

file(READ "${RELIABLE_HEADER}" header)

set(failures "")

# const views

if(NOT header MATCHES "RELIABLE_EXPORT RELIABLE_CONST uint16_t \\* reliable_endpoint_get_acks")
    list(APPEND failures "reliable_endpoint_get_acks must return RELIABLE_CONST uint16_t *")
endif()

if(NOT header MATCHES "RELIABLE_EXPORT RELIABLE_CONST uint64_t \\* reliable_endpoint_counters")
    list(APPEND failures "reliable_endpoint_counters must return RELIABLE_CONST uint64_t *")
endif()

# documented pointer lifetimes

if(NOT header MATCHES "pointer lifetimes")
    list(APPEND failures "the header must carry a pointer lifetimes section")
else()
    string(REGEX MATCH "pointer lifetimes.*\n\n" lifetimes "${header}")
    foreach(subject context allocator_context transmit_packet_function process_packet_function
                    allocate_function free_function packet_data)
        if(NOT lifetimes MATCHES "${subject}")
            list(APPEND failures "the pointer lifetimes section says nothing about ${subject}")
        endif()
    endforeach()
endif()

# the array getters say how long their view stays valid

foreach(getter reliable_endpoint_get_acks reliable_endpoint_counters)
    string(FIND "${header}" "${getter}" position)
    if(position EQUAL -1)
        list(APPEND failures "${getter} is not declared")
    endif()
endforeach()

if(NOT header MATCHES "stays valid until the next call to[ \t\r\n/]*reliable_endpoint_receive_packet")
    list(APPEND failures "reliable_endpoint_get_acks must say when its view stops being valid")
endif()

if(failures)
    foreach(failure IN LISTS failures)
        message(SEND_ERROR "${failure}")
    endforeach()
    message(FATAL_ERROR "reliable.h does not keep the contract")
endif()

message(STATUS "reliable.h declares const views and documents every callback pointer lifetime")
