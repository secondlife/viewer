# -*- cmake -*-

if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

use_prebuilt_binary(havok-source)

set(Havok_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/havok/Source)
list(APPEND Havok_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/havok/Demo)

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
    if("${havok_${HAVOK_LIB}_extracted}" STREQUAL "" AND EXISTS "${CMAKE_BINARY_DIR}/temp/havok_${HAVOK_LIB}_extracted")
      file(READ ${CMAKE_BINARY_DIR}/temp/havok_${HAVOK_LIB}_extracted "havok_${HAVOK_LIB}_extracted")
      if(DEBUG_PREBUILT)
        message(STATUS "havok_${HAVOK_LIB}_extracted: \"${havok_${HAVOK_LIB}_extracted}\"")
      endif(DEBUG_PREBUILT)
    endif("${havok_${HAVOK_LIB}_extracted}" STREQUAL "" AND EXISTS "${CMAKE_BINARY_DIR}/temp/havok_${HAVOK_LIB}_extracted")

    if(${CMAKE_BINARY_DIR}/temp/havok-source_installed IS_NEWER_THAN ${CMAKE_BINARY_DIR}/temp/havok_${HAVOK_LIB}_extracted OR NOT ${havok_${HAVOK_LIB}_extracted} EQUAL 0)
      if(DEBUG_PREBUILT)
        MESSAGE(STATUS "Extracting ${HAVOK_LIB}...")
      endif(DEBUG_PREBUILT)
      set(cmd "mkdir")

      if(DEBUG_PREBUILT)
        MESSAGE(STATUS "${cmd} ${debug_dir}")
      endif(DEBUG_PREBUILT)
      exec_program( ${cmd} ${HAVOK_DEBUG_LIBRARY_PATH} ARGS ${debug_dir} OUTPUT_VARIABLE rv)

      if(DEBUG_PREBUILT)
        MESSAGE(STATUS "${cmd} ${release_dir}")
      endif(DEBUG_PREBUILT)
      exec_program( ${cmd} ${HAVOK_RELEASE_LIBRARY_PATH} ARGS ${release_dir} OUTPUT_VARIABLE rv)

      if(DEBUG_PREBUILT)
        MESSAGE(STATUS "${cmd} ${relwithdebinfo_dir}")
      endif(DEBUG_PREBUILT)
      exec_program( ${cmd} ${HAVOK_RELWITHDEBINFO_LIBRARY_PATH} ARGS ${relwithdebinfo_dir} OUTPUT_VARIABLE rv)

      set(cmd "ar")
      set(arg " -xv")
      set(arg "${arg} ../lib${HAVOK_LIB}.a")
      if(DEBUG_PREBUILT)
        MESSAGE(STATUS "cd ${debug_dir} && ${cmd} ${arg}")
      endif(DEBUG_PREBUILT)
      exec_program( ${cmd} ${debug_dir} ARGS ${arg} OUTPUT_VARIABLE rv)

      if(DEBUG_PREBUILT)
        MESSAGE(STATUS "cd ${release_dir} && ${cmd} ${arg}")
      endif(DEBUG_PREBUILT)
      exec_program( ${cmd} ${release_dir} ARGS ${arg} OUTPUT_VARIABLE rv)

      if(DEBUG_PREBUILT)
        MESSAGE(STATUS "cd ${relwithdebinfo_dir} && ${cmd} ${arg}")
      endif(DEBUG_PREBUILT)
      exec_program( ${cmd} ${relwithdebinfo_dir} ARGS ${arg} OUTPUT_VARIABLE rv)

      # Just assume success for now.
      set(havok_${HAVOK_LIB}_extracted 0)
      file(WRITE ${CMAKE_BINARY_DIR}/temp/havok_${HAVOK_LIB}_extracted "${havok_${HAVOK_LIB}_extracted}")

    endif(${CMAKE_BINARY_DIR}/temp/havok-source_installed IS_NEWER_THAN ${CMAKE_BINARY_DIR}/temp/havok_${HAVOK_LIB}_extracted OR NOT ${havok_${HAVOK_LIB}_extracted} EQUAL 0)

    file(GLOB extracted_debug "${debug_dir}/*.o")
    file(GLOB extracted_release "${release_dir}/*.o")
    file(GLOB extracted_relwithdebinfo "${relwithdebinfo_dir}/*.o")

    if(DEBUG_PREBUILT)
      MESSAGE(STATUS "extracted_debug ${debug_dir}/*.o")
      MESSAGE(STATUS "extracted_release ${release_dir}/*.o")
      MESSAGE(STATUS "extracted_relwithdebinfo ${relwithdebinfo_dir}/*.o")
    endif(DEBUG_PREBUILT)

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
