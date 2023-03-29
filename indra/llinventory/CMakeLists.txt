# -*- cmake -*-

project(llinventory)

include(00-Common)
include(LLCommon)
include(LLCoreHttp)

set(llinventory_SOURCE_FILES
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
    
set(llinventory_HEADER_FILES
    CMakeLists.txt

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

list(APPEND llinventory_SOURCE_FILES ${llinventory_HEADER_FILES})

add_library (llinventory ${llinventory_SOURCE_FILES})

target_link_libraries( llinventory llcommon llmath llmessage llxml )
target_include_directories( llinventory  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})

#add unit tests
if (LL_TESTS)
    INCLUDE(LLAddBuildTest)
    SET(llinventory_TEST_SOURCE_FILES
      # no real unit tests yet!
      )
    LL_ADD_PROJECT_UNIT_TESTS(llinventory "${llinventory_TEST_SOURCE_FILES}")

    #set(TEST_DEBUG on)
    set(test_libs llinventory llmath llcorehttp llfilesystem )
    LL_ADD_INTEGRATION_TEST(inventorymisc "" "${test_libs}")
    LL_ADD_INTEGRATION_TEST(llparcel "" "${test_libs}")
endif (LL_TESTS)
