/** 
 * @file fmodwrapper.cpp
 * @brief dummy source file for building a shared library to wrap libfmod.a
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDIR_SOLARIS_H
#define LL_LLDIR_SOLARIS_H

#include "lldir.h"

#include <stdio.h>
#include <dirent.h>
#include <errno.h>

class LLDir_Solaris : public LLDir
{
public:
	LLDir_Solaris();
	virtual ~LLDir_Solaris();

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

#endif // LL_LLDIR_SOLARIS_H


