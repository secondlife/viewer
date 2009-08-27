# -*- cmake -*-

add_subdirectory(base)

add_subdirectory(webkit)

add_subdirectory(gstreamer010)

if (WINDOWS OR DARWIN)
    add_subdirectory(quicktime)
    add_subdirectory(awesomium)
endif (WINDOWS OR DARWIN)

if (WINDOWS)
    add_subdirectory(flash_activex)
endif(WINDOWS)
