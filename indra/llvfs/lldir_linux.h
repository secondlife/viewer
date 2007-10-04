/** 
 * @file lldir_linux.h
 * @brief Definition of directory utilities class for linux 
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDIR_LINUX_H
#define LL_LLDIR_LINUX_H

#include "lldir.h"

#include <dirent.h>
#include <errno.h>

class LLDir_Linux : public LLDir
{
public:
	LLDir_Linux();
	virtual ~LLDir_Linux();

	virtual void initAppDirs(const std::string &app_name);
public:	
	virtual std::string getCurPath();
	virtual U32 countFilesInDir(const std::string &dirname, const std::string &mask);
	virtual BOOL getNextFileInDir(const std::string &dirname, const std::string &mask, std::string &fname, BOOL wrap);
	virtual void getRandomFileInDir(const std::string &dirname, const std::string &mask, std::string &fname);
	/*virtual*/ BOOL fileExists(const std::string &filename);

private:
	DIR *mDirp;
	int mCurrentDirIndex;
	int mCurrentDirCount;
	std::string mCurrentDir;	
};

#endif // LL_LLDIR_LINUX_H


