# ll_deploy_sharedlibs_command
# target_exe: the cmake target of the executable for which the shared libs will be deployed.
macro(ll_deploy_sharedlibs_command target_exe) 
  get_target_property(TARGET_LOCATION ${target_exe} LOCATION)
  get_filename_component(OUTPUT_PATH ${TARGET_LOCATION} PATH)
  
  if(DARWIN)
    SET_TEST_PATH(SEARCH_DIRS)
    get_target_property(IS_BUNDLE ${target_exe} MACOSX_BUNDLE)
    if(IS_BUNDLE)
      # If its a bundle the exe is not in the target location, this should find it.
      get_filename_component(TARGET_FILE ${TARGET_LOCATION} NAME)
      set(OUTPUT_PATH ${TARGET_LOCATION}.app/Contents/MacOS)
      set(TARGET_LOCATION ${OUTPUT_PATH}/${TARGET_FILE})
      set(OUTPUT_PATH ${OUTPUT_PATH}/../Resources)
    endif(IS_BUNDLE)
  elseif(WINDOWS)
    SET_TEST_PATH(SEARCH_DIRS)
    LIST(APPEND SEARCH_DIRS "$ENV{SystemRoot}/system32")
  elseif(LINUX)
    SET_TEST_PATH(SEARCH_DIRS)
    set(OUTPUT_PATH ${OUTPUT_PATH}/lib)
  endif(DARWIN)

  add_custom_command(
    TARGET ${target_exe} POST_BUILD
    COMMAND ${CMAKE_COMMAND} 
    ARGS
    "-DBIN_NAME=\"${TARGET_LOCATION}\""
    "-DSEARCH_DIRS=\"${SEARCH_DIRS}\""
    "-DDST_PATH=\"${OUTPUT_PATH}\""
    "-P"
    "${CMAKE_SOURCE_DIR}/cmake/DeploySharedLibs.cmake"
    )

endmacro(ll_deploy_sharedlibs_command)

# ll_stage_sharedlib
# Performs config and adds a copy command for a sharedlib target.
macro(ll_stage_sharedlib DSO_TARGET)
  # target gets written to the DLL staging directory.
  # Also this directory is shared with RunBuildTest.cmake, y'know, for the tests.
  set_target_properties(${DSO_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SHARED_LIB_STAGING_DIR})
  if(NOT WINDOWS)
    get_target_property(DSO_PATH ${DSO_TARGET} LOCATION)
    get_filename_component(DSO_FILE ${DSO_PATH} NAME)
    if(DARWIN)
      set(SHARED_LIB_STAGING_DIR_CONFIG ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/Resources)
    else(DARWIN)
      set(SHARED_LIB_STAGING_DIR_CONFIG ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR})
    endif(DARWIN)

      # *TODO - maybe make this a symbolic link? -brad
      add_custom_command(
        TARGET ${DSO_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS
          -E
          copy_if_different
          ${DSO_PATH}
          ${SHARED_LIB_STAGING_DIR_CONFIG}/${DSO_FILE}
          COMMENT "Copying llcommon to the staging folder."
        )
    endif(NOT WINDOWS)

  if (DARWIN)
    set_target_properties(${DSO_TARGET} PROPERTIES
      BUILD_WITH_INSTALL_RPATH 1
      INSTALL_NAME_DIR "@executable_path/../Resources"
      )
  endif(DARWIN)

endmacro(ll_stage_sharedlib)
