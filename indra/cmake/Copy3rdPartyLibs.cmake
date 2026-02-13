# -*- cmake -*-

# The copy_win_libs folder contains file lists and a script used to
# copy dlls, exes and such needed to run the SecondLife from within
# VisualStudio.
include_guard()
include(CMakeCopyIfDifferent)
include(Linking)
if (USE_DISCORD)
  include(Discord)
endif ()
include(OpenAL)

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


    # ND, it seems there is no such thing defined. At least when building a viewer
    # Does this maybe matter on some LL buildserver? Otherwise this and the snippet using slvoice_src_dir
    # can all go
    if(USE_VCPKG)
        set(vivox_lib_dir "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/slvoice/windows64")
    elseif( ARCH_PREBUILT_BIN_RELEASE )
        set(vivox_lib_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    endif()
    list(APPEND vivox_libs
        vivoxsdk_x64.dll
        ortp_x64.dll
        SLVoice.exe
        )


    #*******************************
    # Misc shared libs

    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(release_files
        )

    if (NOT USE_VCPKG)
      if (USE_SDL_WINDOW)
        list(APPEND release_files SDL3.dll)
      endif ()
        list(APPEND release_files openjp2.dll)
    endif ()

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

    if (TARGET ll::discord_sdk)
        list(APPEND release_files discord_partner_sdk.dll)
    endif ()

    if (TARGET ll::openal AND NOT USE_VCPKG)
        list(APPEND release_files openal32.dll alut.dll)
    endif ()

    #*******************************
    # Copy MS C runtime dlls, required for packaging.
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)

    foreach(system_lib_file IN LISTS CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        get_filename_component(system_lib_directory ${system_lib_file} DIRECTORY)
        get_filename_component(system_lib_filename ${system_lib_file} NAME )
        MESSAGE(STATUS "Copying redist file from ${system_lib_directory}/${system_lib_filename}")
        to_staging_dirs(
            ${system_lib_directory}
            third_party_targets
            ${system_lib_filename}
        )
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
       )

    if (TARGET ll::discord_sdk)
      list(APPEND release_files libdiscord_partner_sdk.dylib)
    endif ()

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
       )

    list( APPEND release_files
        libSDL3.so
        libSDL3.so.0
        libSDL3.so.0.2.24
        )
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

to_staging_dirs(
    ${vivox_lib_dir}
    third_party_targets
    ${vivox_libs}
    )
if(NOT USE_VCPKG)
    to_staging_dirs(
        ${release_src_dir}
        third_party_targets
        ${release_files}
        )
endif()

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
