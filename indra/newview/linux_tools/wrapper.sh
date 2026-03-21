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

# Re-register the secondlife:// protocol handler every launch, for now.
#./etc/register_secondlifeprotocol.sh

# Re-register the application with the desktop system every launch, for now.
#./etc/refresh_desktop_app_entry.sh

# Above re-registering no longer used as viewer now registers itself via XDG and the Desktop Environment.

#Below is a function to check if any additional parameters passed are a valid SLURL.
function is_valid_secondlife_uri() {
	local uri="$1"
	# Check if it starts with secondlife://
	if [[ ! "$uri" =~ ^secondlife:// ]]; then
		return 1
	fi
	# Pattern: secondlife://<region>/<x>/<y>/<z> with optional additional parameters
	if [[ "$uri" =~ ^secondlife://[^/]+/[0-9]+/[0-9]+/[0-9]+(/.*)?$ ]]; then
		return 0
	else
		return 1
	fi
}

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

#Check if any additional arguments are valid SLURLs, and if so, check if a viewer instance is already running. If so, send the SLURL to be handled by the running viewer, instead of starting a new instance.
#Poll DBus to get a list of registered services, then look through the list for the Second Life API Service - if present, this means a viewer is running, if not, then no viewer is running and a new instance should be launched.
service_name="com.secondlife.ViewerAppAPIService" #Name of Second Life DBus service. This should be the same across all viewers.
if is_valid_secondlife_uri "${ARGS[@]}" && \
	dbus-send --print-reply --dest=org.freedesktop.DBus  /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep -q "${service_name}"; then
	echo "Found a Second Life compatible Viewer running, sending SLURL to DBus...";
	exec dbus-send --type=method_call --dest="${service_name}"  /com/secondlife/ViewerAppAPI com.secondlife.ViewerAppAPI.GoSLURL string:"${ARGS[@]}"
else
	# Run the program.
	# Don't quote $LL_WRAPPER because, if empty, it should simply vanish from the
	# command line. But DO quote "${ARGS[@]}": preserve separate args as
	# individually quoted.
	echo "No running Second Life Viewer found, launching new instance...";
	$LL_WRAPPER bin/do-not-directly-run-secondlife-bin "${ARGS[@]}"
	LL_RUN_ERR=$?
fi

# Handle any resulting errors
if [ $LL_RUN_ERR -ne 0 ]; then
	# generic error running the binary
	echo '*** Bad shutdown ($LL_RUN_ERR). ***'
fi
