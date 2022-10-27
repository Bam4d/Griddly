
function(register_nor_target target_name)
    if (NOT ARGN)
        message(FATAL_ERROR "You must pass at least one source file to define the target '${target_name}'")
    endif ()
    set(${target_name}_sources_list ${ARGN})
    # this set needs to be set in the parent scope as well in order to remain present
    set(${target_name}_sources_list ${ARGN} PARENT_SCOPE)
    list(APPEND REGISTERED_TARGET_NAMES_LIST ${target_name})
    list(APPEND REGISTERED_TEST_SOURCES_LIST ${target_name}_sources_list)
    # update the lists in the parent scope, since function holds copies of these variables for its own scope
    set(REGISTERED_TARGET_NAMES_LIST ${REGISTERED_TARGET_NAMES_LIST} PARENT_SCOPE)
    set(REGISTERED_TEST_SOURCES_LIST ${REGISTERED_TEST_SOURCES_LIST} PARENT_SCOPE)

    message(STATUS "Target: ${target_name}")
    message(STATUS "Target Source Contents: ${${target_name}_sources_list}")

    list(TRANSFORM ${target_name}_sources_list PREPEND "${PROJECT_TEST_DIR}/libnor/")

    message(STATUS "Target Source Contents (pathed): ${${target_name}_sources_list}")

    add_executable(
            ${target_name}
            ${PROJECT_TEST_DIR}/main_tests.cpp
            ${${target_name}_sources_list})  # a list of source files to append

    target_link_libraries(
            ${target_name}
            PRIVATE
            shared_test_libs
    )

    add_test(
            NAME Test_${target_name}
            COMMAND ${target_name}
    )
endfunction()


function(register_game_target target_name lib_name game_folder_name)
    if (NOT ARGN)
        message(FATAL_ERROR "You must pass at least one source file to define the target '${target_name}'")
    endif ()
    # append the prefix 'game' and suffix 'test' to each target name
    set(target_name "game_${target_name}_test")
    set(${target_name}_sources_list ${ARGN})
    # this set needs to be set in the parent scope as well in order to remain present
    set(${target_name}_sources_list ${ARGN} PARENT_SCOPE)

    list(APPEND REGISTERED_GAME_TARGET_NAMES_LIST ${target_name})
    list(APPEND REGISTERED_GAME_TEST_SOURCES_LIST ${target_name}_sources_list)
    # update the lists in the parent scope, since function holds copies of these variables for its own scope
    set(REGISTERED_TARGET_NAMES_LIST ${REGISTERED_TARGET_NAMES_LIST} PARENT_SCOPE)
    set(REGISTERED_TEST_SOURCES_LIST ${REGISTERED_TEST_SOURCES_LIST} PARENT_SCOPE)

    message(STATUS "Target: ${target_name}")
    message(STATUS "Target Source Contents: ${${target_name}_sources_list}")
    list(TRANSFORM ${target_name}_sources_list PREPEND "${PROJECT_TEST_DIR}/games/${game_folder_name}/")
    message(STATUS "Target Source Contents (pathed): ${${target_name}_sources_list}")

    add_executable(
            ${target_name}
            ${PROJECT_TEST_DIR}/main_tests.cpp
            ${${target_name}_sources_list})  # a list of source files to append

    set_target_properties(${target_name} PROPERTIES
            EXCLUDE_FROM_ALL True  # don't build tests when ALL is asked to be built. Only on demand.
            )

    target_link_libraries(
            ${target_name}
            PRIVATE
            shared_test_libs
            ${lib_name}
    )

    add_test(
            NAME Test_${target_name}
            COMMAND ${target_name}
    )
endfunction()


# Get all propreties that cmake supports
if(NOT CMAKE_PROPERTY_LIST)
    execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

    # Convert command output into a CMake list
    string(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
    string(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
endif()

function(print_properties)
    message("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction()

function(print_target_properties target)
    if(NOT TARGET ${target})
        message(STATUS "There is no target named '${target}'")
        return()
    endif()

    foreach(property ${CMAKE_PROPERTY_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" property ${property})

        # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
        if(property STREQUAL "LOCATION" OR property MATCHES "^LOCATION_" OR property MATCHES "_LOCATION$")
            continue()
        endif()

        get_property(was_set TARGET ${target} PROPERTY ${property} SET)
        if(was_set)
            get_target_property(value ${target} ${property})
            message("${target} ${property} = ${value}")
        endif()
    endforeach()
endfunction()