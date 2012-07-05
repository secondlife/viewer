# -*- cmake -*-

use_prebuilt_binary(havok-source)
set(Havok_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/havok/Source)
list(APPEND Havok_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/havok/Demo)

set(HAVOK_DEBUG_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/lib/debug/havok)
set(HAVOK_RELEASE_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/lib/release/havok)

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
unset(HK_LIBRARIES)

foreach(HAVOK_LIB ${HAVOK_LIBS})
    find_library(HAVOK_DEBUG_LIB_${HAVOK_LIB}   ${HAVOK_LIB} PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
    find_library(HAVOK_RELEASE_LIB_${HAVOK_LIB} ${HAVOK_LIB} PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
    
    if(LINUX)
    set(cmd "mkdir")
    set(debug_dir "${HAVOK_DEBUG_LIBRARY_PATH}/${HAVOK_LIB}")
    set(release_dir "${HAVOK_RELEASE_LIBRARY_PATH}/${HAVOK_LIB}")
    exec_program( ${cmd} ${HAVOK_DEBUG_LIBRARY_PATH} ARGS ${debug_dir} OUTPUT_VARIABLE rv)
    exec_program( ${cmd} ${HAVOK_RELEASE_LIBRARY_PATH} ARGS ${release_dir} OUTPUT_VARIABLE rv)

    set(cmd "ar")
    set(arg " -xv")
    set(arg "${arg} ../lib${HAVOK_LIB}.a")
    exec_program( ${cmd} ${debug_dir} ARGS ${arg} OUTPUT_VARIABLE rv)
    exec_program( ${cmd} ${release_dir} ARGS ${arg} OUTPUT_VARIABLE rv)

    file(GLOB extracted_debug "${debug_dir}/*.o")
    file(GLOB extracted_release "${release_dir}/*.o")
    list(APPEND HK_DEBUG_LIBRARIES ${extracted_debug})
    list(APPEND HK_RELEASE_LIBRARIES ${extracted_release})
    list(APPEND HK_LIBRARIES debug      ${extracted_debug}
                 optimized  ${extracted_release})
    else(LINUX)
        list(APPEND HK_DEBUG_LIBRARIES   ${HAVOK_DEBUG_LIB_${HAVOK_LIB}})
        list(APPEND HK_RELEASE_LIBRARIES ${HAVOK_RELEASE_LIB_${HAVOK_LIB}})
        list(APPEND HK_LIBRARIES debug     ${HAVOK_DEBUG_LIB_${HAVOK_LIB}}
                                 optimized ${HAVOK_RELEASE_LIB_${HAVOK_LIB}})
    endif (LINUX)
endforeach(HAVOK_LIB)

