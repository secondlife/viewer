# -*- cmake -*-

project(llcharacter)

include(00-Common)
include(LLCommon)

set(llcharacter_SOURCE_FILES
    llanimationstates.cpp
    llbvhloader.cpp
    llcharacter.cpp
    lleditingmotion.cpp
    llgesture.cpp
    llhandmotion.cpp
    llheadrotmotion.cpp
    llik.cpp
    lljoint.cpp
    lljointsolverrp3.cpp
    llkeyframefallmotion.cpp
    llkeyframemotion.cpp
    llkeyframemotionparam.cpp
    llkeyframestandmotion.cpp
    llkeyframewalkmotion.cpp
    llmotioncontroller.cpp
    llmotion.cpp
    llmultigesture.cpp
    llpose.cpp
    llstatemachine.cpp
    lltargetingmotion.cpp
    llvisualparam.cpp
    )

set(llcharacter_HEADER_FILES
    CMakeLists.txt

    llanimationstates.h
    llbvhloader.h
    llbvhconsts.h
    llcharacter.h
    lleditingmotion.h
    llgesture.h
    llhandmotion.h
    llheadrotmotion.h
    llik.h
    lljoint.h
    lljointsolverrp3.h
    lljointstate.h
    llkeyframefallmotion.h
    llkeyframemotion.h
    llkeyframemotionparam.h
    llkeyframestandmotion.h
    llkeyframewalkmotion.h
    llmotion.h
    llmotioncontroller.h
    llmultigesture.h
    llpose.h
    llstatemachine.h
    lltargetingmotion.h
    llvisualparam.h
    )

list(APPEND llcharacter_SOURCE_FILES ${llcharacter_HEADER_FILES})

add_library (llcharacter ${llcharacter_SOURCE_FILES})
target_include_directories( llcharacter  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
        llcharacter
        llcommon
        llmath
        llmessage
        llfilesystem
        llxml
    )

# Add tests
if (LL_TESTS)
  include(LLAddBuildTest)
  # UNIT TESTS
  SET(llcharacter_TEST_SOURCE_FILES
    #llik.cpp
    lljoint.cpp
  )
  set_source_files_properties(
    ${llcharacter_TEST_SOURCE_FILES}
    PROPERTIES
    LL_TEST_ADDITIONAL_LIBRARIES "${BOOST_THREAD_LIBRARY}"
  )
  set_source_files_properties(
    #llik.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_SOURCE_FILES lljoint.cpp
  )
  LL_ADD_PROJECT_UNIT_TESTS(llcharacter "${llcharacter_TEST_SOURCE_FILES}")
endif (LL_TESTS)

