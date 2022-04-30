# -*- cmake -*-

project(llplugin)

include(00-Common)
include(CURL)
include(LLCommon)
include(LLImage)
include(LLWindow)

set(llplugin_SOURCE_FILES
    llpluginclassmedia.cpp
    llplugininstance.cpp
    llpluginmessage.cpp
    llpluginmessagepipe.cpp
    llpluginprocesschild.cpp
    llpluginprocessparent.cpp
    llpluginsharedmemory.cpp
    )

set(llplugin_HEADER_FILES
    CMakeLists.txt
    llpluginclassmedia.h
    llpluginclassmediaowner.h
    llplugininstance.h
    llpluginmessage.h
    llpluginmessageclasses.h
    llpluginmessagepipe.h
    llpluginprocesschild.h
    llpluginprocessparent.h
    llpluginsharedmemory.h
    )

if(NOT ADDRESS_SIZE EQUAL 32)
  if(WINDOWS)
    ##add_definitions(/FIXED:NO)
  else(WINDOWS) # not windows therefore gcc LINUX and DARWIN
    add_definitions(-fPIC)
  endif(WINDOWS)
endif(NOT ADDRESS_SIZE EQUAL 32)

list(APPEND llplugin_SOURCE_FILES ${llplugin_HEADER_FILES})

add_library (llplugin ${llplugin_SOURCE_FILES})
target_include_directories( llplugin  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries( llplugin llcommon llmath llrender llmessage )
add_subdirectory(slplugin)

