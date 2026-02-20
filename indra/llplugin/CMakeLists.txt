# -*- cmake -*-

add_library (llplugin STATIC)

target_sources(llplugin
    PRIVATE
    llpluginclassmedia.cpp
    llplugininstance.cpp
    llpluginmessage.cpp
    llpluginmessagepipe.cpp
    llpluginprocesschild.cpp
    llpluginprocessparent.cpp
    llpluginsharedmemory.cpp

    PUBLIC
    FILE_SET HEADERS
    FILES
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

target_link_libraries(llplugin PUBLIC llcommon llmath llmessage llxml)
target_include_directories(llplugin INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(llplugin REUSE_FROM llprecompiled)

add_subdirectory(slplugin)

