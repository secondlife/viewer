# -*- cmake -*-

include(OpenGL)

add_library( ll::pluginlibraries INTERFACE IMPORTED )

if (WINDOWS)
  target_link_libraries( ll::pluginlibraries INTERFACE
      wsock32
      ws2_32
      psapi
      netapi32
      advapi32
      user32
      )
endif (WINDOWS)

target_link_libraries( ll::pluginlibraries INTERFACE OpenGL::GL)

target_include_directories( ll::pluginlibraries INTERFACE ${CMAKE_SOURCE_DIR}/llimage ${CMAKE_SOURCE_DIR}/llrender)
