# -*- cmake -*-
include(Prebuilt)

if( TARGET glod::glod )
  return()
endif()
create_target( glod::glod )

if (NOT USESYSTEMLIBS)
  use_prebuilt_binary(glod)
endif (NOT USESYSTEMLIBS)



set(GLODLIB ON CACHE BOOL "Using GLOD library")

set_target_include_dirs( glod::glod ${LIBS_PREBUILT_DIR}/include)
set_target_libraries( glod::glod GLOD )
