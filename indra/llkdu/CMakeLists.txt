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

set(llkdu_SOURCE_FILES
    llimagej2ckdu.cpp
    llkdumem.cpp
    )

set(llkdu_HEADER_FILES
    CMakeLists.txt
    llimagej2ckdu.h
    llkdumem.h
    )

list(APPEND llkdu_SOURCE_FILES ${llkdu_HEADER_FILES})

# Our KDU package is built with KDU_X86_INTRINSICS in its .vcxproj file.
# Unless that macro is also set for every consumer build, KDU freaks out,
# spamming the viewer log with alignment FUD.
set_source_files_properties(${llkdu_SOURCE_FILES}
                            PROPERTIES
                            COMPILE_DEFINITIONS
                            "KDU_X86_INTRINSICS")

if (USE_KDU)
  add_library (llkdu ${llkdu_SOURCE_FILES})

  target_link_libraries(llkdu ll::kdu llimage llcommon)
  target_include_directories( llkdu  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})

  # Add tests
  # ND: llkdu tests are very strange as they include stubs for KDU classes/methods
  # if not having access to the right KDU version this test will fail to compile, incidentally I do not
  # have access to a matching version of KDU and thus cannot get this tests to compile
  if (LL_TESTS_KDU)
    include(LLAddBuildTest)
    include(Tut)
    SET(llkdu_TEST_SOURCE_FILES
      llimagej2ckdu.cpp
      )
    SET(llkdu_test_additional_HEADER_FILES
      llimagej2ckdu.h
      llkdumem.h
      lltut.h
      )

    get_property( llimage_include_dir TARGET llimage PROPERTY INTERFACE_INCLUDE_DIRECTORIES )
    set_property( SOURCE ${llkdu_TEST_SOURCE_FILES} PROPERTY LL_TEST_ADDITIONAL_LIBRARIES  ll::kdu llcommon)
    set_property( SOURCE ${llkdu_TEST_SOURCE_FILES} PROPERTY LL_TEST_ADDITIONAL_INCLUDE_DIRS ${llimage_include_dir})

    LL_ADD_PROJECT_UNIT_TESTS(llkdu "${llkdu_TEST_SOURCE_FILES}")
  endif (LL_TESTS_KDU)

endif (USE_KDU)
