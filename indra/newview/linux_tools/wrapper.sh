#!/bin/bash

## Here are some configuration options for Linux Client Users.

## - Avoids using any OpenAL audio driver.
#export LL_BAD_OPENAL_DRIVER=x

## GL Driver Options
export mesa_glthread=true

## Everything below this line is just for advanced troubleshooters.
##-------------------------------------------------------------------

## - For advanced debugging cases, you can run the viewer under the
##   control of another program, such as strace, gdb, or valgrind.  If
##   you're building your own viewer, bear in mind that the executable
##   in the bin directory will be stripped: you should replace it with
##   an unstripped binary before you run.
#export LL_WRAPPER='gdb --args'
#export LL_WRAPPER='valgrind --smc-check=all --error-limit=no --log-file=secondlife.vg --leak-check=full --suppressions=/usr/lib/valgrind/glibc-2.5.supp --suppressions=secondlife-i686.supp'
#export ASAN_OPTIONS="halt_on_error=0 detect_leaks=1 symbolize=1"
#export UBSAN_OPTIONS="print_stacktrace=1 print_summary=1 halt_on_error=0"

## Nothing worth editing below this line.
##-------------------------------------------------------------------

SCRIPTSRC=`readlink -f "$0" || echo "$0"`
RUN_PATH=`dirname "${SCRIPTSRC}" || echo .`
echo "Running from ${RUN_PATH}"
cd "${RUN_PATH}"

# libcef.so (used for in-viewer web media -- login page, prim/parcel media)
# depends on NSS/NSPR at runtime but does not bundle them; this matches
# CEF's own upstream distribution convention, not a gap in this package.
# Most desktop Linux installs already have them (Firefox depends on the
# same libraries), but a minimal or server install may not, and the
# resulting failure is silent -- media just never appears, no obvious
# error. Warn, don't block -- everything else still runs fine without media.
if command -v ldconfig >/dev/null 2>&1; then
    MISSING_MEDIA_LIBS=""
    ldconfig -p | grep -q 'libnss3\.so'  || MISSING_MEDIA_LIBS="${MISSING_MEDIA_LIBS}libnss3 "
    ldconfig -p | grep -q 'libnspr4\.so' || MISSING_MEDIA_LIBS="${MISSING_MEDIA_LIBS}libnspr4 "
    if [ -n "$MISSING_MEDIA_LIBS" ]; then
        echo "*** Missing system libraries needed for in-viewer web media: ${MISSING_MEDIA_LIBS}"
        echo "*** Install with, e.g.: sudo apt install libnss3 libnspr4   (Debian/Ubuntu)"
        echo "***                 or: sudo dnf install nss nspr          (Fedora)"
        echo "*** Without them, web media (login page, prim/parcel media) will not appear."
    fi

    # Unlike libcef.so above, libvlc is NOT linked into this binary at all any
    # more -- since Phase 2 of the licensing split (2026-10-01),
    # LLStreamingAudio_LibVLC (parcel/streaming audio) is an IPC client of the
    # separate SLVlcProducer process, the same way RTSP/RTMP-style prim media
    # already was since the 2026-09-30 producer split. Unlike Windows/macOS,
    # which vendor their own copy, Linux links against the system's own
    # installed libvlc (see LibVLCPlugin.cmake), needed only by
    # SLVlcProducer.exe. If it's missing, this main binary starts and runs
    # fine regardless -- only SLVlcProducer fails to launch, silently losing
    # parcel audio and LibVLC-backed prim media -- so warn clearly before
    # that happens, rather than leaving the user to notice media just never
    # works with no obvious error.
    MISSING_VLC_LIBS=""
    ldconfig -p | grep -q 'libvlc\.so\.5'     || MISSING_VLC_LIBS="${MISSING_VLC_LIBS}libvlc5 "
    ldconfig -p | grep -q 'libvlccore\.so\.9' || MISSING_VLC_LIBS="${MISSING_VLC_LIBS}libvlccore9 "
    if [ -n "$MISSING_VLC_LIBS" ]; then
        echo "*** Missing system libraries needed for parcel audio and LibVLC-backed prim media: ${MISSING_VLC_LIBS}"
        echo "*** Install with, e.g.: sudo apt install libvlc5 libvlccore9   (Debian/Ubuntu)"
        echo "***                 or: sudo dnf install vlc-libs               (Fedora)"
        echo "*** Without them, those two features will silently not work; everything else is unaffected."
    fi
fi

# Re-register the secondlife:// protocol handler every launch, for now.
./etc/register_secondlifeprotocol.sh

# Re-register the application with the desktop system every launch, for now.
./etc/refresh_desktop_app_entry.sh

## Before we mess with LD_LIBRARY_PATH, save the old one to restore for
##  subprocesses that care.
export SAVED_LD_LIBRARY_PATH="${LD_LIBRARY_PATH}"

# Add our library directory
export LD_LIBRARY_PATH="$PWD/lib:${LD_LIBRARY_PATH}"

# Copy "$@" to ARGS array specifically to delete the --skip-gridargs switch.
# The gridargs.dat file is no more, but we still want to avoid breaking
# scripts that invoke this one with --skip-gridargs.
ARGS=()
for ARG in "$@"; do
    if [ "--skip-gridargs" != "$ARG" ]; then
        ARGS[${#ARGS[*]}]="$ARG"
    fi
done

# Run the program.
# Don't quote $LL_WRAPPER because, if empty, it should simply vanish from the
# command line. But DO quote "${ARGS[@]}": preserve separate args as
# individually quoted.
$LL_WRAPPER bin/do-not-directly-run-secondlife-bin "${ARGS[@]}"
LL_RUN_ERR=$?

# Handle any resulting errors
if [ $LL_RUN_ERR -ne 0 ]; then
	# generic error running the binary
	echo '*** Bad shutdown ($LL_RUN_ERR). ***'
fi
