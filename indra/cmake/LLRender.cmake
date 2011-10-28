# -*- cmake -*-

include(FreeType)
include(GLH)

set(LLRENDER_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llrender
    ${GLH_INCLUDE_DIR}
    )

if (SERVER AND LINUX)
  set(LLRENDER_LIBRARIES
      llrenderheadless
      )
else (SERVER AND LINUX)
set(LLRENDER_LIBRARIES
    llrender
    )
endif (SERVER AND LINUX)

# mapserver requires certain files to be copied so LL_MESA_HEADLESS can be set
# differently for different object files.
macro (copy_server_sources )
  foreach (PREFIX ${ARGV})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PREFIX}_server.cpp
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${PREFIX}.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/${PREFIX}_server.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PREFIX}.cpp
        )
    list(APPEND server_SOURCE_FILES ${PREFIX}_server.cpp)
  endforeach (PREFIX ${_copied_SOURCES})
endmacro (copy_server_sources _copied_SOURCES)
