#!/bin/sh
## Here are some configuration options for Linux Client Alpha Testers.
## These options are for self-assisted troubleshooting during this alpha
## testing phase; you should not usually need to touch them.

## - Avoids using the ESD audio driver.
#export LL_BAD_ESD=x

## - Avoids using the OSS audio driver.
#export LL_BAD_OSS=x

## - Avoids using the ALSA audio driver.
#export LL_BAD_ALSA=x

## - Avoids the optional OpenGL extensions which have proven most problematic
##   on some hardware.  Disabling this option may cause crashes and hangs on
##   some unstable combinations of drivers and hardware.
export LL_GL_BASICEXT=x

## - Avoids *all* optional OpenGL extensions.  This is the safest and least-
##   exciting option.  Enable this if you experience stability issues, and
##   report whether it helps in the Linux Client Alpha Testers forum.
#export LL_GL_NOEXT=x

## - For advanced troubleshooters, this lets you disable specific GL
##   extensions, each of which is represented by a letter a-o.  If you can
##   narrow down a stability problem on your system to just one or two
##   extensions then please post details of your hardware (and drivers) to
##   the Linux Client Alpha Testers forum along with the minimal
##   LL_GL_BLACKLIST which solves your problems.
#export LL_GL_BLACKLIST=abcdefghijklmno

## - Avoids an often-buggy X feature that doesn't really benefit us anyway.
export SDL_VIDEO_X11_DGAMOUSE=0

## Nothing worth editing below this line.
##-------------------------------------------------------------------

RUN_PATH=`dirname "$0" || echo .`
cd "${RUN_PATH}"
LD_LIBRARY_PATH="`pwd`"/lib:"`pwd`"/app_settings/mozilla:"${LD_LIBRARY_PATH}" bin/do-not-directly-run-secondlife-bin `cat gridargs.dat` $@ | cat

echo
echo '*********************************************************'
echo 'This is an ALPHA release of the Second Life linux client.'
echo 'Thank you for testing!'
echo 'You can visit the Linux Client Alpha Testers forum at:'
echo 'http://forums.secondlife.com/forumdisplay.php?forumid=263'
echo 'Please see README-linux.txt before reporting problems.'
echo
