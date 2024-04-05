# -*- cmake -*-
include(Prebuilt)
include(FreeType)
include(GLIB)

add_library( ll::uilibraries INTERFACE IMPORTED )

if (LINUX)
  target_compile_definitions(ll::uilibraries INTERFACE LL_X11=1 )

  if( USE_CONAN )
    return()
  endif()

  find_package(FLTK REQUIRED )
  find_library(ND_FLTK_STATIC_LIBRARY libfltk.a PATH_SUFFIXES fltk )

  if( NOT ND_FLTK_STATIC_LIBRARY )
    message(FATAL_ERROR "libfltk.a not found")
  else()
    message("libfltk.a found ${ND_FLTK_STATIC_LIBRARY}")
  endif()

  target_include_directories( ll::uilibraries SYSTEM INTERFACE   ${FLTK_INCLUDE_DIR})
  target_compile_definitions( ll::uilibraries INTERFACE LL_FLTK=1 )
  target_link_libraries( ll::uilibraries INTERFACE
          ${ND_FLTK_STATIC_LIBRARY}
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
