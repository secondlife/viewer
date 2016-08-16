#!/usr/bin/env python

"""\
@file update_manager.py
@author coyot
@date 2016-05-16
@brief executes viewer update checking and manages downloading and applying of updates

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

import os

#NOTA BENE: 
#   For POSIX platforms, llbase will be imported from the same directory.  
#   For Windows, llbase will be compiled into the executable by pyinstaller
try:
    from llbase import llrest
    from llbase.llrest import RESTError
    from llbase import llsd    
except:
    #if Windows, this is expected, if not, we're dead
    if os.name == 'nt':
        pass

from copy import deepcopy
from datetime import datetime
from urlparse import urljoin

import apply_update
import download_update
import errno
import fnmatch
import hashlib
import InstallerUserMessage
import json
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import thread
import urllib

def silent_write(log_file_handle, text):
    #if we have a log file, write.  If not, do nothing.
    #this is so we don't have to keep trapping for an exception with a None handle
    #oh and because it is best effort, it is also a holey_write ;)
    if (log_file_handle):
        #prepend text for easy grepping
        timestamp = datetime.utcnow().strftime("%Y-%m-%D %H:%M:%S")
        log_file_handle.write(timestamp + " UPDATE MANAGER: " + text + "\n")

def after_frame(message, timeout = 10000):
    #pop up a InstallerUserMessage.basic_message that kills itself after timeout milliseconds
    #note that this blocks the caller for the duration of timeout
    frame = InstallerUserMessage.InstallerUserMessage(title = "Second Life Installer", icon_name="head-sl-logo.gif")
    #this is done before basic_message so that we aren't blocked by mainloop()
    #frame.after(timeout, lambda: frame._delete_window)
    frame.after(timeout, lambda: frame.destroy())
    frame.basic_message(message = message)

def convert_version_file_style(version):
    #converts a version string a.b.c.d to a_b_c_d as used in downloaded filenames
    #re will throw a TypeError if it gets None, just return that.
    try:
        pattern = re.compile('\.')
        return pattern.sub('_', version)
    except TypeError, te:
        return None

def get_platform_key():
    #this is the name that is inserted into the VVM URI
    #and carried forward through the rest of the updater to determine
    #platform specific actions as appropriate
    platform_dict = {'Darwin':'mac', 'Linux':'lnx', 'Windows':'win'}
    platform_uname = platform.system()
    try:
        return platform_dict[platform_uname]
    except KeyError:
        return None

def get_summary(platform_name, launcher_path):
    #get the contents of the summary.json file.
    #for linux and windows, this file is in the same directory as the script
    #for mac, the script is in ../Contents/MacOS/ and the file is in ../Contents/Resources/
    script_dir = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
    if (platform_name == 'mac'):
        summary_dir = os.path.abspath(os.path.join(script_dir, "../Resources"))
    else:
        summary_dir = script_dir
    summary_file = os.path.join(summary_dir,"summary.json")
    with open(summary_file) as summary_handle:
        return json.load(summary_handle)

def get_parent_path(platform_name):
    #find the parent of the logs and user_settings directories
    if (platform_name == 'mac'):
        settings_dir = os.path.join(os.path.expanduser('~'),'Library','Application Support','SecondLife')
    elif (platform_name == 'lnx'): 
        settings_dir = os.path.join(os.path.expanduser('~'),'.secondlife')
    #using list format of join is important here because the Windows pathsep in a string escapes the next char
    elif (platform_name == 'win'):
        settings_dir = os.path.join(os.path.expanduser('~'),'AppData','Roaming','SecondLife')
    else:
        settings_dir = None
    return settings_dir

def make_download_dir(parent_dir, new_version):
    #make a canonical download dir if it does not already exist
    #format: ../user_settings/downloads/1.2.3.456789
    #we do this so that multiple viewers on the same host can update separately
    #this also functions as a getter
    try:
        download_dir = os.path.join(parent_dir, "downloads", new_version)
        os.makedirs(download_dir)
    except OSError, hell:
        #Directory already exists, that's okay.  Other OSErrors are not okay.
        if hell[0] == errno.EEXIST:  
            pass
        else:
            raise hell
    return download_dir

def check_for_completed_download(download_dir):
    #there will be two files on completion, the download and a marker file called "".done""
    #for optional upgrades, there may also be a .skip file to skip this particular upgrade 
    #or .next to install on next run
    completed = None
    marker_regex = '*' + '.done'
    skip_regex = '*' + '.skip'
    next_regex = '*' + '.next'
    for filename in os.listdir(download_dir):
        if fnmatch.fnmatch(filename, marker_regex):
            completed = 'done'
        elif fnmatch.fnmatch(filename, skip_regex):
            completed = 'skip'
        elif fnmatch.fnmatch(filename, next_regex):
            #so we don't skip infinitely
            os.remove(filename)
            completed = 'next'
    if not completed:
        #cleanup
        shutil.rmtree(download_dir)
    return completed  

def get_settings(log_file_handle, parent_dir):
    #return the settings file parsed into a dict
    try:
        settings_file = os.path.abspath(os.path.join(parent_dir,'user_settings','settings.xml'))
        #this happens when the path to settings file happens on the command line
        #we get a full path and don't need to munge it
        if not os.path.exists(settings_file):
            settings_file = parent_dir
        settings = llsd.parse((open(settings_file)).read())
    except llsd.LLSDParseError as lpe:
        silent_write(log_file_handle, "Could not parse settings file %s" % lpe)
        return None
    return settings

def get_log_file_handle(parent_dir, filename = None):
    #return a write handle on the log file
    #plus log rotation and not dying on failure
    if not filename:
        return None
    log_file = os.path.join(parent_dir, filename)
    old_file = log_file + '.old'
    #if someone's log files are present but not writable, they've screwed up their install.
    if os.access(log_file, os.W_OK):
        if os.access(old_file, os.W_OK):
            os.unlink(old_file)
        os.rename(log_file, old_file)
    elif not os.path.exists(log_file):
        #reimplement TOUCH(1) in Python
        #perms default to 644 which is fine
        open(log_file, 'w+').close()
    try:
        f = open(log_file,'w+')
    except Exception as e:
        #we don't have a log file to write to, make a best effort and sally onward
        print "Could not open update manager log file %s" % log_file
        f = None
    return f

def make_VVM_UUID_hash(platform_key):
    #NOTE: There is no python library support for a persistent machine specific UUID
    #      AND all three platforms do this a different way, so exec'ing out is really the best we can do
    #Lastly, this is a best effort service.  If we fail, we should still carry on with the update 
    uuid = None
    if (platform_key == 'lnx'):
        uuid = subprocess.check_output(['/usr/bin/hostid']).rstrip()
    elif (platform_key == 'mac'):
        #this is absurdly baroque
        #/usr/sbin/system_profiler SPHardwareDataType | fgrep 'Serial' | awk '{print $NF}'
        uuid = subprocess.check_output(["/usr/sbin/system_profiler", "SPHardwareDataType"])
        #findall[0] does the grep for the value we are looking for: "Serial Number (system): XXXXXXXX"
        #split(:)[1] gets us the XXXXXXX part
        #lstrip shaves off the leading space that was after the colon
        uuid = re.split(":", re.findall('Serial Number \(system\): \S*', uuid)[0])[1].lstrip()
    elif (platform_key == 'win'):
        # wmic csproduct get UUID | grep -v UUID
        uuid = subprocess.check_output(['wmic','csproduct','get','UUID'])
        #outputs in two rows:
        #UUID
        #XXXXXXX-XXXX...
        uuid = re.split('\n',uuid)[1].rstrip()
    if uuid is not None:
        return hashlib.md5(uuid).hexdigest()
    else:
        #fake it
        return hashlib.md5(str(uuid.uuid1())).hexdigest()

def query_vvm(log_file_handle = None, platform_key = None, settings = None, summary_dict = None, UpdaterServiceURL = None, UpdaterWillingToTest = None):
    result_data = None
    baseURI = None
    #URI template /update/v1.1/channelname/version/platformkey/platformversion/willing-to-test/uniqueid
    #https://wiki.lindenlab.com/wiki/Viewer_Version_Manager_REST_API#Viewer_Update_Query
    #note that the only two valid options are:
    # # version-phx0.damballah.lindenlab.com
    # # version-qa.secondlife-staging.com
    print "updater service host: " + repr(UpdaterServiceURL)
    if UpdaterServiceURL:
        #we can't really expect the users to put the protocol or base dir on, they will give us a host
        base_URI = urljoin('https://' + UpdaterServiceURL[0], '/update/')
    else:
        base_URI = 'https://update.secondlife.com/update/'
    channelname = summary_dict['Channel']
    #this is kind of a mess because the settings value is a) in a map and b) is both the cohort and the version in one string
    version = summary_dict['Version']
    #we need to use the dotted versions of the platform versions in order to be compatible with VVM rules and arithmetic
    if platform_key == 'win':
        platform_version = platform.win32_ver()[1]
    elif platform_key == 'mac':
        platform_version = platform.mac_ver()[0]
    else:
        platform_version = platform.release()
    #this will always return something usable, error handling in method
    UUID = str(make_VVM_UUID_hash(platform_key))
    #note that this will not normally be in a settings.xml file and is only here for test builds.
    #for test builds, add this key to the ../user_settings/settings.xml
    """
        <key>test</key>
        <map>
        <key>Comment</key>
            <string>Tell update manager you aren't willing to test.</string>
        <key>Type</key>
            <string>String</string>
        <key>Value</key>
            <integer>testno</integer>
        </map>
    </map>
    """
    if UpdaterWillingToTest is not None:
        if UpdaterWillingToTest:
            test_ok = 'testok'
        else:
            test_ok = 'testno'
    else:   
        try:
            test_ok = settings['test']['Value']
        except KeyError:
            #normal case, no testing key
            test_ok = 'testok'
    #because urljoin can't be arsed to take multiple elements
    #channelname is a list because although it is only one string, it is a kind of argument and viewer args can take multiple keywords.
    query_string =  urllib.quote('v1.0/' + channelname[0] + '/' + version + '/' + platform_key + '/' + platform_version + '/' + test_ok + '/' + UUID)
    silent_write(log_file_handle, "About to query VVM: %s" % base_URI + query_string)
    VVMService = llrest.SimpleRESTService(name='VVM', baseurl=base_URI)
    try:
        result_data = VVMService.get(query_string)
    except RESTError as re:
        silent_write(log_file_handle, "Failed to query VVM using %s failed as %s" % (urljoin(base_URI,query_string), re))
        return None
    return result_data

def download(url = None, version = None, download_dir = None, size = 0, background = False, chunk_size = None, log_file_handle = None):
    download_tries = 0
    download_success = False
    if not chunk_size:
        chunk_size = 1024
    #for background execution
    path_to_downloader = os.path.join(os.path.dirname(os.path.realpath(__file__)), "download_update.py")
    #three strikes and you're out
    while download_tries < 3 and not download_success:
        #323: Check for a partial update of the required update; in either event, display an alert that a download is required, initiate the download, and then install and launch
        if download_tries == 0:
            after_frame(message = "Downloading new version " + version + " Please wait.", timeout = 5000)
        else:
            after_frame(message = "Trying again to download new version " + version + " Please wait.", timeout = 5000)
        if not background:
            try:
                download_update.download_update(url = url, download_dir = download_dir, size = size, progressbar = True, chunk_size = chunk_size)
                download_success = True
            except Exception, e:
                download_tries += 1    
                silent_write(log_file_handle, "Failed to download new version " + version + ". Trying again.")
                silent_write(log_file_handle, "Logging download exception: %s" % e.message)
        else:
            try:
                #Python does not have a facility to multithread a method, so we make the method a standalone
                #and subprocess that
                subprocess.call(path_to_downloader, "--url = %s --dir = %s --pb --size = %s --chunk_size = %s" % (url, download_dir, size, chunk_size))
                download_success = True
            except:
                download_tries += 1
                silent_write(log_file_handle, "Failed to download new version " + version + ". Trying again.")
    if not download_success:
        silent_write(log_file_handle, "Failed to download new version " + version)
        after_frame(message = "Failed to download new version " + version + " Please check connectivity.")
        return False
    return True

def install(platform_key = None, download_dir = None, log_file_handle = None, in_place = None, downloaded = None):
    #user said no to this one
    if downloaded != 'skip':
        after_frame(message = "New version downloaded.  Installing now, please wait.")
        success = apply_update.apply_update(download_dir, platform_key, log_file_handle, in_place)
        version = download_dir.split('/')[-1]
        if success:
            silent_write(log_file_handle, "successfully updated to " + version)
            shutil.rmtree(download_dir)
            #this is either True for in place or the path to the new install for not in place
            return success
        else:
            after_frame(message = "Failed to apply " + version)
            silent_write(log_file_handle, "Failed to update viewer to " + version)
            return False
        
def download_and_install(downloaded = None, url = None, version = None, download_dir = None, size = None, 
                         platform_key = None, log_file_handle = None, in_place = None, chunk_size = 1024):
    #extracted to a method because we do it twice in update_manager() and this makes the logic clearer
    if not downloaded:
        #do the download, exit if we fail
        if not download(url = url, version = version, download_dir = download_dir, size = size, chunk_size = chunk_size, log_file_handle = log_file_handle): 
            return (False, 'download', version)  
    #do the install
    path_to_new_launcher = install(platform_key = platform_key, download_dir = download_dir, 
                                   log_file_handle = log_file_handle, in_place = in_place, downloaded = downloaded)
    if path_to_new_launcher:
        #if we succeed, propagate the success type upwards
        if in_place:
            return (True, 'in place', True)
        else:
            return (True, 'in place', path_to_new_launcher)
    else:
        #propagate failure
        return (False, 'apply', version)    
            
def update_manager(cli_overrides = None):
    #cli_overrides is a dict where the keys are specific parameters of interest and the values are the arguments to 
    #comments that begin with '323:' are steps taken from the algorithm in the description of SL-323. 
    #  Note that in the interest of efficiency, such as determining download success once at the top
    #  The code does follow precisely the same order as the algorithm.
    #return values rather than exit codes.  All of them are to communicate with launcher
    #we print just before we return so that __main__ outputs something - returns are swallowed
    #  (False, 'setup', None): error occurred before we knew what the update was (e.g., in setup or parsing)
    #  (False, 'download', version): we failed to download the new version
    #  (False, 'apply', version): we failed to apply the new version
    #  (True, None, None): No update found
    #  (True, 'in place, True): update applied in place
    #  (True, 'in place', path_to_new_launcher): Update applied by a new install to a new location
    #  (True, 'background', True): background download initiated

    #setup and getting initial parameters
    platform_key = get_platform_key()
    parent_dir = get_parent_path(platform_key)
    log_file_handle = get_log_file_handle(parent_dir, 'update_manager.log')
    settings = None

    #check to see if user has install rights
    #get the owner of the install and the current user
    script_owner_id = os.stat(os.path.realpath(__file__)).st_uid
    user_id = os.geteuid()
    #if we are on lnx or mac, we can pretty print the IDs as names using the pwd module
    #win does not provide this support and Python will throw an ImportError there, so just use raw IDs
    if script_owner_id != user_id:
        if platform_key != 'win':
            import pwd
            script_owner_name = pwd.getpwuid(script_owner_id)[0]
            username = pwd.getpwuid(user_id)[0]
        else:
            username = user_id
            script_owner_name = script_owner_id
        silent_write(log_file_handle, "Upgrade notification attempted by userid " + username)    
        frame = InstallerUserMessage(title = "Second Life Installer", icon_name="head-sl-logo.gif")
        frame.binary_choice_message(message = "Second Life was installed by userid " + script_owner_name 
            + ".  Do you have privileges to install?", true = "Yes", false = 'No')
        if not frame.choice.get():
            silent_write(log_file_handle, "Upgrade attempt declined by userid " + username)
            after_frame(message = "Please find a system admin to upgrade Second Life")
            print "Update manager exited with (%s, %s, %s)" % (False, 'setup', None)
            return (False, 'setup', None)

    if cli_overrides is not None: 
        if 'settings' in cli_overrides.keys():
            if cli_overrides['settings'] is not None:
                settings = get_settings(log_file_handle, cli_overrides['settings'][0])
        else:
            settings = get_settings(log_file_handle, parent_dir)   
        
    if settings is None:
        silent_write(log_file_handle, "Failed to load viewer settings")
        print "Update manager exited with (%s, %s, %s)" % (False, 'setup', None)
        return (False, 'setup', None)

    #323: If a complete download of that update is found, check the update preference:
    #settings['UpdaterServiceSetting'] = 0 is manual install
    """
    <key>UpdaterServiceSetting</key>
        <map>
        <key>Comment</key>
            <string>Configure updater service.</string>
        <key>Type</key>
            <string>U32</string>
        <key>Value</key>
            <string>0</string>
        </map>
    """
    if cli_overrides is not None: 
        if 'set' in cli_overrides.keys():
            if 'UpdaterServiceSetting' in cli_overrides['set'].keys():
                install_automatically = cli_overrides['set']['UpdaterServiceSetting']
    else:
        try:
            install_automatically = settings['UpdaterServiceSetting']['Value']
        #because, for some godforsaken reason, we delete the setting rather than changing the value
        except KeyError:
            install_automatically = 1
    
    #use default chunk size if none is given     
    if cli_overrides is not None: 
        if 'set' in cli_overrides.keys():
            if 'UpdaterMaximumBandwidth' in cli_overrides['set'].keys():    
                chunk_size = cli_overrides['set']['UpdaterMaximumBandwidth']
    else:
        chunk_size = 1024

    #get channel and version
    try:
        summary_dict = get_summary(platform_key, os.path.abspath(os.path.realpath(__file__)))
        #we send the override to the VVM, but retain the summary.json version for in_place computations
        channel_override_summary = deepcopy(summary_dict)        
        if cli_overrides is not None:
            if 'channel' in cli_overrides.keys():
                channel_override_summary['Channel'] = cli_overrides['channel']
    except Exception, e:
        silent_write(log_file_handle, "Could not obtain channel and version, exiting.")
        silent_write(log_file_handle, e.message)
        print "Update manager exited with (%s, %s, %s)" % (False, 'setup', None)
        return (False, 'setup', None)        

    #323: On launch, the Viewer Manager should query the Viewer Version Manager update api.
    if cli_overrides is not None:
        if 'update-service' in cli_overrides.keys():
            UpdaterServiceURL = cli_overrides['update-service']
        else:
            #tells query_vvm to use the default
            UpdaterServiceURL = None
    else:
        UpdaterServiceURL = None
    result_data = query_vvm(log_file_handle, platform_key, settings, channel_override_summary, UpdaterServiceURL)
    
    #nothing to do or error
    if not result_data:
        silent_write(log_file_handle, "No update found.")
        print "Update manager exited with (%s, %s, %s)" % (True, None, None)
        return (True, None, None)

    #get download directory, if there are perm issues or similar problems, give up
    try:
        download_dir = make_download_dir(parent_dir, result_data['version'])
    except Exception, e:
        print "Update manager exited with (%s, %s, %s)" % (False, 'setup', None)
        return (False, 'setup', None)
    
    #if the channel name of the response is the same as the channel we are launched from, the update is "in place"
    #and launcher will launch the viewer in this install location.  Otherwise, it will launch the Launcher from 
    #the new location and kill itself.
    in_place = (summary_dict['Channel'] == result_data['channel'])
    print "summary %s, result %s, in_place %s" % (summary_dict['Channel'], result_data['channel'], in_place)
    
    #determine if we've tried this download before
    downloaded = check_for_completed_download(download_dir)

    #323: If the response indicates that there is a required update: 
    if result_data['required'] or (not result_data['required'] and install_automatically):
        #323: Check for a completed download of the required update; if found, display an alert, install the required update, and launch the newly installed viewer.
        #323: If [optional download and] Install Automatically: display an alert, install the update and launch updated viewer.
        return download_and_install(downloaded = downloaded, url = result_data['url'], version = result_data['version'], download_dir = download_dir, 
                                    size = result_data['size'], platform_key = platform_key, log_file_handle = log_file_handle, in_place = in_place, chunk_size = chunk_size)
    else:
        #323: If the update response indicates that there is an optional update: 
        #323: Check to see if the optional update has already been downloaded.
        #323: If a complete download of that update is found, check the update preference: 
        #note: automatic install handled above as the steps are the same as required upgrades
        #323: If Install Manually: display a message with the update information and ask the user whether or not to install the update with three choices:
        #323: Skip this update: create a marker that subsequent launches should not prompt for this update as long as it is optional, 
        #     but leave the download in place so that if it becomes required it will be there.
        #323: Install next time: create a marker that skips the prompt and installs on the next launch
        #323: Install and launch now: do it.
        if downloaded is not None and downloaded != 'skip':
            frame = InstallerUserMessage(title = "Second Life Installer", icon_name="head-sl-logo.gif")
            #The choices are reordered slightly to encourage immediate install and slightly discourage skipping
            frame.trinary_message(message = "Please make a selection", 
                one = "Install new version now.", two = 'Install the next time the viewer is launched.', three = 'Skip this update.')
            choice = frame.choice.get()
            if choice == 1:
                return download_and_install(downloaded = downloaded, url = result_data['url'], version = result_data['version'], download_dir = download_dir, 
                                    size = result_data['size'], platform_key = platform_key, log_file_handle = log_file_handle, in_place = in_place, chunk_size = chunk_size)
            elif choice == 2:
                tempfile.mkstmp(suffix = ".next", dir = download_dir)
                return (True, None, None)
            else:
                tempfile.mkstmp(suffix = ".skip", dir = download_dir)
                return (True, None, None)
        else:
            #multithread a download
            download(url = result_data['url'], version = result_data['version'], download_dir = download_dir, size = result_data['size'], background = True, log_file_handle = log_file_handle)
            print "Update manager exited with (%s, %s, %s)" % (True, 'background', True)
            return (True, 'background', True)                  


if __name__ == '__main__':
    #there is no argument parsing or other main() work to be done
    update_manager()
