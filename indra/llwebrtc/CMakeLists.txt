# -*- cmake -*-

# some webrtc headers require C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(00-Common)
include(Linking)
include(WebRTC)

project(llwebrtc)

set(llwebrtc_SOURCE_FILES
    llwebrtc.cpp
    )

set(llwebrtc_HEADER_FILES
    CMakeLists.txt
    llwebrtc.h
    llwebrtc_impl.h
    )

list(APPEND llwebrtc_SOURCE_FILES ${llwebrtc_HEADER_FILES})

# Compile and link settings for any target that links the native webrtc
# library directly: llwebrtc itself, and the test peer used by its unit tests.
# The prebuilt webrtc is a release build compiled with clang against the
# static CRT, so such targets must be built the same way.
function(llwebrtc_use_native_webrtc target)
    if (WINDOWS)
        set_target_properties(${target}
            PROPERTIES
            LINK_FLAGS "/debug /LARGEADDRESSAWARE"
            )
        target_link_libraries(${target} PRIVATE ll::webrtc
                                           secur32
                                           winmm
                                           dmoguids
                                           wmcodecdspuuid
                                           msdmo
                                           strmiids
                                           iphlpapi
                                           crypt32
                                           libcmt)
        # as the webrtc libraries are release, build this binary as release as well.
        target_compile_options(${target} PRIVATE "/MT" "/Zc:wchar_t")
        set_property(TARGET ${target} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    elseif (DARWIN)
        target_link_libraries(${target} PRIVATE ll::webrtc
                                       "-framework AVFoundation"
                                       "-framework AudioToolbox"
                                       "-framework CoreAudio"
                                       "-framework CoreMedia"
                                       "-framework CoreVideo"
                                       "-framework Foundation"
                                       "-framework Security"
        )
    elseif (LINUX)
        target_link_libraries(${target} PRIVATE ll::webrtc)
    endif (WINDOWS)

    if (NOT WINDOWS)
        target_compile_options(${target} PRIVATE -Wno-deprecated-declarations) # webrtc::CreateAudioDeviceWithDataObserver is deprecated
    endif ()
endfunction()

add_library (llwebrtc SHARED ${llwebrtc_SOURCE_FILES})

set_target_properties(llwebrtc PROPERTIES PUBLIC_HEADER llwebrtc.h)

if (WINDOWS)
    cmake_policy(SET CMP0091 NEW)
endif (WINDOWS)

llwebrtc_use_native_webrtc(llwebrtc)

if (USE_BUGSPLAT)
    if (WINDOWS)
        set_target_properties(llwebrtc PROPERTIES PDB_OUTPUT_DIRECTORY "${SYMBOLS_STAGING_DIR}")
    elseif (DARWIN)
        set_target_properties(llwebrtc PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
                                                  XCODE_ATTRIBUTE_DWARF_DSYM_FOLDER_PATH "${SYMBOLS_STAGING_DIR}/dSYMs")
    endif ()
endif (USE_BUGSPLAT)

target_include_directories( llwebrtc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

ADD_CUSTOM_COMMAND(TARGET llwebrtc POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       $<TARGET_FILE:llwebrtc>
                               ${SHARED_LIB_STAGING_DIR})
# Add tests
if (LL_TESTS)
    include(LLAddGTest)

    # A minimal stand-in for the Second Life voice server, so the tests can
    # bring up real connections.  It wraps native webrtc, so like llwebrtc
    # it's a shared library built with llwebrtc's settings.
    add_library(llwebrtc_testpeer SHARED
        gtests/testpeer/llwebrtc_testpeer.cpp
        gtests/testpeer/llwebrtc_testpeer.h
        )
    llwebrtc_use_native_webrtc(llwebrtc_testpeer)
    target_include_directories(llwebrtc_testpeer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/gtests/testpeer)
    set_target_properties(llwebrtc_testpeer PROPERTIES FOLDER "Tests")
    if (LINUX)
        # Keep this library's copy of webrtc private, so it can't interpose
        # on llwebrtc's when both are loaded into the test process.
        target_compile_options(llwebrtc_testpeer PRIVATE -fvisibility=hidden)
        target_link_options(llwebrtc_testpeer PRIVATE "LINKER:--exclude-libs,ALL")
    endif ()
    add_custom_command(TARGET llwebrtc_testpeer POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           $<TARGET_FILE:llwebrtc_testpeer>
                           ${SHARED_LIB_STAGING_DIR})

    # The tests link llwebrtc the same way the viewer does, through the
    # public llwebrtc.h interface only.
    set(llwebrtc_gtest_LIBRARIES llwebrtc llwebrtc_testpeer)
    if (WINDOWS)
        list(APPEND llwebrtc_gtest_LIBRARIES ws2_32) # WSAStartup, see llwebrtc_test_support.cpp
    endif ()

    set(llwebrtc_gtest_SUPPORT_FILES
        gtests/llwebrtc_test_support.h
        gtests/llwebrtc_test_support.cpp
        )

    # All llwebrtc tests hold the 'llwebrtc' lock: peer connections bind a
    # fixed port range (60000-60100), so they shouldn't run in parallel.

    # Tests that share one llwebrtc instance for the whole run.
    ll_add_gtest(llwebrtc
        RESOURCE_LOCK llwebrtc
        SOURCES
            ${llwebrtc_gtest_SUPPORT_FILES}
            gtests/llwebrtc_test_environment.cpp
            gtests/llwebrtc_types_test.cpp
            gtests/llwebrtc_devices_test.cpp
            gtests/llwebrtc_peerconnection_test.cpp
            gtests/llwebrtc_connection_test.cpp
        LIBRARIES
            ${llwebrtc_gtest_LIBRARIES}
        )

    # Shutdown tests init and terminate llwebrtc in every test, so they need
    # their own executable.  Each test also runs in its own process, so a
    # crash during shutdown fails only the test that crashed.
    set(llwebrtc_shutdown_gtest_SOURCES
        ${llwebrtc_gtest_SUPPORT_FILES}
        gtests/llwebrtc_shutdown_test.h
        gtests/llwebrtc_shutdown_test.cpp
        )
    # Tests that open real audio devices, for machines that have them.
    if (LL_TESTS_AUDIO_DEVICES)
        list(APPEND llwebrtc_shutdown_gtest_SOURCES gtests/llwebrtc_shutdown_audio_device_test.cpp)
    endif ()
    ll_add_gtest(llwebrtc_shutdown
        PER_TEST_PROCESS
        RESOURCE_LOCK llwebrtc
        SOURCES
            ${llwebrtc_shutdown_gtest_SOURCES}
        LIBRARIES
            ${llwebrtc_gtest_LIBRARIES}
        )
endif (LL_TESTS)
