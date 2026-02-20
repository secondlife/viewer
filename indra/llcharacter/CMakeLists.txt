# -*- cmake -*-

add_library(llcharacter STATIC)

target_sources(llcharacter
    PRIVATE
    llanimationstates.cpp
    llbvhloader.cpp
    llcharacter.cpp
    lleditingmotion.cpp
    llgesture.cpp
    llhandmotion.cpp
    llheadrotmotion.cpp
    lljoint.cpp
    lljointsolverrp3.cpp
    llkeyframefallmotion.cpp
    llkeyframemotion.cpp
    llkeyframestandmotion.cpp
    llkeyframewalkmotion.cpp
    llmotioncontroller.cpp
    llmotion.cpp
    llmultigesture.cpp
    llpose.cpp
    lltargetingmotion.cpp
    llvisualparam.cpp
    )

target_sources(llcharacter
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
    llanimationstates.h
    llbvhloader.h
    llbvhconsts.h
    llcharacter.h
    lleditingmotion.h
    llgesture.h
    llhandmotion.h
    llheadrotmotion.h
    lljoint.h
    lljointsolverrp3.h
    lljointstate.h
    llkeyframefallmotion.h
    llkeyframemotion.h
    llkeyframestandmotion.h
    llkeyframewalkmotion.h
    llmotion.h
    llmotioncontroller.h
    llmultigesture.h
    llpose.h
    lltargetingmotion.h
    llvisualparam.h
    )

target_include_directories(llcharacter INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
    llcharacter
    PUBLIC
    llcommon
    llmath
    llmessage
    llfilesystem
    llxml
    )

target_precompile_headers(llcharacter REUSE_FROM llprecompiled)

if (BUILD_TESTING)
#   SET(llcharacter_TEST_SOURCE_FILES
#     )
#   LL_ADD_PROJECT_UNIT_TESTS(llcharacter "${llcharacter_TEST_SOURCE_FILES}")

  #set(TEST_DEBUG on)
  set(test_libs llcommon llmath)
  set(test_project llcharacter)
  LL_ADD_INTEGRATION_TEST(lljoint "lljoint.cpp" "${test_libs}" "${test_project}")
endif()
