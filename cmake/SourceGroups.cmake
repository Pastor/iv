#include(SourceGroups)

set(SSRC ${CMAKE_SOURCE_DIR})
set(BSRC ${CMAKE_BINARY_DIR})

set(_CPP ".*\\.cpp")
set(CPP "${_CPP}$")

set(_H ".*\\.h")
set(H "${_H}$")

set(H_CPP "(${H}|${CPP})")

source_group("Resource files" ".*\\.(rc|ico)")

source_group("restserver"          "${SSRC}/restserver/${H_CPP}")
#source_group("ccmain"       "${SSRC}/ccmain/${H_CPP}")
