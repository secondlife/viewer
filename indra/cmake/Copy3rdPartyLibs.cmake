# -*- cmake -*-

# The copy_win_libs folder contains file lists and a script used to
# copy dlls, exes and such needed to run the SecondLife from within
# VisualStudio.

include(CMakeCopyIfDifferent)

if(WINDOWS)
#*******************************
# VIVOX - *NOTE: no debug version
set(vivox_src_dir "${CMAKE_SOURCE_DIR}/newview/vivox-runtime/i686-win32")
set(vivox_files
    SLVoice.exe
    alut.dll
    vivoxsdk.dll
    ortp.dll
    wrap_oal.dll
    )
copy_if_different(
    ${vivox_src_dir}
    "${SHARED_LIB_STAGING_DIR}/Debug"
    out_targets 
   ${vivox_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${vivox_src_dir}
    "${SHARED_LIB_STAGING_DIR}/Release"
    out_targets
    ${vivox_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${vivox_src_dir}
    "${SHARED_LIB_STAGING_DIR}/RelWithDebInfo"
    out_targets
    ${vivox_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

#*******************************
# Misc shared libs 
set(debug_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-win32/lib/debug")
set(debug_files
    openjpegd.dll
    libtcmalloc_minimal-debug.dll
    libapr-1.dll
    libaprutil-1.dll
    libapriconv-1.dll
    )
if (FMOD_SDK_DIR)
    set(fmod_files fmod.dll)
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

copy_if_different(
    ${debug_src_dir}
    "${SHARED_LIB_STAGING_DIR}/Debug"
    out_targets
    ${debug_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

set(release_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-win32/lib/release")
set(release_files
    openjpeg.dll
    libtcmalloc_minimal.dll
    libapr-1.dll
    libaprutil-1.dll
    libapriconv-1.dll
    )

copy_if_different(
    ${release_src_dir}
    "${SHARED_LIB_STAGING_DIR}/Release"
    out_targets
    ${release_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${release_src_dir}
    "${SHARED_LIB_STAGING_DIR}/RelWithDebInfo"
    out_targets
    ${release_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

#*******************************
# LLKDU
set(internal_llkdu_path "${CMAKE_SOURCE_DIR}/llkdu")
if(NOT EXISTS ${internal_llkdu_path})
    if (EXISTS "${debug_src_dir}/llkdu.dll")
        set(debug_llkdu_src "${debug_src_dir}/llkdu.dll")
        set(debug_llkdu_dst "${SHARED_LIB_STAGING_DIR}/Debug/llkdu.dll")
        ADD_CUSTOM_COMMAND(
            OUTPUT  ${debug_llkdu_dst}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${debug_llkdu_src} ${debug_llkdu_dst}
            DEPENDS ${debug_llkdu_src}
            COMMENT "Copying llkdu.dll ${SHARED_LIB_STAGING_DIR}/Debug"
            )
        set(third_party_targets ${third_party_targets} $} ${debug_llkdu_dst})
    endif (EXISTS "${debug_src_dir}/llkdu.dll")

    if (EXISTS "${release_src_dir}/llkdu.dll")
        set(release_llkdu_src "${release_src_dir}/llkdu.dll")
        set(release_llkdu_dst "${SHARED_LIB_STAGING_DIR}/Release/llkdu.dll")
        ADD_CUSTOM_COMMAND(
            OUTPUT  ${release_llkdu_dst}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${release_llkdu_src} ${release_llkdu_dst}
            DEPENDS ${release_llkdu_src}
            COMMENT "Copying llkdu.dll ${SHARED_LIB_STAGING_DIR}/Release"
            )
        set(third_party_targets ${third_party_targets} ${release_llkdu_dst})

        set(relwithdebinfo_llkdu_dst "${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/llkdu.dll")
        ADD_CUSTOM_COMMAND(
            OUTPUT  ${relwithdebinfo_llkdu_dst}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${release_llkdu_src} ${relwithdebinfo_llkdu_dst}
            DEPENDS ${release_llkdu_src}
            COMMENT "Copying llkdu.dll ${SHARED_LIB_STAGING_DIR}/RelWithDebInfo"
            )
        set(third_party_targets ${third_party_targets} ${relwithdebinfo_llkdu_dst})
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
            "${SHARED_LIB_STAGING_DIR}/Debug"
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
            "${SHARED_LIB_STAGING_DIR}/Release"
            out_targets
            ${release_msvc8_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})

        copy_if_different(
            ${release_msvc8_redist_path}
            "${SHARED_LIB_STAGING_DIR}/RelWithDebInfo"
            out_targets
            ${release_msvc8_files}
            )
        set(third_party_targets ${third_party_targets} ${out_targets})
          
    endif (EXISTS ${release_msvc8_redist_path})
endif (MSVC80)

elseif(DARWIN)
    
elseif(LINUX)
    
else(WINDOWS)
    message(STATUS "WARNING: unrecognized platform for staging 3rd party libs, skipping...")
endif(WINDOWS)

add_custom_target(stage_third_party_libs ALL
  DEPENDS 
    ${third_party_targets}
  )
