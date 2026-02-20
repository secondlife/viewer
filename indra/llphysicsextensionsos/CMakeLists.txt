# -*- cmake -*-

add_library(llphysicsextensionsos STATIC)

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

target_precompile_headers(llphysicsextensionsos REUSE_FROM llprecompiled)

if(WINDOWS)
  target_compile_options(llphysicsextensionsos PRIVATE /bigobj)
endif()

# Add tests

