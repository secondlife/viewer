# -*- cmake -*-
include(Prebuilt)

set(PNG_FIND_QUIETLY ON)
set(PNG_FIND_REQUIRED ON)

if (USESYSTEMLIBS)
  include(FindPNG)
else (USESYSTEMLIBS)
  use_prebuilt_binary(libpng)
  if (WINDOWS)
    set(PNG_LIBRARIES libpng16)
    set(PNG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/libpng16)
  elseif(DARWIN)
    set(PNG_LIBRARIES png16)
    set(PNG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/libpng16)
  else()
    #
    # When we have updated static libraries in competition with older
    # shared libraries and we want the former to win, we need to do some
    # extra work.  The *_PRELOAD_ARCHIVES settings are invoked early
    # and will pull in the entire archive to the binary giving it 
    # priority in symbol resolution.  Beware of cmake moving the
    # achive load itself to another place on the link command line.  If
    # that happens, you can try something like -Wl,-lpng16 here to hide
    # the archive.  Also be aware that the linker will not tolerate a
    # second whole-archive load of the archive.  See viewer's
    # CMakeLists.txt for more information.
    #
    set(PNG_PRELOAD_ARCHIVES -Wl,--whole-archive png16 -Wl,--no-whole-archive)
    set(PNG_LIBRARIES png16)
    set(PNG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/libpng16)
  endif()
endif (USESYSTEMLIBS)
