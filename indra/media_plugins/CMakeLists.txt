# -*- cmake -*-

add_subdirectory(base)

if (BUILD_EXAMPLE_PLUGIN)
    add_subdirectory(example)
endif()

if (BUILD_CEF_PLUGIN)
    add_subdirectory(cef)
endif()

if (BUILD_VLC_PLUGIN)
    add_subdirectory(libvlc)
endif()

if (BUILD_GSTREAMER_PLUGIN)
    add_subdirectory(gstreamer10)
endif ()
