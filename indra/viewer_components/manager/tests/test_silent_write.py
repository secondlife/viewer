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
@file   test_silent_write.py
@author coyot
@date   2016-06-02
"""

from nose.tools import *

import tempfile
import update_manager

def test_silent_write_to_file(): 
    test_log = tempfile.TemporaryFile()
    try:
        update_manager.silent_write(test_log, "This is a test.")
    except Exception, e:
        print "Test failed due to: %s" % str(e)
        assert False

def test_silent_write_to_null(): 
    try:
        update_manager.silent_write(None, "This is a test.")
    except Exception, e:
        print "Test failed due to: %s" % str(e)
        assert False