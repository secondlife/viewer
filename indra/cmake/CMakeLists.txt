# -*- cmake -*-

include(00-Common)

project(cmake)

set(cmake_SOURCE_FILES
        CMakeLists.txt
        00-Common.cmake
        APR.cmake
        Boost.cmake
        bugsplat.cmake
        BuildPackagesInfo.cmake
        BuildVersion.cmake
        CEFPlugin.cmake
        CMakeCopyIfDifferent.cmake
        Copy3rdPartyLibs.cmake
        CURL.cmake
        Discord.cmake
        DragDrop.cmake
        EXPAT.cmake
        FindAutobuild.cmake
        FindPipeWire.cmake
        FreeType.cmake
        GLEXT.cmake
        GLIB.cmake
        GLM.cmake
        GStreamer10Plugin.cmake
        Havok.cmake
        Hunspell.cmake
        JPEG.cmake
        LLAddBuildTest.cmake
        LLCA.cmake
        LLKDU.cmake
        LLPhysicsExtensions.cmake
        LLPrimitive.cmake
        LLTestCommand.cmake
        Linking.cmake
        Meshoptimizer.cmake
        NDOF.cmake
        NVAPI.cmake
        OpenAL.cmake
        OpenGL.cmake
        OpenJPEG.cmake
        OpenSSL.cmake
        PNG.cmake
        PluginAPI.cmake
        Prebuilt.cmake
        PulseAudio.cmake
        Python.cmake
        SDL3.cmake
        SSE2NEON.cmake
        TemplateCheck.cmake
        TinyEXR.cmake
        TinyGLTF.cmake
        Tracy.cmake
        Tut.cmake
        UI.cmake
        UnixInstall.cmake
        Variables.cmake
        VHACD.cmake
        ViewerManager.cmake
        ViewerMiscLibs.cmake
        VisualLeakDetector.cmake
        Vorbis.cmake
        LibVLCPlugin.cmake
        WebRTC.cmake
        xxHash.cmake
        ZLIBNG.cmake
        )

source_group("Shared Rules" FILES ${cmake_SOURCE_FILES})

set(master_SOURCE_FILES
        ../CMakeLists.txt
        )

source_group("Master Rules" FILES ${master_SOURCE_FILES})

add_library(cmake
        cmake_dummy.cpp
        ${cmake_SOURCE_FILES}
        ${master_SOURCE_FILES}
        )
