# -*- cmake -*-

project(llphysicsextensionsos)

include(00-Common)
include(LLCommon)
include(LLMath)
include(VHACD)

set(llphysicsextensionsos_SOURCE_FILES
        llconvexdecomposition.cpp
        llconvexdecompositionvhacd.cpp
        llpathinglib.cpp
        LLPathingLibStubImpl.cpp
        llphysicsextensions.cpp
        LLPhysicsExtensionsStubImpl.cpp
        )

set(llphysicsextensionsos_HEADER_FILES
        CMakeLists.txt
        llconvexdecomposition.h
        llconvexdecompositionvhacd.h
        llpathinglib.h
        LLPathingLibStubImpl.h
        llphysicsextensions.h
        LLPhysicsExtensionsStubImpl.h
        )

set_source_files_properties(${llphysicsextensionsos_HEADER_FILES}
        PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llphysicsextensionsos_SOURCE_FILES ${llphysicsextensionsos_HEADER_FILES})

add_library (llphysicsextensionsos ${llphysicsextensionsos_SOURCE_FILES})
target_include_directories(llphysicsextensionsos INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(llphysicsextensionsos
        llcommon
        llmath
        ll::vhacd)

if(WINDOWS)
  target_compile_options(llphysicsextensionsos PRIVATE /bigobj)
endif()

if (USE_PRECOMPILED_HEADERS)
  target_precompile_headers(llphysicsextensionsos REUSE_FROM llprecompiled)
endif ()

# Add tests

