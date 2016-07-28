#!/usr/bin/env python

"""\
@file   test_convert_version_file_style.py
@author coyot
@date   2016-06-01

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


from nose.tools import assert_equal

import update_manager

def test_normal_form():
    version = '1.2.3.456789'
    golden = '1_2_3_456789'
    converted = update_manager.convert_version_file_style(version)
    
    assert_equal(golden, converted)

def test_short_form():
    version = '1.23'
    golden = '1_23'
    converted = update_manager.convert_version_file_style(version)
    
    assert_equal(golden, converted)

def test_idempotent():
    version = '123'
    golden = '123'
    converted = update_manager.convert_version_file_style(version)
    
    assert_equal(golden, converted)

def test_none():
    version = None
    golden = None
    converted = update_manager.convert_version_file_style(version)
    
    assert_equal(golden, converted)
