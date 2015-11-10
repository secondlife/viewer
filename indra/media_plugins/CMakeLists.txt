# -*- cmake -*-

add_subdirectory(base)

add_subdirectory(webkit)

add_subdirectory(gstreamer010)

if (WINDOWS OR DARWIN)
    add_subdirectory(quicktime)
endif (WINDOWS OR DARWIN)

if (WINDOWS)
    add_subdirectory(winmmshim)
endif (WINDOWS)

add_subdirectory(example)
