# -*- cmake -*-

project(mac_updater)

include(00-Common)
include(OpenSSL)
include(CURL)
include(CARes)
include(LLCommon)
include(LLMessage)
include(LLVFS)
include(Linking)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${CARES_INCLUDE_DIRS}
    )

set(mac_updater_SOURCE_FILES
    mac_updater.cpp
    )

set(mac_updater_HEADER_FILES
    CMakeLists.txt
    )

set_source_files_properties(${mac_updater_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND mac_updater_SOURCE_FILES ${mac_updater_HEADER_FILES})


set(mac_updater_RESOURCE_FILES
  AutoUpdater.nib/
  )
set_source_files_properties(
  ${mac_updater_RESOURCE_FILES}
  PROPERTIES
  HEADER_FILE_ONLY TRUE
  )
SOURCE_GROUP("Resources" FILES ${mac_updater_RESOURCE_FILES})
list(APPEND mac_updater_SOURCE_FILES ${mac_updater_RESOURCE_FILES})

add_executable(mac-updater
  MACOSX_BUNDLE
  ${mac_updater_SOURCE_FILES})

set_target_properties(mac-updater
  PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
  )

target_link_libraries(mac-updater
    ${LLMESSAGE_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${CRYPTO_LIBRARIES}
    ${CURL_LIBRARIES}
    ${CARES_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    )

add_custom_command(
  TARGET mac-updater POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  ARGS
    -E
    copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/AutoUpdater.nib
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/mac-updater.app/Contents/Resources/AutoUpdater.nib
  )

ll_deploy_sharedlibs_command(mac-updater) 
