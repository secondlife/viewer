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
@file   test_summary.py
@author coyot
@date   2016-06-02
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