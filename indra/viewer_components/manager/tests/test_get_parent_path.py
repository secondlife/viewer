#!/usr/bin/env python


"""
@file   test_get_parent_path.py
@author coyot
@date   2016-06-02

$LicenseInfo:firstyear=2016&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2016, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
"""

from nose.tools import *
from nose import with_setup

import os
import shutil
import update_manager
import with_setup_args

def get_parent_path_setup():
    key = update_manager.get_platform_key()
    try:
        if key == 'mac':
            settings_dir = os.path.join(os.path.expanduser('~'),'Library','Application Support','SecondLife')
        elif key == 'lnx':
            settings_dir = os.path.join(os.path.expanduser('~'),'.secondlife')
        elif key == 'win':
            settings_dir = os.path.join(os.path.expanduser('~'),'AppData','Roaming','SecondLife')
        else:
            raise Exception("Invalid Platform Key")

        #preserve existing settings dir if any
        if os.path.exists(settings_dir):
            old_dir = settings_dir + ".tmp" 
            if os.path.exists(old_dir):
                shutil.rmtree(old_dir, ignore_errors = True)
            os.rename(settings_dir, old_dir)
        os.makedirs(settings_dir)
    except Exception, e:
        print "get_parent_path_setup failed due to: %s" % str(e)
        assert False

    #this is we don't have to rediscover settings_dir for test and teardown
    return [settings_dir], {}
 
def get_parent_path_teardown(settings_dir):
    try:
        shutil.rmtree(settings_dir, ignore_errors = True)
        #restore previous settings dir if any
        old_dir = settings_dir + ".tmp"
        if os.path.exists(old_dir):
            os.rename(old_dir, settings_dir)
    except:
        #cleanup is best effort
        pass

@with_setup_args.with_setup_args(get_parent_path_setup, get_parent_path_teardown)
def test_get_parent_path(settings_dir):
    key = update_manager.get_platform_key()
    got_settings_dir = update_manager.get_parent_path(key)
    
    assert settings_dir, "test_get_parent_path failed to obtain parent path"

    assert_equal(settings_dir, got_settings_dir)
