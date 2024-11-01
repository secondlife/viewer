# -*- cmake -*-

# The copy_win_libs folder contains file lists and a script used to
# copy dlls, exes and such needed to run the SecondLife from within
# VisualStudio.

include(CMakeCopyIfDifferent)
include(Linking)
include(OPENAL)

# When we copy our dependent libraries, we almost always want to copy them to
# both the Release and the RelWithDebInfo staging directories. This has
# resulted in duplicate (or worse, erroneous attempted duplicate)
# copy_if_different commands. Encapsulate that usage.
# Pass FROM_DIR, TARGETS and the files to copy. TO_DIR is implicit.
# to_staging_dirs diverges from copy_if_different in that it appends to TARGETS.
macro(to_staging_dirs from_dir targets)
    set( targetDir "${SHARED_LIB_STAGING_DIR}")
    copy_if_different("${from_dir}" "${targetDir}" out_targets ${ARGN})

    list(APPEND "${targets}" "${out_targets}")
endmacro()

###################################################################
# set up platform specific lists of files that need to be copied
###################################################################
if(WINDOWS)
    #*******************************
    # VIVOX - *NOTE: no debug version
    set(vivox_lib_dir "${ARCH_PREBUILT_DIRS_RELEASE}")

    # ND, it seems there is no such thing defined. At least when building a viewer
    # Does this maybe matter on some LL buildserver? Otherwise this and the snippet using slvoice_src_dir
    # can all go
    if( ARCH_PREBUILT_BIN_RELEASE )
        set(slvoice_src_dir "${ARCH_PREBUILT_BIN_RELEASE}")
    endif()
    set(slvoice_files SLVoice.exe )
    if (ADDRESS_SIZE EQUAL 64)
        list(APPEND vivox_libs
            vivoxsdk_x64.dll
            ortp_x64.dll
            )
    else (ADDRESS_SIZE EQUAL 64)
        list(APPEND vivox_libs
            vivoxsdk.dll
            ortp.dll
            )
    endif (ADDRESS_SIZE EQUAL 64)

    #*******************************
    # Misc shared libs

    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(release_files
        openjp2.dll
        SDL2.dll
        )

    if(LLCOMMON_LINK_SHARED)
        set(release_files ${release_files} libapr-1.dll)
        set(release_files ${release_files} libaprutil-1.dll)
    endif()

    # Filenames are different for 32/64 bit BugSplat file and we don't
    # have any control over them so need to branch.
    if (USE_BUGSPLAT)
      if(ADDRESS_SIZE EQUAL 32)
        set(release_files ${release_files} BugSplat.dll)
        set(release_files ${release_files} BugSplatRc.dll)
        set(release_files ${release_files} BsSndRpt.exe)
      else(ADDRESS_SIZE EQUAL 32)
        set(release_files ${release_files} BugSplat64.dll)
        set(release_files ${release_files} BugSplatRc64.dll)
        set(release_files ${release_files} BsSndRpt64.exe)
      endif(ADDRESS_SIZE EQUAL 32)
    endif (USE_BUGSPLAT)

    if (TARGET ll::openal)
        list(APPEND release_files openal32.dll alut.dll)
    endif ()

    #*******************************
    # Copy MS C runtime dlls, required for packaging.
    if (MSVC80)
        set(MSVC_VER 80)
    elseif (MSVC_VERSION EQUAL 1600) # VisualStudio 2010
        MESSAGE(STATUS "MSVC_VERSION ${MSVC_VERSION}")
    elseif (MSVC_VERSION EQUAL 1800) # VisualStudio 2013, which is (sigh) VS 12
        set(MSVC_VER 120)
    elseif (MSVC_VERSION GREATER_EQUAL 1910 AND MSVC_VERSION LESS 1920) # Visual Studio 2017
        set(MSVC_VER 140)
        set(MSVC_TOOLSET_VER 141)
    elseif (MSVC_VERSION GREATER_EQUAL 1920 AND MSVC_VERSION LESS 1930) # Visual Studio 2019
        set(MSVC_VER 140)
        set(MSVC_TOOLSET_VER 142)
    elseif (MSVC_VERSION GREATER_EQUAL 1930 AND MSVC_VERSION LESS 1950) # Visual Studio 2022
        set(MSVC_VER 140)
        set(MSVC_TOOLSET_VER 143)
    else (MSVC80)
        MESSAGE(WARNING "New MSVC_VERSION ${MSVC_VERSION} of MSVC: adapt Copy3rdPartyLibs.cmake")
    endif (MSVC80)

    if (MSVC_TOOLSET_VER AND DEFINED ENV{VCTOOLSREDISTDIR})
        if(ADDRESS_SIZE EQUAL 32)
            set(redist_find_path "$ENV{VCTOOLSREDISTDIR}x86\\Microsoft.VC${MSVC_TOOLSET_VER}.CRT")
        else(ADDRESS_SIZE EQUAL 32)
            set(redist_find_path "$ENV{VCTOOLSREDISTDIR}x64\\Microsoft.VC${MSVC_TOOLSET_VER}.CRT")
        endif(ADDRESS_SIZE EQUAL 32)
        get_filename_component(redist_path_component "${redist_find_path}" ABSOLUTE)
        set(redist_path ${redist_path_component} CACHE INTERNAL "MSVC Redist Path" FORCE)
        MESSAGE(STATUS "VC Runtime redist path: ${redist_path}")
    endif (MSVC_TOOLSET_VER AND DEFINED ENV{VCTOOLSREDISTDIR})

    if(ADDRESS_SIZE EQUAL 32)
        # this folder contains the 32bit DLLs.. (yes really!)
        set(registry_find_path "[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/SysWOW64")
    else(ADDRESS_SIZE EQUAL 32)
        # this folder contains the 64bit DLLs.. (yes really!)
        set(registry_find_path "[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/System32")
    endif(ADDRESS_SIZE EQUAL 32)

    # Having a string containing the system registry path is a start, but to
    # get CMake to actually read the registry, we must engage some other
    # operation.
    get_filename_component(registry_path "${registry_find_path}" ABSOLUTE)

    # These are candidate DLL names. Empirically, VS versions before 2015 have
    # msvcp*.dll and msvcr*.dll. VS 2017 has msvcp*.dll and vcruntime*.dll.
    # Check each of them.
    foreach(release_msvc_file
            msvcp${MSVC_VER}.dll
            msvcp${MSVC_VER}_1.dll
            msvcp${MSVC_VER}_2.dll
            msvcp${MSVC_VER}_atomic_wait.dll
            msvcp${MSVC_VER}_codecvt_ids.dll
            vcruntime${MSVC_VER}.dll
            vcruntime${MSVC_VER}_1.dll
            vcruntime${MSVC_VER}_threads.dll
            )
        if(DEFINED redist_path AND EXISTS "${redist_path}/${release_msvc_file}")
            MESSAGE(STATUS "Copying redist file from ${redist_path}/${release_msvc_file}")
            to_staging_dirs(
                ${redist_path}
                third_party_targets
                ${release_msvc_file})
        elseif(EXISTS "${registry_path}/${release_msvc_file}")
            MESSAGE(STATUS "Copying redist file from ${registry_path}/${release_msvc_file}")
            to_staging_dirs(
                ${registry_path}
                third_party_targets
                ${release_msvc_file})
        else()
            # This isn't a WARNING because, as noted above, every VS version
            # we've observed has only a subset of the specified DLL names.
            MESSAGE(STATUS "Redist lib ${release_msvc_file} not found")
        endif()
    endforeach()

elseif(DARWIN)
    set(vivox_lib_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(slvoice_files SLVoice)
    set(vivox_libs
        libortp.dylib
        libvivoxsdk.dylib
       )
    set(debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
    set(debug_files
       )
    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(release_files
        libndofdev.dylib
        libSDL2.dylib
        libSDL2-2.0.dylib
       )

    if(LLCOMMON_LINK_SHARED)
        set(release_files ${release_files}
            libapr-1.0.dylib
            libapr-1.dylib
            libaprutil-1.0.dylib
            libaprutil-1.dylib
            )
    endif()

    if (TARGET ll::openal)
      list(APPEND release_files libalut.dylib libopenal.dylib)
    endif ()

elseif(LINUX)
    # linux is weird, multiple side by side configurations aren't supported
    # and we don't seem to have any debug shared libs built yet anyways...
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}")

    set(vivox_lib_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(vivox_libs
        libsndfile.so.1
        libortp.so
        libvivoxoal.so.1
        libvivoxsdk.so
        )
    set(slvoice_files SLVoice)

    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
    set(debug_files
       )
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    # *FIX - figure out what to do with duplicate libalut.so here -brad
    set(release_files
        libSDL2-2.0.so.0
        )

     if( USE_AUTOBUILD_3P )
         list( APPEND release_files
                 libapr-1.so.0
                 libaprutil-1.so.0
                 )

        if(LLCOMMON_LINK_SHARED)
            set(release_files ${release_files}
                libapr-1.so.0
                libaprutil-1.so.0
                )
        endif()
     endif()

else(WINDOWS)
    message(STATUS "WARNING: unrecognized platform for staging 3rd party libs, skipping...")
    set(vivox_lib_dir "${CMAKE_SOURCE_DIR}/newview/vivox-runtime/i686-linux")
    set(vivox_libs "")
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(debug_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-linux/lib/debug")
    set(debug_files "")
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(release_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-linux/lib/release")
    set(release_files "")

    set(debug_llkdu_src "")
    set(debug_llkdu_dst "")
    set(release_llkdu_src "")
    set(release_llkdu_dst "")
    set(relwithdebinfo_llkdu_dst "")
endif(WINDOWS)


################################################################
# Done building the file lists, now set up the copy commands.
################################################################

# Curiously, slvoice_files are only copied to SHARED_LIB_STAGING_DIR_RELEASE.
# It's unclear whether this is oversight or intentional, but anyway leave the
# single copy_if_different command rather than using to_staging_dirs.

if( slvoice_src_dir )
    copy_if_different(
            ${slvoice_src_dir}
            "${SHARED_LIB_STAGING_DIR_RELEASE}"
            out_targets
            ${slvoice_files}
    )
    list(APPEND third_party_targets ${out_targets})
endif()

to_staging_dirs(
    ${vivox_lib_dir}
    third_party_targets
    ${vivox_libs}
    )

to_staging_dirs(
    ${release_src_dir}
    third_party_targets
    ${release_files}
    )

add_custom_target(
        stage_third_party_libs ALL
        DEPENDS ${third_party_targets}
)

if(DARWIN)
    # Support our "@executable_path/../Resources" load path for executables
    # that end up in any of the above SHARED_LIB_STAGING_DIR_MUMBLE
    # directories.
    add_custom_command( TARGET stage_third_party_libs POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${SHARED_LIB_STAGING_DIR} ${CMAKE_BINARY_DIR}/sharedlibs/Resources
            )
endif()
