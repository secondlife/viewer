# -*- cmake -*-

include(00-Common)
include(LLCommon)
include(LLMath)
include(LScript)

set(lscript_library_SOURCE_FILES
    lscript_alloc.cpp
    lscript_export.cpp
    lscript_library.cpp
    )

set(lscript_library_HEADER_FILES
    CMakeLists.txt

    ../lscript_library.h
    ../lscript_export.h
    )

set_source_files_properties(${lscript_library_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND lscript_library_SOURCE_FILES ${lscript_library_HEADER_FILES})

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LSCRIPT_INCLUDE_DIRS}
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )

add_library (lscript_library ${lscript_library_SOURCE_FILES})
