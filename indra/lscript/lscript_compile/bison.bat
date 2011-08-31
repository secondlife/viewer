@REM Run bison under Windows.  This script is needed so that bison can
@REM find m4, even if neither program is present in PATH.

@set bison=%1
set M4PATH=%2
set M4=
@set output=%3
@set input=%4

set PATH=%M4PATH%;%PATH%
%bison% -d -o %output% %input%
