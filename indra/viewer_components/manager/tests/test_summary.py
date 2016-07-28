#!/usr/bin/env python


"""
@file   test_summary.py
@author coyot
@date   2016-06-02

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

import os.path
import tempfile
import update_manager

def test_get_summary():
    key = update_manager.get_platform_key()
    #launcher is one dir above tests
    launcher_path = os.path.dirname(os.path.dirname(os.path.abspath(os.path.realpath(__file__))))
    summary_json = update_manager.get_summary(key, launcher_path)

    #we aren't testing the JSON library, one key pair is enough
    #so we will use the one pair that is actually a constant
    assert_equal(summary_json['Type'],'viewer')
