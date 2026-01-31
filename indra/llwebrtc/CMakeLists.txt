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
    cmake_policy(SET CMP0091 NEW)
    set_target_properties(llwebrtc
        PROPERTIES
        LINK_FLAGS "/debug /LARGEADDRESSAWARE /NODEFAULTLIB:\"LIBCMTD;MSVCRT;MSVCRTD\""
        MSVC_RUNTIME_LIBRARY "MultiThreaded"
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

    ADD_CUSTOM_COMMAND(TARGET llwebrtc POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:llwebrtc>
                                ${SHARED_LIB_STAGING_DIR})
elseif(DARWIN)
    target_link_libraries(llwebrtc PRIVATE ll::webrtc)
    target_compile_options(llwebrtc PRIVATE -Wno-deprecated-declarations) # webrtc::CreateAudioDeviceWithDataObserver is deprecated

    if(NOT (${CMAKE_GENERATOR} STREQUAL "Xcode"))
        find_program(DSYMUTIL dsymutil REQUIRED)
        add_custom_command(TARGET llwebrtc POST_BUILD
                COMMAND dsymutil $<TARGET_FILE:llwebrtc>
                COMMENT "Generating llwebrtc.dSYM")
    endif()

    if (USE_BUGSPLAT)
        set_target_properties(llwebrtc PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
                                                  XCODE_ATTRIBUTE_DWARF_DSYM_FOLDER_PATH "${SYMBOLS_STAGING_DIR}/dSYMs")
    endif (USE_BUGSPLAT)
else()
    target_link_libraries(llwebrtc PRIVATE ll::webrtc)
    target_compile_options(llwebrtc PRIVATE -Wno-deprecated-declarations) # webrtc::CreateAudioDeviceWithDataObserver is deprecated
endif (WINDOWS)

target_include_directories(llwebrtc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Add tests
if (LL_TESTS)
endif (LL_TESTS)
