# -*- cmake -*-
include(Prebuilt)
include(FreeType)

add_library( ll::uilibraries INTERFACE IMPORTED )

if (LINUX)
  target_compile_definitions(ll::uilibraries INTERFACE LL_GTK=0 LL_X11=1 )

  if( USE_CONAN )
    return()
  endif()

  find_package(PkgConfig REQUIRED)
  pkg_search_module(GLIB REQUIRED glib-2.0)
  pkg_search_module(GLIB REQUIRED gio-2.0)

  find_package(FLTK REQUIRED )

  target_include_directories( ll::uilibraries SYSTEM INTERFACE  ${GLIB_INCLUDE_DIRS}  ${FLTK_INCLUDE_DIR})

  target_link_libraries(ll::uilibraries INTERFACE ${GLIB_LDFLAGS})
  target_link_libraries(ll::uilibraries INTERFACE ${FLTK_BASE_LIBRARY_RELEASE})

  target_link_libraries( ll::uilibraries INTERFACE
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

