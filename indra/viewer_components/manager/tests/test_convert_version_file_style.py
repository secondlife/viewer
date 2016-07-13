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
@file   test_convert_version_file_style.py
@author coyot
@date   2016-06-01
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