# -*- cmake -*-

project(llvfs)

include(00-Common)
include(LLCommon)
include(UnixInstall)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    )

set(llvfs_SOURCE_FILES
    lldir.cpp
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
    lllfsthread.h
    llpidlock.h
    llvfile.h
    llvfs.h
    llvfsthread.h
    )

if (DARWIN)
  LIST(APPEND llvfs_SOURCE_FILES lldir_mac.cpp)
  LIST(APPEND llvfs_HEADER_FILES lldir_mac.h)
endif (DARWIN)

if (LINUX)
  LIST(APPEND llvfs_SOURCE_FILES lldir_linux.cpp)
  LIST(APPEND llvfs_HEADER_FILES lldir_linux.h)

  if (VIEWER AND INSTALL)
    set_source_files_properties(lldir_linux.cpp
                                PROPERTIES COMPILE_FLAGS
                                "-DAPP_RO_DATA_DIR=\\\"${APP_SHARE_DIR}\\\""
                                )
  endif (VIEWER AND INSTALL)
endif (LINUX)

if (WINDOWS)
  LIST(APPEND llvfs_SOURCE_FILES lldir_win32.cpp)
  LIST(APPEND llvfs_HEADER_FILES lldir_win32.h)
endif (WINDOWS)

set_source_files_properties(${llvfs_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llvfs_SOURCE_FILES ${llvfs_HEADER_FILES})

add_library (llvfs ${llvfs_SOURCE_FILES})

if (DARWIN)
  include(CMakeFindFrameworks)
  find_library(CARBON_LIBRARY Carbon)
  target_link_libraries(llvfs ${CARBON_LIBRARY})
endif (DARWIN)


# Add tests
if (LL_TESTS)
	include(LLAddBuildTest)
	# UNIT TESTS
	SET(llvfs_TEST_SOURCE_FILES
	  # none so far
	  )
	LL_ADD_PROJECT_UNIT_TESTS(llvfs "${llvfs_TEST_SOURCE_FILES}")

	# INTEGRATION TESTS
	set(test_libs llmath llcommon llvfs ${LLCOMMON_LIBRARIES} ${WINDOWS_LIBRARIES})
	# TODO: Some of these need refactoring to be proper Unit tests rather than Integration tests.
	LL_ADD_INTEGRATION_TEST(lldir "" "${test_libs}")
endif (LL_TESTS)
