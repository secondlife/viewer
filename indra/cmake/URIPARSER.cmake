# -*- cmake -*-

set(URIPARSER_FIND_QUIETLY ON)
set(URIPARSER_FIND_REQUIRED ON)

include(Prebuilt)

if (USESYSTEMLIBS)
  include(FindURIPARSER)
else (USESYSTEMLIBS)
  use_prebuilt_binary(uriparser)
  if (WINDOWS)
    set(URIPARSER_LIBRARIES
      debug uriparserd
      optimized uriparser)
  elseif (LINUX)
    #
    # When we have updated static libraries in competition with older
    # shared libraries and we want the former to win, we need to do some
    # extra work.  The *_PRELOAD_ARCHIVES settings are invoked early
    # and will pull in the entire archive to the binary giving it.
    # priority in symbol resolution.  Beware of cmake moving the
    # achive load itself to another place on the link command line.  If
    # that happens, you can try something like -Wl,-luriparser here to hide
    # the archive.  Also be aware that the linker will not tolerate a
    # second whole-archive load of the archive.  See viewer's
    # CMakeLists.txt for more information.
    #
    set(URIPARSER_PRELOAD_ARCHIVES -Wl,--whole-archive uriparser -Wl,--no-whole-archive)
    set(URIPARSER_LIBRARIES uriparser)
  elseif (DARWIN)
    set(URIPARSER_LIBRARIES liburiparser.dylib)
  endif (WINDOWS)
  set(URIPARSER_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/uriparser)
endif (USESYSTEMLIBS)
