# -*- cmake -*-
include_guard()
include(ZLIBNG)

add_library( ll::colladadom INTERFACE IMPORTED )

find_path(COLLADA_DOM_INCLUDE_DIRS NAMES dae.h PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/collada-dom2.5" REQUIRED NO_DEFAULT_PATH)

find_library(COLLADA_LIBRARY_RELEASE
    NAMES collada-dom2.5-dp-vc140-mt collada-dom2.5-dp
    PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib"
    REQUIRED
    NO_DEFAULT_PATH
)

find_library(COLLADA_LIBRARY_DEBUG
    NAMES collada-dom2.5-dp-vc140-mt collada-dom2.5-dp
    PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib"
    REQUIRED
    NO_DEFAULT_PATH
)

target_link_libraries(ll::colladadom INTERFACE debug ${COLLADA_LIBRARY_DEBUG} optimized ${COLLADA_LIBRARY_RELEASE})
target_include_directories(ll::colladadom SYSTEM INTERFACE "${COLLADA_DOM_INCLUDE_DIRS}" "${COLLADA_DOM_INCLUDE_DIRS}/1.4")
target_compile_definitions(ll::colladadom INTERFACE COLLADA_DOM_SUPPORT141=1 COLLADA_DOM_SUPPORT150=1 COLLADA_DOM_DAEFLOAT_IS64=1 COLLADA_DOM_USING_141=1)

if(WINDOWS)
    target_compile_definitions(ll::colladadom INTERFACE DOM_DYNAMIC=1)
endif()

if(DARWIN OR LINUX)
    find_library(COLLADA141_LIBRARY_RELEASE
        NAMES colladadom141
        PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib"
        REQUIRED
        NO_DEFAULT_PATH
    )

    find_library(COLLADA141_LIBRARY_DEBUG
        NAMES colladadom141
        PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib"
        REQUIRED
        NO_DEFAULT_PATH
    )

    find_library(COLLADA150_LIBRARY_RELEASE
        NAMES colladadom150
        PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib"
        REQUIRED
        NO_DEFAULT_PATH
    )

    find_library(COLLADA150_LIBRARY_DEBUG
        NAMES colladadom150
        PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib"
        REQUIRED
        NO_DEFAULT_PATH
    )

    target_link_libraries(ll::colladadom INTERFACE debug ${COLLADA150_LIBRARY_DEBUG} optimized ${COLLADA150_LIBRARY_RELEASE} debug ${COLLADA141_LIBRARY_DEBUG} optimized ${COLLADA141_LIBRARY_RELEASE})
    find_package(unofficial-minizip CONFIG REQUIRED)
    find_package(LibXml2 REQUIRED)
    target_link_libraries(ll::colladadom INTERFACE LibXml2::LibXml2 unofficial::minizip::minizip ZLIB::ZLIB)
endif()
