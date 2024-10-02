# -*- cmake -*-
include(Prebuilt)
include(FreeType)
include(GLIB)

add_library( ll::uilibraries INTERFACE IMPORTED )

if (LINUX)
  use_prebuilt_binary(fltk)
  target_compile_definitions(ll::uilibraries INTERFACE LL_FLTK=1 LL_X11=1 )

  if( USE_CONAN )
    return()
  endif()

  if( COMPILE_WAYLAND_SUPPORT )
      include(FindPkgConfig)
      pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)

      target_link_libraries( ll::uilibraries INTERFACE ${WAYLAND_CLIENT_LIBRARIES} )
      target_compile_definitions( ll::uilibraries INTERFACE LL_WAYLAND=1)
  endif()

  target_link_libraries( ll::uilibraries INTERFACE
          fltk
          Xrender
          Xcursor
          Xfixes
          Xext
          Xft
          Xinerama
          ll::fontconfig
          ll::freetype
          ll::SDL
          ll::glib
          ll::gio
  )

endif (LINUX)
if( WINDOWS )
  target_link_libraries( ll::uilibraries INTERFACE
          opengl32
          comdlg32
          dxguid
          kernel32
          odbc32
          odbccp32
          oleaut32
          shell32
          Vfw32
          wer
          winspool
          imm32
          )
endif()

target_include_directories( ll::uilibraries SYSTEM INTERFACE
        ${LIBS_PREBUILT_DIR}/include
        )
