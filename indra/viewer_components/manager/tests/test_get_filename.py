#!/usr/bin/env python


"""
@file   test_get_filename.py
@author coyot
@date   2016-06-30

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
from nose import with_setup

import os
import shutil
import tempfile
import apply_update
import with_setup_args

def get_filename_setup():
    tmpdir1 = tempfile.mkdtemp(prefix = 'lnx')
    tmpdir2 = tempfile.mkdtemp(prefix = 'mac')
    tmpdir3 = tempfile.mkdtemp(prefix = 'win')
    tmpdir4 = tempfile.mkdtemp(prefix = 'bad')
    tempfile.mkstemp(suffix = '.bz2', dir = tmpdir1)
    tempfile.mkstemp(suffix = '.dmg', dir = tmpdir2)
    tempfile.mkstemp(suffix = '.exe', dir = tmpdir3)

    return [tmpdir1,tmpdir2, tmpdir3, tmpdir4], {}

def get_filename_teardown(tmpdir1,tmpdir2, tmpdir3, tmpdir4):
    shutil.rmtree(tmpdir1, ignore_errors = True)
    shutil.rmtree(tmpdir2, ignore_errors = True)
    shutil.rmtree(tmpdir3, ignore_errors = True)
    shutil.rmtree(tmpdir4, ignore_errors = True)

@with_setup_args.with_setup_args(get_filename_setup, get_filename_teardown)
def test_get_filename(tmpdir1, tmpdir2, tmpdir3, tmpdir4):
    assert_is_not_none(apply_update.get_filename(tmpdir1)), "Failed to find installable"
    assert_is_not_none(apply_update.get_filename(tmpdir2)), "Failed to find installable"
    assert_is_not_none(apply_update.get_filename(tmpdir3)), "Failed to find installable"

@with_setup_args.with_setup_args(get_filename_setup, get_filename_teardown)
def test_missing_get_filename(tmpdir1, tmpdir2, tmpdir3, tmpdir4):
    not_found = not apply_update.get_filename(tmpdir4)
    assert not_found, "False positive, should not find an installable in an empty dir"
