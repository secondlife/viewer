# -*- cmake -*-

set(MONO_PREBUILT_LIBRARIES_DIR ${LIBS_PREBUILT_DIR}/mono/1.0)

set(MONO_PREBUILT_LIBRARIES
     Iesi.Collections.dll
     Iesi.Collections.pdb
     Mono.CompilerServices.SymbolWriter.dll
     Mono.PEToolkit.dll
     Mono.PEToolkit.pdb
     Mono.Security.dll
     PEAPI.dll
     RAIL.dll
     RAIL.pdb
  )
  
  set(MONO_CORE_LIBRARIES
    System.dll
    System.Xml.dll
    mscorlib.dll)
    
if(WINDOWS)
    set(MONO_DEPENDENCIES
        DomainCreator
        DomainRegister
        LslLibrary
        LslUserScript
        Script
        ScriptTypes
        TestFormat
        UserScript
        UThread
        UThreadInjector
        )
else(WINDOWS)
    set(MONO_DEPENDENCIES
        DomainCreator_POST_BUILD
        DomainRegister_POST_BUILD
        LslLibrary_POST_BUILD
        LslUserScript_POST_BUILD
        Script_POST_BUILD
        ScriptTypes_POST_BUILD
        TestFormat_POST_BUILD
        UserScript_POST_BUILD
        UThread_POST_BUILD
        UThreadInjector_POST_BUILD
        )
endif(WINDOWS)
