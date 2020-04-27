#!/usr/bin/env python
"""\
This module takes a single command line argument and tests
for it being the path to a valid file (not dir) then 
displays a message to the console accordingly. It is used
for testing/editing the code self-signing feature for
Windows that is initiated by the 'sign' function im
viewer_manifest.py. You must set the SIGN environment
variable to point to this script and rebuild the 
Viewer *Solution* to initiate an installer build.

$LicenseInfo:firstyear=2014&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2020, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
"""
import sys
import os.path
from os import path

signing_target = sys.argv[1]

print 'Local self-signing proxy:'
print '    target is: "%s"' % signing_target

exists = path.exists(signing_target)
is_file = path.isfile(signing_target)

if exists == True:
    if is_file == True:
        print '    SUCCESS: Target exists and is a file'
    else:
        print '    ERROR: Target exists but it is not a file'
else:
    print '    ERROR: Target does not exist'
