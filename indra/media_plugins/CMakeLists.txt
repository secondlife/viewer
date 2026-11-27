# -*- cmake -*-

add_subdirectory(base)
add_subdirectory(cef)
add_subdirectory(libvlc)
add_subdirectory(example)

if (LINUX)
    add_subdirectory(gstreamer10)
endif (LINUX)
