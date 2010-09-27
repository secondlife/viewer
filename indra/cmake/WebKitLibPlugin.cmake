# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (STANDALONE)
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
    set(WEBKIT_PLUGIN_LIBRARIES
        llqtwebkit

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

        jpeg
        fontconfig
        X11
        Xrender
        GL

#        sqlite3
#        Xi
#        SM
        )
endif (WINDOWS)
