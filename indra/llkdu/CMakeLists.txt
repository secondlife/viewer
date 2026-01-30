# -*- cmake -*-
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

if (USE_KDU)
  add_library (llkdu ${llkdu_SOURCE_FILES})

  target_link_libraries(llkdu ll::kdu llimage llcommon)
  target_include_directories( llkdu  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})

  # Add tests
  if (LL_TESTS)
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
  endif (LL_TESTS)

endif (USE_KDU)
