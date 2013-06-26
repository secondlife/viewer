# -*- cmake -*-

project(mac_crash_logger)

include(00-Common)
include(LLCommon)
include(LLCrashLogger)
include(LLMath)
include(LLMessage)
include(LLVFS)
include(LLXML)
include(Linking)
include(LLSharedLibs)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCRASHLOGGER_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    ${LLXML_SYSTEM_INCLUDE_DIRS}
    )

set(mac_crash_logger_SOURCE_FILES
    mac_crash_logger.cpp
    llcrashloggermac.cpp
    llcrashloggermacdelegate.mm
    )

set(mac_crash_logger_HEADER_FILES
    CMakeLists.txt

    llcrashloggermac.h
    llcrashloggermacdelegate.h
    )

set_source_files_properties(${mac_crash_logger_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)
list(APPEND mac_crash_logger_SOURCE_FILES ${mac_crash_logger_HEADER_FILES})

set(mac_crash_logger_RESOURCE_FILES
  CrashReporter.nib/
  )
set_source_files_properties(
  ${mac_crash_logger_RESOURCE_FILES}
  PROPERTIES
  HEADER_FILE_ONLY TRUE
  )
SOURCE_GROUP("Resources" FILES ${mac_crash_logger_RESOURCE_FILES})
list(APPEND mac_crash_logger_SOURCE_FILES ${mac_crash_logger_RESOURCE_FILES})

add_executable(mac-crash-logger
  MACOSX_BUNDLE
  ${mac_crash_logger_SOURCE_FILES})

set_target_properties(mac-crash-logger
  PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
  )

find_library(COCOA_LIBRARY Cocoa)

target_link_libraries(mac-crash-logger
    ${LLCRASHLOGGER_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${COCOA_LIBRARIES}
    ${LLXML_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    )

add_custom_command(
  TARGET mac-crash-logger POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  ARGS
    -E
    copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/CrashReporter.nib
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/mac-crash-logger.app/Contents/Resources/CrashReporter.nib
  )

ll_deploy_sharedlibs_command(mac-crash-logger)


