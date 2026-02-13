# -*- cmake -*-
include_guard()

if (OS_DRAG_DROP)
    if (WINDOWS)
        add_definitions(-DLL_OS_DRAGDROP_ENABLED=1)
    endif (WINDOWS)
    if (DARWIN)
        add_definitions(-DLL_OS_DRAGDROP_ENABLED=1)
    endif (DARWIN)
    if (LINUX)
        add_definitions(-DLL_OS_DRAGDROP_ENABLED=1)
    endif (LINUX)
endif (OS_DRAG_DROP)

