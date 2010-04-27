# -*- cmake -*-

project(winmm_shim)

### winmm_shim

set(winmm_shim_SOURCE_FILES
    forwarding_api.cpp
    winmm_shim.cpp
    )

set(winmm_shim_HEADER_FILES
    forwarding_api.h
    winmm.def
    )

list(APPEND winmm_shim_SOURCE_FILES ${winmm_shim_HEADER_FILES})

set_source_files_properties(${media_plugin_webkit_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

add_library(winmm_shim
    SHARED
    ${winmm_shim_SOURCE_FILES}
)

if (WINDOWS)
  set_target_properties(
    winmm_shim
    PROPERTIES
    LINK_FLAGS "/MANIFEST:NO"
	OUTPUT_NAME "winmm"
    )
endif (WINDOWS)
