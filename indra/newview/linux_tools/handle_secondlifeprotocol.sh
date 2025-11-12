#!/usr/bin/env sh

# Send a URL of the form secondlife://... to any running viewer, if not, launch the default viewer.
#

sl_url="$*"

echo "Got SLURL: ${sl_url}"
if [ -z "${sl_url}" ]; then
	echo "Usage: $0 secondlife:// ..."
	exit
fi

run_path=$(dirname "$0" || echo .)

#Poll DBus to get a list of registered services, then look through the list for the Second Life API Service - if present, this means a viewer is running, if not, then no viewer is running and a new instance should be launched
service_name="com.secondlife.ViewerAppAPIService" #Name of Second Life DBus service. This should be the same across all viewers.
if dbus-send --print-reply --dest=org.freedesktop.DBus  /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep -q "${service_name}"; then
	echo "Second Life running, sending to DBus...";
	exec dbus-send --type=method_call --dest="${service_name}"  /com/secondlife/ViewerAppAPI com.secondlife.ViewerAppAPI.GoSLURL string:"${sl_url}"
else
	echo "Second Life not running, launching new instance...";
	cd "${run_path}"/.. || exit
	#Go to .sh location (/etc), then up a directory to the viewer location
	exec ./secondlife -url "${sl_url}"
fi
