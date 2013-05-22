#!/usr/bin/env python
"""\
@file test_win32_manifest.py
@brief Test an assembly binding version and uniqueness in a windows dll or exe.  

$LicenseInfo:firstyear=2009&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2009-2011, Linden Research, Inc.

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
    supported_products = (r'VisualStudio', r'VCExpress')
    value_str = (r'ProductDir')
    
    for product in supported_products:
        for version in supported_versions:
            key_str = (r'SOFTWARE\Microsoft\%s\%s\Setup\VC' %
                      (product, version))
            try:
                return get_HKLM_registry_value(key_str, value_str)
            except WindowsError, err:
                x64_key_str = (r'SOFTWARE\Wow6432Node\Microsoft\VisualStudio\%s\Setup\VS' %
                        version)
                try:
                    return get_HKLM_registry_value(x64_key_str, value_str)
                except:
                    print >> sys.stderr, "Didn't find MS %s version %s " % (product,version)
        
    raise

def find_mt_path():
    vc_dir = find_vc_dir()
    mt_path = '\"%sbin\\mt.exe\"' % vc_dir
    return mt_path
    
def test_assembly_binding(src_filename, assembly_name, assembly_ver):
    print "checking %s dependency %s..." % (src_filename, assembly_name)

    (tmp_file_fd, tmp_file_name) = tempfile.mkstemp(suffix='.xml')
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

    
