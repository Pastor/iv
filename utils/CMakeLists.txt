cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake Targets")


foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
   if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
   endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)

project(utils C CXX)

########################################
# hget
########################################
set(HGET_VERSION_MAJOR 1)
set(HGET_VERSION_MINOR 0)
set(HGET_VERSION_PLAIN ${HGET_VERSION_MAJOR}.${HGET_VERSION_MINOR})


file(GLOB hget_src
    "hget.cpp"
)
add_executable                  (hget ${hget_src})
target_link_libraries           (hget Urlmon)
set_target_properties           (hget PROPERTIES OUTPUT_NAME hget)
set_target_properties           (hget PROPERTIES VERSION ${HGET_VERSION_PLAIN})
export(TARGETS hget FILE ${CMAKE_BINARY_DIR}/HGetTargets.cmake)

########################################
