# -*- cmake -*-

project(llappearance)

include(00-Common)
include(LLCommon)
include(Viewer)
include(LLAudio)
include(LLCharacter)
include(LLCommon)
include(LLImage)
include(LLImageJ2COJ)
include(LLInventory)
include(LLMath)
include(LLMessage)
include(LLPhysicsExtensions)
include(LLPlugin)
include(LLPrimitive)
include(LLRender)
include(LLUI)
include(LLVFS)
include(LLWindow)
include(LLXML)
include(Linking)
include(LLKDU)
include(ViewerMiscLibs)
include(LLLogin)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCHARACTER_INCLUDE_DIRS}
    ${LLPHYSICS_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLKDU_INCLUDE_DIRS}
    ${LLINVENTORY_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLUI_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    )

set(llappearance_SOURCE_FILES
    llavatarappearance.cpp
    llinventoryicon.cpp
    lllocaltextureobject.cpp
    lltexglobalcolor.cpp
    lltexlayer.cpp
    lltexlayerparams.cpp
    lltexturemanagerbridge.cpp
    llwearable.cpp
    llwearabletype.cpp
    llviewervisualparam.cpp
    llavatarappearancedefines.cpp
    )
    
set(llappearance_HEADER_FILES
    CMakeLists.txt

    llavatarappearance.h
    llinventoryicon.h
    lljointpickname.h
    lllocaltextureobject.h
    lltexglobalcolor.h
    lltexlayer.h
    lltexlayerparams.h
    lltexturemanagerbridge.h
    llwearable.h
    llwearabletype.h
    llviewervisualparam.h
    llavatarappearancedefines.h
    )

set_source_files_properties(${llappearance_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llappearance_SOURCE_FILES ${llappearance_HEADER_FILES})

add_library (llappearance ${llappearance_SOURCE_FILES})

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
