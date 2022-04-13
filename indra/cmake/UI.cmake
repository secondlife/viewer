# -*- cmake -*-
include(Prebuilt)
include(FreeType)

if (LINUX)
  use_prebuilt_binary(gtk-atk-pango-glib)
endif (LINUX)

if (LINUX)
  set(UI_LIBRARIES
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
          ${FREETYPE_LIBRARIES}
          )
endif (LINUX)

include_directories (
        ${LIBS_PREBUILT_DIR}/include
        ${LIBS_PREBUILT_DIR}/include
)
foreach(include ${${LL_ARCH}_INCLUDES})
  include_directories(${LIBS_PREBUILT_DIR}/include/${include})
endforeach(include)

if (LINUX)
  add_definitions(-DLL_GTK=1 -DLL_X11=1)
endif (LINUX)
