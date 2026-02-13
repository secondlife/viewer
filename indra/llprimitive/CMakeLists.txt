# -*- cmake -*-

add_library (llprimitive STATIC)

target_sources(llprimitive
    PRIVATE
    lldaeloader.cpp
    llgltfmaterial.cpp
    llmaterialid.cpp
    llmaterial.cpp
    llmaterialtable.cpp
    llmediaentry.cpp
    llmodel.cpp
    llmodelloader.cpp
    llprimitive.cpp
    llprimtexturelist.cpp
    lltextureanim.cpp
    lltextureentry.cpp
    lltreeparams.cpp
    llvolumemessage.cpp
    material_codes.cpp

    PUBLIC
    FILE_SET HEADERS
    FILES
    CMakeLists.txt
    lldaeloader.h
    llgltfmaterial.h
    llgltfmaterial_templates.h
    legacy_object_types.h
    llmaterial.h
    llmaterialid.h
    llmaterialtable.h
    llmediaentry.h
    llmodel.h
    llmodelloader.h
    llprimitive.h
    llprimtexturelist.h
    lllslconstants.h
    lltextureanim.h
    lltextureentry.h
    lltreeparams.h
    lltree_common.h
    llvolumemessage.h
    material_codes.h
    object_flags.h
    )

target_include_directories( llprimitive  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(llprimitive
        PUBLIC
        llcharacter
        llrender
        llmessage
        llcorehttp
        llxml
        llcommon
        llmath
        ll::colladadom
        ll::glm
        )

if (HAVOK OR HAVOK_TPV)
    target_link_libraries(llprimitive PUBLIC
        llphysicsextensions_impl
        )
else()
    target_link_libraries(llprimitive PUBLIC
        llphysicsextensionsos
        )
endif ()

target_precompile_headers(llprimitive REUSE_FROM llprecompiled)

# add unit tests
if (BUILD_TESTING)
    INCLUDE(LLAddBuildTest)
    SET(llprimitive_TEST_SOURCE_FILES
      llmediaentry.cpp
      llprimitive.cpp
      llgltfmaterial.cpp
      )

    set_property(SOURCE llprimitive.cpp PROPERTY LL_TEST_ADDITIONAL_LIBRARIES llmessage)
    LL_ADD_PROJECT_UNIT_TESTS(llprimitive "${llprimitive_TEST_SOURCE_FILES}")
endif ()
