/** 
 * @file lldir.h
 * @brief Definition of directory utilities class
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDIR_H
#define LL_LLDIR_H

typedef enum ELLPath
{
	LL_PATH_NONE = 0,
	LL_PATH_USER_SETTINGS = 1,
	LL_PATH_APP_SETTINGS = 2,	
	LL_PATH_PER_SL_ACCOUNT = 3,	
	LL_PATH_CACHE = 4,	
	LL_PATH_CHARACTER = 5,	
	LL_PATH_MOTIONS = 6,
	LL_PATH_HELP = 7,		
	LL_PATH_LOGS = 8,
	LL_PATH_TEMP = 9,
	LL_PATH_SKINS = 10,
	LL_PATH_TOP_SKIN = 11,
	LL_PATH_CHAT_LOGS = 12,
	LL_PATH_PER_ACCOUNT_CHAT_LOGS = 13,
	LL_PATH_MOZILLA_PROFILE = 14,
	LL_PATH_COUNT = 15
} ELLPath;


class LLDir
{
 public:
	LLDir();
	virtual ~LLDir();

	virtual void initAppDirs(const std::string &app_name) = 0;
 public:	
	 virtual S32 deleteFilesInDir(const std::string &dirname, const std::string &mask);

// pure virtual functions
	 virtual U32 countFilesInDir(const std::string &dirname, const std::string &mask) = 0;
	 virtual BOOL getNextFileInDir(const std::string &dirname, const std::string &mask, std::string &fname, BOOL wrap) = 0;
	 virtual void getRandomFileInDir(const std::string &dirname, const std::string &mask, std::string &fname) = 0;
	virtual std::string getCurPath() = 0;
	virtual BOOL fileExists(const std::string &filename) = 0;

	const std::string findFile(const std::string &filename, const std::string searchPath1 = "", const std::string searchPath2 = "", const std::string searchPath3 = "");
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
	const std::string &getCAFile() const;			// File containing TLS certificate authorities
	const std::string &getDirDelimiter() const;	// directory separator for platform (ie. '\' or '/' or ':')
	const std::string &getSkinDir() const;		// User-specified skin folder.

	// Expanded filename
	std::string getExpandedFilename(ELLPath location, const std::string &filename) const;
	std::string getExpandedFilename(ELLPath location, const std::string &subdir, const std::string &filename) const;

	// random filename in common temporary directory
	std::string getTempFilename() const;

	virtual void setChatLogsDir(const std::string &path);		// Set the chat logs dir to this user's dir
	virtual void setPerAccountChatLogsDir(const std::string &first, const std::string &last);		// Set the per user chat log directory.
	virtual void setLindenUserDir(const std::string &first, const std::string &last);		// Set the linden user dir to this user's dir
	virtual void setSkinFolder(const std::string &skin_folder);
	virtual bool setCacheDir(const std::string &path);

	virtual void dumpCurrentDirectories();
	
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
	std::string mCacheDir;
	std::string mDirDelimiter;
	std::string mSkinDir;			// Location for u ser-specified skin info.
};

void dir_exists_or_crash(const std::string &dir_name);

extern LLDir *gDirUtilp;

#endif // LL_LLDIR_H
