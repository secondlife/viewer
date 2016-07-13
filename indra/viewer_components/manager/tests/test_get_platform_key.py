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
@file   test_get_platform_key.py
@author coyot
@date   2016-06-01
"""

from nose.tools import assert_equal

import platform
import update_manager

def test_get_platform_key():
    key = update_manager.get_platform_key()
    if key == 'mac':
        assert_equal(platform.system(),'Darwin')
    elif key == 'lnx':
        assert_equal(platform.system(),'Linux')
    elif key == 'win':
        assert_equal(platform.system(),'Windows')
    else:
        assert_equal(key, None)
    