vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO RyeMutt/3p-colladadom
    REF b8da6cd54083bf24dff3ac03be465e710c135195
    SHA512 bb5e6c09777d0dfd0093ff99f4114873bec7c0094e4ec15594d188c0a2bf44a0fffbcf91dfa3c4f73a924f32e6fe3eb0cf1cbe4053a6bb40979e9c2962f649b1
    HEAD_REF master
    # PATCHES
    #     use-vcpkg-minizip.patch
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DCMAKE_CXX_STANDARD=17
)

vcpkg_cmake_install()

if(VCPKG_LIBRARY_LINKAGE MATCHES "dynamic" AND VCPKG_TARGET_IS_WINDOWS)
    set(RELEASE_COLLADA_DLL "${CURRENT_PACKAGES_DIR}/lib/collada14dom.dll")
    if(EXISTS ${RELEASE_COLLADA_DLL})
        file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/bin")
        file(RENAME ${RELEASE_COLLADA_DLL} "${CURRENT_PACKAGES_DIR}/bin/collada14dom.dll")
    endif()
    set(DEBUG_COLLADA_DLL "${CURRENT_PACKAGES_DIR}/debug/lib/collada14dom.dll")
    if(EXISTS ${DEBUG_COLLADA_DLL})
        file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/debug/bin")
        file(RENAME ${DEBUG_COLLADA_DLL} "${CURRENT_PACKAGES_DIR}/debug/bin/collada14dom.dll")
    endif()
endif()

if(VCPKG_LIBRARY_LINKAGE MATCHES "static" AND (VCPKG_TARGET_IS_OSX OR VCPKG_TARGET_IS_LINUX))
    if(EXISTS "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel/src/1.4/libcollada14dom.a")
        file(INSTALL "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel/src/1.4/libcollada14dom.a" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
    endif()
    if(EXISTS "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-dbg/src/1.4/libcollada14dom.a")
        file(INSTALL "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-dbg/src/1.4/libcollada14dom.a" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
    endif()
endif()

#vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/collada-dom-2.5)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

# Handle copyright
file(INSTALL "${SOURCE_PATH}/license.txt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

vcpkg_fixup_pkgconfig()
