#!/usr/bin/env python

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
# $LicenseInfo:firstyear=2016&license=viewerlgpl$
# Copyright (c) 2016, Linden Research, Inc.
# $/LicenseInfo$

"""
@file   apply_update.py
@author coyot
@date   2016-06-28
"""

"""
Applies an already downloaded update.
"""

import argparse
import errno
import fnmatch
import InstallerUserMessage as IUM
import os
import os.path
import plistlib
import shutil
import subprocess
import sys
import tarfile
import tempfile

#fnmatch expressions
LNX_REGEX = '*' + '.bz2'
MAC_REGEX = '*' + '.dmg'
MAC_APP_REGEX = '*' + '.app'
WIN_REGEX = '*' + '.exe'

#which install the updater is run from
INSTALL_DIR = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))

#whether the update is to the INSTALL_DIR or not.  Most of the time this is the case.
IN_PLACE = True

BUNDLE_IDENTIFIER = "com.secondlife.indra.viewer"
# Magic OS directory name that causes Cocoa viewer to crash on OS X 10.7.5
# (see MAINT-3331)
STATE_DIR = os.path.join(os.environ["HOME"], "Library", "Saved Application State",
    BUNDLE_IDENTIFIER + ".savedState")

def silent_write(log_file_handle, text):
    #if we have a log file, write.  If not, do nothing.
    if (log_file_handle):
        #prepend text for easy grepping
        log_file_handle.write("APPLY UPDATE: " + text + "\n")

def get_filename(download_dir = None):
    #given a directory that supposedly has the download, find the installable
    for filename in os.listdir(download_dir):
        if (fnmatch.fnmatch(filename, LNX_REGEX) 
          or fnmatch.fnmatch(filename, MAC_REGEX) 
          or fnmatch.fnmatch(filename, WIN_REGEX)):            
            return os.path.join(download_dir, filename)
    #someone gave us a bad directory
    return None  
          
def try_dismount(log_file_handle = None, installable = None, tmpdir = None):
    #best effort cleanup try to dismount the dmg file if we have mounted one
    #the French judge gave it a 5.8
    try:
        command = ["df", os.path.join(tmpdir, "Second Life Installer")]
        output = subprocess.check_output(command)
        #first word of second line of df output is the device name
        mnt_dev = output.split('\n')[1].split()[0]
        command = ["hdiutil", "detach", "-force", mnt_dev]
        output = subprocess.check_output(command)
        silent_write(log_file_handle, "hdiutil detach succeeded")
        silent_write(log_file_handle, output)
    except Exception, e:
        silent_write(log_file_handle, "Could not detach dmg file %s.  Error messages: %s" % (installable, e.message))    

def apply_update(download_dir = None, platform_key = None, log_file_handle = None):
    #for lnx and mac, returns path to newly installed viewer
    #for win, return the name of the executable
    #returns None on failure for all three
    #throws an exception if it can't find an installable at all
    
    installable = get_filename(download_dir)
    if not installable:
        #could not find download
        raise ValueError("Could not find installable in " + download_dir)
    
    if platform_key == 'lnx':
        installed = apply_linux_update(installable, log_file_handle)
    elif platform_key == 'mac':
        installed = apply_mac_update(installable, log_file_handle)
    elif platform_key == 'win':
        installed = apply_windows_update(installable, log_file_handle)
    else:
        #wtf?
        raise ValueError("Unknown Platform: " + platform_key)
    
    if not installed:
        #only mark the download as done when everything is done
        done_filename = os.path.join(os.path.dirname(installable), ".done")
        open(done_filename, 'w+').close()
        
    return installed
    
def apply_linux_update(installable = None, log_file_handle = None):
    try:
        #untar to tmpdir
        tmpdir = tempfile.mkdtemp()
        tar = tarfile.open(name = installable, mode="r:bz2")
        tar.extractall(path = tmpdir)
        if IN_PLACE:
            #rename current install dir
            shutil.move(INSTALL_DIR,install_dir + ".bak")
        #mv new to current
        shutil.move(tmpdir, INSTALL_DIR)
        #delete tarball on success
        os.remove(installable)
    except Exception, e:
        silent_write(log_file_handle, "Update failed due to " + repr(e))
        return None
    return INSTALL_DIR

def apply_mac_update(installable = None, log_file_handle = None):
    #INSTALL_DIR is something like /Applications/Second Life Viewer.app/Contents/MacOS, need to jump up two levels for the install base
    install_base = os.path.dirname(INSTALL_DIR)
    install_base = os.path.dirname(install_base)
    
    #verify dmg file
    try:
        output = subprocess.check_output(["hdiutil", "verify", installable], stderr=subprocess.STDOUT)
        silent_write(log_file_handle, "dmg verification succeeded")
        silent_write(log_file_handle, output)
    except Exception, e:
        silent_write(log_file_handle, "Could not verify dmg file %s.  Error messages: %s" % (installable, e.message))
        return None
    #make temp dir and mount & attach dmg
    tmpdir = tempfile.mkdtemp()
    try:
        output = subprocess.check_output(["hdiutil", "attach", installable, "-mountroot", tmpdir])
        silent_write(log_file_handle, "hdiutil attach succeeded")
        silent_write(log_file_handle, output)
    except Exception, e:
        silent_write(log_file_handle, "Could not attach dmg file %s.  Error messages: %s" % (installable, e.message))
        return None
    #verify plist
    mounted_appdir = None
    for top_dir in os.listdir(tmpdir):
        for appdir in os.listdir(os.path.join(tmpdir, top_dir)):
            appdir = os.path.join(os.path.join(tmpdir, top_dir), appdir)
            if fnmatch.fnmatch(appdir, MAC_APP_REGEX):
                try:
                    plist = os.path.join(appdir, "Contents", "Info.plist")
                    CFBundleIdentifier = plistlib.readPlist(plist)["CFBundleIdentifier"]
                    mounted_appdir = appdir
                except:
                    #there is no except for this try because there are multiple directories that legimately don't have what we are looking for
                    pass
    if not mounted_appdir:
        silent_write(log_file_handle, "Could not find app bundle in dmg %s." % (installable,))
        return None        
    if CFBundleIdentifier != BUNDLE_IDENTIFIER:
        silent_write(log_file_handle, "Wrong or null bundle identifier for dmg %s.  Bundle identifier: %s" % (installable, CFBundleIdentifier))
        try_dismount(log_file_handle, installable, tmpdir)                   
        return None
    #do the install, finally
    if IN_PLACE:
        #  swap out old install directory
        bundlename = os.path.basename(mounted_appdir)
        silent_write(log_file_handle, "Updating %s" % bundlename)
        swapped_out = os.path.join(tmpdir, INSTALL_DIR.lstrip('/'))
        shutil.move(install_base, swapped_out)               
    else:
        silent_write(log_file_handle, "Installing %s" % install_base)
        
    #   copy over the new bits    
    try:
        shutil.copytree(mounted_appdir, install_base, symlinks=True)
        retcode = 0
    except Exception, e:
        # try to restore previous viewer
        if os.path.exists(swapped_out):
            silent_write(log_file_handle, "Install of %s failed, rolling back to previous viewer." % installable)
            shutil.move(swapped_out, installed_test)
            retcode = 1
    finally:
        try_dismount(log_file_handle, installable, tmpdir)
        if retcode:
            return None
    
    #see MAINT-3331        
    try:
        shutil.rmtree(STATE_DIR)  
    except Exception, e:
        #if we fail to delete something that isn't there, that's okay
        if e[0] == errno.ENOENT:
            pass
        else:
            raise e
    
    os.remove(installable)
    return install_base
    
def apply_windows_update(installable = None, log_file_handle = None):
    #the windows install is just running the NSIS installer executable
    #from VMP's perspective, it is a black box
    try:
        output = subprocess.check_output(installable, stderr=subprocess.STDOUT)
        silent_write(log_file_handle, "Install of %s succeeded." % installable)
        silent_write(log_file_handle, output)
    except subprocess.CalledProcessError, cpe:
        silent_write(log_file_handle, "%s failed with return code %s. Error messages: %s." % 
                     (cpe.cmd, cpe.returncode, cpe.message))
        return None
    return installable

def main():
    parser = argparse.ArgumentParser("Apply Downloaded Update")
    parser.add_argument('--dir', dest = 'download_dir', help = 'directory to find installable', required = True)
    parser.add_argument('--pkey', dest = 'platform_key', help =' OS: lnx|mac|win', required = True)
    parser.add_argument('--in_place', action = 'store_false', help = 'This upgrade is for a different channel', default = True)
    parser.add_argument('--log_file', dest = 'log_file', default = None, help = 'file to write messages to')
    args = parser.parse_args()
    
    if args.log_file:
        try:
            f = open(args.log_file,'w+') 
        except:
            print "%s could not be found or opened" % args.log_file
            sys.exit(1)
    
    IN_PLACE = args.in_place
    result = apply_update(download_dir = args.download_dir, platform_key = args.platform_key, log_file_handle = f)
    if not result:
        sys.exit("Update failed")
    else:
        sys.exit(0)


if __name__ == "__main__":
    main()
