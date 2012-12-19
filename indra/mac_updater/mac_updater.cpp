/** 
 * @file mac_updater.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include <boost/format.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem.hpp>

#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <curl/curl.h>

#include "llerror.h"
#include "lltimer.h"
#include "lldir.h"
#include "llfile.h"

#include "llstring.h"

#include "llerrorcontrol.h"
#include "mac_updater.h"
#include <sstream>

pthread_t updatethread;

LLMacUpdater* LLMacUpdater::sInstance = NULL;

LLMacUpdater::LLMacUpdater():
                mUpdateURL   (NULL),
                mProductName (NULL),
                mBundleID    (NULL),
                mDmgFile     (NULL),
                mMarkerPath  (NULL)
{
    sInstance    = this;
}

void LLMacUpdater::doUpdate()
{
  	// We assume that all the logs we're looking for reside on the current drive
	gDirUtilp->initAppDirs("SecondLife");
    
	LLError::initForApplication( gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, ""));
    
	// Rename current log file to ".old"
	std::string old_log_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "updater.log.old");
	std::string log_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "updater.log");
	LLFile::rename(log_file.c_str(), old_log_file.c_str());
    
	// Set the log file to updater.log
	LLError::logToFile(log_file);
    
	if ((mUpdateURL == NULL) && (mDmgFile == NULL))
	{
		llinfos << "Usage: mac_updater -url <url> | -dmg <dmg file> [-name <product_name>] [-program <program_name>]" << llendl;
		exit(1);
	}
	else
	{
		llinfos << "Update url is: " << mUpdateURL << llendl;
		if (mProductName)
		{
			llinfos << "Product name is: " << *mProductName << llendl;
		}
		else
		{
			mProductName = new std::string("Second Life");
		}
		if (mBundleID)
		{
			llinfos << "Bundle ID is: " << *mBundleID << llendl;
		}
		else
		{
			mBundleID = new std::string("com.secondlife.indra.viewer");
		}
	}
	
	llinfos << "Starting " << *mProductName << " Updater" << llendl;
    
    pthread_create(&updatethread, 
                   NULL,
                   &sUpdatethreadproc, 
                   NULL);
    
    
	void *threadresult;
    
	pthread_join(updatethread, &threadresult);
    
	if(gCancelled || gFailure)
	{
        sendStopAlert();
        
		if(mMarkerPath != 0)
		{
			// Create a install fail marker that can be used by the viewer to
			// detect install problems.
			std::ofstream stream(mMarkerPath->c_str());
			if(stream) stream << -1;
		}
		exit(-1);
	} else {
		exit(0);
	}
	
	return;  
}

//SPATTERS TODO this should be moved to lldir_mac.cpp
const std::string LLMacUpdater::walkParents( signed int depth, const std::string& childpath )
{
    boost::filesystem::path  fullpath(childpath.c_str());
    
    while (depth > 0 && fullpath.has_parent_path())
    {
        fullpath = boost::filesystem::path(fullpath.parent_path());
        --depth;
    }
    
    return fullpath.string();
}

//#if 0
//size_t curl_download_callback(void *data, size_t size, size_t nmemb,
//										  void *user_data)
//{
//	S32 bytes = size * nmemb;
//	char *cdata = (char *) data;
//	for (int i =0; i < bytes; i += 1)
//	{
//		gServerResponse.append(cdata[i]);
//	}
//	return bytes;
//}
//#endif

int curl_progress_callback_func(void *clientp,
							  double dltotal,
							  double dlnow,
							  double ultotal,
							  double ulnow)
{
	int max = (int)(dltotal / 1024.0);
	int cur = (int)(dlnow / 1024.0);
	setProgress(cur, max);
	
	if(gCancelled)
		return(1);

	return(0);
}

bool LLMacUpdater::isApplication(const std::string& app_str)
{
    return  !(bool) app_str.compare( app_str.length()-4, 4, ".app");
}
                                     
// Search through the directory specified by 'parent' for an item that appears to be a Second Life viewer.
bool LLMacUpdater::findAppBundleOnDiskImage(const boost::filesystem::path& dir_path,
                              boost::filesystem::path& path_found)
{
    if ( !boost::filesystem::exists( dir_path ) ) return false;

    boost::filesystem::directory_iterator end_itr; 
        
    for ( boost::filesystem::directory_iterator itr( dir_path );
         itr != end_itr;
         ++itr )
    {
        if ( boost::filesystem::is_directory(itr->status()) )
        {
            std::string dir_name = itr->path().string();
            if ( isApplication(dir_name) ) 
            {
                if(isFSRefViewerBundle(dir_name))
                {
                    llinfos << dir_name << " is the one" << llendl;

                    path_found = itr->path();
                    return true;
                }
            }
        }
    }
    return false;
}

bool LLMacUpdater::verifyDirectory(const boost::filesystem::path* directory, bool isParent)
{
    bool replacingTarget;
    std::string app_str = directory->string(); 
        
    if (boost::filesystem::is_directory(*directory))
    {        
        // This is fine, just means we're not replacing anything.
        replacingTarget = true;
    }
    else
    {
        replacingTarget = isParent;
    }
    
    //Check that the directory is writeable. 
    if(!isDirWritable(app_str))
    {
        // Parent directory isn't writable.
        llinfos << "Target directory not writable." << llendl;
        replacingTarget = false;
    }
    return replacingTarget;
}
                                     
bool LLMacUpdater::getViewerDir(boost::filesystem::path &app_dir)
{
    std::string  app_dir_str;
    
    //Walk up 6 levels from the App Updater's installation point.
    app_dir_str = walkParents( 6, *mApplicationPath );

    app_dir = boost::filesystem::path(app_dir_str);
    
    //Check to see that the directory's name ends in .app  Lame but it's the best thing we have to go on.
    //If it's not there, we're going to default to /Applications/VIEWERNAME
    if (!isApplication(app_dir_str))
    {
        llinfos << "Target search failed, defaulting to /Applications/" << *mProductName << ".app." << llendl;
        std::string newpath = std::string("/Applications/") + mProductName->c_str();
        app_dir = boost::filesystem::path(newpath);
    }    
    return verifyDirectory(&app_dir);    
}

bool LLMacUpdater::downloadDMG(const std::string& dmgName, boost::filesystem::path* temp_dir)
{
	LLFILE *downloadFile = NULL;
	char temp[PATH_MAX] = "";	/* Flawfinder: ignore */

    chdir(temp_dir->string().c_str());
    
    snprintf(temp, sizeof(temp), "SecondLife.dmg");		
    
    downloadFile = LLFile::fopen(temp, "wb");		/* Flawfinder: ignore */
    if(downloadFile == NULL)
    {
        return false;
    }
    
    bool success = false;
    
    CURL *curl = curl_easy_init();
    
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    //		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_download_callback);
    curl_easy_setopt(curl, CURLOPT_FILE, downloadFile);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, &curl_progress_callback_func);
    curl_easy_setopt(curl, CURLOPT_URL,	mUpdateURL);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    
    sendProgress(0, 1, std::string("Downloading..."));
    
    CURLcode result = curl_easy_perform(curl);
    
    curl_easy_cleanup(curl);
    
    if(gCancelled)
    {
        llinfos << "User cancel, bailing out."<< llendl;
        goto close_file;
    }
    
    if(result != CURLE_OK)
    {
        llinfos << "Error " << result << " while downloading disk image."<< llendl;
        goto close_file;
    }
    
    fclose(downloadFile);
    downloadFile = NULL;
    
    success = true;
    
close_file:
    // Close disk image file if necessary
	if(downloadFile != NULL)
	{
		llinfos << "Closing download file." << llendl;
        
		fclose(downloadFile);
		downloadFile = NULL;
	}

    return success;
}
                                     
bool LLMacUpdater::doMount(const std::string& dmgName, char* deviceNode, const boost::filesystem::path& temp_dir)
{
	char temp[PATH_MAX] = "";	/* Flawfinder: ignore */
    
    sendProgress(0, 0, std::string("Mounting image..."));
    chdir(temp_dir.string().c_str());
    std::string mnt_dir = temp_dir.string() + std::string("/mnt");
    LLFile::mkdir(mnt_dir.c_str(), 0700);
    
    // NOTE: we could add -private at the end of this command line to keep the image from showing up in the Finder,
    //		but if our cleanup fails, this makes it much harder for the user to unmount the image.
    std::string mountOutput;
    boost::format cmdFormat("hdiutil attach %s -mountpoint mnt");
    cmdFormat % dmgName;
    FILE* mounter = popen(cmdFormat.str().c_str(), "r");		/* Flawfinder: ignore */
    
    if(mounter == NULL)
    {
        llinfos << "Failed to mount disk image, exiting."<< llendl;
        return false;
    }
    
    // We need to scan the output from hdiutil to find the device node it uses to attach the disk image.
    // If we don't have this information, we can't detach it later.
    while(mounter != NULL)
    {
        size_t len = fread(temp, 1, sizeof(temp)-1, mounter);
        temp[len] = 0;
        mountOutput.append(temp);
        if(len < sizeof(temp)-1)
        {
            // End of file or error.
            int result = pclose(mounter);
            if(result != 0)
            {
                // NOTE: We used to abort here, but pclose() started returning 
                // -1, possibly when the size of the DMG passed a certain point 
                llinfos << "Unexpected result closing pipe: " << result << llendl; 
            }
            mounter = NULL;
        }
    }
    
    if(!mountOutput.empty())
    {
        const char *s = mountOutput.c_str();
        const char *prefix = "/dev/";
        char *sub = strstr(s, prefix);
        
        if(sub != NULL)
        {
            sub += strlen(prefix);	/* Flawfinder: ignore */
            sscanf(sub, "%1023s", deviceNode);	/* Flawfinder: ignore */
        }
    }
    
    if(deviceNode[0] != 0)
    {
        llinfos << "Disk image attached on /dev/" << deviceNode << llendl;
    }
    else
    {
        llinfos << "Disk image device node not found!" << llendl;
        return false; 
    }
    
    return true;
}

bool LLMacUpdater::moveApplication (const boost::filesystem::path& app_dir, 
                 const boost::filesystem::path& temp_dir, 
                 boost::filesystem::path& aside_dir)
{
    try
    {
        //Grab filename from installdir append to tempdir move set aside_dir to moved path.
        std::string install_str = app_dir.parent_path().string();
        std::string temp_str = temp_dir.string();
        std::string app_str = app_dir.filename().string();
        aside_dir = boost::filesystem::path( boost::filesystem::operator/(temp_dir,app_str) );
        std::cout << "Attempting to move " << app_dir.string() << " to " << aside_dir.string() << std::endl;
    
        boost::filesystem::rename(app_dir, aside_dir);
    }
    catch(boost::filesystem::filesystem_error e) 
    {
        llinfos << "Application move failed." << llendl;
        return false;
    }
    return true;
}

bool LLMacUpdater::doInstall(const boost::filesystem::path& app_dir, 
               const boost::filesystem::path& temp_dir,
               boost::filesystem::path& mount_dir,
               bool replacingTarget)
{       
    std::string temp_name = temp_dir.string() + std::string("/mnt");
    
    llinfos << "Disk image mount point is: " << temp_name << llendl;
    
    mount_dir = boost::filesystem::path(temp_name.c_str());
    
    if (! boost::filesystem::exists ( mount_dir ) )
    {
        llinfos << "Couldn't make FSRef to disk image mount point." << llendl;
        return false;
    }
    
    sendProgress(0, 0, std::string("Searching for the app bundle..."));
    
    boost::filesystem::path source_dir;
    
    if ( !findAppBundleOnDiskImage(mount_dir, source_dir) )
    {
        llinfos << "Couldn't find application bundle on mounted disk image." << llendl;
        return false;
    }
    else
    {
        llinfos << "found the bundle." << llendl;
    }
    
    sendProgress(0, 0, std::string("Preparing to copy files..."));
    
    // this will hold the name of the destination target
    boost::filesystem::path aside_dir;
    
    if(replacingTarget)
    {
        
        if (! moveApplication (app_dir, temp_dir, aside_dir) )
        {
            llwarns << "failed to move aside old version." << llendl;
            return false;
        }
    }
    
    sendProgress(0, 0, std::string("Copying files..."));
    
    llinfos << "Starting copy..." << llendl;
    //  If we were replacingTarget, we've moved the app to a temp directory.
    //  Otherwise the destination should be empty.
    //  We have mounted the DMG as a volume so we should be able to just 
    //  move the app from the volume to the destination and everything  will just work.
    
    
    // Copy the new version from the disk image to the target location.   
 
    //The installer volume is mounted read-only so we can't move.  Instead copy and then unmount.
    if (! copyDir(source_dir.string(), app_dir.string()) )
    {
        llwarns << "Failed to copy " << source_dir.string() << " to " << app_dir.string() << llendl;
        
        // Something went wrong during the copy.  Attempt to put the old version back and bail.
        boost::filesystem::rename(app_dir, aside_dir);
        return false;
        
    }
        
    // The update has succeeded.  Clear the cache directory.
    
    sendProgress(0, 0, std::string("Clearing cache..."));
    
    llinfos << "Clearing cache..." << llendl;
    
    gDirUtilp->deleteFilesInDir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE,""), "*.*");
    
    llinfos << "Clear complete." << llendl;
        
    return true;
}

void* LLMacUpdater::updatethreadproc(void*)
{
	char tempDir[PATH_MAX] = "";		/* Flawfinder: ignore */
	char temp[PATH_MAX] = "";	/* Flawfinder: ignore */
	// *NOTE: This buffer length is used in a scanf() below.
	char deviceNode[1024] = "";	/* Flawfinder: ignore */
    
	bool replacingTarget = false;

    boost::filesystem::path app_dir;
    boost::filesystem::path temp_dir;
    boost::filesystem::path mount_dir;
    
    // Attempt to get a reference to the Second Life application bundle containing this updater.
    // Any failures during this process will cause us to default to updating /Applications/Second Life.app

	try
	{        
        replacingTarget = getViewerDir( app_dir );
        
        if (!mkTempDir(temp_dir))
        {
            throw 0;
        }
       
        //In case the dir doesn't exist, try to create it.  If create fails, verify it exists. 
        if (! boost::filesystem::create_directory(app_dir))
        {


            if(isFSRefViewerBundle(app_dir.string()))
            {
                // This is the bundle we're looking for.
                replacingTarget = true;
            }
            else 
            {
                throw 0; 
            }
        }
        
        if ( !verifyDirectory(&app_dir, true) )
        {
            // We're so hosed.
            llinfos << "Applications directory not found, giving up." << llendl;
            throw 0;
        }    
		
		// Skip downloading the file if the dmg was passed on the command line.
		std::string dmgName;
		if(mDmgFile != NULL) {
            //Create a string from the mDmgFile then a dir reference to that.
            //change to that directory and begin install.
            
            boost::filesystem::path dmg_path(*mDmgFile);
            
			dmgName = dmg_path.string();  
            std::string* dmgPath = new std::string(dmg_path.parent_path().string());
            if ( !boost::filesystem::exists( dmg_path.parent_path() ) )            {
                llinfos << "Path " << *dmgPath << " is not writeable.   Aborting." << llendl;
                throw 0;
            }

			chdir(dmgPath->c_str());
		} else {
			// Continue on to download file.
			dmgName = "SecondLife.dmg";
            

            if (!downloadDMG(dmgName, &temp_dir))
            {
                throw 0;
            }
        }
        
        if (!doMount(dmgName, deviceNode, temp_dir))
        {
            throw 0;
        }
        
        if (!doInstall( app_dir, temp_dir, mount_dir, replacingTarget ))
        {
            throw 0;
        }

	}
	catch(...)
	{
		if(!gCancelled)
            gFailure = true;
	}

	// Failures from here on out are all non-fatal and not reported.
	sendProgress(0, 3, std::string("Cleaning up..."));

	setProgress(1, 3);
	// Unmount image
	if(deviceNode[0] != 0)
	{
		llinfos << "Detaching disk image." << llendl;

		snprintf(temp, sizeof(temp), "hdiutil detach '%s'", deviceNode);		
		system(temp);		/* Flawfinder: ignore */
	}

	setProgress(2, 3);
    std::string *trash_str=getUserTrashFolder();

	// Move work directory to the trash
	if(tempDir[0] != 0)
	{
		llinfos << "Moving work directory to the trash." << llendl;
                
        try 
        {
            boost::filesystem::path trash_dir(*trash_str);
            boost::filesystem::rename(mount_dir, trash_dir);
        }
        catch(boost::filesystem::filesystem_error e) 
        { 
            llwarns << "Failed to move " << mount_dir.string() << " to " << *trash_str << llendl;
            return (NULL);
        }
	}
	
    std::string app_name_str = app_dir.string();

	if(!gCancelled  && !gFailure && !app_name_str.empty())
	{
        //SPATTERS todo is there no better way to do this than system calls?
		llinfos << "Touching application bundle." << llendl;
        
        std::stringstream touch_str;

        touch_str << "touch '" << app_name_str << "'";
        
		system(touch_str.str().c_str());		/* Flawfinder: ignore */

		llinfos << "Launching updated application." << llendl;
        
        std::stringstream open_str;
        
        open_str << "open '" << app_name_str << "'";

		system(open_str.str().c_str());		/* Flawfinder: ignore */
	}

	sendDone();
	
	return (NULL);
}

//static
void* LLMacUpdater::sUpdatethreadproc(void* vptr)
{
    if (!sInstance)
    {
        llerrs << "LLMacUpdater not instantiated before use.  Aborting." << llendl;
        return (NULL);
    }
    return sInstance->updatethreadproc(vptr);
}

