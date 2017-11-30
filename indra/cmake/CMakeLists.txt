# -*- cmake -*-

include(00-Common)

project(cmake)

set(cmake_SOURCE_FILES
    CMakeLists.txt

    00-Common.cmake
    APR.cmake
    Audio.cmake
    BerkeleyDB.cmake
    Boost.cmake
    BuildVersion.cmake
    CEFPlugin.cmake
    CEFPlugin.cmake
    CMakeCopyIfDifferent.cmake
    ConfigurePkgConfig.cmake
    CURL.cmake
    Copy3rdPartyLibs.cmake
    DBusGlib.cmake
    DeploySharedLibs.cmake
    DirectX.cmake
    DragDrop.cmake
    EXPAT.cmake
    FindAPR.cmake
    FindAutobuild.cmake
    FindBerkeleyDB.cmake
    FindFMODEX.cmake
    FindGLH.cmake
    FindGoogleBreakpad.cmake
    FindHUNSPELL.cmake
    FindJsonCpp.cmake
    FindNDOF.cmake
    FindOpenJPEG.cmake
    FindSCP.cmake
    FindURIPARSER.cmake
    FindXmlRpcEpi.cmake
    FindZLIB.cmake
    FMODEX.cmake
    FreeType.cmake
    GLEXT.cmake
    GLH.cmake
    GLOD.cmake
##  GStreamer010Plugin.cmake
    GoogleBreakpad.cmake
    GoogleMock.cmake
    Havok.cmake
    Hunspell.cmake
    JPEG.cmake
    JsonCpp.cmake
    LLAddBuildTest.cmake
    LLAppearance.cmake
    LLAudio.cmake
    LLCharacter.cmake
    LLCommon.cmake
    LLCrashLogger.cmake
    LLImage.cmake
    LLImageJ2COJ.cmake
    LLInventory.cmake
    LLKDU.cmake
    LLLogin.cmake
    LLMath.cmake
    LLMessage.cmake
    LLPhysicsExtensions.cmake
    LLPlugin.cmake
    LLPrimitive.cmake
    LLRender.cmake
    LLSharedLibs.cmake
    LLTestCommand.cmake
    LLUI.cmake
    LLVFS.cmake
    LLWindow.cmake
    LLXML.cmake
    Linking.cmake
    MediaPluginBase.cmake
    NDOF.cmake
    OPENAL.cmake
    OpenGL.cmake
    OpenJPEG.cmake
    OpenSSL.cmake
    PNG.cmake
    PluginAPI.cmake
    Prebuilt.cmake
    PulseAudio.cmake
    Python.cmake
    TemplateCheck.cmake
    Tut.cmake
    UI.cmake
    UnixInstall.cmake
    URIPARSER.cmake
    Variables.cmake
    ViewerMiscLibs.cmake
    VisualLeakDetector.cmake
    LibVLCPlugin.cmake
    XmlRpcEpi.cmake
    ZLIB.cmake
    )

source_group("Shared Rules" FILES ${cmake_SOURCE_FILES})

set(master_SOURCE_FILES
    ../CMakeLists.txt
    )

source_group("Master Rules" FILES ${master_SOURCE_FILES})

set_source_files_properties(${cmake_SOURCE_FILES} ${master_SOURCE_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

add_library(cmake
            cmake_dummy.cpp
            ${cmake_SOURCE_FILES}
            ${master_SOURCE_FILES}
            )
