/** 
 * @file lldir.cpp
 * @brief implementation of directory utilities base class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#if !LL_WINDOWS
#include <sys/stat.h>
#include <sys/types.h>
#else
#include <direct.h>
#endif

#include "lldir.h"
#include "llerror.h"
#include "lluuid.h"

#if LL_WINDOWS
#include "lldir_win32.h"
LLDir_Win32 gDirUtil;
#elif LL_DARWIN
#include "lldir_mac.h"
LLDir_Mac gDirUtil;
#else
#include "lldir_linux.h"
LLDir_Linux gDirUtil;
#endif

LLDir *gDirUtilp = (LLDir *)&gDirUtil;

LLDir::LLDir()
:	mAppName(""),
	mExecutablePathAndName(""),
	mExecutableFilename(""),
	mExecutableDir(""),
	mAppRODataDir(""),
	mOSUserDir(""),
	mOSUserAppDir(""),
	mLindenUserDir(""),
	mCAFile(""),
	mTempDir(""),
	mDirDelimiter("")
{
}

LLDir::~LLDir()
{
}


S32 LLDir::deleteFilesInDir(const std::string &dirname, const std::string &mask)
{
	S32 count = 0;
	std::string filename; 
	std::string fullpath;
	S32 result;
	while (getNextFileInDir(dirname, mask, filename, FALSE))
	{
		if ((filename == ".") || (filename == ".."))
		{
			// skipping directory traversal filenames
			count++;
			continue;
		}
		fullpath = dirname;
		fullpath += getDirDelimiter();
		fullpath += filename;

		S32 retry_count = 0;
		while (retry_count < 5)
		{
			if (0 != LLFile::remove(fullpath.c_str()))
			{
				result = errno;
				llwarns << "Problem removing " << fullpath << " - errorcode: "
						<< result << " attempt " << retry_count << llendl;
				ms_sleep(1000);
			}
			else
			{
				if (retry_count)
				{
					llwarns << "Successfully removed " << fullpath << llendl;
				}
				break;
			}
			retry_count++;
		}
		count++;
	}
	return count;
}

const std::string LLDir::findFile(const std::string &filename, 
						   const std::string searchPath1, 
						   const std::string searchPath2, 
						   const std::string searchPath3)
{
	std::vector<std::string> search_paths;
	search_paths.push_back(searchPath1);
	search_paths.push_back(searchPath2);
	search_paths.push_back(searchPath3);

	std::vector<std::string>::iterator search_path_iter;
	for (search_path_iter = search_paths.begin();
		search_path_iter != search_paths.end();
		++search_path_iter)
	{
		if (!search_path_iter->empty())
		{
			std::string filename_and_path = (*search_path_iter) + getDirDelimiter() + filename;
			if (fileExists(filename_and_path))
			{
				return filename_and_path;
			}
		}
	}
	return "";
}


const std::string &LLDir::getExecutablePathAndName() const
{
	return mExecutablePathAndName;
}

const std::string &LLDir::getExecutableFilename() const
{
	return mExecutableFilename;
}

const std::string &LLDir::getExecutableDir() const
{
	return mExecutableDir;
}

const std::string &LLDir::getWorkingDir() const
{
	return mWorkingDir;
}

const std::string &LLDir::getAppName() const
{
	return mAppName;
}

const std::string &LLDir::getAppRODataDir() const
{
	return mAppRODataDir;
}

const std::string &LLDir::getOSUserDir() const
{
	return mOSUserDir;
}

const std::string &LLDir::getOSUserAppDir() const
{
	return mOSUserAppDir;
}

const std::string &LLDir::getLindenUserDir() const
{
	return mLindenUserDir;
}

const std::string &LLDir::getChatLogsDir() const
{
	return mChatLogsDir;
}

const std::string &LLDir::getPerAccountChatLogsDir() const
{
	return mPerAccountChatLogsDir;
}

const std::string &LLDir::getTempDir() const
{
	return mTempDir;
}

const std::string  LLDir::getCacheDir(bool get_default) const
{
	if (mCacheDir.empty() || get_default)
	{
		std::string res;
		if (getOSUserAppDir().empty())
		{
			res = "data";
		}
		else
		{
			res = getOSUserAppDir() + mDirDelimiter + "cache";
		}
		return res;
	}
	else
	{
		return mCacheDir;
	}
}

const std::string &LLDir::getCAFile() const
{
	return mCAFile;
}

const std::string &LLDir::getDirDelimiter() const
{
	return mDirDelimiter;
}

const std::string &LLDir::getSkinDir() const
{
	return mSkinDir;
}

std::string LLDir::getExpandedFilename(ELLPath location, const std::string& filename) const
{
	return getExpandedFilename(location, "", filename);
}

std::string LLDir::getExpandedFilename(ELLPath location, const std::string& subdir, const std::string& in_filename) const
{
	std::string prefix;
	switch (location)
	{
	case LL_PATH_NONE:
		// Do nothing
		break;

	case LL_PATH_APP_SETTINGS:
		prefix = getAppRODataDir();
		prefix += mDirDelimiter;
		prefix += "app_settings";
		break;
		
	case LL_PATH_CHARACTER:
		prefix = getAppRODataDir();
		prefix += mDirDelimiter;
		prefix += "character";
		break;
		
	case LL_PATH_MOTIONS:
		prefix = getAppRODataDir();
		prefix += mDirDelimiter;
		prefix += "motions";
		break;
		
	case LL_PATH_HELP:
		prefix = "help";
		break;
		
	case LL_PATH_CACHE:
	    prefix = getCacheDir();
		break;
		
	case LL_PATH_USER_SETTINGS:
		prefix = getOSUserAppDir();
		prefix += mDirDelimiter;
		prefix += "user_settings";
		break;

	case LL_PATH_PER_SL_ACCOUNT:
		prefix = getLindenUserDir();
		break;
		
	case LL_PATH_CHAT_LOGS:
		prefix = getChatLogsDir();
		break;
		
	case LL_PATH_PER_ACCOUNT_CHAT_LOGS:
		prefix = getPerAccountChatLogsDir();
		break;

	case LL_PATH_LOGS:
		prefix = getOSUserAppDir();
		prefix += mDirDelimiter;
		prefix += "logs";
		break;

	case LL_PATH_TEMP:
		prefix = getTempDir();
		break;

	case LL_PATH_TOP_SKIN:
		prefix = getSkinDir();
		break;

	case LL_PATH_SKINS:
		prefix = getAppRODataDir();
		prefix += mDirDelimiter;
		prefix += "skins";
		break;

	case LL_PATH_MOZILLA_PROFILE:
		prefix = getOSUserAppDir();
		prefix += mDirDelimiter;
		prefix += "browser_profile";
		break;
		
	default:
		llassert(0);
	}

	std::string filename = in_filename;
	if (!subdir.empty())
	{
		filename = subdir + mDirDelimiter + in_filename;
	}
	else
	{
		filename = in_filename;
	}
	
	std::string expanded_filename;
	if (!filename.empty())
	{
		if (!prefix.empty())
		{
			expanded_filename += prefix;
			expanded_filename += mDirDelimiter;
			expanded_filename += filename;
		}
		else
		{
			expanded_filename = filename;
		}
	}
	else if (!prefix.empty())
	{
		// Directory only, no file name.
		expanded_filename = prefix;
	}
	else
	{
		expanded_filename.assign("");
	}

	//llinfos << "*** EXPANDED FILENAME: <" << mExpandedFilename << ">" << llendl;

	return expanded_filename;
}

std::string LLDir::getTempFilename() const
{
	LLUUID random_uuid;
	char uuid_str[64];	/* Flawfinder: ignore */ 

	random_uuid.generate();
	random_uuid.toString(uuid_str);

	std::string temp_filename = getTempDir();
	temp_filename += mDirDelimiter;
	temp_filename += uuid_str;
	temp_filename += ".tmp";

	return temp_filename;
}

void LLDir::setLindenUserDir(const std::string &first, const std::string &last)
{
	// if both first and last aren't set, assume we're grabbing the cached dir
	if (!first.empty() && !last.empty())
	{
		// some platforms have case-sensitive filesystems, so be
		// utterly consistent with our firstname/lastname case.
		LLString firstlower(first);
		LLString::toLower(firstlower);
		LLString lastlower(last);
		LLString::toLower(lastlower);
		mLindenUserDir = getOSUserAppDir();
		mLindenUserDir += mDirDelimiter;
		mLindenUserDir += firstlower.c_str();
		mLindenUserDir += "_";
		mLindenUserDir += lastlower.c_str();
	}
	else
	{
		llerrs << "Invalid name for LLDir::setLindenUserDir" << llendl;
	}

	dumpCurrentDirectories();	
}

void LLDir::setChatLogsDir(const std::string &path)
{
	if (!path.empty() )
	{
		mChatLogsDir = path;
	}
	else
	{
		llwarns << "Invalid name for LLDir::setChatLogsDir" << llendl;
	}
}

void LLDir::setPerAccountChatLogsDir(const std::string &first, const std::string &last)
{
	// if both first and last aren't set, assume we're grabbing the cached dir
	if (!first.empty() && !last.empty())
	{
		// some platforms have case-sensitive filesystems, so be
		// utterly consistent with our firstname/lastname case.
		LLString firstlower(first);
		LLString::toLower(firstlower);
		LLString lastlower(last);
		LLString::toLower(lastlower);
		mPerAccountChatLogsDir = getChatLogsDir();
		mPerAccountChatLogsDir += mDirDelimiter;
		mPerAccountChatLogsDir += firstlower.c_str();
		mPerAccountChatLogsDir += "_";
		mPerAccountChatLogsDir += lastlower.c_str();
	}
	else
	{
		llwarns << "Invalid name for LLDir::setPerAccountChatLogsDir" << llendl;
	}
}

void LLDir::setSkinFolder(const std::string &skin_folder)
{
	mSkinDir = getAppRODataDir();
	mSkinDir += mDirDelimiter;
	mSkinDir += "skins";
	mSkinDir += mDirDelimiter;
	mSkinDir += skin_folder;
}

bool LLDir::setCacheDir(const std::string &path)
{
	if (path.empty() )
	{
		// reset to default
		mCacheDir = "";
		return true;
	}
	else
	{
		LLFile::mkdir(path.c_str());
		std::string tempname = path + mDirDelimiter + "temp";
		LLFILE* file = LLFile::fopen(tempname.c_str(),"wt");
		if (file)
		{
			fclose(file);
			LLFile::remove(tempname.c_str());
			mCacheDir = path;
			return true;
		}
		return false;
	}
}

void LLDir::dumpCurrentDirectories()
{
	llinfos << "Current Directories:" << llendl;

	llinfos << "  CurPath:               " << getCurPath() << llendl;
	llinfos << "  AppName:               " << getAppName() << llendl;
	llinfos << "  ExecutableFilename:    " << getExecutableFilename() << llendl;
	llinfos << "  ExecutableDir:         " << getExecutableDir() << llendl;
	llinfos << "  ExecutablePathAndName: " << getExecutablePathAndName() << llendl;
	llinfos << "  WorkingDir:            " << getWorkingDir() << llendl;
	llinfos << "  AppRODataDir:          " << getAppRODataDir() << llendl;
	llinfos << "  OSUserDir:             " << getOSUserDir() << llendl;
	llinfos << "  OSUserAppDir:          " << getOSUserAppDir() << llendl;
	llinfos << "  LindenUserDir:         " << getLindenUserDir() << llendl;
	llinfos << "  TempDir:               " << getTempDir() << llendl;
	llinfos << "  CAFile:				 " << getCAFile() << llendl;
	llinfos << "  SkinDir:               " << getSkinDir() << llendl;
}


void dir_exists_or_crash(const std::string &dir_name)
{
#if LL_WINDOWS
	// *FIX: lame - it doesn't do the same thing on windows. not so
	// important since we don't deploy simulator to windows boxes.
	LLFile::mkdir(dir_name.c_str(), 0700);
#else
	struct stat dir_stat;
	if(0 != LLFile::stat(dir_name.c_str(), &dir_stat))
	{
		S32 stat_rv = errno;
		if(ENOENT == stat_rv)
		{
		   if(0 != LLFile::mkdir(dir_name.c_str(), 0700))		// octal
		   {
			   llerrs << "Unable to create directory: " << dir_name << llendl;
		   }
		}
		else
		{
			llerrs << "Unable to stat: " << dir_name << " errno = " << stat_rv
				   << llendl;
		}
	}
	else
	{
		// data_dir exists, make sure it's a directory.
		if(!S_ISDIR(dir_stat.st_mode))
		{
			llerrs << "Data directory collision: " << dir_name << llendl;
		}
	}
#endif
}
