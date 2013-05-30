@REM Run bison under Windows.  This script is needed so that bison can
@REM find m4, even if neither program is present in PATH.

@set bison=%1
shift
set M4PATH=%1
shift
set M4=

set PATH=%M4PATH%;%PATH%
@REM %* does not work with shift...
%bison% %1 %2 %3 %4 %5 %6 %7 %8 %9
