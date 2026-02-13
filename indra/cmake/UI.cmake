# -*- cmake -*-
include_guard()

include(FreeType)
include(GLIB)

add_library(ll::uilibraries INTERFACE IMPORTED)

if (LINUX)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(WAYLAND_CLIENT wayland-client)

  if(WAYLAND_CLIENT_FOUND)
      target_include_directories(ll::uilibraries SYSTEM INTERFACE ${WAYLAND_CLIENT_INCLUDE_DIRS})
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


  target_link_libraries(ll::uilibraries INTERFACE
          ll::fontconfig
          ll::freetype
          ll::glib
          ll::gio
  )
elseif(DARWIN)
  target_link_libraries(ll::uilibraries INTERFACE
          ${CARBON_LIBRARY}
          )
elseif(WINDOWS)
  target_link_libraries(ll::uilibraries INTERFACE
          comdlg32 # Common Dialogs for ChooseColor
          ole32
          dxgi
          d3d9
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
