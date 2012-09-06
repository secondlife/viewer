# -*- cmake -*-

project(appearance_utility)

include(00-Common)
include(CURL)
include(CARes)
include(OpenSSL)
include(UI)
include(LLAppearance)
include(LLCommon)
include(LLVFS)
include(LLXML)
include(LLUI)
include(Linking)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${LLUI_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${CARES_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${UI_INCLUDE_DIRS}
    ${LLAPPEARANCE_INCLUDE_DIRS}
    )

set(appearance_utility_SOURCE_FILES appearance_utility.cpp)

set(appearance_utility_HEADER_FILES CMakeLists.txt)

set_source_files_properties(${appearance_utility_HEADER_FILES}
                            PROPERTIES HEADER_FILES_ONLY TRUE)

list(APPEND appearance_utility_SOURCE_FILES ${appearance_utility_HEADER_FILES})

add_executable(appearance-utility-bin ${appearance_utility_SOURCE_FILES})

target_link_libraries(appearance-utility-bin
    ${CURL_LIBRARIES}
    ${CARES_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${CRYPTO_LIBRARIES}
    ${UI_LIBRARIES}
    ${LLAPPEARANCE_LIBRARIES}
    ${LLXML_LIBRARIES}
    ${LLUI_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    )

add_custom_target(appearance-utility-bin-target ALL
                  DEPENDS appearance-utility-bin)
