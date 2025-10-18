# -*- cmake -*-

project(llconvexdecomposition)

include(00-Common)
include(LLCommon)
include(LLMath)
include(VHACD)

set(llconvexdecomposition_SOURCE_FILES
        llconvexdecomposition.cpp
        llconvexdecompositionvhacd.cpp
        )

set(llconvexdecomposition_HEADER_FILES
        CMakeLists.txt
        llconvexdecomposition.h
        llconvexdecompositionvhacd.h
        )

set_source_files_properties(${llconvexdecomposition_HEADER_FILES}
        PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llconvexdecomposition_SOURCE_FILES ${llconvexdecomposition_HEADER_FILES})

add_library (llconvexdecomposition ${llconvexdecomposition_SOURCE_FILES})
target_include_directories(llconvexdecomposition INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(llconvexdecomposition
        llcommon
        llmath
        ll::vhacd)

if(WINDOWS)
  target_compile_options(llconvexdecomposition PRIVATE /bigobj)
endif()

# Add tests

