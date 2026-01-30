# -*- cmake -*-

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
        Collada-Dom.cmake
        Copy3rdPartyLibs.cmake
        CURL.cmake
        Discord.cmake
        EXPAT.cmake
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
        LLKDU.cmake
        LLPhysicsExtensions.cmake
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
        ViewerMiscLibs.cmake
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
