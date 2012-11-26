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
#include "stringize.h"
#include <boost/foreach.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <algorithm>

using boost::assign::list_of;
using boost::assign::map_list_of;

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

/// Values for findSkinnedFilenames(subdir) parameter
const char
	*LLDir::XUI      = "xui",
	*LLDir::TEXTURES = "textures",
	*LLDir::SKINBASE = "";

static const char* const empty = "";

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
	mDirDelimiter("/"), // fallback to forward slash if not overridden
	mLanguage("en")
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
		fullpath = add(dirname, filename);

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
			res = add(getOSUserAppDir(), "cache");
		}
	}
	else
	{
		res = add(getOSCacheDir(), "SecondLife");
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

const std::string& LLDir::getDefaultSkinDir() const
{
	return mDefaultSkinDir;
}

const std::string &LLDir::getSkinDir() const
{
	return mSkinDir;
}

const std::string &LLDir::getUserDefaultSkinDir() const
{
    return mUserDefaultSkinDir;
}

const std::string &LLDir::getUserSkinDir() const
{
	return mUserSkinDir;
}

const std::string LLDir::getSkinBaseDir() const
{
	return mSkinBaseDir;
}

const std::string &LLDir::getLLPluginDir() const
{
	return mLLPluginDir;
}

static std::string ELLPathToString(ELLPath location)
{
	typedef std::map<ELLPath, const char*> ELLPathMap;
#define ENT(symbol) (symbol, #symbol)
	static const ELLPathMap sMap = map_list_of
		ENT(LL_PATH_NONE)
		ENT(LL_PATH_USER_SETTINGS)
		ENT(LL_PATH_APP_SETTINGS)
		ENT(LL_PATH_PER_SL_ACCOUNT) // returns/expands to blank string if we don't know the account name yet
		ENT(LL_PATH_CACHE)
		ENT(LL_PATH_CHARACTER)
		ENT(LL_PATH_HELP)
		ENT(LL_PATH_LOGS)
		ENT(LL_PATH_TEMP)
		ENT(LL_PATH_SKINS)
		ENT(LL_PATH_TOP_SKIN)
		ENT(LL_PATH_CHAT_LOGS)
		ENT(LL_PATH_PER_ACCOUNT_CHAT_LOGS)
		ENT(LL_PATH_USER_SKIN)
		ENT(LL_PATH_LOCAL_ASSETS)
		ENT(LL_PATH_EXECUTABLE)
		ENT(LL_PATH_DEFAULT_SKIN)
		ENT(LL_PATH_FONTS)
		ENT(LL_PATH_LAST)
	;
#undef ENT

	ELLPathMap::const_iterator found = sMap.find(location);
	if (found != sMap.end())
		return found->second;
	return STRINGIZE("Invalid ELLPath value " << location);
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
		prefix = add(getAppRODataDir(), "app_settings");
		break;
	
	case LL_PATH_CHARACTER:
		prefix = add(getAppRODataDir(), "character");
		break;
		
	case LL_PATH_HELP:
		prefix = "help";
		break;
		
	case LL_PATH_CACHE:
		prefix = getCacheDir();
		break;
		
	case LL_PATH_USER_SETTINGS:
		prefix = add(getOSUserAppDir(), "user_settings");
		break;

	case LL_PATH_PER_SL_ACCOUNT:
		prefix = getLindenUserDir();
		if (prefix.empty())
		{
			// if we're asking for the per-SL-account directory but we haven't
			// logged in yet (or otherwise don't know the account name from
			// which to build this string), then intentionally return a blank
			// string to the caller and skip the below warning about a blank
			// prefix.
			LL_DEBUGS("LLDir") << "getLindenUserDir() not yet set: "
							   << ELLPathToString(location)
							   << ", '" << subdir1 << "', '" << subdir2 << "', '" << in_filename
							   << "' => ''" << LL_ENDL;
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
		prefix = add(getOSUserAppDir(), "logs");
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
		prefix = add(getAppRODataDir(), "local_assets");
		break;

	case LL_PATH_EXECUTABLE:
		prefix = getExecutableDir();
		break;
		
	case LL_PATH_FONTS:
		prefix = add(getAppRODataDir(), "fonts");
		break;
		
	default:
		llassert(0);
	}

	if (prefix.empty())
	{
		llwarns << ELLPathToString(location)
				<< ", '" << subdir1 << "', '" << subdir2 << "', '" << in_filename
				<< "': prefix is empty, possible bad filename" << llendl;
	}

	std::string expanded_filename = add(add(prefix, subdir1), subdir2);
	if (expanded_filename.empty() && in_filename.empty())
	{
		return "";
	}
	// Use explicit concatenation here instead of another add() call. Callers
	// passing in_filename as "" expect to obtain a pathname ending with
	// mDirSeparator so they can later directly concatenate with a specific
	// filename. A caller using add() doesn't care, but there's still code
	// loose in the system that uses std::string::operator+().
	expanded_filename += mDirDelimiter;
	expanded_filename += in_filename;

	LL_DEBUGS("LLDir") << ELLPathToString(location)
					   << ", '" << subdir1 << "', '" << subdir2 << "', '" << in_filename
					   << "' => '" << expanded_filename << "'" << LL_ENDL;
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

std::string LLDir::findSkinnedFilenameBaseLang(const std::string &subdir,
											   const std::string &filename,
											   ESkinConstraint constraint) const
{
	// This implementation is basically just as described in the declaration comments.
	std::vector<std::string> found(findSkinnedFilenames(subdir, filename, constraint));
	if (found.empty())
	{
		return "";
	}
	return found.front();
}

std::string LLDir::findSkinnedFilename(const std::string &subdir,
									   const std::string &filename,
									   ESkinConstraint constraint) const
{
	// This implementation is basically just as described in the declaration comments.
	std::vector<std::string> found(findSkinnedFilenames(subdir, filename, constraint));
	if (found.empty())
	{
		return "";
	}
	return found.back();
}

// This method exists because the two code paths for
// findSkinnedFilenames(ALL_SKINS) and findSkinnedFilenames(CURRENT_SKIN) must
// generate the list of candidate pathnames in identical ways. The only
// difference is in the body of the inner loop.
template <typename FUNCTION>
void LLDir::walkSearchSkinDirs(const std::string& subdir,
							   const std::vector<std::string>& subsubdirs,
							   const std::string& filename,
							   const FUNCTION& function) const
{
	BOOST_FOREACH(std::string skindir, mSearchSkinDirs)
	{
		std::string subdir_path(add(skindir, subdir));
		BOOST_FOREACH(std::string subsubdir, subsubdirs)
		{
			std::string full_path(add(add(subdir_path, subsubdir), filename));
			if (fileExists(full_path))
			{
				function(subsubdir, full_path);
			}
		}
	}
}

// ridiculous little helper function that should go away when we can use lambda
inline void push_back(std::vector<std::string>& vector, const std::string& value)
{
	vector.push_back(value);
}

typedef std::map<std::string, std::string> StringMap;
// ridiculous little helper function that should go away when we can use lambda
inline void store_in_map(StringMap& map, const std::string& key, const std::string& value)
{
	map[key] = value;
}

std::vector<std::string> LLDir::findSkinnedFilenames(const std::string& subdir,
													 const std::string& filename,
													 ESkinConstraint constraint) const
{
	// Recognize subdirs that have no localization.
	static const std::set<std::string> sUnlocalized = list_of
		("")                        // top-level directory not localized
		("textures")                // textures not localized
	;

	LL_DEBUGS("LLDir") << "subdir '" << subdir << "', filename '" << filename
					   << "', constraint "
					   << ((constraint == CURRENT_SKIN)? "CURRENT_SKIN" : "ALL_SKINS")
					   << LL_ENDL;

	// Cache the default language directory for each subdir we've encountered.
	// A cache entry whose value is the empty string means "not localized,
	// don't bother checking again."
	static StringMap sLocalized;

	// Check whether we've already discovered if this subdir is localized.
	StringMap::const_iterator found = sLocalized.find(subdir);
	if (found == sLocalized.end())
	{
		// We have not yet determined that. Is it one of the subdirs "known"
		// to be unlocalized?
		if (sUnlocalized.find(subdir) != sUnlocalized.end())
		{
			// This subdir is known to be unlocalized. Remember that.
			found = sLocalized.insert(StringMap::value_type(subdir, "")).first;
		}
		else
		{
			// We do not recognize this subdir. Investigate.
			std::string subdir_path(add(getDefaultSkinDir(), subdir));
			if (fileExists(add(subdir_path, "en")))
			{
				// defaultSkinDir/subdir contains subdir "en". That's our
				// default language; this subdir is localized.
				found = sLocalized.insert(StringMap::value_type(subdir, "en")).first;
			}
			else if (fileExists(add(subdir_path, "en-us")))
			{
				// defaultSkinDir/subdir contains subdir "en-us" but not "en".
				// Set as default language; this subdir is localized.
				found = sLocalized.insert(StringMap::value_type(subdir, "en-us")).first;
			}
			else
			{
				// defaultSkinDir/subdir contains neither "en" nor "en-us".
				// Assume it's not localized. Remember that assumption.
				found = sLocalized.insert(StringMap::value_type(subdir, "")).first;
			}
		}
	}
	// Every code path above should have resulted in 'found' becoming a valid
	// iterator to an entry in sLocalized.
	llassert(found != sLocalized.end());

	// Now -- is this subdir localized, or not? The answer determines what
	// subdirectories we check (under subdir) for the requested filename.
	std::vector<std::string> subsubdirs;
	if (found->second.empty())
	{
		// subdir is not localized. filename should be located directly within it.
		subsubdirs.push_back("");
	}
	else
	{
		// subdir is localized, and found->second is the default language
		// directory within it. Check both the default language and the
		// current language -- if it differs from the default, of course.
		subsubdirs.push_back(found->second);
		if (mLanguage != found->second)
		{
			subsubdirs.push_back(mLanguage);
		}
	}

	// Build results vector.
	std::vector<std::string> results;
	// The process we use depends on 'constraint'.
	if (constraint != CURRENT_SKIN) // meaning ALL_SKINS
	{
		// ALL_SKINS is simpler: just return every pathname generated by
		// walkSearchSkinDirs(). Tricky bit: walkSearchSkinDirs() passes its
		// FUNCTION the subsubdir as well as the full pathname. We just want
		// the full pathname.
		walkSearchSkinDirs(subdir, subsubdirs, filename,
						   boost::bind(push_back, boost::ref(results), _2));
	}
	else                            // CURRENT_SKIN
	{
		// CURRENT_SKIN turns out to be a bit of a misnomer because we might
		// still return files from two different skins. In any case, this
		// value of 'constraint' means we will return at most two paths: one
		// for the default language, one for the current language (supposing
		// those differ).
		// It is important to allow a user to override only the localization
		// for a particular file, for all viewer installs, without also
		// overriding the default-language file.
		// It is important to allow a user to override only the default-
		// language file, for all viewer installs, without also overriding the
		// applicable localization of that file.
		// Therefore, treat the default language and the current language as
		// two separate cases. For each, capture the most-specialized file
		// that exists.
		// Use a map keyed by subsubdir (i.e. language code). This allows us
		// to handle the case of a single subsubdirs entry with the same logic
		// that handles two. For every real file path generated by
		// walkSearchSkinDirs(), update the map entry for its subsubdir.
		StringMap path_for;
		walkSearchSkinDirs(subdir, subsubdirs, filename,
						   boost::bind(store_in_map, boost::ref(path_for), _1, _2));
		// Now that we have a path for each of the default language and the
		// current language, copy them -- in proper order -- into results.
		// Don't drive this by walking the map itself: it matters that we
		// generate results in the same order as subsubdirs.
		BOOST_FOREACH(std::string subsubdir, subsubdirs)
		{
			StringMap::const_iterator found(path_for.find(subsubdir));
			if (found != path_for.end())
			{
				results.push_back(found->second);
			}
		}
	}

	LL_DEBUGS("LLDir") << empty;
	const char* comma = "";
	BOOST_FOREACH(std::string path, results)
	{
		LL_CONT << comma << "'" << path << "'";
		comma = ", ";
	}
	LL_CONT << LL_ENDL;

	return results;
}

std::string LLDir::getTempFilename() const
{
	LLUUID random_uuid;
	std::string uuid_str;

	random_uuid.generate();
	random_uuid.toString(uuid_str);

	return add(getTempDir(), uuid_str + ".tmp");
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
		mLindenUserDir = add(getOSUserAppDir(), userlower);
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
		mPerAccountChatLogsDir = add(getChatLogsDir(), userlower);
	}
	else
	{
		llerrs << "NULL name for LLDir::setPerAccountChatLogsDir" << llendl;
	}
	
}

void LLDir::setSkinFolder(const std::string &skin_folder, const std::string& language)
{
	LL_DEBUGS("LLDir") << "Setting skin '" << skin_folder << "', language '" << language << "'"
					   << LL_ENDL;
	mSkinName = skin_folder;
	mLanguage = language;

	// This method is called multiple times during viewer initialization. Each
	// time it's called, reset mSearchSkinDirs.
	mSearchSkinDirs.clear();

	// base skin which is used as fallback for all skinned files
	// e.g. c:\program files\secondlife\skins\default
	mDefaultSkinDir = getSkinBaseDir();
	append(mDefaultSkinDir, "default");
	// This is always the most general of the search skin directories.
	addSearchSkinDir(mDefaultSkinDir);

	mSkinDir = getSkinBaseDir();
	append(mSkinDir, skin_folder);
	// Next level of generality is a skin installed with the viewer.
	addSearchSkinDir(mSkinDir);

	// user modifications to skins, current and default
	// e.g. c:\documents and settings\users\username\application data\second life\skins\dazzle
	mUserSkinDir = getOSUserAppDir();
	append(mUserSkinDir, "skins");
	mUserDefaultSkinDir = mUserSkinDir;
	append(mUserDefaultSkinDir, "default");
	append(mUserSkinDir, skin_folder);
	// Next level of generality is user modifications to default skin...
	addSearchSkinDir(mUserDefaultSkinDir);
	// then user-defined skins.
	addSearchSkinDir(mUserSkinDir);
}

void LLDir::addSearchSkinDir(const std::string& skindir)
{
	if (std::find(mSearchSkinDirs.begin(), mSearchSkinDirs.end(), skindir) == mSearchSkinDirs.end())
	{
		LL_DEBUGS("LLDir") << "search skin: '" << skindir << "'" << LL_ENDL;
		mSearchSkinDirs.push_back(skindir);
	}
}

std::string LLDir::getSkinFolder() const
{
	return mSkinName;
}

std::string LLDir::getLanguage() const
{
	return mLanguage;
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
		std::string tempname = add(path, "temp");
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

std::string LLDir::add(const std::string& path, const std::string& name) const
{
	std::string destpath(path);
	append(destpath, name);
	return destpath;
}

void LLDir::append(std::string& destpath, const std::string& name) const
{
	// Delegate question of whether we need a separator to helper method.
	SepOff sepoff(needSep(destpath, name));
	if (sepoff.first)               // do we need a separator?
	{
		destpath += mDirDelimiter;
	}
	// If destpath ends with a separator, AND name starts with one, skip
	// name's leading separator.
	destpath += name.substr(sepoff.second);
}

LLDir::SepOff LLDir::needSep(const std::string& path, const std::string& name) const
{
	if (path.empty() || name.empty())
	{
		// If either path or name are empty, we do not need a separator
		// between them.
		return SepOff(false, 0);
	}
	// Here we know path and name are both non-empty. But if path already ends
	// with a separator, or if name already starts with a separator, we need
	// not add one.
	std::string::size_type seplen(mDirDelimiter.length());
	bool path_ends_sep(path.substr(path.length() - seplen) == mDirDelimiter);
	bool name_starts_sep(name.substr(0, seplen) == mDirDelimiter);
	if ((! path_ends_sep) && (! name_starts_sep))
	{
		// If neither path nor name brings a separator to the junction, then
		// we need one.
		return SepOff(true, 0);
	}
	if (path_ends_sep && name_starts_sep)
	{
		// But if BOTH path and name bring a separator, we need not add one.
		// Moreover, we should actually skip the leading separator of 'name'.
		return SepOff(false, seplen);
	}
	// Here we know that either path_ends_sep or name_starts_sep is true --
	// but not both. So don't add a separator, and don't skip any characters:
	// simple concatenation will do the trick.
	return SepOff(false, 0);
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
