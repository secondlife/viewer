# -*- cmake -*-

# The copy_win_libs folder contains file lists and a script used to
# copy dlls, exes and such needed to run the SecondLife from within
# VisualStudio.

include(CMakeCopyIfDifferent)

###################################################################
# set up platform specific lists of files that need to be copied
###################################################################
if(WINDOWS)
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}/Debug")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}/RelWithDebInfo")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}/Release")

    #*******************************
    # VIVOX - *NOTE: no debug version
    set(vivox_src_dir "${CMAKE_SOURCE_DIR}/newview/vivox-runtime/i686-win32")
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

    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(debug_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-win32/lib/debug")
    set(debug_files
        openjpegd.dll
        libapr-1.dll
        libaprutil-1.dll
        libapriconv-1.dll
        libcollada14dom21-d.dll
        glod.dll
        )

    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(release_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-win32/lib/release")
    set(release_files
        openjpeg.dll
        libapr-1.dll
        libaprutil-1.dll
        libapriconv-1.dll
        libcollada14dom21.dll
        glod.dll
        )

    if(USE_GOOGLE_PERFTOOLS)
      set(debug_files ${debug_files} libtcmalloc_minimal-debug.dll)
      set(release_files ${release_files} libtcmalloc_minimal.dll)
    endif(USE_GOOGLE_PERFTOOLS)

    if (FMOD)
      set(debug_files ${debug_files} fmod.dll)
      set(release_files ${release_files} fmod.dll)
    endif (FMOD)

    #*******************************
    # LLKDU
    set(internal_llkdu_path "${CMAKE_SOURCE_DIR}/llkdu")
    if(NOT EXISTS ${internal_llkdu_path})
        if (EXISTS "${debug_src_dir}/llkdu.dll")
            set(debug_llkdu_src "${debug_src_dir}/llkdu.dll")
            set(debug_llkdu_dst "${SHARED_LIB_STAGING_DIR_DEBUG}/llkdu.dll")
        endif (EXISTS "${debug_src_dir}/llkdu.dll")

        if (EXISTS "${release_src_dir}/llkdu.dll")
            set(release_llkdu_src "${release_src_dir}/llkdu.dll")
            set(release_llkdu_dst "${SHARED_LIB_STAGING_DIR_RELEASE}/llkdu.dll")
            set(relwithdebinfo_llkdu_dst "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}/llkdu.dll")
        endif (EXISTS "${release_src_dir}/llkdu.dll")
    endif (NOT EXISTS ${internal_llkdu_path})

#*******************************
# Copy MS C runtime dlls, required for packaging.
# *TODO - Adapt this to support VC9
if (MSVC80)
    FIND_PATH(debug_msvc8_redist_path msvcr80d.dll
        PATHS
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
endif (MSVC80)

elseif(DARWIN)
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}/Debug/Resources")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/Resources")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}/Release/Resources")

    set(vivox_src_dir "${CMAKE_SOURCE_DIR}/newview/vivox-runtime/universal-darwin")
    set(vivox_files
        SLVoice
        libsndfile.dylib
        libvivoxoal.dylib
        libortp.dylib
        libvivoxplatform.dylib
        libvivoxsdk.dylib
       )
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(debug_src_dir "${CMAKE_SOURCE_DIR}/../libraries/universal-darwin/lib_debug")
    set(debug_files
       )
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(release_src_dir "${CMAKE_SOURCE_DIR}/../libraries/universal-darwin/lib_release")
    set(release_files
        libapr-1.0.3.7.dylib
        libapr-1.dylib
        libaprutil-1.0.3.8.dylib
        libaprutil-1.dylib
        libexpat.0.5.0.dylib
        libexpat.dylib
        libGLOD.dylib
	libllqtwebkit.dylib
        libndofdev.dylib
        libexception_handler.dylib
	libcollada14dom.dylib
       )

    # fmod is statically linked on darwin
    set(fmod_files "")

    #*******************************
    # LLKDU
    set(internal_llkdu_path "${CMAKE_SOURCE_DIR}/llkdu")
    if(NOT EXISTS ${internal_llkdu_path})
        if (EXISTS "${debug_src_dir}/libllkdu.dylib")
            set(debug_llkdu_src "${debug_src_dir}/libllkdu.dylib")
            set(debug_llkdu_dst "${SHARED_LIB_STAGING_DIR_DEBUG}/libllkdu.dylib")
        endif (EXISTS "${debug_src_dir}/libllkdu.dylib")

        if (EXISTS "${release_src_dir}/libllkdu.dylib")
            set(release_llkdu_src "${release_src_dir}/libllkdu.dylib")
            set(release_llkdu_dst "${SHARED_LIB_STAGING_DIR_RELEASE}/libllkdu.dylib")
            set(relwithdebinfo_llkdu_dst "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}/libllkdu.dylib")
        endif (EXISTS "${release_src_dir}/libllkdu.dylib")
    endif (NOT EXISTS ${internal_llkdu_path})
elseif(LINUX)
    # linux is weird, multiple side by side configurations aren't supported
    # and we don't seem to have any debug shared libs built yet anyways...
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}")

    set(vivox_src_dir "${CMAKE_SOURCE_DIR}/newview/vivox-runtime/i686-linux")
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
    set(debug_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-linux/lib_debug")
    set(debug_files
       )
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(release_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-linux/lib_release_client")
    # *FIX - figure out what to do with duplicate libalut.so here -brad
    set(release_files
        libapr-1.so.0
        libaprutil-1.so.0
        libatk-1.0.so
        libbreakpad_client.so.0
        libcrypto.so.0.9.7
        libdb-4.2.so
        libexpat.so
        libexpat.so.1
        libgmock_main.so
        libgmock.so.0
        libgmodule-2.0.so
        libgobject-2.0.so
        libgtest_main.so
        libgtest.so.0
        libopenal.so
        libopenjpeg.so
        libssl.so
        libstacktrace.so
        libtcmalloc_minimal.so
	libtcmalloc_minimal.so.0
        libssl.so.0.9.7
       )

    if (FMOD)
      set(release_files ${release_files} "libfmod-3.75.so")
    endif (FMOD)

    #*******************************
    # LLKDU
    set(internal_llkdu_path "${CMAKE_SOURCE_DIR}/llkdu")
    if(NOT EXISTS ${internal_llkdu_path})
        if (EXISTS "${debug_src_dir}/libllkdu.so")
            set(debug_llkdu_src "${debug_src_dir}/libllkdu.so")
            set(debug_llkdu_dst "${SHARED_LIB_STAGING_DIR_DEBUG}/libllkdu.so")
        endif (EXISTS "${debug_src_dir}/libllkdu.so")

        if (EXISTS "${release_src_dir}/libllkdu.so")
            set(release_llkdu_src "${release_src_dir}/libllkdu.so")
            set(release_llkdu_dst "${SHARED_LIB_STAGING_DIR_RELEASE}/libllkdu.so")
            set(relwithdebinfo_llkdu_dst "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}/libllkdu.so")
        endif (EXISTS "${release_src_dir}/libllkdu.so")
    endif(NOT EXISTS ${internal_llkdu_path})
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

#*******************************
# LLKDU
set(internal_llkdu_path "${CMAKE_SOURCE_DIR}/llkdu")
if(NOT EXISTS ${internal_llkdu_path})
    if (EXISTS "${debug_llkdu_src}")
        ADD_CUSTOM_COMMAND(
            OUTPUT  ${debug_llkdu_dst}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${debug_llkdu_src} ${debug_llkdu_dst}
            DEPENDS ${debug_llkdu_src}
            COMMENT "Copying llkdu.dll ${SHARED_LIB_STAGING_DIR_DEBUG}"
            )
        set(third_party_targets ${third_party_targets} $} ${debug_llkdu_dst})
    endif (EXISTS "${debug_llkdu_src}")

    if (EXISTS "${release_llkdu_src}")
        ADD_CUSTOM_COMMAND(
            OUTPUT  ${release_llkdu_dst}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${release_llkdu_src} ${release_llkdu_dst}
            DEPENDS ${release_llkdu_src}
            COMMENT "Copying llkdu.dll ${SHARED_LIB_STAGING_DIR_RELEASE}"
            )
        set(third_party_targets ${third_party_targets} ${release_llkdu_dst})

        ADD_CUSTOM_COMMAND(
            OUTPUT  ${relwithdebinfo_llkdu_dst}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${release_llkdu_src} ${relwithdebinfo_llkdu_dst}
            DEPENDS ${release_llkdu_src}
            COMMENT "Copying llkdu.dll ${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
            )
        set(third_party_targets ${third_party_targets} ${relwithdebinfo_llkdu_dst})
    endif (EXISTS "${release_llkdu_src}")

endif (NOT EXISTS ${internal_llkdu_path})


add_custom_target(stage_third_party_libs ALL
  DEPENDS 
    ${third_party_targets}
  )
