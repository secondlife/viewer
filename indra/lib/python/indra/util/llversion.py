"""@file llversion.py
@brief Utility for parsing llcommon/llversion${server}.h
       for the version string and channel string
       Utility that parses svn info for branch and revision

Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
$License$
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


