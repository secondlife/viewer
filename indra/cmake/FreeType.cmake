# -*- cmake -*-
include(Prebuilt)

include_guard()
add_library( ll::freetype INTERFACE IMPORTED )

if( LINUX )
    find_package(Freetype REQUIRED )
    target_link_libraries(ll::freetype INTERFACE Freetype::Freetype)
else ()
    use_system_binary(freetype)
    use_prebuilt_binary(freetype)
    target_include_directories( ll::freetype SYSTEM INTERFACE  ${LIBS_PREBUILT_DIR}/include/freetype2/)
    target_link_libraries( ll::freetype INTERFACE freetype )
endif()