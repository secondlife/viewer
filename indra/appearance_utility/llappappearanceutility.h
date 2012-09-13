/** 
 * @file llappappearanceutility.h
 * @brief Declaration of LLAppAppearanceUtility class.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_LLAPPAPPEARANCEUTILITY_H
#define LL_LLAPPAPPEARANCEUTILITY_H

#include <exception>

#include "llapp.h"

enum EResult
{
	RV_SUCCESS = 0,
	RV_UNKNOWN_ERROR,
	RV_BAD_ARGUMENTS,
	RV_UNABLE_OPEN,
	RV_UNABLE_TO_PARSE,
};

extern const std::string NOTHING_EXTRA;

class LLAppAppearanceUtility;
class LLBakingProcess;

// Translate error status into error messages.
class LLAppException : public std::exception
{
public:
	LLAppException(EResult status_code, const std::string& extra = NOTHING_EXTRA);
	EResult getStatusCode() { return mStatusCode; }

private:
	void printErrorLLSD(const std::string& key, const std::string& message);
	EResult mStatusCode;
};


class LLAppAppearanceUtility : public LLApp
{
public:
	LLAppAppearanceUtility(int argc, char** argv);
	virtual ~LLAppAppearanceUtility();

	// LLApp interface.
	/*virtual*/ bool init();
	/*virtual*/ bool cleanup();
	/*virtual*/ bool mainLoop();

private:
	// Option parsing.
	void verifyNoProcess();
	void parseArguments();
	void validateArguments();
	void initializeIO();
public:
	void usage(std::ostream& ostr);


private:
	int mArgc;
	char** mArgv;
	LLBakingProcess* mProcess;
	std::istream* mInput;
	std::ostream* mOutput;
	std::string mAppName;
	std::string mInputFilename;
	std::string mOutputFilename;
	LLUUID mAgentID;
	LLSD mInputData;
};


#endif /* LL_LLAPPAPPEARANCEUTILITY_H */

