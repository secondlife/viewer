/** 
 * @file lldir_stub.cpp
 * @brief  stub class to allow unit testing
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Use me only if you need to stub out some helper functions, not if you e.g. need sane numbers from countFilesInDir

LLDir::LLDir() {}
LLDir::~LLDir() {}
BOOL LLDir::deleteFilesInDir(const std::string &dirname, const std::string &mask) { return true; }
void LLDir::setChatLogsDir(const std::string &path) {}
void LLDir::setPerAccountChatLogsDir(const std::string &first, const std::string &last) {}
void LLDir::setLindenUserDir(const std::string &first, const std::string &last) {}
void LLDir::setSkinFolder(const std::string &skin_folder) {}
bool LLDir::setCacheDir(const std::string &path) { return true; }
void LLDir::dumpCurrentDirectories() {}

class LLDir_stub : public LLDir
{
public:
	LLDir_stub() {}
	~LLDir_stub() {}

	/*virtual*/ void initAppDirs(const std::string &app_name) {}

	/*virtual*/ std::string getCurPath() { return "CUR_PATH_FROM_LLDIR"; }
	/*virtual*/ U32 countFilesInDir(const std::string &dirname, const std::string &mask) { return 42; }
	/*virtual*/ BOOL getNextFileInDir(const std::string &dirname, const std::string &mask, std::string &fname, BOOL wrap) { fname = fname + "_NEXT"; return false; }
	/*virtual*/ void getRandomFileInDir(const std::string &dirname, const std::string &mask, std::string &fname) { fname = "RANDOM_FILE"; }
	/*virtual*/ BOOL fileExists(const std::string &filename) const { return false; }
};

LLDir_stub gDirUtil;

LLDir* gDirUtilp = &gDirUtil;

std::string LLDir::getExpandedFilename(ELLPath loc, const std::string& subdir, const std::string& filename) const
{
	return subdir + " --- " + filename + " --- expanded!";
}

