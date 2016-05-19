# -*- cmake -*-

add_subdirectory(base)

if (LINUX)
    add_subdirectory(gstreamer010)
endif (LINUX)

if (WINDOWS OR DARWIN)
    add_subdirectory(quicktime)
    add_subdirectory(cef)
endif (WINDOWS OR DARWIN)

if (WINDOWS)
    add_subdirectory(winmmshim)
endif (WINDOWS)

### add_subdirectory(example)
