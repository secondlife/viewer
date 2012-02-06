#!/bin/bash

cd "$(dirname "$0")"

# turn on verbose debugging output for parabuild logs.
set -x
# make errors fatal
set -e

if [ -z "$AUTOBUILD" ] ; then
    fail
fi

if [ "$OSTYPE" = "cygwin" ] ; then
    export AUTOBUILD="$(cygpath -u $AUTOBUILD)"
fi

# load autbuild provided shell functions and variables
eval "$("$AUTOBUILD" source_environment)"

projectDir="$(pwd)"

#directories we need the headers from
projects="llcommon llimage llmath llrender"

source="$projectDir/indra"

stage="$projectDir/stage/include"
mkdir -p $stage

for project in $projects
do
	dstIncludeDir="$stage/$project"
	mkdir -p $dstIncludeDir
	headers="$source/$project/*.h"
	cp $headers "$dstIncludeDir"
	headers="$source/$project/*.inl"
	# not all projects have .inl files
	files=$(ls $headers 2> /dev/null | wc -l)
	if [ "$files" != "0" ] ; then
	    cp $headers "$dstIncludeDir"
	fi
done
	
# Copy the license files into place for packaging
srcLicenseDir="$projectDir/doc"
dstLicenseDir="$projectDir/stage/LICENSES"

mkdir -p "$dstLicenseDir"
cp "$srcLicenseDir/LGPL-licence.txt" "$dstLicenseDir/LGPL-licence.txt"

pass
