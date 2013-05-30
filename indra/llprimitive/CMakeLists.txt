# -*- cmake -*-

project(llprimitive)

include(00-Common)
include(LLCommon)
include(LLMath)
include(LLMessage)
include(LLXML)
include(LLPhysicsExtensions)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${LIBS_PREBUILT_DIR}/include/collada
    ${LIBS_PREBUILT_DIR}/include/collada/1.4
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    ${LLXML_SYSTEM_INCLUDE_DIRS}
    ${LLPHYSICSEXTENSIONS_INCLUDE_DIRS}
    )

set(llprimitive_SOURCE_FILES
    llmaterialtable.cpp
    llmediaentry.cpp
    llmodel.cpp
    llprimitive.cpp
    llprimtexturelist.cpp
    lltextureanim.cpp
    lltextureentry.cpp
    lltreeparams.cpp
    llvolumemessage.cpp
    llvolumexml.cpp
    material_codes.cpp
    )

set(llprimitive_HEADER_FILES
    CMakeLists.txt

    legacy_object_types.h
    llmaterialtable.h
    llmediaentry.h
    llmodel.h
    llprimitive.h
    llprimtexturelist.h
    lltextureanim.h
    lltextureentry.h
    lltreeparams.h
    lltree_common.h
    llvolumemessage.h
    llvolumexml.h
    material_codes.h
    object_flags.h
    )
        
set_source_files_properties(${llprimitive_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llprimitive_SOURCE_FILES ${llprimitive_HEADER_FILES})

add_library (llprimitive ${llprimitive_SOURCE_FILES})

target_link_libraries(llprimitive
    ${LLCOMMON_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${LLXML_LIBRARIES}
    ${LLPHYSICSEXTENSIONS_LIBRARIES}
    )


#add unit tests
if (LL_TESTS)
    INCLUDE(LLAddBuildTest)
    SET(llprimitive_TEST_SOURCE_FILES
      llmediaentry.cpp
      )
    LL_ADD_PROJECT_UNIT_TESTS(llprimitive "${llprimitive_TEST_SOURCE_FILES}")
endif (LL_TESTS)
