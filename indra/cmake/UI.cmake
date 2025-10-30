# -*- cmake -*-
include(Prebuilt)
include(FreeType)
include(GLIB)

include_guard()
add_library( ll::uilibraries INTERFACE IMPORTED )

if (LINUX)
  if( USE_CONAN )
    return()
  endif()

  find_package(PkgConfig REQUIRED)
  pkg_check_modules(WAYLAND_CLIENT wayland-client)

  if(WAYLAND_CLIENT_FOUND)
      target_compile_definitions(ll::uilibraries INTERFACE LL_WAYLAND=1)
  else()
      message("pkgconfig could not find wayland client, compiling without full wayland support")
  endif()

  find_package(X11)
  if(X11_FOUND)
      target_compile_definitions(ll::uilibraries INTERFACE LL_X11=1)
  else()
      message("Could not find X11, compiling without full X11 support")
  endif()


  target_link_libraries( ll::uilibraries INTERFACE
          ll::fontconfig
          ll::freetype
          ll::SDL3
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

