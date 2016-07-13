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
@file   test_InstallerError.py
@author coyot
@date   2016-06-01
"""

from nose.tools import assert_equal

import InstallerError
import os

def test_InstallerError():
    try:
        #try to make our own homedir, this will fail on all three platforms
        homedir = os.path.abspath(os.path.expanduser('~'))
        os.mkdir(homedir)
    except OSError, oe:
        ie = InstallerError.InstallerError(oe, "Installer failed to create a homedir that already exists.")

    assert_equal( str(ie), 
        "[Errno [Errno 17] File exists: '%s'] Installer failed to create a homedir that already exists." % homedir)
