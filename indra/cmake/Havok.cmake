# -*- cmake -*-
include(Prebuilt)
include_guard()

use_prebuilt_binary(havok-source)

set(Havok_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/havok/Source)
list(APPEND Havok_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/havok/Demo)

# HK_DISABLE_IMPLICIT_VVECTOR3_CONVERSION suppresses an intended conversion
# function which Xcode scolds us will unconditionally enter infinite
# recursion if called. This hides that function.
add_definitions("-DHK_DISABLE_IMPLICIT_VVECTOR3_CONVERSION")

set(HAVOK_DEBUG_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/lib/debug/havok-fulldebug)
set(HAVOK_RELEASE_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/lib/release/havok)

if (LL_DEBUG_HAVOK)
   if (WIN32)
      # Always link relwithdebinfo to havok-hybrid on windows.
      set(HAVOK_RELWITHDEBINFO_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/lib/debug/havok-hybrid)
   else (WIN32)
      set(HAVOK_RELWITHDEBINFO_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/lib/debug/havok-fulldebug)
   endif (WIN32)
else (LL_DEBUG_HAVOK)
   set(HAVOK_RELWITHDEBINFO_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/lib/release/havok)
endif (LL_DEBUG_HAVOK)

set(HAVOK_LIBS
    hkgpConvexDecomposition
    hkGeometryUtilities
    hkSerialize
    hkSceneData
    hkpCollide
    hkpUtilities
    hkpConstraintSolver
    hkpDynamics
    hkaiPathfinding
    hkaiAiPhysicsBridge
    hkcdCollide
    hkpVehicle
    hkVisualize
    hkaiVisualize
    hkaiInternal
    hkcdInternal
    hkpInternal
    hkInternal
    hkCompat
    hkBase
)

unset(HK_DEBUG_LIBRARIES)
unset(HK_RELEASE_LIBRARIES)
unset(HK_RELWITHDEBINFO_LIBRARIES)

if (DEBUG_PREBUILT)
  # DEBUG_MESSAGE() displays debugging message
  function(DEBUG_MESSAGE)
    # prints message args separated by semicolons rather than spaces,
    # but making it pretty is a lot more work
    message(STATUS "${ARGN}")
  endfunction(DEBUG_MESSAGE)
  function(DEBUG_EXEC_FUNC)
    execute_process(COMMAND ${ARGN})
  endfunction(DEBUG_EXEC_FUNC)
else (DEBUG_PREBUILT)
  # without DEBUG_PREBUILT, DEBUG_MESSAGE() is a no-op
  function(DEBUG_MESSAGE)
  endfunction(DEBUG_MESSAGE)
  function(DEBUG_EXEC_FUNC)
    execute_process(COMMAND ${ARGN} OUTPUT_QUIET)
  endfunction(DEBUG_EXEC_FUNC)
endif (DEBUG_PREBUILT)

# DEBUG_EXEC() reports each execute_process() before invoking
function(DEBUG_EXEC)
  DEBUG_MESSAGE(${ARGN})
  DEBUG_EXEC_FUNC(${ARGN})
endfunction(DEBUG_EXEC)

# *TODO: Figure out why we need to extract like this...
foreach(HAVOK_LIB ${HAVOK_LIBS})
  find_library(HAVOK_DEBUG_LIB_${HAVOK_LIB}   ${HAVOK_LIB} PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
  find_library(HAVOK_RELEASE_LIB_${HAVOK_LIB} ${HAVOK_LIB} PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
  find_library(HAVOK_RELWITHDEBINFO_LIB_${HAVOK_LIB} ${HAVOK_LIB} PATHS ${HAVOK_RELWITHDEBINFO_LIBRARY_PATH})

  if(LINUX)
    set(release_dir "${HAVOK_RELEASE_LIBRARY_PATH}/${HAVOK_LIB}")

    # Try to avoid extracting havok library each time we run cmake.
    if("${havok_${HAVOK_LIB}_extracted}" STREQUAL "" AND EXISTS "${PREBUILD_TRACKING_DIR}/havok_${HAVOK_LIB}_extracted")
      file(READ ${PREBUILD_TRACKING_DIR}/havok_${HAVOK_LIB}_extracted "havok_${HAVOK_LIB}_extracted")
      DEBUG_MESSAGE("havok_${HAVOK_LIB}_extracted: \"${havok_${HAVOK_LIB}_extracted}\"")
    endif("${havok_${HAVOK_LIB}_extracted}" STREQUAL "" AND EXISTS "${PREBUILD_TRACKING_DIR}/havok_${HAVOK_LIB}_extracted")

    if(${PREBUILD_TRACKING_DIR}/havok_source_installed IS_NEWER_THAN ${PREBUILD_TRACKING_DIR}/havok_${HAVOK_LIB}_extracted OR NOT ${havok_${HAVOK_LIB}_extracted} EQUAL 0)
      DEBUG_MESSAGE("Extracting ${HAVOK_LIB}...")

      foreach(lib ${release_dir})
        DEBUG_EXEC("mkdir" "-p" ${lib})
        DEBUG_EXEC("ar" "-xv" "../lib${HAVOK_LIB}.a"
          WORKING_DIRECTORY ${lib})
      endforeach(lib)

      # Just assume success for now.
      set(havok_${HAVOK_LIB}_extracted 0)
      file(WRITE ${PREBUILD_TRACKING_DIR}/havok_${HAVOK_LIB}_extracted "${havok_${HAVOK_LIB}_extracted}")

    endif()

    file(GLOB extracted_release "${release_dir}/*.o")
    DEBUG_MESSAGE("extracted_release ${release_dir}/*.o")

    list(APPEND HK_DEBUG_LIBRARIES ${extracted_release})
    list(APPEND HK_RELEASE_LIBRARIES ${extracted_release})
    list(APPEND HK_RELWITHDEBINFO_LIBRARIES ${extracted_release})
  else(LINUX)
    # Win32
    list(APPEND HK_DEBUG_LIBRARIES   ${HAVOK_DEBUG_LIB_${HAVOK_LIB}})
    list(APPEND HK_RELEASE_LIBRARIES ${HAVOK_RELEASE_LIB_${HAVOK_LIB}})
    list(APPEND HK_RELWITHDEBINFO_LIBRARIES ${HAVOK_RELWITHDEBINFO_LIB_${HAVOK_LIB}})
  endif (LINUX)
endforeach(HAVOK_LIB)

