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
include(LLFileSystem)
include(LLXML)
include(Hunspell)
include(Linking)
# include(Tut)

set(llui_libtest_SOURCE_FILES
    llui_libtest.cpp
    llwidgetreg.cpp
    )

set(llui_libtest_HEADER_FILES
    CMakeLists.txt
    llui_libtest.h
    llwidgetreg.h
    )

list(APPEND llui_libtest_SOURCE_FILES ${llui_libtest_HEADER_FILES})

add_executable(llui_libtest ${llui_libtest_SOURCE_FILES})

# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
target_link_libraries(llui_libtest
        llui
        llinventory
        llmessage
        llrender
        llimage
        llkdu
        llimagej2coj
        )

if (WINDOWS)
    set_target_properties(llui_libtest
        PROPERTIES 
        LINK_FLAGS "/NODEFAULTLIB:LIBCMT"
        )

    # Copy over OpenJPEG.dll
    # *NOTE: On Windows with VS2005, only the first comment prints
    set(OPENJPEG_RELEASE
        "${ARCH_PREBUILT_DIRS_RELEASE}/openjp2.dll")
    add_custom_command( TARGET llui_libtest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${OPENJPEG_RELEASE} ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Copying OpenJPEG DLLs to binary directory"
        )
    set(OPENJPEG_DEBUG
        "${ARCH_PREBUILT_DIRS_DEBUG}/openjp2.dll")
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
