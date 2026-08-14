# -*- cmake -*-
include_guard()

include(Linking)
include(Prebuilt)

# ==============================================================================
# ll::shmframe -- shared-memory frame/command transport (llshmframe repo).
# Needed by llembeddedbrowser (the viewer's own consumer side) today.
#
# LLSHMFRAME_LOCAL_BUILD_DIR: for active development on llshmframe itself,
# set this to that repo's own local `stage` directory (produced by running
# `autobuild build` there -- entirely local, no network/publish round trip)
# to skip the autobuild-package fetch and link straight against a freshly
# rebuilt local copy. Leave unset for the normal case (a real published
# package via use_prebuilt_binary()).
add_library(ll::shmframe INTERFACE IMPORTED)

if (LLSHMFRAME_LOCAL_BUILD_DIR)
    target_include_directories(ll::shmframe SYSTEM INTERFACE "${LLSHMFRAME_LOCAL_BUILD_DIR}/include")
    if (WINDOWS)
        target_link_libraries(ll::shmframe INTERFACE "${LLSHMFRAME_LOCAL_BUILD_DIR}/lib/release/llshmframe.lib")
    endif ()
else ()
    use_prebuilt_binary(llshmframe)
    target_include_directories(ll::shmframe SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include")
    if (WINDOWS)
        target_link_libraries(ll::shmframe INTERFACE llshmframe.lib)
    endif ()
endif ()

# ==============================================================================
# ll::cefbrowser -- CEF wrapper library (llcefbrowser repo). Not linked by
# anything in the viewer's own code yet -- this is groundwork for the future
# in-viewer CEF producer process, mirrored on ll::shmframe above.
#
# Its package deliberately ships only llcefbrowser.lib + its own headers --
# not libcef.lib/libcef_dll_wrapper.lib or any CEF runtime files, since those
# are the exact same CEF distribution the "dullahan" package (CEFPlugin.cmake)
# already installs for the legacy media plugin, and autobuild refuses to
# install two packages that write the same path. So ll::cefbrowser depends on
# ll::cef for those shared pieces -- a consumer only needs to link
# ll::cefbrowser and gets both.
#
# LLCEFBROWSER_LOCAL_BUILD_DIR: same idea as LLSHMFRAME_LOCAL_BUILD_DIR --
# point this at llcefbrowser's own local `stage` directory (from running
# `autobuild build` there) to skip the autobuild-package fetch.
include(CEFPlugin)

add_library(ll::cefbrowser INTERFACE IMPORTED)

if (LLCEFBROWSER_LOCAL_BUILD_DIR)
    target_include_directories(ll::cefbrowser SYSTEM INTERFACE "${LLCEFBROWSER_LOCAL_BUILD_DIR}/include/llcefbrowser")
    if (WINDOWS)
        target_link_libraries(ll::cefbrowser INTERFACE "${LLCEFBROWSER_LOCAL_BUILD_DIR}/lib/release/llcefbrowser.lib")
    endif ()
else ()
    use_prebuilt_binary(llcefbrowser)
    target_include_directories(ll::cefbrowser SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include/llcefbrowser")
    if (WINDOWS)
        target_link_libraries(ll::cefbrowser INTERFACE llcefbrowser.lib)
    endif ()
endif ()

target_link_libraries(ll::cefbrowser INTERFACE ll::cef)
target_compile_definitions(ll::cefbrowser INTERFACE NOMINMAX)
