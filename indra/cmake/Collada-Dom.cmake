# -*- cmake -*-
include_guard()
include(ZLIBNG)

add_library( ll::colladadom INTERFACE IMPORTED )

find_path(COLLADA_DOM_INCLUDE_DIRS NAMES dae.h PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/collada-dom" REQUIRED NO_DEFAULT_PATH)

target_include_directories(ll::colladadom SYSTEM INTERFACE "${COLLADA_DOM_INCLUDE_DIRS}" "${COLLADA_DOM_INCLUDE_DIRS}/1.4")

if(WINDOWS)
    target_compile_definitions(ll::colladadom INTERFACE DOM_DYNAMIC=1)
endif()


find_library(COLLADA14_LIBRARY_RELEASE
    NAMES collada14dom
    PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib"
    REQUIRED
    NO_DEFAULT_PATH
)

target_link_libraries(ll::colladadom INTERFACE optimized ${COLLADA14_LIBRARY_RELEASE})

find_library(COLLADA14_LIBRARY_DEBUG
    NAMES collada14dom
    PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib"
    NO_DEFAULT_PATH
)

if (NOT COLLADA14_LIBRARY_DEBUG STREQUAL "COLLADA14_LIBRARY_DEBUG-NOTFOUND")
    target_link_libraries(ll::colladadom INTERFACE debug ${COLLADA14_LIBRARY_DEBUG})
endif()

find_package(minizip CONFIG REQUIRED)
find_package(LibXml2 REQUIRED)
target_link_libraries(ll::colladadom INTERFACE LibXml2::LibXml2 MINIZIP::minizip ZLIB::ZLIB)
