#!/usr/bin/env python

"""\
@file InstallerError.py
@author coyot
@date 2016-05-16
@brief custom exception class for VMP

$LicenseInfo:firstyear=2016&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2016, Linden Research, Inc.

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

"""
usage:

>>> import InstallerError
>>> import os
>>> try:
...    os.mkdir('/tmp')
... except OSError, oe:
...    ie = InstallerError.InstallerError(oe, "foo")
...    raise ie

Traceback (most recent call last):
  File "<stdin>", line 5, in <module>
InstallerError.InstallerError: [Errno [Errno 17] File exists: '/tmp'] foo
"""

class InstallerError(OSError):
    def __init___(self, message):
        Exception.__init__(self, message)
