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

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/../../cmake")

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

project(support C CXX)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PLAIN ${VERSION_MAJOR}.${VERSION_MINOR})

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
    link_directories(../../3rdparty/ftdi/${PROJECT_PLATFORM})
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

include_directories(src)
include_directories(../../3rdparty/ftdi)
include_directories(../../3rdparty/rawhid)

include_directories(${JAVA_HOME}/include)
if (WIN32)
    include_directories(${JAVA_HOME}/include/win32)
endif()


########################################
# LIBRARY
########################################

#string(SUBSTRING ${VERSION_MINOR} 0 1 VERSION_MINOR_0)
#string(SUBSTRING ${VERSION_MINOR} 1 1 VERSION_MINOR_1)

file(GLOB support_src
    "src/*.cpp"
    "src/*.c"
)
file(GLOB support_hdr
    "src/*.h"
)

set(other_src ${other_src}
     ../../3rdparty/rawhid/hid_WINDOWS.c
)
set(other_hdr ${other_hdr}
     ../../3rdparty/rawhid/hid.h
)

########################################
# EXECUTABLE support
########################################

add_library                     (support SHARED ${support_src} ${other_src})
target_link_libraries           (support ${LIB_Ws2_32} ${LIB_pthread} ftd2xx setupapi)
if(UNIX)
    target_link_libraries       (support dl)
endif()
set_target_properties           (support PROPERTIES OUTPUT_NAME support)
set_target_properties           (support PROPERTIES VERSION ${VERSION_MAJOR}.${VERSION_MINOR_0}.${VERSION_MINOR_1})
export(TARGETS support FILE ${CMAKE_BINARY_DIR}/ServerTargets.cmake)

########################################
add_custom_command(
    TARGET support
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/../../../../3rdparty/ftdi/${PROJECT_PLATFORM}/ftd2xx.dll ${PROJECT_BINARY_DIR}/../bin
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/support.dll ${PROJECT_BINARY_DIR}/../bin
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/support.lib ${PROJECT_BINARY_DIR}/../lib
)
