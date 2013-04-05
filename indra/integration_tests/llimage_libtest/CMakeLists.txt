# -*- cmake -*-

# Integration tests of the llimage library (JPEG2000, PNG, jpeg, etc... images reading and writing)

project (llimage_libtest)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLImageJ2COJ) 
include(LLKDU)
include(LLVFS)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )

set(llimage_libtest_SOURCE_FILES
    llimage_libtest.cpp
    )

set(llimage_libtest_HEADER_FILES
    CMakeLists.txt
    llimage_libtest.h
    )

set_source_files_properties(${llimage_libtest_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llimage_libtest_SOURCE_FILES ${llimage_libtest_HEADER_FILES})

add_executable(llimage_libtest
    WIN32
    MACOSX_BUNDLE
    ${llimage_libtest_SOURCE_FILES}
)

set_target_properties(llimage_libtest
    PROPERTIES
    WIN32_EXECUTABLE
    FALSE
)

# OS-specific libraries
if (DARWIN)
  include(CMakeFindFrameworks)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  set(OS_LIBRARIES ${COREFOUNDATION_LIBRARY})
elseif (WINDOWS)
  set(OS_LIBRARIES)
elseif (LINUX)
  set(OS_LIBRARIES)
else (DARWIN)
  message(FATAL_ERROR "Unknown platform")
endif (DARWIN)

# Libraries on which this application depends on
# Sort by high-level to low-level
target_link_libraries(llimage_libtest
    ${LLCOMMON_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLIMAGE_LIBRARIES}
    ${LLKDU_LIBRARIES}
    ${KDU_LIBRARY}
    ${LLIMAGEJ2COJ_LIBRARIES}
    ${OS_LIBRARIES}
    )
    
if (DARWIN)
  # Path inside the app bundle where we'll need to copy libraries
  set(LLIMAGE_LIBTEST_DESTINATION_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/llimage_libtest.app/Contents/Resources
  )
  # Create the Contents/Resources directory
  add_custom_command(
    TARGET llimage_libtest POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS
      -E
      make_directory
      ${LLIMAGE_LIBTEST_DESTINATION_DIR}
    COMMENT "Creating Resources directory in app bundle."
  ) 
else (DARWIN)
  set(LLIMAGE_LIBTEST_DESTINATION_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
  )
endif (DARWIN)

get_target_property(BUILT_LLCOMMON llcommon LOCATION)
add_custom_command(TARGET llimage_libtest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_LLCOMMON}  ${LLIMAGE_LIBTEST_DESTINATION_DIR}
  DEPENDS ${BUILT_LLCOMMON}
)

if (DARWIN)
  # Copy the required libraries to the package app
  add_custom_command(TARGET llimage_libtest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../build-darwin-i386/packages/lib/release/libapr-1.0.dylib ${LLIMAGE_LIBTEST_DESTINATION_DIR}
    DEPENDS ${CMAKE_SOURCE_DIR}/../build-darwin-i386/packages/lib/release/libapr-1.0.dylib
  )
  add_custom_command(TARGET llimage_libtest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../build-darwin-i386/packages/lib/release/libaprutil-1.0.dylib ${LLIMAGE_LIBTEST_DESTINATION_DIR}
    DEPENDS ${CMAKE_SOURCE_DIR}/../build-darwin-i386/packages/lib/release/libaprutil-1.0.dylib
  )
  add_custom_command(TARGET llimage_libtest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../build-darwin-i386/packages/lib/release/libexception_handler.dylib ${LLIMAGE_LIBTEST_DESTINATION_DIR}
    DEPENDS ${CMAKE_SOURCE_DIR}/../build-darwin-i386/packages/lib/release/libexception_handler.dylib
  )
  add_custom_command(TARGET llimage_libtest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../build-darwin-i386/packages/lib/release/libexpat.1.5.2.dylib ${LLIMAGE_LIBTEST_DESTINATION_DIR}
    DEPENDS ${CMAKE_SOURCE_DIR}/../build-darwin-i386/packages/lib/release/libexpat.1.5.2.dylib
  )
endif (DARWIN)

if (WINDOWS)
  # Check indra/test_apps/llplugintest/CMakeLists.txt for an example of what to copy over for Windows and how
endif (WINDOWS)

# Ensure people working on the viewer don't break this library
# *NOTE: This could be removed, or only built by TeamCity, if the build
# and link times become too long.
add_dependencies(viewer llimage_libtest)
