/** 
 * @file lldir_win32.h
 * @brief Definition of directory utilities class for windows
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDIR_WIN32_H
#define LL_LLDIR_WIN32_H

#include "lldir.h"

class LLDir_Win32 : public LLDir
{
public:
	LLDir_Win32();
	virtual ~LLDir_Win32();

	/*virtual*/ void initAppDirs(const std::string &app_name);

	/*virtual*/ std::string getCurPath();
	/*virtual*/ U32 countFilesInDir(const std::string &dirname, const std::string &mask);
	/*virtual*/ BOOL getNextFileInDir(const std::string &dirname, const std::string &mask, std::string &fname, BOOL wrap);
	/*virtual*/ void getRandomFileInDir(const std::string &dirname, const std::string &mask, std::string &fname);
	/*virtual*/ BOOL fileExists(const std::string &filename);

private:
	BOOL LLDir_Win32::getNextFileInDir(const llutf16string &dirname, const std::string &mask, std::string &fname, BOOL wrap);
	
	void* mDirSearch_h;
	llutf16string mCurrentDir;
};

#endif // LL_LLDIR_WIN32_H


