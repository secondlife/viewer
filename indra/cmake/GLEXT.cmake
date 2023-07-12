# -*- cmake -*-
include(Prebuilt)
include(GLH)

add_library( ll::glext INTERFACE IMPORTED )
if (WINDOWS OR LINUX)
  use_system_binary(glext)
  use_prebuilt_binary(glext)
endif (WINDOWS OR LINUX)


