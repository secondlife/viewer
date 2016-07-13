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
@file   test_make_download_dir.py
@author coyot
@date   2016-06-03
"""

from nose.tools import *

import update_manager

def test_make_download_dir():
    key = update_manager.get_platform_key()
    path = update_manager.get_parent_path(key)
    version = '1.2.3.456789'
    try:
        download_dir = update_manager.make_download_dir(path, version)
    except OSError, e:
        print "make_download_dir failed to eat OSError %s" % str(e)
        assert False
    except Exception, e:
        print "make_download_dir raised an unexpected exception %s" % str(e)
        assert False

    assert download_dir, "make_download_dir returned None for path %s and version %s" % (path, version)