# -*- cmake -*-

add_library(llphysicsextensionsos)

target_sources(llphysicsextensionsos
        PRIVATE
        llconvexdecomposition.cpp
        llconvexdecompositionvhacd.cpp
        llpathinglib.cpp
        LLPathingLibStubImpl.cpp
        llphysicsextensions.cpp
        LLPhysicsExtensionsStubImpl.cpp
        )

target_sources(llphysicsextensionsos
        PUBLIC
        FILE_SET HEADERS
        FILES
        CMakeLists.txt
        llconvexdecomposition.h
        llconvexdecompositionvhacd.h
        llpathinglib.h
        LLPathingLibStubImpl.h
        llphysicsextensions.h
        LLPhysicsExtensionsStubImpl.h
        )

target_include_directories(llphysicsextensionsos INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(llphysicsextensionsos
        PUBLIC
        llmath
        llcommon
        ll::vhacd)

if(WINDOWS)
  target_compile_options(llphysicsextensionsos PRIVATE /bigobj)
endif()

if (USE_PRECOMPILED_HEADERS)
  target_precompile_headers(llphysicsextensionsos REUSE_FROM llprecompiled)
endif ()

# Add tests

