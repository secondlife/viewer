# -*- cmake -*-

# The copy_win_libs folder contains file lists and a script used to
# copy dlls, exes and such needed to run the SecondLife from within
# VisualStudio.
include_guard()
include(CMakeCopyIfDifferent)
include(Linking)

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
    set(vivox_lib_dir "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/slvoice/windows64")
    list(APPEND vivox_libs
        vivoxsdk_x64.dll
        ortp_x64.dll
        SLVoice.exe
        )

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
    set(vivox_lib_dir "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/slvoice/darwin64")
    set(vivox_libs
        libortp.dylib
        libvivoxsdk.dylib
       )
    set(slvoice_files SLVoice)
elseif(LINUX)
    set(vivox_lib_dir "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/slvoice/linux")
    set(vivox_libs
        libsndfile.so.1
        libortp.so
        libvivoxoal.so.1
        libvivoxplatform.so
        libvivoxsdk.so
        )
    set(slvoice_files SLVoice)
else(WINDOWS)
    message(STATUS "WARNING: unrecognized platform for staging 3rd party libs, skipping...")
    set(vivox_lib_dir "${CMAKE_SOURCE_DIR}/newview/vivox-runtime/i686-linux")
    set(vivox_libs "")
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

add_custom_target(
        stage_third_party_libs ALL
        DEPENDS ${third_party_targets}
)
