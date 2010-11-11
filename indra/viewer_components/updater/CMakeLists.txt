# -*- cmake -*-

project(updater_service)

include(00-Common)
if(LL_TESTS)
  include(LLAddBuildTest)
endif(LL_TESTS)
include(CMakeCopyIfDifferent)
include(CURL)
include(LLCommon)
include(LLMessage)
include(LLPlugin)
include(LLVFS)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLPLUGIN_INCLUDE_DIRS}
	${LLVFS_INCLUDE_DIRS}
	${CURL_INCLUDE_DIRS}
    )

set(updater_service_SOURCE_FILES
    llupdaterservice.cpp
    llupdatechecker.cpp
    llupdatedownloader.cpp
    llupdateinstaller.cpp
    )

set(updater_service_HEADER_FILES
    llupdaterservice.h
    llupdatechecker.h
    llupdatedownloader.h
    llupdateinstaller.h
    )

set_source_files_properties(${updater_service_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND 
    updater_service_SOURCE_FILES 
    ${updater_service_HEADER_FILES} 
    )

add_library(llupdaterservice
            ${updater_service_SOURCE_FILES}
            )

target_link_libraries(llupdaterservice
    ${LLCOMMON_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${LLPLUGIN_LIBRARIES}
	${LLVFS_LIBRARIES}
    )

if(LL_TESTS)
  SET(llupdater_service_TEST_SOURCE_FILES
      llupdaterservice.cpp
      )

# *NOTE:Mani - I was trying to use the preprocessor seam to mock out
#              llifstream (and other) llcommon classes. I didn't work
#              because of the windows declspec(dllimport)attribute.
#set_source_files_properties(
#    llupdaterservice.cpp
#    PROPERTIES
#      LL_TEST_ADDITIONAL_CFLAGS "-Dllifstream=llus_mock_llifstream"
#    )

  LL_ADD_PROJECT_UNIT_TESTS(llupdaterservice "${llupdater_service_TEST_SOURCE_FILES}")
endif(LL_TESTS)

set(UPDATER_INCLUDE_DIRS 
  ${LIBS_OPEN_DIR}/viewer_components/updater 
  CACHE INTERNAL ""
)

set(UPDATER_LIBRARIES 
  llupdaterservice
  CACHE INTERNAL ""
)

# Copy install script.
if(DARWIN)
	copy_if_different(
		"${CMAKE_CURRENT_SOURCE_DIR}/scripts/darwin"
		"${CMAKE_CURRENT_BINARY_DIR}"
		update_installer_targets
		"update_install"
		)
elseif(WINDOWS)
	copy_if_different(
		"${CMAKE_CURRENT_SOURCE_DIR}/scripts/windows"
		"${CMAKE_CURRENT_BINARY_DIR}"
		update_installer_targets
		"update_install.bat"
		)
endif()
add_custom_target(copy_update_install ALL DEPENDS ${update_installer_targets})
add_dependencies(llupdaterservice copy_update_install)


 