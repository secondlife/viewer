# -*- cmake -*-
include_guard()

include(Linking)
include(Prebuilt)

add_library(ll::embeddedbrowser INTERFACE IMPORTED)

target_include_directories(ll::embeddedbrowser SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include/cef")

#use_prebuilt_binary(embeddedbrowser)

#find_library(EMBEDDEDBROWSER_LIBRARY
#    NAMES
#    embeddedbrowser
#    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

#target_link_libraries(ll::embeddedbrowser INTERFACE ${EMBEDDEDBROWSER_LIBRARY})

if (DARWIN)
    target_link_libraries(ll::embeddedbrowser INTERFACE ll::oslibraries)
elseif (LINUX)
    target_link_libraries( ll::embeddedbrowser INTERFACE X11)
endif ()


