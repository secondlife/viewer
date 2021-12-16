# -*- cmake -*-

project(llfilesystem)

include(00-Common)
include(LLCommon)
include(UnixInstall)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )

set(llfilesystem_SOURCE_FILES
    lldir.cpp
    lldiriterator.cpp
    lllfsthread.cpp
    lldiskcache.cpp
    llfilesystem.cpp
    )

set(llfilesystem_HEADER_FILES
    CMakeLists.txt
    lldir.h
    lldirguard.h
    lldiriterator.h
    lllfsthread.h
    lldiskcache.h
    llfilesystem.h
    )

if (DARWIN)
  LIST(APPEND llfilesystem_SOURCE_FILES lldir_utils_objc.mm)
  LIST(APPEND llfilesystem_SOURCE_FILES lldir_utils_objc.h)
  LIST(APPEND llfilesystem_SOURCE_FILES lldir_mac.cpp)
  LIST(APPEND llfilesystem_HEADER_FILES lldir_mac.h)
endif (DARWIN)

if (LINUX)
  LIST(APPEND llfilesystem_SOURCE_FILES lldir_linux.cpp)
  LIST(APPEND llfilesystem_HEADER_FILES lldir_linux.h)

  if (INSTALL)
    set_source_files_properties(lldir_linux.cpp
                                PROPERTIES COMPILE_FLAGS
                                "-DAPP_RO_DATA_DIR=\\\"${APP_SHARE_DIR}\\\""
                                )
  endif (INSTALL)
endif (LINUX)

if (WINDOWS)
  LIST(APPEND llfilesystem_SOURCE_FILES lldir_win32.cpp)
  LIST(APPEND llfilesystem_HEADER_FILES lldir_win32.h)
endif (WINDOWS)

set_source_files_properties(${llfilesystem_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llfilesystem_SOURCE_FILES ${llfilesystem_HEADER_FILES})

add_library (llfilesystem ${llfilesystem_SOURCE_FILES})

set(cache_BOOST_LIBRARIES
    ${BOOST_FILESYSTEM_LIBRARY}
    ${BOOST_SYSTEM_LIBRARY}
    )

target_link_libraries(llfilesystem
    ${LLCOMMON_LIBRARIES}
    ${cache_BOOST_LIBRARIES}
    )

if (DARWIN)
  include(CMakeFindFrameworks)
  find_library(COCOA_LIBRARY Cocoa)
  target_link_libraries(llfilesystem ${COCOA_LIBRARY})
endif (DARWIN)


# Add tests
if (LL_TESTS)
    include(LLAddBuildTest)
    # UNIT TESTS
    SET(llfilesystem_TEST_SOURCE_FILES
    lldiriterator.cpp
    )

    set_source_files_properties(lldiriterator.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_LIBRARIES "${cache_BOOST_LIBRARIES}"
    )
    LL_ADD_PROJECT_UNIT_TESTS(llfilesystem "${llfilesystem_TEST_SOURCE_FILES}")

    # INTEGRATION TESTS
    set(test_libs llmath llcommon llfilesystem ${LLCOMMON_LIBRARIES} ${WINDOWS_LIBRARIES})

    # TODO: Some of these need refactoring to be proper Unit tests rather than Integration tests.
    LL_ADD_INTEGRATION_TEST(lldir "" "${test_libs}")
endif (LL_TESTS)
