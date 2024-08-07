# -*- cmake -*-

# some webrtc headers require C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(00-Common)
include(Linking)
include(WebRTC)

project(llwebrtc)

if (LINUX)
    add_compile_options(-Wno-deprecated-declarations) # webrtc::CreateAudioDeviceWithDataObserver is deprecated
endif (LINUX)

set(llwebrtc_SOURCE_FILES
    llwebrtc.cpp
    )

set(llwebrtc_HEADER_FILES
    CMakeLists.txt
    llwebrtc.h
    llwebrtc_impl.h
    )

list(APPEND llwebrtc_SOURCE_FILES ${llwebrtc_HEADER_FILES})

add_library (llwebrtc SHARED ${llwebrtc_SOURCE_FILES})

set_target_properties(llwebrtc PROPERTIES PUBLIC_HEADER llwebrtc.h)

if (WINDOWS)
    target_link_libraries(llwebrtc PRIVATE ll::webrtc 
                                       secur32
                                       winmm
                                       dmoguids
                                       wmcodecdspuuid
                                       msdmo
                                       strmiids
                                       iphlpapi)
elseif (DARWIN)
    target_link_libraries(llwebrtc PRIVATE ll::webrtc)
    set_target_properties(llwebrtc PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
                                              XCODE_ATTRIBUTE_DWARF_DSYM_FOLDER_PATH "${SYMBOLS_STAGING_DIR}/${VIEWER_CHANNEL}/dSYMs")
elseif (LINUX)
    target_link_libraries(llwebrtc PRIVATE ll::webrtc)
endif (WINDOWS)
    
target_include_directories( llwebrtc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (WINDOWS)
    set_property(TARGET llwebrtc PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
endif (WINDOWS)

ADD_CUSTOM_COMMAND(TARGET llwebrtc POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       $<TARGET_FILE:llwebrtc>
                               ${SHARED_LIB_STAGING_DIR})
# Add tests
if (LL_TESTS)
endif (LL_TESTS)
