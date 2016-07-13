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
@file   test_make_VVM_UUID_hash.py
@author coyot
@date   2016-06-03
"""

from nose.tools import *

import update_manager

def test_make_VVM_UUID_hash():
    #because the method returns different results on different hosts
    #it is not easy to unit test it reliably.  
    #About the best we can do is check for the exception from subprocess
    key = update_manager.get_platform_key()
    try:
        UUID_hash = update_manager.make_VVM_UUID_hash(key)
    except Exception, e:
        print "Test failed due to: %s" % str(e)
        assert False

    #make_UUID_hash returned None
    assert UUID_hash, "make_UUID_hash failed to make a hash."

