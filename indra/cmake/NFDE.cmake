# -*- cmake -*-
if(LINUX)
    set(USE_NFDE ON CACHE BOOL "Use Native File Dialog wrapper library")
    set(USE_NFDE_PORTAL ON CACHE BOOL "Use NFDE XDG Portals")
endif()

include_guard()

add_library(ll::nfde INTERFACE IMPORTED)
if(USE_NFDE)
    include(Prebuilt)
    use_prebuilt_binary(nfde)

    target_compile_definitions( ll::nfde INTERFACE LL_NFD=1)

    if (WINDOWS)
        target_link_libraries( ll::nfde INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/nfd.lib)
    elseif (DARWIN)
        target_link_libraries( ll::nfde INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libnfd.a)
    elseif (LINUX)
        if(USE_NFDE_PORTAL)
            target_link_libraries( ll::nfde INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libnfd_portal.a)
        else()
            target_link_libraries( ll::nfde INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libnfd_gtk.a)
        endif()
    endif ()

    if (LINUX)
        find_package(PkgConfig REQUIRED)
        if(NOT USE_NFDE_PORTAL)
            pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
            target_link_libraries(ll::nfde INTERFACE ${GTK3_LINK_LIBRARIES})
        else()
            pkg_check_modules(DBUS REQUIRED dbus-1)
            target_link_libraries(ll::nfde INTERFACE ${DBUS_LINK_LIBRARIES})
        endif()
    endif()

    target_include_directories( ll::nfde SYSTEM INTERFACE
            ${LIBS_PREBUILT_DIR}/include/nfde
            )
endif()
