@echo off
REM Generate compile_commands.json for clang-tidy using Ninja generator
REM Must be run from a VS Developer Command Prompt or will set up vcvars itself

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

REM Set LL_BUILD (from build-variables/variables, resolved for Windows RelWithDebInfo)
set "LL_BUILD=/MD /Od /Ob0 /std:c++20 /permissive- /Zc:wchar_t /Zi /GR /DEBUG /DLL_RELEASE=1 /DLL_RELEASE_WITH_DEBUG_INFO=1 /DNDEBUG /D_SECURE_STL=0 /D_HAS_ITERATOR_DEBUGGING=0 /DWIN32 /D_WINDOWS /DLL_WINDOWS=1 /DUNICODE /D_UNICODE /DWINVER=0x0602 /D_WIN32_WINNT=0x0602 /DLL_OS_DRAGDROP_ENABLED=1 /DLIB_NDOF=1"

REM Autobuild variables
set "AUTOBUILD_VARIABLES_FILE=C:\Users\frog_king\Desktop\sl_dev\build-variables\variables"
set "AUTOBUILD_ADDRSIZE=64"
set "AUTOBUILD_VSVER=170"

REM Clean previous attempt
if exist build-clang-tidy rd /s /q build-clang-tidy

echo.
echo Generating compile_commands.json with Ninja generator...
echo.
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja -B build-clang-tidy -S indra ^
  -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
  -DADDRESS_SIZE:STRING=64 ^
  -DINSTALL_PROPRIETARY=FALSE ^
  -DUSE_OPENAL:BOOL=ON ^
  -DHAVOK:BOOL=OFF ^
  -DLL_TESTS:BOOL=OFF

if exist build-clang-tidy\compile_commands.json (
    echo.
    echo SUCCESS: compile_commands.json generated
    echo Location: %cd%\build-clang-tidy\compile_commands.json
) else (
    echo.
    echo FAILED: compile_commands.json was not generated
    echo Check cmake output above for errors
)
pause
