# -*- cmake -*-
include(Prebuilt)
include(FreeType)

add_library( ll::uilibraries INTERFACE IMPORTED )

if (LINUX)
  target_compile_definitions(ll::uilibraries INTERFACE LL_GTK=1 LL_X11=1 )

  if( USE_CONAN )
    target_link_libraries( ll::uilibraries INTERFACE CONAN_PKG::gtk )
    return()
  endif()
  use_prebuilt_binary(gtk-atk-pango-glib)

  target_link_libraries( ll::uilibraries INTERFACE
          atk-1.0
          gdk-x11-2.0
          gdk_pixbuf-2.0
          Xinerama
          glib-2.0
          gmodule-2.0
          gobject-2.0
          gthread-2.0
          gtk-x11-2.0
          pango-1.0
          pangoft2-1.0
          pangox-1.0
          pangoxft-1.0
          Xinerama
          ll::freetype
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

