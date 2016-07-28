#!/usr/bin/env python


"""
@file   test_get_settings.py
@author coyot
@date   2016-06-03

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

def get_settings_setup():
    try:
        key = update_manager.get_platform_key()
        settings_dir = os.path.join(update_manager.get_parent_path(key), "user_settings")
        print settings_dir

        #preserve existing settings dir if any
        if os.path.exists(settings_dir):
            old_dir = settings_dir + ".tmp"
            if os.path.exists(old_dir):
                shutil.rmtree(old_dir, ignore_errors = True)
            os.rename(settings_dir, old_dir)  
        os.makedirs(settings_dir)

        #the data subdir of the tests dir that this script is in
        data_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), "data")
        #the test settings file
        settings_file = os.path.join(data_dir, "settings.xml")
        shutil.copyfile(settings_file, os.path.join(settings_dir, "settings.xml"))

    except Exception, e:
        print "get_settings_setup failed due to: %s" % str(e)
        assert False

    #this is we don't have to rediscover settings_dir for test and teardown
    return [settings_dir], {}
 
def get_settings_teardown(settings_dir):
    try:
        shutil.rmtree(settings_dir, ignore_errors = True)
        #restore previous settings dir if any
        old_dir = settings_dir + ".tmp"
        if os.path.exists(old_dir):
            os.rename(old_dir, settings_dir)
    except:
        #cleanup is best effort
        pass

@with_setup_args.with_setup_args(get_settings_setup, get_settings_teardown)
def test_get_settings(settings_dir):
    key = update_manager.get_platform_key()
    parent = update_manager.get_parent_path(key)
    log_file = update_manager.get_log_file_handle(parent)
    got_settings = update_manager.get_settings(log_file, parent)

    assert got_settings, "test_get_settings failed to find a settings.xml file"

    #test one key just to make sure it parsed
    assert_equal(got_settings['CurrentGrid']['Value'], 'util.agni.lindenlab.com')
