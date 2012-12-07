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

/// Directory operations
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
	const std::string &getDefaultSkinDir() const;	// folder for default skin. e.g. c:\program files\second life\skins\default
	const std::string &getSkinDir() const;		// User-specified skin folder.
	const std::string &getUserDefaultSkinDir() const; // dir with user modifications to default skin
	const std::string &getUserSkinDir() const;		// User-specified skin folder with user modifications. e.g. c:\documents and settings\username\application data\second life\skins\curskin
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
	// getUserSkinDir(), getUserDefaultSkinDir(), getSkinDir(), getDefaultSkinDir()
	/// param value for findSkinnedFilenames(), explained below
	enum ESkinConstraint { CURRENT_SKIN, ALL_SKINS };
	/**
	 * Given a filename within skin, return an ordered sequence of paths to
	 * search. Nonexistent files will be filtered out -- which means that the
	 * vector might be empty.
	 *
	 * @param subdir Identify top-level skin subdirectory by passing one of
	 * LLDir::XUI (file lives under "xui" subtree), LLDir::TEXTURES (file
	 * lives under "textures" subtree), LLDir::SKINBASE (file lives at top
	 * level of skin subdirectory).
	 * @param filename Desired filename within subdir within skin, e.g.
	 * "panel_login.xml". DO NOT prepend (e.g.) "xui" or the desired language.
	 * @param constraint Callers perform two different kinds of processing.
	 * When fetching a XUI file, for instance, the existence of @a filename in
	 * the specified skin completely supercedes any @a filename in the default
	 * skin. For that case, leave the default @a constraint=CURRENT_SKIN. The
	 * returned vector will contain only
	 * ".../<i>current_skin</i>/xui/en/<i>filename</i>",
	 * ".../<i>current_skin</i>/xui/<i>current_language</i>/<i>filename</i>".
	 * But for (e.g.) "strings.xml", we want a given skin to be able to
	 * override only specific entries from the default skin. Any string not
	 * defined in the specified skin will be sought in the default skin. For
	 * that case, pass @a constraint=ALL_SKINS. The returned vector will
	 * contain at least ".../default/xui/en/strings.xml",
	 * ".../default/xui/<i>current_language</i>/strings.xml",
	 * ".../<i>current_skin</i>/xui/en/strings.xml",
	 * ".../<i>current_skin</i>/xui/<i>current_language</i>/strings.xml".
	 */
	std::vector<std::string> findSkinnedFilenames(const std::string& subdir,
												  const std::string& filename,
												  ESkinConstraint constraint=CURRENT_SKIN) const;
	/// Values for findSkinnedFilenames(subdir) parameter
	static const char *XUI, *TEXTURES, *SKINBASE;
	/**
	 * Return the base-language pathname from findSkinnedFilenames(), or
	 * the empty string if no such file exists. Parameters are identical to
	 * findSkinnedFilenames(). This is shorthand for capturing the vector
	 * returned by findSkinnedFilenames(), checking for empty() and then
	 * returning front().
	 */
	std::string findSkinnedFilenameBaseLang(const std::string &subdir,
											const std::string &filename,
											ESkinConstraint constraint=CURRENT_SKIN) const;
	/**
	 * Return the "most localized" pathname from findSkinnedFilenames(), or
	 * the empty string if no such file exists. Parameters are identical to
	 * findSkinnedFilenames(). This is shorthand for capturing the vector
	 * returned by findSkinnedFilenames(), checking for empty() and then
	 * returning back().
	 */
	std::string findSkinnedFilename(const std::string &subdir,
									const std::string &filename,
									ESkinConstraint constraint=CURRENT_SKIN) const;

	// random filename in common temporary directory
	std::string getTempFilename() const;

	// For producing safe download file names from potentially unsafe ones
	static std::string getScrubbedFileName(const std::string uncleanFileName);
	static std::string getForbiddenFileChars();

	virtual void setChatLogsDir(const std::string &path);		// Set the chat logs dir to this user's dir
	virtual void setPerAccountChatLogsDir(const std::string &username);		// Set the per user chat log directory.
	virtual void setLindenUserDir(const std::string &username);		// Set the linden user dir to this user's dir
	virtual void setSkinFolder(const std::string &skin_folder, const std::string& language);
	virtual std::string getSkinFolder() const;
	virtual std::string getLanguage() const;
	virtual bool setCacheDir(const std::string &path);

	virtual void dumpCurrentDirectories();

	// Utility routine
	std::string buildSLOSCacheDir() const;

	/// Append specified @a name to @a destpath, separated by getDirDelimiter()
	/// if both are non-empty.
	void append(std::string& destpath, const std::string& name) const;
	/// Append specified @a name to @a path, separated by getDirDelimiter()
	/// if both are non-empty. Return result, leaving @a path unmodified.
	std::string add(const std::string& path, const std::string& name) const;

protected:
	// Does an add() or append() call need a directory delimiter?
	typedef std::pair<bool, unsigned short> SepOff;
	SepOff needSep(const std::string& path, const std::string& name) const;
	// build mSearchSkinDirs without adding duplicates
	void addSearchSkinDir(const std::string& skindir);

	// Internal to findSkinnedFilenames()
	template <typename FUNCTION>
	void walkSearchSkinDirs(const std::string& subdir,
							const std::vector<std::string>& subsubdirs,
							const std::string& filename,
							const FUNCTION& function) const;

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
	std::string mSkinName;           // caller-specified skin name
	std::string mSkinBaseDir;			// Base for skins paths.
	std::string mDefaultSkinDir;			// Location for default skin info.
	std::string mSkinDir;			// Location for current skin info.
	std::string mUserDefaultSkinDir;		// Location for default skin info.
	std::string mUserSkinDir;			// Location for user-modified skin info.
	// Skin directories to search, most general to most specific. This order
	// works well for composing fine-grained files, in which an individual item
	// in a specific file overrides the corresponding item in more general
	// files. Of course, for a file-level search, iterate backwards.
	std::vector<std::string> mSearchSkinDirs;
	std::string mLanguage;              // Current viewer language
	std::string mLLPluginDir;			// Location for plugins and plugin shell
};

void dir_exists_or_crash(const std::string &dir_name);

extern LLDir *gDirUtilp;

#endif // LL_LLDIR_H
