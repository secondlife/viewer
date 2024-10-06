#!/usr/bin/env bash
# $LicenseInfo:firstyear=2014&license=viewerlgpl$
# Second Life Viewer Source Code
# Copyright (C) 2011, Linden Research, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation;
# version 2.1 of the License only.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
#
###
### Extract strings modified between some version and the current version
###

Action=DEFAULT
Rev=master
DefaultXuiDir="indra/newview/skins/default/xui"
Verbose=false
ExitStatus=0

while [ $# -ne 0 ]
do
    case ${1} in
        ##
        ## Show usage
        ##
        -h|--help)
            Action=USAGE
            ;;
        
        -v|--verbose)
            Verbose=true
            ;;
        
        ##
        ## Select the revision to compare against
        ##
        -r)
            if [ $# -lt 2 ]
            then
                echo "Must specify <revision> with ${1}" 1>&2
                Action=USAGE
                ExitStatus=1
                break
            else
                Rev=${2}
                shift # consume the switch ( for n values, consume n-1 )
            fi
            ;;

        ##
        ## handle an unknown switch
        ##
        -*)
            Action=USAGE
            ExitStatus=1
            break
            ;;

        *)
            if [ -z "${XuiDir}" ]
            then
                XuiDir=${1}
            else
                echo "Too many arguments supplied: $@" 1>&2
                Action=USAGE
                ExitStatus=1
                break
            fi
            ;;
    esac           

    shift # always consume 1
done

progress()
{
    if $Verbose
    then
        echo $* 1>&2
    fi
}

if [[ $ExitStatus -eq 0 && "${Action}" = "DEFAULT" ]]
then
    if [[ ! -d "${XuiDir:=$DefaultXuiDir}" ]]
    then
        echo "No XUI directory found in '$XuiDir'" 1>&2
        Action=USAGE
        ExitStatus=1
    fi
fi

if [ "${Action}" = "USAGE" ]
then
    cat <<USAGE

Usage:
    
    modified-strings.sh [ { -v | --verbose } ] [-r <revision>] [<path-to-xui>]

    where 
          --verbose shows progress messages on stderr (the command takes a while, so this is reassuring)

          -r <revision> specifies a git revision (branch, tag, commit, or relative specifier)
                     defaults to 'master' so that comparison is against the HEAD of the released viewer branch

          <path-to-xui> is the path to the root directory for XUI files
                                    defaults to '$DefaultXuiDir'

    Emits a tab-separated file with these columns:
    filename
        the path of a file that has a string change (columns 2 and 3 are empty for lines with a filename)
    name
        the name attribute of a string or label whose value changed
    English value    
        the current value of the string or label whose value changed
        for strings, newlines are changed to '\n' and tab characters are changed to '\t' 

    There is also a column for each of the language directories following the English.

USAGE
    exit $ExitStatus
fi

stringval() # reads stdin and prints the escaped value of a string for the requested tag
{
    local tag=$1
    xmllint --xpath "string(/strings/string[@name=\"$tag\"])" - | perl -p -e 'chomp; s/\n/\\n/g; s/\t/\\t/g;'
}

columns="file\tname\tEN"
for lang in $(ls -1 ${XuiDir})
do
    if [[ "$lang" != "en" && -d "${XuiDir}" && -f "${XuiDir}/$lang/strings.xml" ]]
    then
        columns+="\t$lang"
    fi
done
echo -e "$columns"

EnglishStrings="${XuiDir}/en/strings.xml"
progress -n "scanning $EnglishStrings "
echo -e "$EnglishStrings"
# loop over all tags in the current version of the strings file
cat "$EnglishStrings" | xmllint --xpath '/strings/string/@name' - | sed 's/ name="//; s/"$//;' \
| while read name
do
    progress -n "."
    # fetch the $Rev and current values for each tag
    old_stringval=$(git show "$Rev:$EnglishStrings" 2> /dev/null | stringval "$name")
    new_stringval=$(cat           "$EnglishStrings" | stringval "$name")

    if [[ "$old_stringval" != "$new_stringval" ]]
    then
        # the value is different, so print the tag and it's current value separated by a tab
        echo -e "\t$name\t$new_stringval"
    fi
done
progress ""

# loop over all XUI files other than strings.xml finding labels
grep -rlw 'label' "${XuiDir}/en" | grep -v '/strings.xml' \
| while read xuipath
do
    progress -n "scanning $xuipath "
    listed_file=false
    # loop over all elements for which there is a label attribute, getting the name attribute value
    xmllint --xpath '//*[@label]/@name' "$xuipath" 2> /dev/null | sed 's/ name="//; s/"$//;' \
    | while read name
    do
        progress -n "."
        # get the old and new label attribute values for each name
        old_label=$(git show "$Rev:$xuipath" 2> /dev/null | xmllint --xpath "string(//*[@name=\"${name}\"]/@label)" - 2> /dev/null)
        new_label=$(cat           "$xuipath" | xmllint --xpath "string(//*[@name=\"${name}\"]/@label)" - 2> /dev/null)
        if [[ "$old_label" != "$new_label" ]]
        then
            if ! $listed_file
            then
                echo -e "$xuipath"
                listed_file=true
            fi
            echo -e "\t$name\t$new_label"
        fi
    done
    progress ""
done

