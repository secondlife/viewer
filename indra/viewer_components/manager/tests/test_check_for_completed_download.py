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
@file   test_check_for_completed_download.py
@author coyot
@date   2016-06-03
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