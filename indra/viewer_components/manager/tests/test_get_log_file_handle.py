#!/usr/bin/env python

"""
@file   test_get_log_file_handle.py
@author coyot
@date   2016-06-08

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

import os
import shutil
import tempfile
import update_manager
import with_setup_args

def get_log_file_handle_setup():
    tmpdir1 = tempfile.mkdtemp(prefix = 'test1')
    tmpdir2 = tempfile.mkdtemp(prefix = 'test2')
    log_file_path = os.path.abspath(os.path.join(tmpdir1,"update_manager.log"))
    #not using tempfile because we want a particular filename
    open(log_file_path, 'w+').close

    return [tmpdir1,tmpdir2,log_file_path], {}

def get_log_file_handle_teardown(tmpdir1,tmpdir2,log_file_path):
    shutil.rmtree(tmpdir1, ignore_errors = True)
    shutil.rmtree(tmpdir2, ignore_errors = True)
    
@with_setup_args.with_setup_args(get_log_file_handle_setup, get_log_file_handle_teardown)
def test_existing_get_log_file_handle(tmpdir1,tmpdir2,log_file_path):
    handle = update_manager.get_log_file_handle(tmpdir1)
    if not handle:
        print "Failed to find existing log file"
        assert False
    elif not os.path.exists(os.path.abspath(log_file_path+".old")):
        print "Failed to rotate update manager log"
        assert False
    assert True
    
@with_setup_args.with_setup_args(get_log_file_handle_setup, get_log_file_handle_teardown)
def test_missing_get_log_file_handle(tmpdir1,tmpdir2,log_file_path):
    handle = update_manager.get_log_file_handle(tmpdir2)
    if not os.path.exists(log_file_path):
        print "Failed to touch new log file"
        assert False
    assert True
