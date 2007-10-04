/** 
 * @file lldir_mac.h
 * @brief Definition of directory utilities class for Mac OS X
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDIR_MAC_H
#define LL_LLDIR_MAC_H

#include "lldir.h"

#include <dirent.h>

class LLDir_Mac : public LLDir
{
public:
	LLDir_Mac();
	virtual ~LLDir_Mac();

	virtual void initAppDirs(const std::string &app_name);
public:	
	virtual S32 deleteFilesInDir(const std::string &dirname, const std::string &mask);
	virtual std::string getCurPath();
	virtual U32 countFilesInDir(const std::string &dirname, const std::string &mask);
	virtual BOOL getNextFileInDir(const std::string &dirname, const std::string &mask, std::string &fname, BOOL wrap);
	virtual void getRandomFileInDir(const std::string &dirname, const std::string &ask, std::string &fname);
	virtual BOOL fileExists(const std::string &filename);

private:
	int mCurrentDirIndex;
	int mCurrentDirCount;
	std::string mCurrentDir;
};

#endif // LL_LLDIR_MAC_H


