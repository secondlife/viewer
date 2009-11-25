# DeploySharedLibs.cmake
# This is a script to be run at build time! Its not part of the cmake configuration!
# See indra/cmake/LLSharedLibs.cmake for a macro that simplifies adding a command to a target to run this script.

# This  script requires a few cmake variable to be set on the command line:
# BIN_NAME= The full path the the binary to search for dependecies.
# SEARCH_DIRS= The full paths to dirs to search for dependencies.
# DST_PATH= The full path where the dependecies will be copied. 

# *FIX:Mani - I pulled in the CMake 2.8 GetPrerequisites.cmake script here, because it works on windows where 2.6 did not.
# Once we have officially upgraded to 2.8 we can just use that version of GetPrerequisites.cmake.
get_filename_component(current_dir ${CMAKE_CURRENT_LIST_FILE} PATH)
include(${current_dir}/GetPrerequisites_2_8.cmake)

message("Getting recursive dependencies for file: ${BIN_NAME}")

set(EXCLUDE_SYSTEM 1)
set(RECURSE 1)
get_filename_component(EXE_PATH ${BIN_NAME} PATH)

get_prerequisites( ${BIN_NAME} RESULTS ${EXCLUDE_SYSTEM} ${RECURSE} "${EXE_PATH}" "${SEARCH_DIRS}" )

foreach(DEP ${RESULTS})
  Message("Processing dependency: ${DEP}")
  get_filename_component(DEP_FILE ${DEP} NAME)
  set(DEP_FILES ${DEP_FILES} ${DEP_FILE})
endforeach(DEP)

if(DEP_FILES)
  list(REMOVE_DUPLICATES DEP_FILES)
endif(DEP_FILES)

foreach(DEP_FILE ${DEP_FILES})
  if(FOUND_FILES)
    list(FIND FOUND_FILES ${DEP_FILE} FOUND)
  else(FOUND_FILES)
    set(FOUND -1)
  endif(FOUND_FILES)

  if(FOUND EQUAL -1)
    find_path(DEP_PATH ${DEP_FILE} PATHS ${SEARCH_DIRS} NO_DEFAULT_PATH)
    if(DEP_PATH)
      set(FOUND_FILES ${FOUND_FILES} "${DEP_PATH}/${DEP_FILE}")
      set(DEP_PATH NOTFOUND) #reset DEP_PATH for the next find_path call.
    else(DEP_PATH)
      set(MISSING_FILES ${MISSING_FILES} ${DEP_FILE})
    endif(DEP_PATH)
  endif(FOUND EQUAL -1)
endforeach(DEP_FILE)

if(MISSING_FILES)
  message("Missing:")
  foreach(FILE ${MISSING_FILES})
    message("  ${FILE}")
  endforeach(FILE)
  message("Searched in:")
  foreach(SEARCH_DIR ${SEARCH_DIRS})
    message("  ${SEARCH_DIR}")
  endforeach(SEARCH_DIR)
  message(FATAL_ERROR "Failed")
endif(MISSING_FILES)

if(FOUND_FILES)
  foreach(FILE ${FOUND_FILES})
    get_filename_component(DST_FILE ${FILE} NAME)
    set(DST_FILE "${DST_PATH}/${DST_FILE}")
    message("Copying ${FILE} to ${DST_FILE}")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FILE} ${DST_FILE}
      )
  endforeach(FILE ${FOUND_FILES})
endif(FOUND_FILES)
message("Success!")
