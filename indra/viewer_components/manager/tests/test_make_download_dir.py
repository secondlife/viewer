#!/usr/bin/env python

"""
@file   test_make_download_dir.py
@author coyot
@date   2016-06-03

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

from nose.tools import *

import update_manager

def test_make_download_dir():
    key = update_manager.get_platform_key()
    path = update_manager.get_parent_path(key)
    version = '1.2.3.456789'
    try:
        download_dir = update_manager.make_download_dir(path, version)
    except OSError, e:
        print "make_download_dir failed to eat OSError %s" % str(e)
        assert False
    except Exception, e:
        print "make_download_dir raised an unexpected exception %s" % str(e)
        assert False

    assert download_dir, "make_download_dir returned None for path %s and version %s" % (path, version)
