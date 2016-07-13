#!/usr/bin/env python

# $LicenseInfo:firstyear=2016&license=internal$
# 
# Copyright (c) 2016, Linden Research, Inc.
# 
# The following source code is PROPRIETARY AND CONFIDENTIAL. Use of
# this source code is governed by the Linden Lab Source Code Disclosure
# Agreement ("Agreement") previously entered between you and Linden
# Lab. By accessing, using, copying, modifying or distributing this
# software, you acknowledge that you have been informed of your
# obligations under the Agreement and agree to abide by those obligations.
# 
# ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
# WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
# COMPLETENESS OR PERFORMANCE.
# $/LicenseInfo$

"""
@file   test_get_log_file_handle.py
@author coyot
@date   2016-06-08
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