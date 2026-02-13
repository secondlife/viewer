# -*- cmake -*-

add_library(llfilesystem STATIC)

target_sources(llfilesystem
    PRIVATE
    lldir.cpp
    lldiriterator.cpp
    lllfsthread.cpp
    lldiskcache.cpp
    llfilesystem.cpp
    )

target_sources(llfilesystem
    PUBLIC
    FILE_SET HEADERS
    FILES
    CMakeLists.txt
    lldir.h
    lldirguard.h
    lldiriterator.h
    lllfsthread.h
    lldiskcache.h
    llfilesystem.h
    )

if (DARWIN)
  target_sources(llfilesystem
      PRIVATE
      lldir_mac.cpp
      lldir_utils_objc.mm
      PUBLIC
      FILE_SET HEADERS
      FILES
      lldir_mac.h
      lldir_utils_objc.h
      )

  set_source_files_properties(
    lldir_utils_objc.mm
    PROPERTIES
    SKIP_PRECOMPILE_HEADERS TRUE
  )
endif (DARWIN)

if (LINUX)
  target_sources(llfilesystem
      PRIVATE
      lldir_linux.cpp
      PUBLIC
      FILE_SET HEADERS
      FILES
      lldir_linux.h
      )

  if (INSTALL)
    set_source_files_properties(lldir_linux.cpp
                                PROPERTIES COMPILE_FLAGS
                                "-DAPP_RO_DATA_DIR=\\\"${APP_SHARE_DIR}\\\""
                                )
  endif ()
endif ()

if (WINDOWS)
  target_sources(llfilesystem
      PRIVATE
      lldir_win32.cpp
      PUBLIC
      FILE_SET HEADERS
      FILES
      lldir_win32.h
      )
endif ()

target_link_libraries(llfilesystem
      PUBLIC
      llmath
      llcommon
    )
target_include_directories( llfilesystem  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(llfilesystem REUSE_FROM llprecompiled)

# Add tests
if (BUILD_TESTING)
    # UNIT TESTS
    SET(llfilesystem_TEST_SOURCE_FILES
      lldiriterator.cpp
    )

    LL_ADD_PROJECT_UNIT_TESTS(llfilesystem "${llfilesystem_TEST_SOURCE_FILES}")

    # INTEGRATION TESTS
    set(test_libs llmath llcommon llfilesystem )
    set(test_project llfilesystem)

    # TODO: Some of these need refactoring to be proper Unit tests rather than Integration tests.
    LL_ADD_INTEGRATION_TEST(lldir "" "${test_libs}" "${test_project}")
endif ()
