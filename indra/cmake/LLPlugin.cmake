# -*- cmake -*-


set(LLPLUGIN_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llplugin
    )

if (LINUX)
    # In order to support using ld.gold on linux, we need to explicitely
    # specify all libraries that llplugin uses.
	set(LLPLUGIN_LIBRARIES llplugin pthread)
else (LINUX)
	set(LLPLUGIN_LIBRARIES llplugin)
endif (LINUX)
