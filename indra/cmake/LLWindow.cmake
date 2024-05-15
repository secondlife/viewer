# -*- cmake -*-

include(Variables)
include(GLEXT)
include(Prebuilt)

include_guard()
add_library( ll::SDL INTERFACE IMPORTED )


if (LINUX)
  #Must come first as use_system_binary can exit this file early
  target_compile_definitions( ll::SDL INTERFACE LL_SDL=1)

  use_system_binary(SDL)
  use_prebuilt_binary(SDL)

  target_include_directories( ll::SDL SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)
  target_link_libraries( ll::SDL INTERFACE SDL directfb fusion direct X11)
endif (LINUX)


