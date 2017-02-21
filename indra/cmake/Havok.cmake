# -*- cmake -*-
include(Prebuilt)
if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

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
    hkBase
    hkCompat
    hkGeometryUtilities
    hkInternal
    hkSerialize
    hkSceneData
    hkpCollide
    hkpUtilities
    hkpConstraintSolver
    hkpDynamics
    hkpInternal
    hkaiInternal
    hkaiPathfinding
    hkaiAiPhysicsBridge
    hkcdInternal
    hkcdCollide
    hkpVehicle
    hkVisualize
    hkaiVisualize
    hkgpConvexDecomposition
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
else (DEBUG_PREBUILT)
  # without DEBUG_PREBUILT, DEBUG_MESSAGE() is a no-op
  function(DEBUG_MESSAGE)
  endfunction(DEBUG_MESSAGE)
endif (DEBUG_PREBUILT)

# DEBUG_EXEC() reports each execute_process() before invoking
function(DEBUG_EXEC)
  DEBUG_MESSAGE(${ARGN})
  execute_process(COMMAND ${ARGN})
endfunction(DEBUG_EXEC)

# *TODO: Figure out why we need to extract like this...
foreach(HAVOK_LIB ${HAVOK_LIBS})
  find_library(HAVOK_DEBUG_LIB_${HAVOK_LIB}   ${HAVOK_LIB} PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
  find_library(HAVOK_RELEASE_LIB_${HAVOK_LIB} ${HAVOK_LIB} PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
  find_library(HAVOK_RELWITHDEBINFO_LIB_${HAVOK_LIB} ${HAVOK_LIB} PATHS ${HAVOK_RELWITHDEBINFO_LIBRARY_PATH})
  
  if(LINUX)
    set(debug_dir "${HAVOK_DEBUG_LIBRARY_PATH}/${HAVOK_LIB}")
    set(release_dir "${HAVOK_RELEASE_LIBRARY_PATH}/${HAVOK_LIB}")
    set(relwithdebinfo_dir "${HAVOK_RELWITHDEBINFO_LIBRARY_PATH}/${HAVOK_LIB}")

    # Try to avoid extracting havok library each time we run cmake.
    if("${havok_${HAVOK_LIB}_extracted}" STREQUAL "" AND EXISTS "${PREBUILD_TRACKING_DIR}/havok_${HAVOK_LIB}_extracted")
      file(READ ${PREBUILD_TRACKING_DIR}/havok_${HAVOK_LIB}_extracted "havok_${HAVOK_LIB}_extracted")
      DEBUG_MESSAGE("havok_${HAVOK_LIB}_extracted: \"${havok_${HAVOK_LIB}_extracted}\"")
    endif("${havok_${HAVOK_LIB}_extracted}" STREQUAL "" AND EXISTS "${PREBUILD_TRACKING_DIR}/havok_${HAVOK_LIB}_extracted")

    if(${PREBUILD_TRACKING_DIR}/havok_source_installed IS_NEWER_THAN ${PREBUILD_TRACKING_DIR}/havok_${HAVOK_LIB}_extracted OR NOT ${havok_${HAVOK_LIB}_extracted} EQUAL 0)
      DEBUG_MESSAGE("Extracting ${HAVOK_LIB}...")

      foreach(lib ${debug_dir} ${release_dir} ${relwithdebinfo_dir})
        DEBUG_EXEC("mkdir" ${lib})
        DEBUG_EXEC("ar" "-xv" "../lib${HAVOK_LIB}.a"
          WORKING_DIRECTORY ${lib})
      endforeach(lib)

      # Just assume success for now.
      set(havok_${HAVOK_LIB}_extracted 0)
      file(WRITE ${PREBUILD_TRACKING_DIR}/havok_${HAVOK_LIB}_extracted "${havok_${HAVOK_LIB}_extracted}")

    endif()

    file(GLOB extracted_debug "${debug_dir}/*.o")
    file(GLOB extracted_release "${release_dir}/*.o")
    file(GLOB extracted_relwithdebinfo "${relwithdebinfo_dir}/*.o")

    DEBUG_MESSAGE("extracted_debug ${debug_dir}/*.o")
    DEBUG_MESSAGE("extracted_release ${release_dir}/*.o")
    DEBUG_MESSAGE("extracted_relwithdebinfo ${relwithdebinfo_dir}/*.o")

    list(APPEND HK_DEBUG_LIBRARIES ${extracted_debug})
    list(APPEND HK_RELEASE_LIBRARIES ${extracted_release})
    list(APPEND HK_RELWITHDEBINFO_LIBRARIES ${extracted_relwithdebinfo})
  else(LINUX)
    # Win32
    list(APPEND HK_DEBUG_LIBRARIES   ${HAVOK_DEBUG_LIB_${HAVOK_LIB}})
    list(APPEND HK_RELEASE_LIBRARIES ${HAVOK_RELEASE_LIB_${HAVOK_LIB}})
    list(APPEND HK_RELWITHDEBINFO_LIBRARIES ${HAVOK_RELWITHDEBINFO_LIB_${HAVOK_LIB}})
  endif (LINUX)
endforeach(HAVOK_LIB)

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
