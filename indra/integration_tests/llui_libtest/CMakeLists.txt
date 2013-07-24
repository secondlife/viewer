# -*- cmake -*-

# Only set this up for viewer builds, because the llui library is most closely
# related to the viewer
if (VIEWER)

project (llui_libtest)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLImageJ2COJ)   # ugh, needed for images
include(LLKDU)
include(LLMath)
include(LLMessage)
include(LLRender)
include(LLWindow)
include(LLUI)
include(LLVFS)        # ugh, needed for LLDir
include(LLXML)
include(Hunspell)
include(Linking)
# include(Tut)

include_directories(
    ${FREETYPE_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLUI_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${LIBS_PREBUILD_DIR}/include/hunspell
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    ${LLXML_SYSTEM_INCLUDE_DIRS}
    )

set(llui_libtest_SOURCE_FILES
    llui_libtest.cpp
    llwidgetreg.cpp
    )

set(llui_libtest_HEADER_FILES
    CMakeLists.txt
    llui_libtest.h
    llwidgetreg.h
    )

set_source_files_properties(${llui_libtest_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llui_libtest_SOURCE_FILES ${llui_libtest_HEADER_FILES})

add_executable(llui_libtest ${llui_libtest_SOURCE_FILES})

# Link with OS-specific libraries for LLWindow dependency
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
target_link_libraries(llui_libtest
    llui
    llinventory
    llmessage
    ${LLRENDER_LIBRARIES}
    ${LLIMAGE_LIBRARIES}
    ${LLKDU_LIBRARIES}
    ${KDU_LIBRARY}
    ${LLIMAGEJ2COJ_LIBRARIES}
    ${OS_LIBRARIES}
    ${GOOGLE_PERFTOOLS_LIBRARIES}
    ${HUNSPELL_LIBRARY}
    )

if (WINDOWS)
    set_target_properties(llui_libtest
        PROPERTIES 
        LINK_FLAGS "/NODEFAULTLIB:LIBCMT"
        LINK_FLAGS_DEBUG "/NODEFAULTLIB:MSVCRT /NODEFAULTLIB:LIBCMTD"
        )

    # Copy over OpenJPEG.dll
    # *NOTE: On Windows with VS2005, only the first comment prints
    set(OPENJPEG_RELEASE
        "${ARCH_PREBUILT_DIRS_RELEASE}/openjpeg.dll")
    add_custom_command( TARGET llui_libtest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            ${OPENJPEG_RELEASE} ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Copying OpenJPEG DLLs to binary directory"
        )
    set(OPENJPEG_DEBUG
        "${ARCH_PREBUILT_DIRS_DEBUG}/openjpegd.dll")
    add_custom_command( TARGET llui_libtest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            ${OPENJPEG_DEBUG} ${CMAKE_CURRENT_BINARY_DIR}
        )
  
endif (WINDOWS)

# Ensure people working on the viewer don't break this library
# *NOTE: This could be removed, or only built by Parabuild, if the build
# and link times become too long. JC
add_dependencies(viewer llui_libtest)

endif (VIEWER)
