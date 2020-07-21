# -*- cmake -*-

project(llvfs)

include(00-Common)
include(LLCommon)
include(SQLite)
include(UnixInstall)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    ${SQLITE_INCLUDE_DIR}
    )

set(llvfs_SOURCE_FILES
    lldir.cpp
    lldiriterator.cpp
    lldiskcache.cpp
    lllfsthread.cpp
    llpidlock.cpp
    llvfile.cpp
    llvfs.cpp
    llvfsthread.cpp
    )

set(llvfs_HEADER_FILES
    CMakeLists.txt

    lldir.h
    lldirguard.h
    lldiriterator.h
    lldiskcache.h
    lllfsthread.h
    llpidlock.h
    llvfile.h
    llvfs.h
    llvfsthread.h
    )

if (DARWIN)
  LIST(APPEND llvfs_SOURCE_FILES lldir_mac.cpp)
  LIST(APPEND llvfs_HEADER_FILES lldir_mac.h)
  LIST(APPEND llvfs_SOURCE_FILES llvfs_objc.mm)
  LIST(APPEND llvfs_HEADER_FILES llvfs_objc.h)
endif (DARWIN)

if (LINUX)
  LIST(APPEND llvfs_SOURCE_FILES lldir_linux.cpp)
  LIST(APPEND llvfs_HEADER_FILES lldir_linux.h)

  if (INSTALL)
    set_source_files_properties(lldir_linux.cpp
                                PROPERTIES COMPILE_FLAGS
                                "-DAPP_RO_DATA_DIR=\\\"${APP_SHARE_DIR}\\\""
                                )
  endif (INSTALL)
endif (LINUX)

if (WINDOWS)
  LIST(APPEND llvfs_SOURCE_FILES lldir_win32.cpp)
  LIST(APPEND llvfs_HEADER_FILES lldir_win32.h)
endif (WINDOWS)

set_source_files_properties(${llvfs_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llvfs_SOURCE_FILES ${llvfs_HEADER_FILES})

add_library (llvfs ${llvfs_SOURCE_FILES})

set(vfs_BOOST_LIBRARIES
    ${BOOST_FILESYSTEM_LIBRARY}
    ${BOOST_SYSTEM_LIBRARY}
    )

target_link_libraries(llvfs
    ${LLCOMMON_LIBRARIES}
    ${vfs_BOOST_LIBRARIES}
    ${SQLITE_LIBRARIES}
    )

if (DARWIN)
  include(CMakeFindFrameworks)
  find_library(COCOA_LIBRARY Cocoa)
  target_link_libraries(llvfs ${COCOA_LIBRARY})
endif (DARWIN)


# Add tests
if (LL_TESTS)
    include(LLAddBuildTest)
    # UNIT TESTS
    SET(llvfs_TEST_SOURCE_FILES
    lldiriterator.cpp
    )

    set_source_files_properties(lldiriterator.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_LIBRARIES "${vfs_BOOST_LIBRARIES}"
    )
    LL_ADD_PROJECT_UNIT_TESTS(llvfs "${llvfs_TEST_SOURCE_FILES}")

    # INTEGRATION TESTS
    set(test_libs llmath llcommon llvfs ${LLCOMMON_LIBRARIES} ${WINDOWS_LIBRARIES})

    # TODO: Some of these need refactoring to be proper Unit tests rather than Integration tests.
    LL_ADD_INTEGRATION_TEST(lldir "" "${test_libs}")
    
endif (LL_TESTS)

### TODO: Move it back inside LL_TESTS block eventually
    ####################################################################################
    # LLVFS perf/stress app
    SET(llvfs_TEST_SOURCE_FILES
      tests/llvfs_perftest.cpp
    )
    add_executable(llvfs_perf_test
      ${llvfs_TEST_SOURCE_FILES}
    )
    set_target_properties(llvfs_perf_test
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${EXE_STAGING_DIR}"
    )
    target_link_libraries(llvfs_perf_test
      ${LLCOMMON_LIBRARIES}
      llvfs
      ${vfs_BOOST_LIBRARIES}
      ${SQLITE_LIBRARIES}
    )
    target_include_directories(
      llvfs_perf_test
      PUBLIC
      ${LIBS_OPEN_DIR}/llvfs
    )
    if (WINDOWS)
      set_target_properties(llvfs_perf_test
        PROPERTIES
        LINK_FLAGS "/debug /NODEFAULTLIB:LIBCMT /SUBSYSTEM:CONSOLE"
        LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMT;LIBCMTD;MSVCRT\" /INCREMENTAL:NO"
        LINK_FLAGS_RELEASE ""
      )
    endif (WINDOWS)
