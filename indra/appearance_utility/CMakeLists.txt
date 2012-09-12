# -*- cmake -*-

project(appearance_utility)

include(00-Common)
include(CURL)
include(CARes)
include(OpenSSL)
include(UI)
include(LLAppearance)
include(LLCharacter)
include(LLCommon)
include(LLImage)
include(LLInventory)
include(LLMath)
include(LLKDU)
include(LLRender)
include(LLVFS)
include(LLXML)
include(LLUI)
include(Linking)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${LLUI_INCLUDE_DIRS}
    ${LLCHARACTER_INCLUDE_DIRS}
    ${LLKDU_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLINVENTORY_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${CARES_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${UI_INCLUDE_DIRS}
    ${LLAPPEARANCE_INCLUDE_DIRS}
    )

set(appearance_utility_SOURCE_FILES appearance_utility.cpp llbakingshadermgr.cpp)

set(appearance_utility_HEADER_FILES CMakeLists.txt llbakingshadermgr.h)

set_source_files_properties(${appearance_utility_HEADER_FILES}
                            PROPERTIES HEADER_FILES_ONLY TRUE)

list(APPEND appearance_utility_SOURCE_FILES ${appearance_utility_HEADER_FILES})

add_executable(appearance-utility-bin ${appearance_utility_SOURCE_FILES})

target_link_libraries(appearance-utility-bin
    ${LLAPPEARANCE_LIBRARIES}
    ${LLCHARACTER_LIBRARIES}
    ${LLRENDER_LIBRARIES}
    ${LLUI_LIBRARIES}
    ${UI_LIBRARIES}
    ${LLIMAGE_LIBRARIES}
    ${LLKDU_LIBRARIES}
    ${KDU_LIBRARY}
    )

add_custom_target(appearance-utility-bin-target ALL
                  DEPENDS appearance-utility-bin)
