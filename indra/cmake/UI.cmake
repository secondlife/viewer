# -*- cmake -*-
include(Prebuilt)
include(FreeType)

if (LINUX)
  use_prebuilt_binary(gtk-atk-pango-glib)
endif (LINUX)

create_target( ll::uilibraries )

if (LINUX)
  set_target_libraries( ll::uilibraries
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
  set_target_libraries( ll::uilibraries
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
          )
endif()

set_target_include_dirs( ll::uilibraries
        ${LIBS_PREBUILT_DIR}/include
        )


# The following made no real sense
# - ${ARCH}_linux_INCLUDES was set in 00-Common.cmake,
# - Here ist is used, but as ${${LL_ARCH}_INCLUDES}
# It would make more sense to use one form ($LL_ARCH)
# Leave this out for the moment, but come back when looking at the Linux build

#set(${ARCH}_linux_INCLUDES
#        atk-1.0
#        glib-2.0
#        gstreamer-0.10
#        gtk-2.0
#        pango-1.0
#        )
#foreach(include ${${LL_ARCH}_INCLUDES})
#  set_target_include_dirs( ll::uilibraries
#          ${LIBS_PREBUILT_DIR}/include/${include}
#          )
#endforeach(include)

if (LINUX)
  set_target_properties(ll::uilibraries PROPERTIES COMPILE_DEFINITIONS LL_GTK=1 LL_X11=1 )
endif (LINUX)
