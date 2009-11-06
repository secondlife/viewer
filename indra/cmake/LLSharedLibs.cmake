
if(DARWIN)
  set(TMP_PATH "../Resource")
elseif(LINUX)
  set(TMP_PATH "../lib")
else(DARWIN)
  set(TMP_PATH ".")
endif(DARWIN)

 set(SHARED_LIB_REL_PATH ${TMP_PATH} CACHE STRING "Relative path from executable to shared libs")

# ll_deploy_sharedlibs_command
# target_exe: the cmake target of the executable for which the shared libs will be deployed.
# search_dirs: a list of dirs to search for the dependencies
# dst_path: path to copy deps to, relative to the output location of the target_exe
macro(ll_deploy_sharedlibs_command target_exe search_dirs dst_path) 
  get_target_property(OUTPUT_LOCATION ${target_exe} LOCATION)
  get_filename_component(OUTPUT_PATH ${OUTPUT_LOCATION} PATH)

  if(DARWIN)
    get_target_property(IS_BUNDLE ${target_exe} MACOSX_BUNDLE)
    if(IS_BUNDLE)
      get_filename_component(TARGET_FILE ${OUTPUT_LOCATION} NAME)
      set(OUTPUT_PATH ${OUTPUT_LOCATION}.app/Contents/MacOS)
      set(OUTPUT_LOCATION ${OUTPUT_PATH}/${TARGET_FILE})
    endif(IS_BUNDLE)
  endif(DARWIN)

  if(WINDOWS)
    set(REAL_SEARCH_DIRS ${search_dirs} "$ENV{SystemRoot}/system32")
  endif(WINDOWS)

  if(LINUX)
    message(FATAL_ERROR "LINUX Unsupported!?!")
  endif(LINUX)
  
  add_custom_command(
    TARGET ${target_exe} POST_BUILD
    COMMAND ${CMAKE_COMMAND} 
    ARGS
    "-DBIN_NAME=\"${OUTPUT_LOCATION}\""
    "-DSEARCH_DIRS=\"${REAL_SEARCH_DIRS}\""
    "-DDST_PATH=\"${OUTPUT_PATH}/${dst_path}\""
    "-P"
    "${CMAKE_SOURCE_DIR}/cmake/DeploySharedLibs.cmake"
    )

endmacro(ll_deploy_sharedlibs_command)

