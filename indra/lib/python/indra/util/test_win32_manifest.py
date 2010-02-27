#!/usr/bin/env python
# @file test_win32_manifest.py
# @brief Test an assembly binding version and uniqueness in a windows dll or exe.  
#
# $LicenseInfo:firstyear=2009&license=viewergpl$
# 
# Copyright (c) 2009, Linden Research, Inc.
# 
# Second Life Viewer Source Code
# The source code in this file ("Source Code") is provided by Linden Lab
# to you under the terms of the GNU General Public License, version 2.0
# ("GPL"), unless you have obtained a separate licensing agreement
# ("Other License"), formally executed by you and Linden Lab.  Terms of
# the GPL can be found in doc/GPL-license.txt in this distribution, or
# online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
# 
# There are special exceptions to the terms and conditions of the GPL as
# it is applied to this Source Code. View the full text of the exception
# in the file doc/FLOSS-exception.txt in this software distribution, or
# online at
# http://secondlifegrid.net/programs/open_source/licensing/flossexception
# 
# By copying, modifying or distributing this software, you acknowledge
# that you have read and understood your obligations described above,
# and agree to abide by those obligations.
# 
# ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
# WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
# COMPLETENESS OR PERFORMANCE.
# $/LicenseInfo$

import sys, os
import tempfile
from xml.dom.minidom import parse

class AssemblyTestException(Exception):
    pass

class NoManifestException(AssemblyTestException):
    pass

class MultipleBindingsException(AssemblyTestException):
    pass

class UnexpectedVersionException(AssemblyTestException):
    pass

class NoMatchingAssemblyException(AssemblyTestException):
    pass

def get_HKLM_registry_value(key_str, value_str):
    import _winreg
    reg = _winreg.ConnectRegistry(None, _winreg.HKEY_LOCAL_MACHINE)
    key = _winreg.OpenKey(reg, key_str)
    value = _winreg.QueryValueEx(key, value_str)[0]
    #print 'Found: %s' % value
    return value
        
def find_vc_dir():
    supported_versions = (r'8.0', r'9.0')
    value_str = (r'ProductDir')
    
    for version in supported_versions:
        key_str = (r'SOFTWARE\Microsoft\VisualStudio\%s\Setup\VC' %
                   version)
        try:
            return get_HKLM_registry_value(key_str, value_str)
        except WindowsError, err:
            x64_key_str = (r'SOFTWARE\Wow6432Node\Microsoft\VisualStudio\%s\Setup\VS' %
                       version)
            try:
                return get_HKLM_registry_value(x64_key_str, value_str)
            except:
                print >> sys.stderr, "Didn't find MS VC version %s " % version
        
    raise

def find_mt_path():
    vc_dir = find_vc_dir()
    print "Found vc_dir: %s" % vc_dir
    mt_path = '\"%s\\VC\\bin\\mt.exe\"' % vc_dir
    return mt_path
    
def test_assembly_binding(src_filename, assembly_name, assembly_ver):
    print "checking %s dependency %s..." % (src_filename, assembly_name)

    (tmp_file_fd, tmp_file_name) = tempfile.mkstemp(suffix='.xml')
    print tmp_file_name
    tmp_file = os.fdopen(tmp_file_fd)
    tmp_file.close()

    mt_path = find_mt_path()
    resource_id = ""
    if os.path.splitext(src_filename)[1].lower() == ".dll":
       resource_id = ";#2"
    system_call = '%s -nologo -inputresource:%s%s -out:%s > NUL' % (mt_path, src_filename, resource_id, tmp_file_name)
    print "Executing: %s" % system_call
    mt_result = os.system(system_call)
    if mt_result == 31:
        print "No manifest found in %s" % src_filename
        raise NoManifestException()

    manifest_dom = parse(tmp_file_name)
    nodes = manifest_dom.getElementsByTagName('assemblyIdentity')

    versions = list()
    for node in nodes:
        if node.getAttribute('name') == assembly_name:
            versions.append(node.getAttribute('version'))

    if len(versions) == 0:
        print "No matching assemblies found in %s" % src_filename
        raise NoMatchingAssemblyException()
        
    elif len(versions) > 1:
        print "Multiple bindings to %s found:" % assembly_name
        print versions
        print 
        raise MultipleBindingsException(versions)

    elif versions[0] != assembly_ver:
        print "Unexpected version found for %s:" % assembly_name
        print "Wanted %s, found %s" % (assembly_ver, versions[0])
        print
        raise UnexpectedVersionException(assembly_ver, versions[0])
            
    os.remove(tmp_file_name)
    
    print "SUCCESS: %s OK!" % src_filename
    print
  
if __name__ == '__main__':

    print
    print "Running test_win32_manifest.py..."
    
    usage = 'test_win32_manfest <srcFileName> <assemblyName> <assemblyVersion>'

    try:
        src_filename = sys.argv[1]
        assembly_name = sys.argv[2]
        assembly_ver = sys.argv[3]
    except:
        print "Usage:"
        print usage
        print
        raise
    
    test_assembly_binding(src_filename, assembly_name, assembly_ver)

    
