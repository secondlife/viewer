# -*- cmake -*-

# Integration tests of the llimage library (JPEG2000 images reading and writing)

if (VIEWER)

project (llimage_libtest)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLImageJ2COJ) 
include(LLKDU)
include(LLMath)
#include(LLVFS)        # ugh, needed for LLDir
#include(LLXML)
#include(LLXUIXML)
include(Linking)
# include(Tut)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
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

add_executable(llimage_libtest ${llimage_libtest_SOURCE_FILES})

# Link with OS-specific libraries
if (DARWIN)
  find_library(COCOA_LIBRARY Cocoa)
  set(OS_LIBRARIES ${COCOA_LIBRARY})
elseif (WINDOWS)
  #ll_stack_trace needs this now...
  list(APPEND WINDOWS_LIBRARIES dbghelp)
  set(OS_LIBRARIES ${WINDOWS_LIBRARIES})
elseif (LINUX)
  set(OS_LIBRARIES)
else (DARWIN)
  message(FATAL_ERROR "unknown platform")
endif (DARWIN)

# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
target_link_libraries(llimage_libtest
    ${LLIMAGE_LIBRARIES}
    ${LLKDU_LIBRARIES}
    ${KDU_LIBRARY}
    ${LLIMAGEJ2COJ_LIBRARIES}
    ${OS_LIBRARIES}
    ${GOOGLE_PERFTOOLS_LIBRARIES}
    )
	
if (DARWIN)
  set_target_properties(llimage_libtest
    PROPERTIES
    OUTPUT_NAME "llimage_libtest"
    MACOSX_BUNDLE_INFO_STRING "llimage test utility"
    MACOSX_BUNDLE_GUI_IDENTIFIER "llimage_libtest"
    MACOSX_BUNDLE_LONG_VERSION_STRING "0.5"
    MACOSX_BUNDLE_BUNDLE_NAME "llimage_libtest"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "v0.5"
    MACOSX_BUNDLE_BUNDLE_VERSION "v0.5"
    MACOSX_BUNDLE_COPYRIGHT "Copyright Linden Lab 2011"
    )
	# TODO : Copy over libs in the bundle 
elseif (WINDOWS)
    set_target_properties(llimage_libtest
        PROPERTIES 
        LINK_FLAGS "/NODEFAULTLIB:LIBCMT"
        LINK_FLAGS_DEBUG "/NODEFAULTLIB:MSVCRT /NODEFAULTLIB:LIBCMTD"
        )

    # Copy over OpenJPEG.dll
    # *NOTE: On Windows with VS2005, only the first comment prints
    set(OPENJPEG_RELEASE
        "${CMAKE_SOURCE_DIR}/../libraries/i686-win32/lib/release/openjpeg.dll")
    add_custom_command( TARGET llimage_libtest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            ${OPENJPEG_RELEASE} ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Copying OpenJPEG DLLs to binary directory"
        )
    set(OPENJPEG_DEBUG
        "${CMAKE_SOURCE_DIR}/../libraries/i686-win32/lib/debug/openjpegd.dll")
    add_custom_command( TARGET llimage_libtest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            ${OPENJPEG_DEBUG} ${CMAKE_CURRENT_BINARY_DIR}
        )
elseif (LINUX)
	# TODO : please help me get this to build and run for Linux!
else (DARWIN)
  message(FATAL_ERROR "unknown platform")
endif (DARWIN)

# Ensure people working on the viewer don't break this library
# *NOTE: This could be removed, or only built by TeamCity, if the build
# and link times become too long.
add_dependencies(viewer llimage_libtest)

endif (VIEWER)
