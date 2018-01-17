# ll_deploy_sharedlibs_command
# target_exe: the cmake target of the executable for which the shared libs will be deployed.
macro(ll_deploy_sharedlibs_command target_exe) 
  set(TARGET_LOCATION $<TARGET_FILE:${target_exe}>)
  get_filename_component(OUTPUT_PATH ${TARGET_LOCATION} PATH)

  # It's not clear that this does anything useful for us on Darwin. It has
  # been broken for some time now; the BIN_NAME was being constructed as a
  # ridiculous nonexistent path with duplicated segments. Fixing that only
  # produces ominous spammy warnings: at the time the command below is run, we
  # have not yet populated the nested mac-crash-logger.app/Contents/Resources
  # with the .dylibs with which it was linked. Moreover, the form of the
  # embedded @executable_path/../Resources/mumble.dylib pathname confuses the
  # GetPrerequisites.cmake tool invoked by DeploySharedLibs.cmake. It seems
  # clear that we have long since accomplished by other means what this was
  # originally supposed to do. Skipping it only eliminates an annoying
  # non-fatal error.
  if(NOT DARWIN)
    if(WINDOWS)
      SET_TEST_PATH(SEARCH_DIRS)
      LIST(APPEND SEARCH_DIRS "$ENV{SystemRoot}/system32")
    elseif(LINUX)
      SET_TEST_PATH(SEARCH_DIRS)
      set(OUTPUT_PATH ${OUTPUT_PATH}/lib)
    endif(WINDOWS)

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
  endif(NOT DARWIN)

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
