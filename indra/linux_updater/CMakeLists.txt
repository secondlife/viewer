# -*- cmake -*-

project(linux_updater)

include(00-Common)
include(CURL)
include(CARes)
include(OpenSSL)
include(UI)
include(LLCommon)
include(LLMessage)
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
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    ${LLXML_SYSTEM_INCLUDE_DIRS}
    )

set(linux_updater_SOURCE_FILES linux_updater.cpp)

set(linux_updater_HEADER_FILES CMakeLists.txt)

set_source_files_properties(${linux_updater_HEADER_FILES}
                            PROPERTIES HEADER_FILES_ONLY TRUE)

list(APPEND linux_updater_SOURCE_FILES ${linux_updater_HEADER_FILES})

add_executable(linux-updater ${linux_updater_SOURCE_FILES})

target_link_libraries(linux-updater
    ${CURL_LIBRARIES}
    ${CARES_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${CRYPTO_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${UI_LIBRARIES}
    ${LLXML_LIBRARIES}
    ${LLUI_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    )

add_custom_target(linux-updater-target ALL
                  DEPENDS linux-updater)
