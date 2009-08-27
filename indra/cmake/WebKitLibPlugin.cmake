# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (STANDALONE)
    set(WEBKITLIBPLUGIN OFF CACHE BOOL
        "WEBKITLIBPLUGIN support for the llplugin/llmedia test apps.")
else (STANDALONE)
    use_prebuilt_binary(webkitlibplugin)
    set(WEBKITLIBPLUGIN ON CACHE BOOL
        "WEBKITLIBPLUGIN support for the llplugin/llmedia test apps.")
endif (STANDALONE)

if (WINDOWS)
    set(WEBKIT_PLUGIN_LIBRARIES 
    debug llwebkitlibd
    debug QtWebKitd4
    debug QtOpenGLd4
    debug QtNetworkd4
    debug QtGuid4
    debug QtCored4
    debug qtmaind
    optimized llwebkitlib
    optimized QtWebKit4
    optimized QtOpenGL4
    optimized QtNetwork4
    optimized QtGui4
    optimized QtCore4
    optimized qtmain
    )
elseif (DARWIN)
    set(WEBKIT_PLUGIN_LIBRARIES
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libllwebkitlib.dylib
        debug ${ARCH_PREBUILT_DIRS_RELEASE}/libllwebkitlib.dylib
        )
elseif (LINUX)
    set(WEBKIT_PLUGIN_LIBRARIES
        llwebkitlib

        qgif
#        qico
        qjpeg
#        qpng
#        qtiff
#        qsvg

#        QtSvg
        QtWebKit
        QtOpenGL
        QtNetwork
        QtGui
        QtCore

        fontconfig
        X11
        GL

#        sqlite3
#        Xi
#        SM
        )
endif (WINDOWS)
