"""@file llversion.py
@brief Utility for parsing llcommon/llversion${server}.h
       for the version string and channel string
       Utility that parses svn info for branch and revision

$LicenseInfo:firstyear=2006&license=mit$

Copyright (c) 2006-2009, Linden Research, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
$/LicenseInfo$
"""

import re, sys, os, commands

# Methods for gathering version information from
# llversionviewer.h and llversionserver.h

def get_src_root():
    indra_lib_python_indra_path = os.path.dirname(__file__)
    return os.path.abspath(os.path.realpath(indra_lib_python_indra_path + "/../../../../../"))

def get_version_file_contents(version_type):
    filepath = get_src_root() + '/indra/llcommon/llversion%s.h' % version_type
    file = open(filepath,"r")
    file_str = file.read()
    file.close()
    return file_str

def get_version(version_type):
    file_str = get_version_file_contents(version_type)
    m = re.search('const S32 LL_VERSION_MAJOR = (\d+);', file_str)
    VER_MAJOR = m.group(1)
    m = re.search('const S32 LL_VERSION_MINOR = (\d+);', file_str)
    VER_MINOR = m.group(1)
    m = re.search('const S32 LL_VERSION_PATCH = (\d+);', file_str)
    VER_PATCH = m.group(1)
    m = re.search('const S32 LL_VERSION_BUILD = (\d+);', file_str)
    VER_BUILD = m.group(1)
    version = "%(VER_MAJOR)s.%(VER_MINOR)s.%(VER_PATCH)s.%(VER_BUILD)s" % locals()
    return version

def get_channel(version_type):
    file_str = get_version_file_contents(version_type)
    m = re.search('const char \* const LL_CHANNEL = "(.+)";', file_str)
    return m.group(1)
    
def get_viewer_version():
    return get_version('viewer')

def get_server_version():
    return get_version('server')

def get_viewer_channel():
    return get_channel('viewer')

def get_server_channel():
    return get_channel('server')

# Methods for gathering subversion information
def get_svn_status_matching(regular_expression):
    # Get the subversion info from the working source tree
    status, output = commands.getstatusoutput('svn info %s' % get_src_root())
    m = regular_expression.search(output)
    if not m:
        print "Failed to parse svn info output, resultfollows:"
        print output
        raise Exception, "No matching svn status in "+src_root
    return m.group(1)

def get_svn_branch():
    branch_re = re.compile('URL: (\S+)')
    return get_svn_status_matching(branch_re)

def get_svn_revision():
    last_rev_re = re.compile('Last Changed Rev: (\d+)')
    return get_svn_status_matching(last_rev_re)


