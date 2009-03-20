/** 
 * @file llcontrolgroupreader.h
 * @brief Interface providing readonly access to LLControlGroup (intended for unit testing)
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#ifndef LL_LLCONTROLGROUPREADER_H
#define LL_LLCONTROLGROUPREADER_H

#include "stdtypes.h"
#include <string>

// Many of the types below are commented out because for the purposes of the early testing we're doing,
// we don't need them and we don't want to pull in all the machinery to support them.
// But the model is here for future unit test extensions.

class LLControlGroupReader
{
public:
	LLControlGroupReader() {}
	virtual ~LLControlGroupReader() {}

	virtual std::string 	getString(const std::string& name) = 0;
	//virtual LLWString	getWString(const std::string& name) = 0;
	virtual std::string	getText(const std::string& name) = 0;
	//virtual LLVector3	getVector3(const std::string& name) = 0;
	//virtual LLVector3d	getVector3d(const std::string& name) = 0;
	//virtual LLRect		getRect(const std::string& name) = 0;
	virtual BOOL		getBOOL(const std::string& name) = 0;
	virtual S32			getS32(const std::string& name) = 0;
	virtual F32			getF32(const std::string& name) = 0;
	virtual U32			getU32(const std::string& name) = 0;
	//virtual LLSD        getLLSD(const std::string& name) = 0;

	//virtual LLColor4	getColor(const std::string& name) = 0;
	//virtual LLColor4U	getColor4U(const std::string& name) = 0;
	//virtual LLColor4	getColor4(const std::string& name) = 0;
	//virtual LLColor3	getColor3(const std::string& name) = 0;
};

#endif /* LL_LLCONTROLGROUPREADER_H */







