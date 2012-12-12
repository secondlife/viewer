# -*- cmake -*-

# The copy_win_libs folder contains file lists and a script used to
# copy dlls, exes and such needed to run the SecondLife from within
# VisualStudio.

include(CMakeCopyIfDifferent)
include(Linking)

###################################################################
# set up platform specific lists of files that need to be copied
###################################################################
if(WINDOWS)
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}/Debug")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}/RelWithDebInfo")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}/Release")

    #*******************************
    # VIVOX - *NOTE: no debug version
    set(vivox_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(vivox_files
        SLVoice.exe
        libsndfile-1.dll
        vivoxplatform.dll
        vivoxsdk.dll
        ortp.dll
        zlib1.dll
        vivoxoal.dll
        )

    #*******************************
    # Misc shared libs 

    set(debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
    set(debug_files
        openjpegd.dll
        libapr-1.dll
        libaprutil-1.dll
        libapriconv-1.dll
        ssleay32.dll
        libeay32.dll
        libcollada14dom22-d.dll
        glod.dll    
        libhunspell.dll
        )

    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(release_files
        openjpeg.dll
        libapr-1.dll
        libaprutil-1.dll
        libapriconv-1.dll
        ssleay32.dll
        libeay32.dll
        libcollada14dom22.dll
        glod.dll
        libhunspell.dll
        )

    if(USE_TCMALLOC)
      set(debug_files ${debug_files} libtcmalloc_minimal-debug.dll)
      set(release_files ${release_files} libtcmalloc_minimal.dll)
    endif(USE_TCMALLOC)

    if (FMOD)
      set(debug_files ${debug_files} fmod.dll)
      set(release_files ${release_files} fmod.dll)
    endif (FMOD)

#*******************************
# Copy MS C runtime dlls, required for packaging.
# *TODO - Adapt this to support VC9
if (MSVC80)
    FIND_PATH(debug_msvc8_redist_path msvcr80d.dll
        PATHS
        ${MSVC_DEBUG_REDIST_PATH}
         [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\8.0\\Setup\\VC;ProductDir]/redist/Debug_NonRedist/x86/Microsoft.VC80.DebugCRT
        NO_DEFAULT_PATH
        NO_DEFAULT_PATH
        )

    if(EXISTS ${debug_msvc8_redist_path})
        set(debug_msvc8_files
            msvcr80d.dll
            msvcp80d.dll
            Microsoft.VC80.DebugCRT.manifest
            )

        copy_if_different(
            ${debug_msvc8_redist_path}
            "${SHARED_LIB_STAGING_DIR_DEBUG}"
            out_targets
            ${debug_msvc8_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})

    endif (EXISTS ${debug_msvc8_redist_path})

    FIND_PATH(release_msvc8_redist_path msvcr80.dll
        PATHS
        ${MSVC_REDIST_PATH}
         [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\8.0\\Setup\\VC;ProductDir]/redist/x86/Microsoft.VC80.CRT
        NO_DEFAULT_PATH
        NO_DEFAULT_PATH
        )

    if(EXISTS ${release_msvc8_redist_path})
        set(release_msvc8_files
            msvcr80.dll
            msvcp80.dll
            Microsoft.VC80.CRT.manifest
            )

        copy_if_different(
            ${release_msvc8_redist_path}
            "${SHARED_LIB_STAGING_DIR_RELEASE}"
            out_targets
            ${release_msvc8_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})

        copy_if_different(
            ${release_msvc8_redist_path}
            "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
            out_targets
            ${release_msvc8_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})
          
    endif (EXISTS ${release_msvc8_redist_path})
elseif (MSVC_VERSION EQUAL 1600) # VisualStudio 2010
    FIND_PATH(debug_msvc10_redist_path msvcr100d.dll
        PATHS
        ${MSVC_DEBUG_REDIST_PATH}
         [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\10.0\\Setup\\VC;ProductDir]/redist/Debug_NonRedist/x86/Microsoft.VC100.DebugCRT
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/SysWOW64
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/System32
        NO_DEFAULT_PATH
        )

    if(EXISTS ${debug_msvc10_redist_path})
        set(debug_msvc10_files
            msvcr100d.dll
            msvcp100d.dll
            )

        copy_if_different(
            ${debug_msvc10_redist_path}
            "${SHARED_LIB_STAGING_DIR_DEBUG}"
            out_targets
            ${debug_msvc10_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})

    endif ()

    FIND_PATH(release_msvc10_redist_path msvcr100.dll
        PATHS
        ${MSVC_REDIST_PATH}
         [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\10.0\\Setup\\VC;ProductDir]/redist/x86/Microsoft.VC100.CRT
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/SysWOW64
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Windows;Directory]/System32
        NO_DEFAULT_PATH
        )

    if(EXISTS ${release_msvc10_redist_path})
        set(release_msvc10_files
            msvcr100.dll
            msvcp100.dll
            )

        copy_if_different(
            ${release_msvc10_redist_path}
            "${SHARED_LIB_STAGING_DIR_RELEASE}"
            out_targets
            ${release_msvc10_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})

        copy_if_different(
            ${release_msvc10_redist_path}
            "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
            out_targets
            ${release_msvc10_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})
          
    endif ()
endif (MSVC80)

elseif(DARWIN)
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}/Debug/Resources")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/Resources")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}/Release/Resources")

    set(vivox_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(vivox_files
        SLVoice
        libsndfile.dylib
        libvivoxoal.dylib
        libortp.dylib
        libvivoxplatform.dylib
        libvivoxsdk.dylib
       )
    set(debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
    set(debug_files
       )
    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(release_files
        libapr-1.0.dylib
        libapr-1.dylib
        libaprutil-1.0.dylib
        libaprutil-1.dylib
        libexpat.1.5.2.dylib
        libexpat.dylib
        libGLOD.dylib
        libllqtwebkit.dylib
        libminizip.a
        libndofdev.dylib
        libhunspell-1.3.0.dylib
        libexception_handler.dylib
        libcollada14dom.dylib
       )

    # fmod is statically linked on darwin
    set(fmod_files "")

elseif(LINUX)
    # linux is weird, multiple side by side configurations aren't supported
    # and we don't seem to have any debug shared libs built yet anyways...
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}")

    set(vivox_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(vivox_files
        libsndfile.so.1
        libortp.so
        libvivoxoal.so.1
        libvivoxplatform.so
        libvivoxsdk.so
        SLVoice
       )
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
        libapr-1.so.0
        libaprutil-1.so.0
        libatk-1.0.so
        libboost_program_options-mt.so.${BOOST_VERSION}.0
        libboost_regex-mt.so.${BOOST_VERSION}.0
        libboost_thread-mt.so.${BOOST_VERSION}.0
        libboost_filesystem-mt.so.${BOOST_VERSION}.0
        libboost_signals-mt.so.${BOOST_VERSION}.0
        libboost_system-mt.so.${BOOST_VERSION}.0
        libbreakpad_client.so.0
        libcollada14dom.so
        libcrypto.so.1.0.0
        libdb-5.1.so
        libexpat.so
        libexpat.so.1
        libGLOD.so
        libgmock_main.so
        libgmock.so.0
        libgmodule-2.0.so
        libgobject-2.0.so
        libgtest_main.so
        libgtest.so.0
        libhunspell-1.3.so.0.0.0
        libminizip.so
        libopenal.so
        libopenjpeg.so
        libssl.so
        libuuid.so.16
        libuuid.so.16.0.22
        libssl.so.1.0.0
        libfontconfig.so.1.4.4
       )

    if (USE_TCMALLOC)
      set(release_files ${release_files} "libtcmalloc_minimal.so")
    endif (USE_TCMALLOC)

    if (FMOD)
      set(release_files ${release_files} "libfmod-3.75.so")
    endif (FMOD)

else(WINDOWS)
    message(STATUS "WARNING: unrecognized platform for staging 3rd party libs, skipping...")
    set(vivox_src_dir "${CMAKE_SOURCE_DIR}/newview/vivox-runtime/i686-linux")
    set(vivox_files "")
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(debug_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-linux/lib/debug")
    set(debug_files "")
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(release_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-linux/lib/release")
    set(release_files "")

    set(fmod_files "")

    set(debug_llkdu_src "")
    set(debug_llkdu_dst "")
    set(release_llkdu_src "")
    set(release_llkdu_dst "")
    set(relwithdebinfo_llkdu_dst "")
endif(WINDOWS)


################################################################
# Done building the file lists, now set up the copy commands.
################################################################

copy_if_different(
    ${vivox_src_dir}
    "${SHARED_LIB_STAGING_DIR_DEBUG}"
    out_targets 
    ${vivox_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${vivox_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELEASE}"
    out_targets
    ${vivox_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${vivox_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
    out_targets
    ${vivox_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})



copy_if_different(
    ${debug_src_dir}
    "${SHARED_LIB_STAGING_DIR_DEBUG}"
    out_targets
    ${debug_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${release_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELEASE}"
    out_targets
    ${release_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${release_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
    out_targets
    ${release_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

if (FMOD_SDK_DIR)
    copy_if_different(
        ${FMOD_SDK_DIR} 
        "${CMAKE_CURRENT_BINARY_DIR}/Debug"
        out_targets 
        ${fmod_files}
        )
    set(all_targets ${all_targets} ${out_targets})
    copy_if_different(
        ${FMOD_SDK_DIR} 
        "${CMAKE_CURRENT_BINARY_DIR}/Release"
        out_targets 
        ${fmod_files}
        )
    set(all_targets ${all_targets} ${out_targets})
    copy_if_different(
        ${FMOD_SDK_DIR} 
        "${CMAKE_CURRENT_BINARY_DIR}/RelWithDbgInfo"
        out_targets 
        ${fmod_files}
        )
    set(all_targets ${all_targets} ${out_targets})
endif (FMOD_SDK_DIR)

if(NOT STANDALONE)
  add_custom_target(
      stage_third_party_libs ALL
      DEPENDS ${third_party_targets}
      )
endif(NOT STANDALONE)
