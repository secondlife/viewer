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
#*******************************
# Misc shared libs

set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
set(release_files
    openjp2.dll
    )

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
    get_filename_component(redist_path "${redist_find_path}" ABSOLUTE)
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
        msvcr${MSVC_VER}.dll
        vcruntime${MSVC_VER}.dll
        vcruntime${MSVC_VER}_1.dll
        vcruntime${MSVC_VER}_threads.dll
        )
    if(redist_path AND EXISTS "${redist_path}/${release_msvc_file}")
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


################################################################
# Done building the file lists, now set up the copy commands.
################################################################

to_staging_dirs(
    ${release_src_dir}
    third_party_targets
    ${release_files}
    )

add_custom_target(
        stage_third_party_libs ALL
        DEPENDS ${third_party_targets}
)
