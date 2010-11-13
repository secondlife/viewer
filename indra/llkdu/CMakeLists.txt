# -*- cmake -*-

project(llkdu)

# Visual Studio 2005 has a dumb bug that causes it to fail compilation
# of KDU if building with both optimisation and /WS (treat warnings as
# errors), even when the specific warnings that make it croak are
# disabled.

set(VS_DISABLE_FATAL_WARNINGS ON)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLKDU)
include(LLMath)
#include(LLVFS)
#include(Linking)

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

if (WINDOWS)
  # This turns off the warning about flow control ending in a destructor.
  set_source_files_properties(
      kdu_image.cpp llkdumem.cpp 
      PROPERTIES
      COMPILE_FLAGS "/wd4702 /wd4722"
      )

  # This turns off the warning about sprintf in the following 2 files.
  set_source_files_properties(
      kde_flow_control.cpp kdc_flow_control.cpp
      PROPERTIES
      COMPILE_FLAGS /D_CRT_SECURE_NO_DEPRECATE
      )
endif (WINDOWS)

if (LLKDU_LIBRARY)
  add_library (${LLKDU_LIBRARY} ${llkdu_SOURCE_FILES})
  
  target_link_libraries(
        ${LLKDU_LIBRARY}
        ${LLMATH_LIBRARIES}
        ${KDU_LIBRARY}
        )
endif (LLKDU_LIBRARY)
