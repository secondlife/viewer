# -*- cmake -*-

add_library(llmeshoptimizer)

target_sources(llmeshoptimizer
    PRIVATE
    llmeshoptimizer.cpp
    PUBLIC
    FILE_SET HEADERS
    FILES
    CMakeLists.txt
    llmeshoptimizer.h
    )
target_include_directories( llmeshoptimizer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(llmeshoptimizer
        PUBLIC
        llmath
        llcommon
        ll::meshoptimizer
        )

if (USE_PRECOMPILED_HEADERS)
  target_precompile_headers(llmeshoptimizer REUSE_FROM llprecompiled)
endif ()

# Add tests

