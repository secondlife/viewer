# -*- cmake -*-

add_subdirectory(base)

if (LINUX)
    add_subdirectory(cef)
    add_subdirectory(example)
    add_subdirectory(gstreamer10)
endif (LINUX)

if (DARWIN)
    add_subdirectory(cef)
    add_subdirectory(libvlc)
    add_subdirectory(example)
endif (DARWIN)

if (WINDOWS)
    add_subdirectory(cef)
    add_subdirectory(libvlc)
    add_subdirectory(example)
endif (WINDOWS)
