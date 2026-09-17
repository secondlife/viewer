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
    elseif (DARWIN)
        target_link_libraries(ll::shmframe INTERFACE "${LLSHMFRAME_LOCAL_BUILD_DIR}/lib/release/libllshmframe.a")
    elseif (LINUX)
        target_link_libraries(ll::shmframe INTERFACE "${LLSHMFRAME_LOCAL_BUILD_DIR}/lib/release/libllshmframe.a")
    endif ()
else ()
    use_prebuilt_binary(llshmframe)
    target_include_directories(ll::shmframe SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include")
    if (WINDOWS)
        target_link_libraries(ll::shmframe INTERFACE llshmframe.lib)
    elseif (DARWIN)
        # Linking.cmake deliberately skips its usual link_directories() setup
        # on Darwin (see its own comment) -- a bare "llshmframe" name (as
        # Windows uses above, relying on that search path) won't resolve, so
        # this needs a real find_library() with an explicit path, same
        # pattern LibVLCPlugin.cmake already uses for this exact platform.
        find_library(LLSHMFRAME_LIBRARY
            NAMES libllshmframe.a
            PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)
        target_link_libraries(ll::shmframe INTERFACE ${LLSHMFRAME_LIBRARY})
    elseif (LINUX)
        # Linux does keep Linking.cmake's usual link_directories() (only
        # Darwin skips it), but an explicit full path avoids any ambiguity
        # over -l name-mangling and doesn't depend on that mechanism at all
        # -- untested on real Linux hardware, so the least assumption-laden
        # option here.
        target_link_libraries(ll::shmframe INTERFACE "${LIBS_PREBUILT_DIR}/lib/release/libllshmframe.a")
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
    elseif (DARWIN)
        target_link_libraries(ll::cefbrowser INTERFACE
            "${LLCEFBROWSER_LOCAL_BUILD_DIR}/lib/release/libllcefbrowser.a"
            "${LLCEFBROWSER_LOCAL_BUILD_DIR}/lib/release/libcef_dll_wrapper.a"
            "${LLCEFBROWSER_LOCAL_BUILD_DIR}/lib/release/Chromium Embedded Framework.framework"
            )
    elseif (LINUX)
        # No framework bundle here (that's Darwin-only) -- the actual CEF
        # runtime is a plain libcef.so, installed under bin/release/ (not
        # lib/release/) to match this package's own autobuild manifest (see
        # llcefbrowser's build-cmd.sh linux64 case and CEF_BINARY_FILES).
        target_link_libraries(ll::cefbrowser INTERFACE
            "${LLCEFBROWSER_LOCAL_BUILD_DIR}/lib/release/libllcefbrowser.a"
            "${LLCEFBROWSER_LOCAL_BUILD_DIR}/lib/release/libcef_dll_wrapper.a"
            "${LLCEFBROWSER_LOCAL_BUILD_DIR}/bin/release/libcef.so"
            )
    endif ()
else ()
    use_prebuilt_binary(llcefbrowser)
    target_include_directories(ll::cefbrowser SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include/llcefbrowser")
    if (WINDOWS)
        target_link_libraries(ll::cefbrowser INTERFACE llcefbrowser.lib libcef.lib libcef_dll_wrapper.lib)
    elseif (DARWIN)
        # See ll::shmframe's own comment on why Darwin needs find_library()
        # with an explicit path rather than a bare linkable name here.
        find_library(LLCEFBROWSER_LIBRARY
            NAMES libllcefbrowser.a
            PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)
        find_library(LIBCEF_DLL_WRAPPER_LIBRARY
            NAMES libcef_dll_wrapper.a
            PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)
        target_link_libraries(ll::cefbrowser INTERFACE
            ${LLCEFBROWSER_LIBRARY}
            ${LIBCEF_DLL_WRAPPER_LIBRARY}
            "${ARCH_PREBUILT_DIRS_RELEASE}/Chromium Embedded Framework.framework"
            )
    elseif (LINUX)
        # Same reasoning as ll::shmframe's own LINUX branch -- explicit full
        # paths, no find_library()/bare -l name assumptions, untested on
        # real hardware. bin/release (not lib/release) for libcef.so itself,
        # matching where build-cmd.sh's linux64 case actually installs it.
        target_link_libraries(ll::cefbrowser INTERFACE
            "${LIBS_PREBUILT_DIR}/lib/release/libllcefbrowser.a"
            "${LIBS_PREBUILT_DIR}/lib/release/libcef_dll_wrapper.a"
            "${LIBS_PREBUILT_DIR}/bin/release/libcef.so"
            )
    endif ()
endif ()

target_compile_definitions(ll::cefbrowser INTERFACE NOMINMAX)
