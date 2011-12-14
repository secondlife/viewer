# -*- cmake -*-
include(Prebuilt)

use_prebuilt_binary(havok)
set(Havok_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/libraries/include/havok/Source)

set(HAVOK_DEBUG_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/libraries/i686-win32/lib/debug/havok)
set(HAVOK_RELEASE_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/libraries/i686-win32/lib/release/havok)

find_library(HK_BASE_DEBUG_LIB                  hkBase              PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_COMPAT_DEBUG_LIB                hkCompat            PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_GEOMETRY_UTILITIES_DEBUG_LIB    hkGeometryUtilities PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_INTERNAL_DEBUG_LIB              hkInternal          PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_SERIALIZE_DEBUG_LIB             hkSerialize         PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_SCENEDATA_DEBUG_LIB             hkSceneData         PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_PHYS_COLLIDE_DEBUG_LIB          hkpCollide          PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_PHYS_UTILITIES_DEBUG_LIB        hkpUtilities        PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_PHYS_CONSTRAINTSOLVER_DEBUG_LIB hkpConstraintSolver PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_PHYS_DYNAMICS_DEBUG_LIB         hkpDynamics         PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_PHYS_INTERNAL_DEBUG_LIB         hkpInternal         PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_AI_INTERNAL_DEBUG_LIB           hkaiInternal        PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_AI_PATHFINDING_DEBUG_LIB        hkaiPathfinding     PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_AI_AIPHYSICSBRIDGE_DEBUG_LIB    hkaiaiphysicsbridge PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_PHYS_UTILITIES_DEBUG_LIB        hkputilities        PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_CD_INTERNAL_DEBUG_LIB           hkcdinternal        PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_PHYS_VEHICLE_DEBUG_LIB          hkpVehicle          PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_VISUALIZE_DEBUG_LIB             hkVisualize         PATHS ${HAVOK_DEBUG_LIBRARY_PATH})
find_library(HK_AI_VISUALIZE_DEBUG_LIB          hkaiVisualize       PATHS ${HAVOK_DEBUG_LIBRARY_PATH})

find_library(HK_BASE_RELEASE_LIB                  hkBase              PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_COMPAT_RELEASE_LIB                hkCompat            PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_GEOMETRY_UTILITIES_RELEASE_LIB    hkGeometryUtilities PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_INTERNAL_RELEASE_LIB              hkInternal          PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_SERIALIZE_RELEASE_LIB             hkSerialize         PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_SCENEDATA_RELEASE_LIB             hkSceneData         PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_PHYS_COLLIDE_RELEASE_LIB          hkpCollide          PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_PHYS_UTILITIES_RELEASE_LIB        hkpUtilities        PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_PHYS_CONSTRAINTSOLVER_RELEASE_LIB hkpConstraintSolver PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_PHYS_DYNAMICS_RELEASE_LIB         hkpDynamics         PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_PHYS_INTERNAL_RELEASE_LIB         hkpInternal         PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_AI_INTERNAL_RELEASE_LIB           hkaiInternal        PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_AI_PATHFINDING_RELEASE_LIB        hkaiPathfinding     PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_AI_AIPHYSICSBRIDGE_RELEASE_LIB    hkaiaiphysicsbridge PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_PHYS_UTILITIES_RELEASE_LIB        hkputilities        PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_CD_INTERNAL_RELEASE_LIB           hkcdinternal        PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_PHYS_VEHICLE_RELEASE_LIB          hkpVehicle          PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_VISUALIZE_RELEASE_LIB             hkVisualize         PATHS ${HAVOK_RELEASE_LIBRARY_PATH})
find_library(HK_AI_VISUALIZE_RELEASE_LIB          hkaiVisualize       PATHS ${HAVOK_RELEASE_LIBRARY_PATH})

set(HK_LIBRARIES

    debug     ${HK_BASE_DEBUG_LIB}
    optimized ${HK_BASE_RELEASE_LIB}

    debug     ${HK_COMPAT_DEBUG_LIB}
    optimized ${HK_COMPAT_RELEASE_LIB}

    debug     ${HK_GEOMETRY_UTILITIES_DEBUG_LIB}
    optimized ${HK_GEOMETRY_UTILITIES_RELEASE_LIB}

    debug     ${HK_INTERNAL_DEBUG_LIB}
    optimized ${HK_INTERNAL_RELEASE_LIB}

    debug     ${HK_SERIALIZE_DEBUG_LIB}
    optimized ${HK_SERIALIZE_RELEASE_LIB}

    debug     ${HK_SCENEDATA_DEBUG_LIB}
    optimized ${HK_SCENEDATA_RELEASE_LIB}

    debug     ${HK_PHYS_COLLIDE_DEBUG_LIB}
    optimized ${HK_PHYS_COLLIDE_RELEASE_LIB}

    debug     ${HK_PHYS_UTILITIES_DEBUG_LIB}
    optimized ${HK_PHYS_UTILITIES_RELEASE_LIB}

    debug     ${HK_PHYS_CONSTRAINTSOLVER_DEBUG_LIB}
    optimized ${HK_PHYS_CONSTRAINTSOLVER_RELEASE_LIB}

    debug     ${HK_PHYS_DYNAMICS_DEBUG_LIB}
    optimized ${HK_PHYS_DYNAMICS_RELEASE_LIB}

    debug     ${HK_PHYS_INTERNAL_DEBUG_LIB}
    optimized ${HK_PHYS_INTERNAL_RELEASE_LIB}

    debug     ${HK_AI_INTERNAL_DEBUG_LIB}
    optimized ${HK_AI_INTERNAL_RELEASE_LIB}

    debug     ${HK_AI_PATHFINDING_DEBUG_LIB}
    optimized ${HK_AI_PATHFINDING_RELEASE_LIB}

    debug     ${HK_AI_AIPHYSICSBRIDGE_DEBUG_LIB}
    optimized ${HK_AI_AIPHYSICSBRIDGE_RELEASE_LIB}

    debug     ${HK_PHYS_UTILITIES_DEBUG_LIB}
    optimized ${HK_PHYS_UTILITIES_RELEASE_LIB}

    debug     ${HK_CD_INTERNAL_DEBUG_LIB}
    optimized ${HK_CD_INTERNAL_RELEASE_LIB}

    debug     ${HK_PHYS_VEHICLE_DEBUG_LIB}
    optimized ${HK_PHYS_VEHICLE_RELEASE_LIB}

    debug     ${HK_VISUALIZE_DEBUG_LIB}
    optimized ${HK_VISUALIZE_RELEASE_LIB}

    debug     ${HK_AI_VISUALIZE_DEBUG_LIB}
    optimized ${HK_AI_VISUALIZE_RELEASE_LIB}
)
