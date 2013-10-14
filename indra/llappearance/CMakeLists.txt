# -*- cmake -*-

project(llappearance)

include(00-Common)
include(LLCommon)
include(LLCharacter)
include(LLImage)
include(LLInventory)
include(LLMath)
include(LLMessage)
include(LLRender)
include(LLVFS)
include(LLWindow)
include(LLXML)
include(Linking)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCHARACTER_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLINVENTORY_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    )

set(llappearance_SOURCE_FILES
    llavatarappearance.cpp
    llavatarjoint.cpp
    llavatarjointmesh.cpp
    lldriverparam.cpp
    lllocaltextureobject.cpp
    llpolyskeletaldistortion.cpp
    llpolymesh.cpp
    llpolymorph.cpp
    lltexglobalcolor.cpp
    lltexlayer.cpp
    lltexlayerparams.cpp
    lltexturemanagerbridge.cpp
    llwearable.cpp
    llwearabledata.cpp
    llwearabletype.cpp
    llviewervisualparam.cpp
    llavatarappearancedefines.cpp
    )
    
set(llappearance_HEADER_FILES
    CMakeLists.txt

    llavatarappearance.h
    llavatarjoint.h
    llavatarjointmesh.h
    lldriverparam.h
    lljointpickname.h
    lllocaltextureobject.h
    llpolyskeletaldistortion.h
    llpolymesh.h
    llpolymorph.h
    lltexglobalcolor.h
    lltexlayer.h
    lltexlayerparams.h
    lltexturemanagerbridge.h
    llwearable.h
    llwearabledata.h
    llwearabletype.h
    llviewervisualparam.h
    llavatarappearancedefines.h
    )

set_source_files_properties(${llappearance_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llappearance_SOURCE_FILES ${llappearance_HEADER_FILES})

add_library (llappearance ${llappearance_SOURCE_FILES})

target_link_libraries(llappearance
    ${LLCHARACTER_LIBRARIES}
    ${LLINVENTORY_LIBRARIES}
    ${LLIMAGE_LIBRARIES}
    ${LLRENDER_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLXML_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    )

if (BUILD_HEADLESS)
  add_library (llappearanceheadless ${llappearance_SOURCE_FILES})
  
  target_link_libraries(llappearanceheadless
      ${LLCHARACTER_LIBRARIES}
      ${LLINVENTORY_LIBRARIES}
      ${LLIMAGE_LIBRARIES}
      ${LLRENDERHEADLESS_LIBRARIES}
      ${LLVFS_LIBRARIES}
      ${LLMATH_LIBRARIES}
      ${LLXML_LIBRARIES}
      ${LLMATH_LIBRARIES}
      ${LLCOMMON_LIBRARIES}
      )
endif (BUILD_HEADLESS)

#add unit tests
#if (LL_TESTS)
#    INCLUDE(LLAddBuildTest)
#    SET(llappearance_TEST_SOURCE_FILES
#      # no real unit tests yet!
#      )
#    LL_ADD_PROJECT_UNIT_TESTS(llappearance "${llappearance_TEST_SOURCE_FILES}")

    #set(TEST_DEBUG on)
#    set(test_libs llappearance ${LLCOMMON_LIBRARIES})
#endif (LL_TESTS)
