#!/usr/bin/python
"""\
@file md5check.py
@brief Replacement for message template compatibility verifier.

$LicenseInfo:firstyear=20i10&license=viewergpl$
Copyright (c) 2010, Linden Research, Inc.

Second Life Viewer Source Code
The source code in this file ("Source Code") is provided by Linden Lab
to you under the terms of the GNU General Public License, version 2.0
("GPL"), unless you have obtained a separate licensing agreement
("Other License"), formally executed by you and Linden Lab.  Terms of
the GPL can be found in doc/GPL-license.txt in this distribution, or
online at http://secondlifegrid.net/programs/open_source/licensing/gplv2

There are special exceptions to the terms and conditions of the GPL as
it is applied to this Source Code. View the full text of the exception
in the file doc/FLOSS-exception.txt in this software distribution, or
online at
http://secondlifegrid.net/programs/open_source/licensing/flossexception

By copying, modifying or distributing this software, you acknowledge
that you have read and understood your obligations described above,
and agree to abide by those obligations.

ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
COMPLETENESS OR PERFORMANCE.
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
