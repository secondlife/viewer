# -*- cmake -*-

project(llrender)

include(00-Common)
include(OpenGL)
include(FreeType)
include(LLCommon)
include(LLImage)
include(LLWindow)
include(DragDrop)
include(UI)
include(ViewerMiscLibs)
include(GLM)

set(llrender_SOURCE_FILES
    llcubemap.cpp
    llcubemaparray.cpp
    llfontbitmapcache.cpp
    llfontfreetype.cpp
    llfontfreetypesvg.cpp
    llfontgl.cpp
    llfontvertexbuffer.cpp
    llfontregistry.cpp
    llgl.cpp
    llglslshader.cpp
    llgltexture.cpp
    llimagegl.cpp
    llrender.cpp
    llrender2dutils.cpp
    llrendernavprim.cpp
    llrendersphere.cpp
    llrendertarget.cpp
    llshadermgr.cpp
    lltexture.cpp
    lltexturemanagerbridge.cpp
    lluiimage.cpp
    llvertexbuffer.cpp
    llglcommonfunc.cpp
    )

set(llrender_HEADER_FILES
    CMakeLists.txt

    llcubemap.h
    llcubemaparray.h
    llfontgl.h
    llfontvertexbuffer.h
    llfontfreetype.h
    llfontfreetypesvg.h
    llfontbitmapcache.h
    llfontregistry.h
    llgl.h
    llglheaders.h
    llglslshader.h
    llglstates.h
    llgltexture.h
    llgltypes.h
    llimagegl.h
    llrender.h
    llrender2dutils.h
    llrendernavprim.h
    llrendersphere.h
    llshadermgr.h
    lltexture.h
    lltexturemanagerbridge.h
    lluiimage.h
    lluiimage.inl
    llvertexbuffer.h
    llglcommonfunc.h
    )

# Former llwindow sources (merged — GL context and window are inseparable)
list(APPEND llrender_SOURCE_FILES
    llcursortypes.cpp
    llkeyboard.cpp
    llkeyboardheadless.cpp
    llwindowheadless.cpp
    llwindowcallbacks.cpp
    llwindow.cpp
    llmousehandler.cpp
    )

list(APPEND llrender_HEADER_FILES
    llcursortypes.h
    llkeyboard.h
    llkeyboardheadless.h
    llwindowheadless.h
    llwindowcallbacks.h
    llwindow.h
    llpreeditor.h
    llmousehandler.h
    )

# Platform-specific window/keyboard sources
if (LINUX)
    list(APPEND llrender_SOURCE_FILES
        llkeyboardsdl.cpp
        llwindowsdl.cpp
        )
    list(APPEND llrender_HEADER_FILES
        llkeyboardsdl.h
        llwindowsdl.h
        )
endif (LINUX)

if (DARWIN)
    list(APPEND llrender_SOURCE_FILES
        llkeyboardmacosx.cpp
        llwindowmacosx.cpp
        llwindowmacosx-objc.mm
        llopenglview-objc.mm
        )
    list(APPEND llrender_HEADER_FILES
        llkeyboardmacosx.h
        llwindowmacosx.h
        llwindowmacosx-objc.h
        llopenglview-objc.h
        llappdelegate-objc.h
        )
    set_source_files_properties(
        llkeyboardmacosx.cpp
        llwindowmacosx.cpp
        PROPERTIES
        COMPILE_FLAGS "-fpascal-strings"
        )
endif (DARWIN)

if (WINDOWS)
    list(APPEND llrender_SOURCE_FILES
        llwindowwin32.cpp
        lldxhardware.cpp
        llkeyboardwin32.cpp
        lldragdropwin32.cpp
        )
    list(APPEND llrender_HEADER_FILES
        llwindowwin32.h
        lldxhardware.h
        llkeyboardwin32.h
        lldragdropwin32.h
        )
endif (WINDOWS)

if (SOLARIS)
    list(APPEND llrender_SOURCE_FILES
        llwindowsolaris.cpp
        )
    list(APPEND llrender_HEADER_FILES
        llwindowsolaris.h
        )
endif (SOLARIS)

list(APPEND llrender_SOURCE_FILES ${llrender_HEADER_FILES})

if (BUILD_HEADLESS)
  add_library (llrenderheadless
    ${llrender_SOURCE_FILES}
    )

  set_property(TARGET llrenderheadless
    PROPERTY COMPILE_DEFINITIONS LL_MESA=1 LL_MESA_HEADLESS=1
      )

  target_link_libraries(llrenderheadless
          llcommon
          llimage
          llmath
          llrender
          llxml
          llfilesystem
          )
endif (BUILD_HEADLESS)

add_library (llrender ${llrender_SOURCE_FILES})
target_include_directories( llrender  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(llrender
        llcommon
        llimage
        llmath
        llfilesystem
        llxml
        ll::freetype
        ll::glm
        ll::glext
        ll::uilibraries
        ll::SDL
        OpenGL::GL
        OpenGL::GLU
        )

if (WINDOWS)
    target_link_libraries(llrender
        comdlg32
        ole32
        dxgi
        d3d9
        )
endif (WINDOWS)

if (DARWIN)
    target_link_libraries(llrender ${CARBON_LIBRARY})
endif (DARWIN)

if (SDL_FOUND)
    set_property(TARGET llrender
        PROPERTY COMPILE_DEFINITIONS LL_SDL=1
        )
endif (SDL_FOUND)
