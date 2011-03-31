#!/usr/bin/env python
"""\
@file md5check.py
@brief Replacement for message template compatibility verifier.

$LicenseInfo:firstyear=2010&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2010-2011, Linden Research, Inc.

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
import hashlib

if len(sys.argv) != 3:
    print """Usage: %s --create|<hash-digest> <file>

Creates an md5sum hash digest of the specified file content
and compares it with the given hash digest.

If --create is used instead of a hash digest, it will simply
print out the hash digest of specified file content.
""" % sys.argv[0]
    sys.exit(1)

if sys.argv[2] == '-':
    fh = sys.stdin
    filename = "<stdin>"
else:
    filename = sys.argv[2]
    fh = open(filename)

hexdigest = hashlib.md5(fh.read()).hexdigest()
if sys.argv[1] == '--create':
    print hexdigest
elif hexdigest == sys.argv[1]:
    print "md5sum check passed:", filename
else:
    print "md5sum check FAILED:", filename
    sys.exit(1)
