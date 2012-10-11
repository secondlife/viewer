# -*- cmake -*-

project(llinventory)

include(00-Common)
include(LLCommon)
include(LLMath)
include(LLMessage)
include(LLVFS)
include(LLXML)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    ${LLXML_SYSTEM_INCLUDE_DIRS}
    )

set(llinventory_SOURCE_FILES
    llcategory.cpp
    lleconomy.cpp
    llinventory.cpp
    llinventorydefines.cpp
    llinventorytype.cpp
    lllandmark.cpp
    llnotecard.cpp
    llparcel.cpp
    llpermissions.cpp
    llsaleinfo.cpp
    lltransactionflags.cpp
    lluserrelations.cpp
    )
    
set(llinventory_HEADER_FILES
    CMakeLists.txt

    llcategory.h
    lleconomy.h
    llinventory.h
    llinventorydefines.h
    llinventorytype.h
    lllandmark.h
    llnotecard.h
    llparcel.h
    llparcelflags.h
    llpermissions.h
    llpermissionsflags.h
    llsaleinfo.h
    lltransactionflags.h
    lltransactiontypes.h
    lluserrelations.h
    )

set_source_files_properties(${llinventory_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llinventory_SOURCE_FILES ${llinventory_HEADER_FILES})

add_library (llinventory ${llinventory_SOURCE_FILES})

target_link_libraries(llinventory
    ${LLCOMMON_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${LLXML_LIBRARIES}
    )


#add unit tests
if (LL_TESTS)
    INCLUDE(LLAddBuildTest)
    SET(llinventory_TEST_SOURCE_FILES
      # no real unit tests yet!
      )
    LL_ADD_PROJECT_UNIT_TESTS(llinventory "${llinventory_TEST_SOURCE_FILES}")

    #set(TEST_DEBUG on)
    set(test_libs llinventory ${LLMESSAGE_LIBRARIES} ${LLVFS_LIBRARIES} ${LLMATH_LIBRARIES} ${LLCOMMON_LIBRARIES} ${WINDOWS_LIBRARIES})
    LL_ADD_INTEGRATION_TEST(inventorymisc "" "${test_libs}")
    LL_ADD_INTEGRATION_TEST(llparcel "" "${test_libs}")
endif (LL_TESTS)
