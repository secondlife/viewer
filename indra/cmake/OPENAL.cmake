# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (LINUX)
  set(OPENAL ON CACHE BOOL "Enable OpenAL")
else (LINUX)
  set(OPENAL OFF CACHE BOOL "Enable OpenAL")
endif (LINUX)

if (OPENAL)
  if( TARGET openal::openal )
    return()
  endif()

  create_target( openal::openal )
  set_target_include_dirs( openal::openal "${LIBS_PREBUILT_DIR}/include/AL")

  use_prebuilt_binary(openal)

  if(WINDOWS)
    set_target_libraries( openal::openal
            OpenAL32
            alut
            )
  elseif(LINUX)
    set_target_libraries( openal::openal
            openal
            alut
            )
  else()
    message(FATAL_ERROR "OpenAL is not available for this platform")
  endif()
endif (OPENAL)
