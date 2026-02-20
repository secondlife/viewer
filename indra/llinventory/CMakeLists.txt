# -*- cmake -*-

add_library(llinventory STATIC)

target_sources(llinventory
    PRIVATE
    llcategory.cpp
    llfoldertype.cpp
    llinventory.cpp
    llinventorydefines.cpp
    llinventorysettings.cpp
    llinventorytype.cpp
    lllandmark.cpp
    llnotecard.cpp
    llparcel.cpp
    llpermissions.cpp
    llsaleinfo.cpp
    llsettingsbase.cpp
    llsettingsdaycycle.cpp
    llsettingssky.cpp
    llsettingswater.cpp
    lltransactionflags.cpp
    lluserrelations.cpp
    )

target_sources(llinventory
    PUBLIC
    FILE_SET HEADERS
    FILES
    llcategory.h
    llfoldertype.h
    llinventory.h
    llinventorydefines.h
    llinventorysettings.h
    llinventorytype.h
    llinvtranslationbrdg.h
    lllandmark.h
    llnotecard.h
    llparcel.h
    llparcelflags.h
    llpermissions.h
    llpermissionsflags.h
    llsaleinfo.h
    llsettingsbase.h
    llsettingsdaycycle.h
    llsettingssky.h
    llsettingswater.h
    lltransactionflags.h
    lltransactiontypes.h
    lluserrelations.h
    )

target_link_libraries(llinventory PUBLIC llcommon llmath llmessage llxml)
target_include_directories( llinventory  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(llinventory REUSE_FROM llprecompiled)

# add unit tests
if (BUILD_TESTING)
    INCLUDE(LLAddBuildTest)
    SET(llinventory_TEST_SOURCE_FILES
      # no real unit tests yet!
      )
    LL_ADD_PROJECT_UNIT_TESTS(llinventory "${llinventory_TEST_SOURCE_FILES}")

    #set(TEST_DEBUG on)
    set(test_libs llinventory llmath llcorehttp llfilesystem )
    set(test_project llinventory)
    LL_ADD_INTEGRATION_TEST(inventorymisc "" "${test_libs}" "${test_project}")
    LL_ADD_INTEGRATION_TEST(llparcel "" "${test_libs}" "${test_project}")
    LL_ADD_INTEGRATION_TEST(llpermissions "" "${test_libs}" "${test_project}")
    LL_ADD_INTEGRATION_TEST(llsaleinfo "" "${test_libs}" "${test_project}")
    LL_ADD_INTEGRATION_TEST(lluserrelations "" "${test_libs}" "${test_project}")
endif ()
