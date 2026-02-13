# -*- cmake -*-

set(llwebrtc_SOURCE_FILES
    llwebrtc.cpp
    )

set(llwebrtc_HEADER_FILES
    CMakeLists.txt
    llwebrtc.h
    llwebrtc_impl.h
    )

list(APPEND llwebrtc_SOURCE_FILES ${llwebrtc_HEADER_FILES})

add_library(llwebrtc SHARED ${llwebrtc_SOURCE_FILES})
set_target_properties(llwebrtc PROPERTIES PUBLIC_HEADER llwebrtc.h)

if (WINDOWS)
    set_target_properties(llwebrtc
        PROPERTIES
        LINK_FLAGS "/debug /LARGEADDRESSAWARE /NODEFAULTLIB:\"LIBCMTD;MSVCRT;MSVCRTD\" /MANIFEST:NO" # Intentionally override all link flags for llwebrtc
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
        EXCLUDE_FROM_DEFAULT_BUILD_DEBUG ON
        RUNTIME_OUTPUT_DIRECTORY "${VIEWER_STAGING_DIR}"
        )
    target_link_libraries(llwebrtc PRIVATE ll::webrtc
                                       secur32
                                       winmm
                                       dmoguids
                                       wmcodecdspuuid
                                       msdmo
                                       strmiids
                                       iphlpapi
                                       libcmt)

    if (USE_BUGSPLAT)
        set_target_properties(llwebrtc PROPERTIES PDB_OUTPUT_DIRECTORY "${SYMBOLS_STAGING_DIR}")
    endif (USE_BUGSPLAT)
elseif(DARWIN)
    target_link_libraries(llwebrtc PRIVATE ll::webrtc)
    target_compile_options(llwebrtc PRIVATE -Wno-deprecated-declarations) # webrtc::CreateAudioDeviceWithDataObserver is deprecated

    if (USE_BUGSPLAT)
        set_target_properties(llwebrtc PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
                                                  XCODE_ATTRIBUTE_DWARF_DSYM_FOLDER_PATH "${SYMBOLS_STAGING_DIR}/dSYMs")
        if(NOT XCODE)
            find_program(DSYMUTIL dsymutil REQUIRED)
            add_custom_command(TARGET llwebrtc POST_BUILD
                    COMMAND dsymutil $<TARGET_FILE:llwebrtc>
                    COMMENT "Generating llwebrtc.dSYM")
        endif()
    endif (USE_BUGSPLAT)
else()
    target_link_libraries(llwebrtc PRIVATE ll::webrtc)
    target_compile_options(llwebrtc PRIVATE -Wno-deprecated-declarations) # webrtc::CreateAudioDeviceWithDataObserver is deprecated
endif (WINDOWS)

target_include_directories(llwebrtc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Add tests
if (BUILD_TESTING)
endif ()
