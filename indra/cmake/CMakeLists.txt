# -*- cmake -*-

include(00-Common)

project(cmake)

set(cmake_SOURCE_FILES
        CMakeLists.txt
        00-Common.cmake
        APR.cmake
        Audio.cmake
        Boost.cmake
        bugsplat.cmake
        BuildPackagesInfo.cmake
        BuildVersion.cmake
        CEFPlugin.cmake
        CMakeCopyIfDifferent.cmake
        Copy3rdPartyLibs.cmake
        CubemapToEquirectangularJS.cmake
        CURL.cmake
        DeploySharedLibs.cmake
        DragDrop.cmake
        EXPAT.cmake
        FindAutobuild.cmake
        FindPipeWire.cmake
        FreeType.cmake
        GLEXT.cmake
        GLH.cmake
        GLIB.cmake
        GLM.cmake
        GStreamer10Plugin.cmake
        Havok.cmake
        Hunspell.cmake
        JPEG.cmake
        JPEGEncoderBasic.cmake
        LibVLCPlugin.cmake
        Linking.cmake
        LLAddBuildTest.cmake
        LLAppearance.cmake
        LLAppearanceUtility.cmake
        LLAudio.cmake
        LLCommon.cmake
        LLCoreHttp.cmake
        LLImage.cmake
        LLKDU.cmake
        LLMath.cmake
        LLPhysicsExtensions.cmake
        LLPrimitive.cmake
        LLSharedLibs.cmake
        LLTestCommand.cmake
        LLWindow.cmake
        Lualibs.cmake
        Meshoptimizer.cmake
        Mikktspace.cmake
        NDOF.cmake
        NFDE.cmake
        NGHTTP2.cmake
        NVAPI.cmake
        OPENAL.cmake
        OpenGL.cmake
        OpenJPEG.cmake
        OpenSSL.cmake
        OpenXR.cmake
        PNG.cmake
        PluginAPI.cmake
        Prebuilt.cmake
        Python.cmake
        SDL2.cmake
        SSE2NEON.cmake
        TemplateCheck.cmake
        ThreeJS.cmake
        TinyEXR.cmake
        TinyGLTF.cmake
        Tracy.cmake
        Tut.cmake
        UI.cmake
        UnixInstall.cmake
        Variables.cmake
        ViewerManager.cmake
        ViewerMiscLibs.cmake
        VisualLeakDetector.cmake
        VulkanGltf.cmake
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
