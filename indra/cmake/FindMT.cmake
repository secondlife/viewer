#Find the windows manifest tool.

FIND_PROGRAM(HAVE_MANIFEST_TOOL NAMES mt
                 PATHS
                 "$ENV{PROGRAMFILES}/Microsoft Visual Studio 8/VC/bin"
                 "$ENV{PROGRAMFILES}/Microsoft Visual Studio 8/Common7/Tools/Bin"
                 "$ENV{PROGRAMFILES}/Microsoft Visual Studio 8/SDK/v2.0/Bin")
IF(HAVE_MANIFEST_TOOL)
    MESSAGE(STATUS "Found Mainfest Tool. Embedding custom manifests.")
ELSE(HAVE_MANIFEST_TOOL)
    MESSAGE(FATAL_ERROR "Manifest tool, mt.exe, can't be found.")
ENDIF(HAVE_MANIFEST_TOOL)

STRING(REPLACE "/MANIFEST" "/MANIFEST:NO" CMAKE_EXE_LINKER_FLAGS 
      ${CMAKE_EXE_LINKER_FLAGS})
