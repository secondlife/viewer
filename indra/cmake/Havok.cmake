# -*- cmake -*-
include(Prebuilt)

use_prebuilt_binary(havok)
set(Havok_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/libraries/include/havok/Source)
if (CMAKE_BUILD_TYPE MATCHES "Debug")
   set(HAVOK_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/libraries/i686-win32/lib/debug/havok)
else (CMAKE_BUILD_TYPE MATCHES "Debug")
   set(HAVOK_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/libraries/i686-win32/lib/release/havok)
endif (CMAKE_BUILD_TYPE MATCHES "Debug")

find_library(HK_BASE_LIB                  hkBase              PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_COMPAT_LIB                hkCompat            PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_GEOMETRY_UTILITIES_LIB    hkGeometryUtilities PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_INTERNAL_LIB              hkInternal          PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_SERIALIZE_LIB             hkSerialize         PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_SCENEDATA_LIB             hkSceneData         PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_PHYS_COLLIDE_LIB          hkpCollide          PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_PHYS_UTILITIES_LIB        hkpUtilities        PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_PHYS_CONSTRAINTSOLVER_LIB hkpConstraintSolver PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_PHYS_DYNAMICS_LIB         hkpDynamics         PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_PHYS_INTERNAL_LIB         hkpInternal         PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_AI_INTERNAL_LIB           hkaiInternal        PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_AI_PATHFINDING_LIB        hkaiPathfinding     PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_AI_AIPHYSICSBRIDGE_LIB    hkaiaiphysicsbridge PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_PHYS_UTILITIES_LIB        hkputilities        PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_CD_INTERNAL_LIB           hkcdinternal        PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_PHYS_VEHICLE_LIB          hkpVehicle          PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_VISUALIZE_LIB             hkVisualize         PATHS ${HAVOK_LIBRARY_PATH})
find_library(HK_AI_VISUALIZE_LIB          hkaiVisualize       PATHS ${HAVOK_LIBRARY_PATH})

set(HK_LIBRARIES
    ${HK_BASE_LIB}
    ${HK_COMPAT_LIB}
    ${HK_GEOMETRY_UTILITIES_LIB}
    ${HK_INTERNAL_LIB}
    ${HK_SERIALIZE_LIB}
    ${HK_SCENEDATA_LIB}
    ${HK_PHYS_COLLIDE_LIB}
    ${HK_PHYS_UTILITIES_LIB}
    ${HK_PHYS_CONSTRAINTSOLVER_LIB}
    ${HK_PHYS_DYNAMICS_LIB}
    ${HK_PHYS_INTERNAL_LIB}
    ${HK_AI_INTERNAL_LIB}
    ${HK_AI_PATHFINDING_LIB}
    ${HK_AI_AIPHYSICSBRIDGE_LIB}
    ${HK_PHYS_UTILITIES_LIB}
    ${HK_CD_INTERNAL_LIB}
    ${HK_PHYS_VEHICLE_LIB}
    ${HK_VISUALIZE_LIB}
    ${HK_AI_VISUALIZE_LIB}
)
