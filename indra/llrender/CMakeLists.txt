# -*- cmake -*-

project(llrender)

include(00-Common)
include(FindOpenGL)
include(FreeType)
include(LLCommon)
include(LLImage)
include(LLMath)
include(LLRender)
include(LLVFS)
include(LLWindow)
include(LLXML)
include(LLVFS)

include_directories(
    ${FREETYPE_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    )

set(llrender_SOURCE_FILES
    llcubemap.cpp
    llfontfreetype.cpp
    llfontgl.cpp
    llfontbitmapcache.cpp
    llfontregistry.cpp
    llgldbg.cpp
    llglslshader.cpp
    llimagegl.cpp
    llpostprocess.cpp
    llrendernavprim.cpp
    llrendersphere.cpp
    llshadermgr.cpp
    lltexture.cpp
    llvertexbuffer.cpp
    )
    
set(llrender_HEADER_FILES
    CMakeLists.txt

    llcubemap.h
    llfontgl.h
    llfontfreetype.h
    llfontbitmapcache.h
    llfontregistry.h
    llgl.h
    llgldbg.h
    llglheaders.h
    llglslshader.h
    llglstates.h
    llgltypes.h
    llimagegl.h
    llpostprocess.h
    llrender.h
    llrendernavprim.h
    llrendersphere.h
    llshadermgr.h
    lltexture.h
    llvertexbuffer.h
    )

set_source_files_properties(${llrender_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llrender_SOURCE_FILES ${llrender_HEADER_FILES})

if (SERVER AND NOT WINDOWS AND NOT DARWIN)
  copy_server_sources(
      llgl
      llrender
      )


  set_source_files_properties(
    ${server_SOURCE_FILES}
    PROPERTIES
    COMPILE_FLAGS "-DLL_MESA=1 -DLL_MESA_HEADLESS=1"
    )
  add_library (llrenderheadless
    ${llrender_SOURCE_FILES}
    ${server_SOURCE_FILES}
    )
else (SERVER AND NOT WINDOWS AND NOT DARWIN)
  list(APPEND llrender_SOURCE_FILES
      llgl.cpp
      llrender.cpp
      llrendertarget.cpp
      )
endif (SERVER AND NOT WINDOWS AND NOT DARWIN)
add_library (llrender ${llrender_SOURCE_FILES})
# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
target_link_libraries(llrender 
    llimage 
    ${FREETYPE_LIBRARIES}
    ${OPENGL_LIBRARIES})
