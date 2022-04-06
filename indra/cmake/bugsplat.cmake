if (INSTALL_PROPRIETARY)
    # Note that viewer_manifest.py makes decision based on BUGSPLAT_DB and not USE_BUGSPLAT
    if (BUGSPLAT_DB)
        set(USE_BUGSPLAT ON  CACHE BOOL "Use the BugSplat crash reporting system")
    else (BUGSPLAT_DB)
        set(USE_BUGSPLAT OFF CACHE BOOL "Use the BugSplat crash reporting system")
    endif (BUGSPLAT_DB)
else (INSTALL_PROPRIETARY)
    set(USE_BUGSPLAT OFF CACHE BOOL "Use the BugSplat crash reporting system")
endif (INSTALL_PROPRIETARY)

if( TARGET bugsplat::bugsplat)
    return()
endif()
create_target(bugsplat::bugsplat)

if (USE_BUGSPLAT)
    if (NOT USESYSTEMLIBS)
        include(Prebuilt)
        use_prebuilt_binary(bugsplat)
        if (WINDOWS)
            set_target_libraries( bugsplat::bugsplat
                ${ARCH_PREBUILT_DIRS_RELEASE}/bugsplat.lib
                )
        elseif (DARWIN)
            find_library(BUGSPLAT_LIBRARIES BugsplatMac REQUIRED
                NO_DEFAULT_PATH PATHS "${ARCH_PREBUILT_DIRS_RELEASE}")
            set_target_libraries( bugsplat::bugsplat
                    ${BUGSPLAT_LIBRARIES}
                    )
        else (WINDOWS)
            message(FATAL_ERROR "BugSplat is not supported; add -DUSE_BUGSPLAT=OFF")
        endif (WINDOWS)
    else (NOT USESYSTEMLIBS)
        include(FindBUGSPLAT)
    endif (NOT USESYSTEMLIBS)

    set(BUGSPLAT_DB "" CACHE STRING "BugSplat crash database name")

    set_target_include_dirs( bugsplat::bugsplat ${LIBS_PREBUILT_DIR}/include/bugsplat)
    target_compile_definitions( bugsplat::bugsplat INTERFACE LL_BUGSPLAT)
endif (USE_BUGSPLAT)

