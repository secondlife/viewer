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
# ll::cefbrowser -- CEF wrapper library (llcefbrowser repo), used by
# llmediaproducer (the in-viewer CEF producer process).
#
# Since 2026-09, this package is self-sufficient: it ships libcef.lib/
# libcef_dll_wrapper.lib and the full CEF runtime (bin/release/*, resources/*)
# alongside its own llcefbrowser.lib, rather than relying on the separate
# "dullahan" package (CEFPlugin.cmake) for those shared pieces. That dullahan
# dependency existed only to avoid two packages writing the same install
# paths (autobuild refuses that); removing it here means a normal viewer
# build no longer pulls in dullahan at all -- CEFPlugin.cmake/dullahan remain
# only for media_plugins/cef (the legacy, disabled-by-default plugin), not
# for this, the actually-shipping path.
#
# LLCEFBROWSER_LOCAL_BUILD_DIR: same idea as LLSHMFRAME_LOCAL_BUILD_DIR --
# point this at llcefbrowser's own local `stage` directory (from running
# `autobuild build` there) to skip the autobuild-package fetch.
add_library(ll::cefbrowser INTERFACE IMPORTED)

if (LLCEFBROWSER_LOCAL_BUILD_DIR)
    target_include_directories(ll::cefbrowser SYSTEM INTERFACE "${LLCEFBROWSER_LOCAL_BUILD_DIR}/include/llcefbrowser")
    if (WINDOWS)
        target_link_libraries(ll::cefbrowser INTERFACE
            "${LLCEFBROWSER_LOCAL_BUILD_DIR}/lib/release/llcefbrowser.lib"
            "${LLCEFBROWSER_LOCAL_BUILD_DIR}/lib/release/libcef.lib"
            "${LLCEFBROWSER_LOCAL_BUILD_DIR}/lib/release/libcef_dll_wrapper.lib"
            )
    endif ()
else ()
    use_prebuilt_binary(llcefbrowser)
    target_include_directories(ll::cefbrowser SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include/llcefbrowser")
    if (WINDOWS)
        target_link_libraries(ll::cefbrowser INTERFACE llcefbrowser.lib libcef.lib libcef_dll_wrapper.lib)
    endif ()
endif ()

target_compile_definitions(ll::cefbrowser INTERFACE NOMINMAX)
