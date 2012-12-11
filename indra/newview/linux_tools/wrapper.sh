#!/bin/bash

## Here are some configuration options for Linux Client Testers.
## These options are for self-assisted troubleshooting during this beta
## testing phase; you should not usually need to touch them.

## - Avoids using any OpenAL audio driver.
#export LL_BAD_OPENAL_DRIVER=x
## - Avoids using any FMOD audio driver.
#export LL_BAD_FMOD_DRIVER=x

## - Avoids using the FMOD ESD audio driver.
#export LL_BAD_FMOD_ESD=x
## - Avoids using the FMOD OSS audio driver.
#export LL_BAD_FMOD_OSS=x
## - Avoids using the FMOD ALSA audio driver.
#export LL_BAD_FMOD_ALSA=x

## - Avoids the optional OpenGL extensions which have proven most problematic
##   on some hardware.  Disabling this option may cause BETTER PERFORMANCE but
##   may also cause CRASHES and hangs on some unstable combinations of drivers
##   and hardware.
## NOTE: This is now disabled by default.
#export LL_GL_BASICEXT=x

## - Avoids *all* optional OpenGL extensions.  This is the safest and least-
##   exciting option.  Enable this if you experience stability issues, and
##   report whether it helps in the Linux Client Testers forum.
#export LL_GL_NOEXT=x

## - For advanced troubleshooters, this lets you disable specific GL
##   extensions, each of which is represented by a letter a-o.  If you can
##   narrow down a stability problem on your system to just one or two
##   extensions then please post details of your hardware (and drivers) to
##   the Linux Client Testers forum along with the minimal
##   LL_GL_BLACKLIST which solves your problems.
#export LL_GL_BLACKLIST=abcdefghijklmno

## - Some ATI/Radeon users report random X server crashes when the mouse
##   cursor changes shape.  If you suspect that you are a victim of this
##   driver bug, try enabling this option and report whether it helps:
#export LL_ATI_MOUSE_CURSOR_BUG=x

if [ "`uname -m`" = "x86_64" ]; then
    echo '64-bit Linux detected.'
fi

## Everything below this line is just for advanced troubleshooters.
##-------------------------------------------------------------------

## - For advanced debugging cases, you can run the viewer under the
##   control of another program, such as strace, gdb, or valgrind.  If
##   you're building your own viewer, bear in mind that the executable
##   in the bin directory will be stripped: you should replace it with
##   an unstripped binary before you run.
#export LL_WRAPPER='gdb --args'
#export LL_WRAPPER='valgrind --smc-check=all --error-limit=no --log-file=secondlife.vg --leak-check=full --suppressions=/usr/lib/valgrind/glibc-2.5.supp --suppressions=secondlife-i686.supp'

## - Avoids an often-buggy X feature that doesn't really benefit us anyway.
export SDL_VIDEO_X11_DGAMOUSE=0

## - Works around a problem with misconfigured 64-bit systems not finding GL
export LIBGL_DRIVERS_PATH="${LIBGL_DRIVERS_PATH}":/usr/lib64/dri:/usr/lib32/dri:/usr/lib/dri

## - The 'scim' GTK IM module widely crashes the viewer.  Avoid it.
if [ "$GTK_IM_MODULE" = "scim" ]; then
    export GTK_IM_MODULE=xim
fi

## - Automatically work around the ATI mouse cursor crash bug:
## (this workaround is disabled as most fglrx users do not see the bug)
#if lsmod | grep fglrx &>/dev/null ; then
#	export LL_ATI_MOUSE_CURSOR_BUG=x
#fi


## Nothing worth editing below this line.
##-------------------------------------------------------------------

SCRIPTSRC=`readlink -f "$0" || echo "$0"`
RUN_PATH=`dirname "${SCRIPTSRC}" || echo .`
echo "Running from ${RUN_PATH}"
cd "${RUN_PATH}"

# Re-register the secondlife:// protocol handler every launch, for now.
./etc/register_secondlifeprotocol.sh

# Re-register the application with the desktop system every launch, for now.
./etc/refresh_desktop_app_entry.sh

## Before we mess with LD_LIBRARY_PATH, save the old one to restore for
##  subprocesses that care.
export SAVED_LD_LIBRARY_PATH="${LD_LIBRARY_PATH}"

# if [ -n "$LL_TCMALLOC" ]; then
#    tcmalloc_libs='/usr/lib/libtcmalloc.so.0 /usr/lib/libstacktrace.so.0 /lib/libpthread.so.0'
#    all=1
#    for f in $tcmalloc_libs; do
#        if [ ! -f $f ]; then
#	    all=0
#	fi
#    done
#    if [ $all != 1 ]; then
#        echo 'Cannot use tcmalloc libraries: components missing' 1>&2
#    else
#	export LD_PRELOAD=$(echo $tcmalloc_libs | tr ' ' :)
#	if [ -z "$HEAPCHECK" -a -z "$HEAPPROFILE" ]; then
#	    export HEAPCHECK=${HEAPCHECK:-normal}
#	fi
#    fi
#fi

export LD_LIBRARY_PATH="$PWD/lib:${LD_LIBRARY_PATH}"

# Have to deal specially with gridargs.dat; typical contents look like:
# --channel "Second Life Developer"  --settings settings_developer.xml
# Simply embedding $(<etc/gridargs.dat) into a command line treats each of
# Second, Life and Developer as separate args -- no good. We need bash to
# process quotes using eval.
# First, check if we have been instructed to skip reading in gridargs.dat:
skip_gridargs=false
argnum=0
for ARG in "$@"; do
    if [ "--skip-gridargs" == "$ARG" ]; then
        skip_gridargs=true
    else
        ARGS[$argnum]="$ARG"
        argnum=$(($argnum+1))
    fi
done

# Second, read it without scanning, then scan that string. Break quoted words
# into a bash array. Note that if gridargs.dat is empty, or contains only
# whitespace, the resulting gridargs array will be empty -- zero entries --
# therefore "${gridargs[@]}" entirely vanishes from the command line below,
# just as we want.
if ! $skip_gridargs ; then
    eval gridargs=("$(<etc/gridargs.dat)")
fi

# Run the program.
# Don't quote $LL_WRAPPER because, if empty, it should simply vanish from the
# command line. But DO quote "$@": preserve separate args as individually
# quoted. Similar remarks about the contents of gridargs.
$LL_WRAPPER bin/do-not-directly-run-secondlife-bin "${gridargs[@]}" "${ARGS[@]}"
LL_RUN_ERR=$?

# Handle any resulting errors
if [ $LL_RUN_ERR -ne 0 ]; then
	# generic error running the binary
	echo '*** Bad shutdown ($LL_RUN_ERR). ***'
	if [ "$(uname -m)" = "x86_64" ]; then
		echo
		cat << EOFMARKER
You are running the Second Life Viewer on a x86_64 platform.  The
most common problems when launching the Viewer (particularly
'bin/do-not-directly-run-secondlife-bin: not found' and 'error while
loading shared libraries') may be solved by installing your Linux
distribution's 32-bit compatibility packages.
For example, on Ubuntu and other Debian-based Linuxes you might run:
$ sudo apt-get install ia32-libs ia32-libs-gtk ia32-libs-kde ia32-libs-sdl
EOFMARKER
	fi
fi

echo
echo '*******************************************************'
echo 'This is a BETA release of the Second Life linux client.'
echo 'Thank you for testing!'
echo 'Please see README-linux.txt before reporting problems.'
echo
