/** 
 * @file lldir.cpp
 * @brief implementation of directory utilities base class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#if !LL_WINDOWS
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#else
#include <direct.h>
#endif

#include "lldir.h"

#include "llerror.h"
#include "lltimer.h"	// ms_sleep()
#include "lluuid.h"

#include "lldiriterator.h"

#if LL_WINDOWS
#include "lldir_win32.h"
LLDir_Win32 gDirUtil;
#elif LL_DARWIN
#include "lldir_mac.h"
LLDir_Mac gDirUtil;
#elif LL_SOLARIS
#include "lldir_solaris.h"
LLDir_Solaris gDirUtil;
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
	mOSCacheDir(""),
	mCAFile(""),
	mTempDir(""),
	mDirDelimiter("/") // fallback to forward slash if not overridden
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

	// File masks starting with "/" will match nothing, so we consider them invalid.
	if (LLStringUtil::startsWith(mask, getDirDelimiter()))
	{
		llwarns << "Invalid file mask: " << mask << llendl;
		llassert(!"Invalid file mask");
	}

	LLDirIterator iter(dirname, mask);
	while (iter.next(filename))
	{
		fullpath = dirname;
		fullpath += getDirDelimiter();
		fullpath += filename;

		if(LLFile::isdir(fullpath))
		{
			// skipping directory traversal filenames
			count++;
			continue;
		}

		S32 retry_count = 0;
		while (retry_count < 5)
		{
			if (0 != LLFile::remove(fullpath))
			{
				retry_count++;
				result = errno;
				llwarns << "Problem removing " << fullpath << " - errorcode: "
						<< result << " attempt " << retry_count << llendl;

				if(retry_count >= 5)
				{
					llwarns << "Failed to remove " << fullpath << llendl ;
					return count ;
				}

				ms_sleep(100);
			}
			else
			{
				if (retry_count)
				{
					llwarns << "Successfully removed " << fullpath << llendl;
				}
				break;
			}			
		}
		count++;
	}
	return count;
}

const std::string LLDir::findFile(const std::string &filename, 
						   const std::string& searchPath1, 
						   const std::string& searchPath2, 
						   const std::string& searchPath3) const
{
	std::vector<std::string> search_paths;
	search_paths.push_back(searchPath1);
	search_paths.push_back(searchPath2);
	search_paths.push_back(searchPath3);
	return findFile(filename, search_paths);
}

const std::string LLDir::findFile(const std::string& filename, const std::vector<std::string> search_paths) const
{
	std::vector<std::string>::const_iterator search_path_iter;
	for (search_path_iter = search_paths.begin();
		search_path_iter != search_paths.end();
		++search_path_iter)
	{
		if (!search_path_iter->empty())
		{
			std::string filename_and_path = (*search_path_iter);
			if (!filename.empty())
			{
				filename_and_path += getDirDelimiter() + filename;
			}
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
	if (mLindenUserDir.empty())
	{
		lldebugs << "getLindenUserDir() called early, we don't have the user name yet - returning empty string to caller" << llendl;
	}

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
		if (!mDefaultCacheDir.empty())
		{	// Set at startup - can't set here due to const API
			return mDefaultCacheDir;
		}
		
		std::string res = buildSLOSCacheDir();
		return res;
	}
	else
	{
		return mCacheDir;
	}
}

// Return the default cache directory
std::string LLDir::buildSLOSCacheDir() const
{
	std::string res;
	if (getOSCacheDir().empty())
	{
		if (getOSUserAppDir().empty())
		{
			res = "data";
		}
		else
		{
			res = getOSUserAppDir() + mDirDelimiter + "cache";
		}
	}
	else
	{
		res = getOSCacheDir() + mDirDelimiter + "SecondLife";
	}
	return res;
}



const std::string &LLDir::getOSCacheDir() const
{
	return mOSCacheDir;
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

const std::string &LLDir::getUserSkinDir() const
{
	return mUserSkinDir;
}

const std::string& LLDir::getDefaultSkinDir() const
{
	return mDefaultSkinDir;
}

const std::string LLDir::getSkinBaseDir() const
{
	return mSkinBaseDir;
}

const std::string &LLDir::getLLPluginDir() const
{
	return mLLPluginDir;
}

std::string LLDir::getExpandedFilename(ELLPath location, const std::string& filename) const
{
	return getExpandedFilename(location, "", filename);
}

std::string LLDir::getExpandedFilename(ELLPath location, const std::string& subdir, const std::string& filename) const
{
	return getExpandedFilename(location, "", subdir, filename);
}

std::string LLDir::getExpandedFilename(ELLPath location, const std::string& subdir1, const std::string& subdir2, const std::string& in_filename) const
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
		if (prefix.empty())
		{
			// if we're asking for the per-SL-account directory but we haven't logged in yet (or otherwise don't know the account name from which to build this string), then intentionally return a blank string to the caller and skip the below warning about a blank prefix.
			return std::string();
		}
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

	case LL_PATH_DEFAULT_SKIN:
		prefix = getDefaultSkinDir();
		break;

	case LL_PATH_USER_SKIN:
		prefix = getUserSkinDir();
		break;

	case LL_PATH_SKINS:
		prefix = getSkinBaseDir();
		break;

	case LL_PATH_LOCAL_ASSETS:
		prefix = getAppRODataDir();
		prefix += mDirDelimiter;
		prefix += "local_assets";
		break;

	case LL_PATH_EXECUTABLE:
		prefix = getExecutableDir();
		break;
		
	case LL_PATH_FONTS:
		prefix = getAppRODataDir();
		prefix += mDirDelimiter;
		prefix += "fonts";
		break;
		
	default:
		llassert(0);
	}

	std::string filename = in_filename;
	if (!subdir2.empty())
	{
		filename = subdir2 + mDirDelimiter + filename;
	}

	if (!subdir1.empty())
	{
		filename = subdir1 + mDirDelimiter + filename;
	}

	if (prefix.empty())
	{
		llwarns << "prefix is empty, possible bad filename" << llendl;
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

	//llinfos << "*** EXPANDED FILENAME: <" << expanded_filename << ">" << llendl;
	return expanded_filename;
}

std::string LLDir::getBaseFileName(const std::string& filepath, bool strip_exten) const
{
	std::size_t offset = filepath.find_last_of(getDirDelimiter());
	offset = (offset == std::string::npos) ? 0 : offset+1;
	std::string res = filepath.substr(offset, std::string::npos);
	if (strip_exten)
	{
		offset = res.find_last_of('.');
		if (offset != std::string::npos &&
		    offset != 0) // if basename STARTS with '.', don't strip
		{
			res = res.substr(0, offset);
		}
	}
	return res;
}

std::string LLDir::getDirName(const std::string& filepath) const
{
	std::size_t offset = filepath.find_last_of(getDirDelimiter());
	S32 len = (offset == std::string::npos) ? 0 : offset;
	std::string dirname = filepath.substr(0, len);
	return dirname;
}

std::string LLDir::getExtension(const std::string& filepath) const
{
	if (filepath.empty())
		return std::string();
	std::string basename = getBaseFileName(filepath, false);
	std::size_t offset = basename.find_last_of('.');
	std::string exten = (offset == std::string::npos || offset == 0) ? "" : basename.substr(offset+1);
	LLStringUtil::toLower(exten);
	return exten;
}

std::string LLDir::findSkinnedFilename(const std::string &filename) const
{
	return findSkinnedFilename("", "", filename);
}

std::string LLDir::findSkinnedFilename(const std::string &subdir, const std::string &filename) const
{
	return findSkinnedFilename("", subdir, filename);
}

std::string LLDir::findSkinnedFilename(const std::string &subdir1, const std::string &subdir2, const std::string &filename) const
{
	// generate subdirectory path fragment, e.g. "/foo/bar", "/foo", ""
	std::string subdirs = ((subdir1.empty() ? "" : mDirDelimiter) + subdir1)
						 + ((subdir2.empty() ? "" : mDirDelimiter) + subdir2);

	std::vector<std::string> search_paths;
	
	search_paths.push_back(getUserSkinDir() + subdirs);		// first look in user skin override
	search_paths.push_back(getSkinDir() + subdirs);			// then in current skin
	search_paths.push_back(getDefaultSkinDir() + subdirs);  // then default skin
	search_paths.push_back(getCacheDir() + subdirs);		// and last in preload directory

	std::string found_file = findFile(filename, search_paths);
	return found_file;
}

std::string LLDir::getTempFilename() const
{
	LLUUID random_uuid;
	std::string uuid_str;

	random_uuid.generate();
	random_uuid.toString(uuid_str);

	std::string temp_filename = getTempDir();
	temp_filename += mDirDelimiter;
	temp_filename += uuid_str;
	temp_filename += ".tmp";

	return temp_filename;
}

// static
std::string LLDir::getScrubbedFileName(const std::string uncleanFileName)
{
	std::string name(uncleanFileName);
	const std::string illegalChars(getForbiddenFileChars());
	// replace any illegal file chars with and underscore '_'
	for( unsigned int i = 0; i < illegalChars.length(); i++ )
	{
		int j = -1;
		while((j = name.find(illegalChars[i])) > -1)
		{
			name[j] = '_';
		}
	}
	return name;
}

// static
std::string LLDir::getForbiddenFileChars()
{
	return "\\/:*?\"<>|";
}

void LLDir::setLindenUserDir(const std::string &username)
{
	// if the username isn't set, that's bad
	if (!username.empty())
	{
		// some platforms have case-sensitive filesystems, so be
		// utterly consistent with our firstname/lastname case.
		std::string userlower(username);
		LLStringUtil::toLower(userlower);
		LLStringUtil::replaceChar(userlower, ' ', '_');
		mLindenUserDir = getOSUserAppDir();
		mLindenUserDir += mDirDelimiter;
		mLindenUserDir += userlower;
	}
	else
	{
		llerrs << "NULL name for LLDir::setLindenUserDir" << llendl;
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

void LLDir::setPerAccountChatLogsDir(const std::string &username)
{
	// if both first and last aren't set, assume we're grabbing the cached dir
	if (!username.empty())
	{
		// some platforms have case-sensitive filesystems, so be
		// utterly consistent with our firstname/lastname case.
		std::string userlower(username);
		LLStringUtil::toLower(userlower);
		LLStringUtil::replaceChar(userlower, ' ', '_');
		mPerAccountChatLogsDir = getChatLogsDir();
		mPerAccountChatLogsDir += mDirDelimiter;
		mPerAccountChatLogsDir += userlower;
	}
	else
	{
		llerrs << "NULL name for LLDir::setPerAccountChatLogsDir" << llendl;
	}
	
}

void LLDir::setSkinFolder(const std::string &skin_folder)
{
	mSkinDir = getSkinBaseDir();
	mSkinDir += mDirDelimiter;
	mSkinDir += skin_folder;

	// user modifications to current skin
	// e.g. c:\documents and settings\users\username\application data\second life\skins\dazzle
	mUserSkinDir = getOSUserAppDir();
	mUserSkinDir += mDirDelimiter;
	mUserSkinDir += "skins";
	mUserSkinDir += mDirDelimiter;	
	mUserSkinDir += skin_folder;

	// base skin which is used as fallback for all skinned files
	// e.g. c:\program files\secondlife\skins\default
	mDefaultSkinDir = getSkinBaseDir();
	mDefaultSkinDir += mDirDelimiter;	
	mDefaultSkinDir += "default";
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
		LLFile::mkdir(path);
		std::string tempname = path + mDirDelimiter + "temp";
		LLFILE* file = LLFile::fopen(tempname,"wt");
		if (file)
		{
			fclose(file);
			LLFile::remove(tempname);
			mCacheDir = path;
			return true;
		}
		return false;
	}
}

void LLDir::dumpCurrentDirectories()
{
	LL_DEBUGS2("AppInit","Directories") << "Current Directories:" << LL_ENDL;

	LL_DEBUGS2("AppInit","Directories") << "  CurPath:               " << getCurPath() << LL_ENDL;
	LL_DEBUGS2("AppInit","Directories") << "  AppName:               " << getAppName() << LL_ENDL;
	LL_DEBUGS2("AppInit","Directories") << "  ExecutableFilename:    " << getExecutableFilename() << LL_ENDL;
	LL_DEBUGS2("AppInit","Directories") << "  ExecutableDir:         " << getExecutableDir() << LL_ENDL;
	LL_DEBUGS2("AppInit","Directories") << "  ExecutablePathAndName: " << getExecutablePathAndName() << LL_ENDL;
	LL_DEBUGS2("AppInit","Directories") << "  WorkingDir:            " << getWorkingDir() << LL_ENDL;
	LL_DEBUGS2("AppInit","Directories") << "  AppRODataDir:          " << getAppRODataDir() << LL_ENDL;
	LL_DEBUGS2("AppInit","Directories") << "  OSUserDir:             " << getOSUserDir() << LL_ENDL;
	LL_DEBUGS2("AppInit","Directories") << "  OSUserAppDir:          " << getOSUserAppDir() << LL_ENDL;
	LL_DEBUGS2("AppInit","Directories") << "  LindenUserDir:         " << getLindenUserDir() << LL_ENDL;
	LL_DEBUGS2("AppInit","Directories") << "  TempDir:               " << getTempDir() << LL_ENDL;
	LL_DEBUGS2("AppInit","Directories") << "  CAFile:				 " << getCAFile() << LL_ENDL;
	LL_DEBUGS2("AppInit","Directories") << "  SkinBaseDir:           " << getSkinBaseDir() << LL_ENDL;
	LL_DEBUGS2("AppInit","Directories") << "  SkinDir:               " << getSkinDir() << LL_ENDL;
}


void dir_exists_or_crash(const std::string &dir_name)
{
#if LL_WINDOWS
	// *FIX: lame - it doesn't do the same thing on windows. not so
	// important since we don't deploy simulator to windows boxes.
	LLFile::mkdir(dir_name, 0700);
#else
	struct stat dir_stat;
	if(0 != LLFile::stat(dir_name, &dir_stat))
	{
		S32 stat_rv = errno;
		if(ENOENT == stat_rv)
		{
		   if(0 != LLFile::mkdir(dir_name, 0700))		// octal
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
