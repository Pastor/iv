function(check_includes files)
    foreach(F ${${files}})
        set(name ${F})
        string(REPLACE "-" "_" name ${name})
        string(REPLACE "." "_" name ${name})
        string(REPLACE "/" "_" name ${name})
        string(TOUPPER ${name} name)
        check_include_files(${F} HAVE_${name})
        file(APPEND ${AUTOCONFIG_SRC} "/* Define to 1 if you have the <${F}> header file. */\n")
        file(APPEND ${AUTOCONFIG_SRC} "#cmakedefine HAVE_${name} 1\n")
        file(APPEND ${AUTOCONFIG_SRC} "\n")
    endforeach()
endfunction(check_includes)

########################################
# FUNCTION check_functions
########################################
function(check_functions functions)
    foreach(F ${${functions}})
        set(name ${F})
        string(TOUPPER ${name} name)
        check_function_exists(${F} HAVE_${name})
        file(APPEND ${AUTOCONFIG_SRC} "/* Define to 1 if you have the `${F}' function. */\n")
        file(APPEND ${AUTOCONFIG_SRC} "#cmakedefine HAVE_${name} 1\n")
        file(APPEND ${AUTOCONFIG_SRC} "\n")
    endforeach()
endfunction(check_functions)

########################################
# FUNCTION check_types
########################################
function(check_types types)
    foreach(T ${${types}})
        set(name ${T})
        string(REPLACE " " "_" name ${name})
        string(REPLACE "-" "_" name ${name})
        string(REPLACE "." "_" name ${name})
        string(REPLACE "/" "_" name ${name})
        string(TOUPPER ${name} name)
        check_type_size(${T} HAVE_${name})
        file(APPEND ${AUTOCONFIG_SRC} "/* Define to 1 if the system has the type `${T}'. */\n")
        file(APPEND ${AUTOCONFIG_SRC} "#cmakedefine HAVE_${name} 1\n")
        file(APPEND ${AUTOCONFIG_SRC} "\n")
    endforeach()
endfunction(check_types)

########################################

include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckPrototypeDefinition)
include(CheckStructHasMember)
include(CheckSymbolExists)
include(CheckTypeSize)
include(TestBigEndian)
