# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (STANDALONE)
  # The minimal version, 4.4.3, is rather arbitrary: it's the version in Debian/Lenny.
  find_package(Qt4 4.4.3 COMPONENTS QtCore QtGui QtNetwork QtOpenGL QtWebKit REQUIRED)
  include(${QT_USE_FILE})
  set(QTDIR $ENV{QTDIR})
  if (QTDIR AND NOT "${QT_BINARY_DIR}" STREQUAL "${QTDIR}/bin")
    message(FATAL_ERROR "\"${QT_BINARY_DIR}\" is unequal \"${QTDIR}/bin\"; "
      "Qt is found by looking for qmake in your PATH. "
      "Please set your PATH such that 'qmake' is found in \$QTDIR/bin, "
      "or unset QTDIR if the found Qt is correct.")
    endif (QTDIR AND NOT "${QT_BINARY_DIR}" STREQUAL "${QTDIR}/bin")
  find_package(LLQtWebkit REQUIRED QUIET)
  # Add the plugins.
  set(QT_PLUGIN_LIBRARIES)
  foreach(qlibname qgif qjpeg)
    find_library(QT_PLUGIN_${qlibname} ${qlibname} PATHS ${QT_PLUGINS_DIR}/imageformats NO_DEFAULT_PATH)
    if (QT_PLUGIN_${qlibname})
      list(APPEND QT_PLUGIN_LIBRARIES ${QT_PLUGIN_${qlibname}})
    else (QT_PLUGIN_${qtlibname})
      message(FATAL_ERROR "Could not find the Qt plugin ${qlibname} in \"${QT_PLUGINS_DIR}/imageformats\"!")
    endif (QT_PLUGIN_${qlibname})
  endforeach(qlibname)
  # qjpeg depends on libjpeg
  list(APPEND QT_PLUGIN_LIBRARIES jpeg)
    set(WEBKITLIBPLUGIN OFF CACHE BOOL
        "WEBKITLIBPLUGIN support for the llplugin/llmedia test apps.")
else (STANDALONE)
    use_prebuilt_binary(llqtwebkit)
    set(WEBKITLIBPLUGIN ON CACHE BOOL
        "WEBKITLIBPLUGIN support for the llplugin/llmedia test apps.")
endif (STANDALONE)

if (WINDOWS)
    set(WEBKIT_PLUGIN_LIBRARIES 
    debug llqtwebkitd
    debug QtWebKitd4
    debug QtOpenGLd4
    debug QtNetworkd4
    debug QtGuid4
    debug QtCored4
    debug qtmaind
    optimized llqtwebkit
    optimized QtWebKit4
    optimized QtOpenGL4
    optimized QtNetwork4
    optimized QtGui4
    optimized QtCore4
    optimized qtmain
    )
elseif (DARWIN)
    set(WEBKIT_PLUGIN_LIBRARIES
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libllqtwebkit.dylib
        debug ${ARCH_PREBUILT_DIRS_RELEASE}/libllqtwebkit.dylib
        )
elseif (LINUX)
    set(WEBKIT_PLUGIN_LIBRARIES ${LLQTWEBKIT_LIBRARY} ${QT_LIBRARIES} ${QT_PLUGIN_LIBRARIES})
    set(WEBKIT_PLUGIN_LIBRARIES
        llqtwebkit
#        qico
#        qpng
#        qtiff
#        qsvg
#        QtSvg
        QtWebKit
        QtOpenGL
        QtNetwork
        QtGui
        QtCore
        jscore
#        qgif
#        qjpeg
#        jpeg
        fontconfig
        X11
        Xrender
        GL

#        sqlite3
#        Xi
#        SM
        )
endif (WINDOWS)
