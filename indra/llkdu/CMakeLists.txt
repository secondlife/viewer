# -*- cmake -*-

project(llkdu)

# Visual Studio 2005 has a dumb bug that causes it to fail compilation
# of KDU if building with both optimisation and /WS (treat warnings as
# errors), even when the specific warnings that make it croak are
# disabled.

#set(VS_DISABLE_FATAL_WARNINGS ON)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLKDU)
include(LLMath)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${KDU_INCLUDE_DIR}
    ${LLMATH_INCLUDE_DIRS}
    )

set(llkdu_SOURCE_FILES
    llimagej2ckdu.cpp
    llkdumem.cpp
    )

set(llkdu_HEADER_FILES
    CMakeLists.txt
	
    llimagej2ckdu.h
    llkdumem.h
    )

set_source_files_properties(${llkdu_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llkdu_SOURCE_FILES ${llkdu_HEADER_FILES})

if (USE_KDU)
  add_library (${LLKDU_LIBRARIES} ${llkdu_SOURCE_FILES})
  
endif (USE_KDU)
