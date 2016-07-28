#!/usr/bin/env python


"""
@file   test_query_vvm.py
@author coyot
@date   2016-06-08

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

import os
import re
import shutil
import tempfile
import update_manager
import with_setup_args

def query_vvm_setup():
    tmpdir1 = tempfile.mkdtemp(prefix = 'test1')
    handle = update_manager.get_log_file_handle(tmpdir1)

    return [tmpdir1,handle], {}

def query_vvm_teardown(tmpdir1, handle):
    shutil.rmtree(tmpdir1, ignore_errors = True)

@with_setup_args.with_setup_args(query_vvm_setup, query_vvm_teardown)
def test_query_vvm(tmpdir1, handle):
    key = update_manager.get_platform_key()
    parent = update_manager.get_parent_path(key)
    settings = update_manager.get_settings(handle, parent)
    launcher_path = os.path.dirname(os.path.dirname(os.path.abspath(os.path.realpath(__file__))))
    summary = update_manager.get_summary(key, launcher_path)

    #for unit testing purposes, just testing a value from results.  If no update, then None and it falls through
    #for formal QA see:
    #   https://docs.google.com/document/d/1WNjOPdKlq0j_7s7gdNe_3QlyGnQDa3bFNvtyVM6Hx8M/edit
    #   https://wiki.lindenlab.com/wiki/Login_Test#Test_Viewer_Updater
    #for test plans on all cases, as it requires setting up a fake VVM service

    try:
        results = update_manager.query_vvm(handle, key, settings, summary)
    except Exception, e:
        print "query_vvm threw unexpected exception %s" % str(e)
        assert False

    if results:
        pattern = re.compile('Second Life')
        assert pattern.search(results['channel']), "Bad results returned %s" % str(results)
        
    assert True
