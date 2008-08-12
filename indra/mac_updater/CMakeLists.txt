# -*- cmake -*-

project(mac_updater)

include(00-Common)
include(CURL)
include(LLCommon)
include(LLVFS)
include(Linking)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    )

set(mac_updater_SOURCE_FILES
    mac_updater.cpp
    FSCopyObject.c
    GenLinkedList.c
    MoreFilesX.c
    )

set(mac_updater_HEADER_FILES
    CMakeLists.txt

    FSCopyObject.h
    GenLinkedList.h
    MoreFilesX.h
    )

set_source_files_properties(
    FSCopyObject.c
    MoreFilesX.c
    PROPERTIES
    COMPILE_FLAGS -Wno-deprecated-declarations
    )

set_source_files_properties(${mac_updater_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND mac_updater_SOURCE_FILES ${mac_updater_HEADER_FILES})


set(mac_updater_RESOURCE_FILES
  Info.plist
  AutoUpdater.nib/
  )
set_source_files_properties(
  ${mac_updater_RESOURCE_FILES}
  PROPERTIES
  HEADER_FILE_ONLY TRUE
  )
set_source_files_properties(
  Info.plist
  PROPERTIES
  MACOSX_PACKAGE_LOCATION . # will it blend? + poppy
  )
SOURCE_GROUP("Resources" FILES ${mac_updater_RESOURCE_FILES})
list(APPEND mac_updater_SOURCE_FILES ${mac_updater_RESOURCE_FILES})


add_executable(mac-updater
  MACOSX_BUNDLE
  ${mac_updater_SOURCE_FILES})

target_link_libraries(mac-updater
    ${LLVFS_LIBRARIES}
    ${CURL_LIBRARIES}
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


