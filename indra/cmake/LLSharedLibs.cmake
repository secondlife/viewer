# ll_deploy_sharedlibs_command
# target_exe: the cmake target of the executable for which the shared libs will be deployed.
# search_dirs: a list of dirs to search for the dependencies
# dst_path: path to copy deps to, relative to the output location of the target_exe
macro(ll_deploy_sharedlibs_command target_exe search_dirs dst_path) 
  get_target_property(OUTPUT_LOCATION ${target_exe} LOCATION)

  if(DARWIN)
    get_target_property(IS_BUNDLE ${target_exe} MACOSX_BUNDLE)
    if(IS_BUNDLE)
      get_filename_component(TARGET_FILE ${OUTPUT_LOCATION} NAME)
      set(OUTPUT_PATH ${OUTPUT_LOCATION}.app/Contents/MacOS)
      set(OUTPUT_LOCATION ${OUTPUT_PATH}/${TARGET_FILE})
    endif(IS_BUNDLE)
  else(DARWIN)
    message(FATAL_ERROR "Only darwin currently supported!")
  endif(DARWIN)
  
  add_custom_command(
    TARGET ${target_exe} POST_BUILD
    COMMAND ${CMAKE_COMMAND} 
    ARGS
    "-DBIN_NAME=\"${OUTPUT_LOCATION}\""
    "-DSEARCH_DIRS=\"${search_dirs}\""
    "-DDST_PATH=\"${OUTPUT_PATH}/${dst_path}\""
    "-P"
    "${CMAKE_SOURCE_DIR}/cmake/DeploySharedLibs.cmake"
    )

endmacro(ll_deploy_sharedlibs_command)

