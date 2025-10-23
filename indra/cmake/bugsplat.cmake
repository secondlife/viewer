if (INSTALL_PROPRIETARY AND NOT LINUX)
    # Note that viewer_manifest.py makes decision based on BUGSPLAT_DB and not USE_BUGSPLAT
    if (BUGSPLAT_DB)
        set(USE_BUGSPLAT ON  CACHE BOOL "Use the BugSplat crash reporting system")
    else (BUGSPLAT_DB)
        set(USE_BUGSPLAT OFF CACHE BOOL "Use the BugSplat crash reporting system")
    endif (BUGSPLAT_DB)
else ()
    set(USE_BUGSPLAT OFF CACHE BOOL "Use the BugSplat crash reporting system")
    set(BUGSPLAT_DB "" CACHE STRING "BugSplat crash database name")
endif ()

include_guard()
add_library( ll::bugsplat INTERFACE IMPORTED )

if (USE_BUGSPLAT AND NOT LINUX)
    include(Prebuilt)
    use_prebuilt_binary(bugsplat)
    if (WINDOWS)
        target_link_libraries( ll::bugsplat INTERFACE
                ${ARCH_PREBUILT_DIRS_RELEASE}/bugsplat.lib
                )
        target_include_directories( ll::bugsplat SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/bugsplat)
    elseif (DARWIN)
        find_library(BUGSPLAT_LIBRARIES BugsplatMac REQUIRED
                NO_DEFAULT_PATH PATHS "${ARCH_PREBUILT_DIRS_RELEASE}")
        target_link_libraries( ll::bugsplat INTERFACE
                ${BUGSPLAT_LIBRARIES}
                )
    else (WINDOWS)
        message(FATAL_ERROR "BugSplat is not supported; add -DUSE_BUGSPLAT=OFF")
    endif (WINDOWS)

    if( NOT BUGSPLAT_DB )
        message( FATAL_ERROR "You need to set BUGSPLAT_DB when setting USE_BUGSPLAT" )
    endif()

    set_property( TARGET ll::bugsplat APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS LL_BUGSPLAT)
else()
    set(USE_BUGSPLAT OFF CACHE BOOL "Use the BugSplat crash reporting system" FORCE)
    set(BUGSPLAT_DB "" CACHE STRING "BugSplat crash database name" FORCE)
endif ()

