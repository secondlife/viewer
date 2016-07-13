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
@file   test_get_filename.py
@author coyot
@date   2016-06-30
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