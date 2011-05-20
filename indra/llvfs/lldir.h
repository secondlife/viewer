/** 
 * @file lldir.h
 * @brief Definition of directory utilities class
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLDIR_H
#define LL_LLDIR_H

#if LL_SOLARIS
#include <sys/param.h>
#define MAX_PATH MAXPATHLEN
#endif

// these numbers *may* get serialized (really??), so we need to be explicit
typedef enum ELLPath
{
	LL_PATH_NONE = 0,
	LL_PATH_USER_SETTINGS = 1,
	LL_PATH_APP_SETTINGS = 2,	
	LL_PATH_PER_SL_ACCOUNT = 3, // returns/expands to blank string if we don't know the account name yet
	LL_PATH_CACHE = 4,	
	LL_PATH_CHARACTER = 5,	
	LL_PATH_HELP = 6,		
	LL_PATH_LOGS = 7,
	LL_PATH_TEMP = 8,
	LL_PATH_SKINS = 9,
	LL_PATH_TOP_SKIN = 10,
	LL_PATH_CHAT_LOGS = 11,
	LL_PATH_PER_ACCOUNT_CHAT_LOGS = 12,
	LL_PATH_USER_SKIN = 14,
	LL_PATH_LOCAL_ASSETS = 15,
	LL_PATH_EXECUTABLE = 16,
	LL_PATH_DEFAULT_SKIN = 17,
	LL_PATH_FONTS = 18,
	LL_PATH_LAST
} ELLPath;


class LLDir
{
 public:
	LLDir();
	virtual ~LLDir();

	// app_name - Usually SecondLife, used for creating settings directories
	// in OS-specific location, such as C:\Documents and Settings
	// app_read_only_data_dir - Usually the source code directory, used
	// for test applications to read newview data files.
	virtual void initAppDirs(const std::string &app_name, 
		const std::string& app_read_only_data_dir = "") = 0;

	virtual S32 deleteFilesInDir(const std::string &dirname, const std::string &mask);

// pure virtual functions
	virtual U32 countFilesInDir(const std::string &dirname, const std::string &mask) = 0;

	virtual std::string getCurPath() = 0;
	virtual BOOL fileExists(const std::string &filename) const = 0;

	const std::string findFile(const std::string& filename, const std::vector<std::string> filenames) const; 
	const std::string findFile(const std::string& filename, const std::string& searchPath1 = "", const std::string& searchPath2 = "", const std::string& searchPath3 = "") const;

	virtual std::string getLLPluginLauncher() = 0; // full path and name for the plugin shell
	virtual std::string getLLPluginFilename(std::string base_name) = 0; // full path and name to the plugin DSO for this base_name (i.e. 'FOO' -> '/bar/baz/libFOO.so')

	const std::string &getExecutablePathAndName() const;	// Full pathname of the executable
	const std::string &getAppName() const;			// install directory under progams/ ie "SecondLife"
	const std::string &getExecutableDir() const;	// Directory where the executable is located
	const std::string &getExecutableFilename() const;// Filename of .exe
	const std::string &getWorkingDir() const; // Current working directory
	const std::string &getAppRODataDir() const;	// Location of read-only data files
	const std::string &getOSUserDir() const;		// Location of the os-specific user dir
	const std::string &getOSUserAppDir() const;	// Location of the os-specific user app dir
	const std::string &getLindenUserDir() const;	// Location of the Linden user dir.
	const std::string &getChatLogsDir() const;	// Location of the chat logs dir.
	const std::string &getPerAccountChatLogsDir() const;	// Location of the per account chat logs dir.
	const std::string &getTempDir() const;			// Common temporary directory
	const std::string  getCacheDir(bool get_default = false) const;	// Location of the cache.
	const std::string &getOSCacheDir() const;		// location of OS-specific cache folder (may be empty string)
	const std::string &getCAFile() const;			// File containing TLS certificate authorities
	const std::string &getDirDelimiter() const;	// directory separator for platform (ie. '\' or '/' or ':')
	const std::string &getSkinDir() const;		// User-specified skin folder.
	const std::string &getUserSkinDir() const;		// User-specified skin folder with user modifications. e.g. c:\documents and settings\username\application data\second life\skins\curskin
	const std::string &getDefaultSkinDir() const;	// folder for default skin. e.g. c:\program files\second life\skins\default
	const std::string getSkinBaseDir() const;		// folder that contains all installed skins (not user modifications). e.g. c:\program files\second life\skins
	const std::string &getLLPluginDir() const;		// Directory containing plugins and plugin shell

	// Expanded filename
	std::string getExpandedFilename(ELLPath location, const std::string &filename) const;
	std::string getExpandedFilename(ELLPath location, const std::string &subdir, const std::string &filename) const;
	std::string getExpandedFilename(ELLPath location, const std::string &subdir1, const std::string &subdir2, const std::string &filename) const;

	// Base and Directory name extraction
	std::string getBaseFileName(const std::string& filepath, bool strip_exten = false) const;
	std::string getDirName(const std::string& filepath) const;
	std::string getExtension(const std::string& filepath) const; // Excludes '.', e.g getExtension("foo.wav") == "wav"

	// these methods search the various skin paths for the specified file in the following order:
	// getUserSkinDir(), getSkinDir(), getDefaultSkinDir()
	std::string findSkinnedFilename(const std::string &filename) const;
	std::string findSkinnedFilename(const std::string &subdir, const std::string &filename) const;
	std::string findSkinnedFilename(const std::string &subdir1, const std::string &subdir2, const std::string &filename) const;

	// random filename in common temporary directory
	std::string getTempFilename() const;

	// For producing safe download file names from potentially unsafe ones
	static std::string getScrubbedFileName(const std::string uncleanFileName);
	static std::string getForbiddenFileChars();

	virtual void setChatLogsDir(const std::string &path);		// Set the chat logs dir to this user's dir
	virtual void setPerAccountChatLogsDir(const std::string &username);		// Set the per user chat log directory.
	virtual void setLindenUserDir(const std::string &username);		// Set the linden user dir to this user's dir
	virtual void setSkinFolder(const std::string &skin_folder);
	virtual bool setCacheDir(const std::string &path);

	virtual void dumpCurrentDirectories();
	
	// Utility routine
	std::string buildSLOSCacheDir() const;

protected:
	std::string mAppName;               // install directory under progams/ ie "SecondLife"   
	std::string mExecutablePathAndName; // full path + Filename of .exe
	std::string mExecutableFilename;    // Filename of .exe
	std::string mExecutableDir;	 	 // Location of executable
	std::string mWorkingDir;	 	 // Current working directory
	std::string mAppRODataDir;			 // Location for static app data
	std::string mOSUserDir;			 // OS Specific user directory
	std::string mOSUserAppDir;			 // OS Specific user app directory
	std::string mLindenUserDir;		 // Location for Linden user-specific data
	std::string mPerAccountChatLogsDir;		 // Location for chat logs.
	std::string mChatLogsDir;		 // Location for chat logs.
	std::string mCAFile;				 // Location of the TLS certificate authority PEM file.
	std::string mTempDir;
	std::string mCacheDir;			// cache directory as set by user preference
	std::string mDefaultCacheDir;	// default cache diretory
	std::string mOSCacheDir;		// operating system cache dir
	std::string mDirDelimiter;
	std::string mSkinBaseDir;			// Base for skins paths.
	std::string mSkinDir;			// Location for current skin info.
	std::string mDefaultSkinDir;			// Location for default skin info.
	std::string mUserSkinDir;			// Location for user-modified skin info.
	std::string mLLPluginDir;			// Location for plugins and plugin shell
};

void dir_exists_or_crash(const std::string &dir_name);

extern LLDir *gDirUtilp;

#endif // LL_LLDIR_H
