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
