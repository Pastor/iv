cmake_minimum_required(VERSION 2.8.11)

# In-source builds are disabled.
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR
        "CMake generation is not possible within the source directory!"
        "\n Remove the CMakeCache.txt file and try again from another folder, e.g.:"
        "\n "
        "\n rm CMakeCache.txt"
        "\n mkdir build"
        "\n cd build"
        "\n cmake .."
    )
endif()

#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")

# Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake Targets")

###############################################################################
#
# project settings
#
###############################################################################

project(main C CXX)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PLAIN ${VERSION_MAJOR}.${VERSION_MINOR})

#set(SQLite_URL       "https://www.sqlite.org/snapshot/sqlite-amalgamation-201601141433.zip")
#set(SQLite_ARCHIVE   "${CMAKE_CURRENT_BINARY_DIR}/sqlite.zip")
#set(SQLite_HASH      "ce0f8700274488fbd4351dba2f82a2822125807d")
#set(SQLite_DIR       "${CMAKE_CURRENT_BINARY_DIR}/sqlite-current")

#file(DOWNLOAD
#    ${SQLite_URL}
#    ${SQLite_ARCHIVE}
#    SHOW_PROGRESS
#    EXPECTED_HASH SHA1=${SQLite_HASH}
#)
#execute_process(COMMAND mkdir ${SQLite_DIR})
#execute_process(COMMAND ${CMAKE_COMMAND} -E tar xz "${SQLite_ARCHIVE}"
#    WORKING_DIRECTORY "${SQLite_DIR}"
#    RESULT_VARIABLE __result
#)
#if(NOT __result EQUAL 0)
#    message(FATAL_ERROR "error ${__result}")
#endif()


###############################################################################
#
# compiler and linker
#
###############################################################################

set(LIBRARY_TYPE SHARED)
if (STATIC)
    set(LIBRARY_TYPE)
endif()

if (WIN32)
    if (MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /wd4267 /wd4244 /wd4996")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /MP /wd4267 /wd4244 /wd4996")
    endif()
    
    set(LIB_Ws2_32 Ws2_32)
    link_directories(3rdparty/ftdi/x86)
endif()

if (CYGWIN)
    add_definitions(-D__CYGWIN__)
endif()

if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
    
    set(LIB_pthread pthread)
endif()

###############################################################################
#
# configure
#
###############################################################################

include(Configure)

###############################################################################
#
# build
#
###############################################################################

include(BuildFunctions)
include(SourceGroups)

add_definitions(-DHAVE_CONFIG_H)
add_definitions(-D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS=1)
add_definitions(-DUSE_STD_NAMESPACE=1)
add_definitions(-DMG_ENABLE_THREADS)
add_definitions(-DSQLITE_THREADSAFE=1)
#add_definitions(-DFTD2XX_STATIC)

include_directories(${ADD_INCLUDE_DIR})
include_directories(${CMAKE_BINARY_DIR})

if (WIN32)
    string(REPLACE "\\" "/" Mongoose_DIR         ${Mongoose_DIR})
    string(REPLACE "\\" "/" SQLite_DIR           ${SQLite_DIR})
endif()

include_directories(ev)
include_directories(${Mongoose_DIR})
include_directories(${SQLite_DIR})
include_directories(3rdparty/ftdi)
include_directories(3rdparty/rawhid)

########################################
# LIBRARY
########################################

#string(SUBSTRING ${VERSION_MINOR} 0 1 VERSION_MINOR_0)
#string(SUBSTRING ${VERSION_MINOR} 1 1 VERSION_MINOR_1)

file(GLOB server_src
    "server/*.cpp"
    "server/*.c"
)
file(GLOB server_hdr
    "server/*.h"
)

set(other_src ${other_src}
     ${Mongoose_DIR}/mongoose.c
     ${SQLite_DIR}/sqlite3.c
     3rdparty/rawhid/hid_WINDOWS.c
)
set(other_hdr ${other_hdr}
     ${Mongoose_DIR}/mongoose.h
     ${SQLite_DIR}/sqlite3.h
     3rdparty/rawhid/hid.h
)

########################################
# EXECUTABLE server
########################################

add_executable                  (server ${server_src} ${other_src})
target_link_libraries           (server ${LIB_Ws2_32} ${LIB_pthread} ftd2xx setupapi)
if(UNIX)
    target_link_libraries       (server dl)
endif()
set_target_properties           (server PROPERTIES OUTPUT_NAME server)
set_target_properties           (server PROPERTIES VERSION ${VERSION_MAJOR}.${VERSION_MINOR_0}.${VERSION_MINOR_1})
export(TARGETS server FILE ${CMAKE_BINARY_DIR}/ServerTargets.cmake)

########################################
