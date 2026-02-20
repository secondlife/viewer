include_guard()
add_library(ll::apr INTERFACE IMPORTED)

if (WINDOWS)
  find_package(apr CONFIG REQUIRED)
  target_link_libraries(ll::apr INTERFACE
    $<$<TARGET_EXISTS:apr::apr-1>:apr::apr-1>
    $<$<TARGET_EXISTS:apr::aprapp-1>:apr::aprapp-1>
    $<$<TARGET_EXISTS:apr::libapr-1>:apr::libapr-1>
    $<$<TARGET_EXISTS:apr::libaprapp-1>:apr::libaprapp-1>
  )

  find_library(APU_LIBRARY_RELEASE
    NAMES aprutil-1 libaprutil-1
    PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib"
    REQUIRED
    NO_DEFAULT_PATH
  )

  target_link_libraries(ll::apr INTERFACE optimized ${APU_LIBRARY_RELEASE})

  find_library(APU_LIBRARY_DEBUG
    NAMES aprutil-1 libaprutil-1
    PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib"
    NO_DEFAULT_PATH
  )

  if (NOT APU_LIBRARY_DEBUG STREQUAL "APU_LIBRARY_DEBUG-NOTFOUND")
    target_link_libraries(ll::apr INTERFACE debug ${APU_LIBRARY_DEBUG})
  endif()
else()
  find_package(PkgConfig)
  pkg_check_modules(APR REQUIRED IMPORTED_TARGET GLOBAL apr-1)
  pkg_check_modules(APR_UTIL REQUIRED IMPORTED_TARGET GLOBAL apr-util-1)
  target_link_libraries(ll::apr INTERFACE PkgConfig::APR_UTIL PkgConfig::APR)
endif()
