# -*- cmake -*-

if( TARGET uriparser::uriparser )
  return()
endif()
create_target( uriparser::uriparser )

include(Prebuilt)

if (USESYSTEMLIBS)
  include(FindURIPARSER)
else (USESYSTEMLIBS)
  use_prebuilt_binary(uriparser)
  if (WINDOWS)
    set_target_libraries( uriparser::uriparser uriparser)
  elseif (LINUX)
    set_target_libraries( uriparser::uriparser uriparser)
  elseif (DARWIN)
    set_target_libraries( uriparser::uriparser liburiparser.dylib)
  endif (WINDOWS)
  set_target_include_dirs( uriparser::uriparser ${LIBS_PREBUILT_DIR}/include/uriparser)
endif (USESYSTEMLIBS)
