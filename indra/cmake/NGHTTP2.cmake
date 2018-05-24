include(Prebuilt)

set(NGHTTP2_FIND_QUIETLY ON)
set(NGHTTP2_FIND_REQUIRED ON)

if (USESYSTEMLIBS)
  include(FindNGHTTP2)
else (USESYSTEMLIBS)
  use_prebuilt_binary(nghttp2)
  if (WINDOWS)
    set(NGHTTP2_LIBRARIES 
      ${ARCH_PREBUILT_DIRS_RELEASE}/nghttp2.lib
      )
  elseif (DARWIN)
    set(NGHTTP2_LIBRARIES libnghttp2.dylib)
  else (WINDOWS)
    set(NGHTTP2_LIBRARIES libnghttp2.a)
  endif (WINDOWS)
  set(NGHTTP2_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/nghttp2)
endif (USESYSTEMLIBS)
