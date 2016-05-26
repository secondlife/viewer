# -*- cmake -*-

add_subdirectory(base)

if (LINUX)
    add_subdirectory(gstreamer010)
endif (LINUX)

if (DARWIN)
    add_subdirectory(quicktime)
    add_subdirectory(cef)
endif (DARWIN)

if (WINDOWS)
    add_subdirectory(cef)
    add_subdirectory(winmmshim)
    add_subdirectory(libvlc)
endif (WINDOWS)

### add_subdirectory(example)
