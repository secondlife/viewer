#!/usr/bin/env python


"""
@file   test_check_for_completed_download.py
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
from nose import with_setup

import os
import shutil
import tempfile
import update_manager
import with_setup_args

def check_for_completed_download_setup():
    tmpdir1 = tempfile.mkdtemp(prefix = 'test1')
    tmpdir2 = tempfile.mkdtemp(prefix = 'test2')
    tempfile.mkstemp(suffix = '.done', dir = tmpdir1)

    return [tmpdir1,tmpdir2], {}

def check_for_completed_download_teardown(tmpdir1,tmpdir2):
    shutil.rmtree(tmpdir1, ignore_errors = True)
    shutil.rmtree(tmpdir2, ignore_errors = True)

@with_setup_args.with_setup_args(check_for_completed_download_setup, check_for_completed_download_teardown)
def test_completed_check_for_completed_download(tmpdir1,tmpdir2):
    assert_equal(update_manager.check_for_completed_download(tmpdir1), 'done'), "Failed to find completion marker"

@with_setup_args.with_setup_args(check_for_completed_download_setup, check_for_completed_download_teardown)
def test_incomplete_check_for_completed_download(tmpdir1,tmpdir2):
    #should return False
    incomplete = not update_manager.check_for_completed_download(tmpdir2)
    assert incomplete, "False positive, should not mark complete without a marker"
