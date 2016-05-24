# -*- cmake -*-

set(ZLIB_FIND_QUIETLY ON)
set(ZLIB_FIND_REQUIRED ON)

include(Prebuilt)

if (USESYSTEMLIBS)
  include(FindZLIB)
else (USESYSTEMLIBS)
  use_prebuilt_binary(zlib)
  if (WINDOWS)
    set(ZLIB_LIBRARIES 
      debug zlibd
      optimized zlib)
  elseif (LINUX)
    #
    # When we have updated static libraries in competition with older
    # shared libraries and we want the former to win, we need to do some
    # extra work.  The *_PRELOAD_ARCHIVES settings are invoked early
    # and will pull in the entire archive to the binary giving it 
    # priority in symbol resolution.  Beware of cmake moving the
    # achive load itself to another place on the link command line.  If
    # that happens, you can try something like -Wl,-lz here to hide
    # the archive.  Also be aware that the linker will not tolerate a
    # second whole-archive load of the archive.  See viewer's
    # CMakeLists.txt for more information.
    #
    set(ZLIB_PRELOAD_ARCHIVES -Wl,--whole-archive z -Wl,--no-whole-archive)
    set(ZLIB_LIBRARIES z)
  elseif (DARWIN)
    set(ZLIB_LIBRARIES z)
  endif (WINDOWS)
  set(ZLIB_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/zlib)
endif (USESYSTEMLIBS)
