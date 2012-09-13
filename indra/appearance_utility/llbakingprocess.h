/** 
 * @file llbakingprocess.h
 * @brief Declaration of LLBakingProcess interface.
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

#ifndef LL_LLBAKINGPROCESS_H
#define LL_LLBAKINGPROCESS_H

#include <ostream>

class LLAppAppearanceUtility;
class LLSD;

// Simple wrapper for various process modes.
class LLBakingProcess
{
private:
	// Hide default constructor.
	LLBakingProcess() {}
public:
	LLBakingProcess(LLAppAppearanceUtility* app) :
		mApp(app) {}
	virtual ~LLBakingProcess() {}

	//virtual std::string getProcessName() const = 0;
	virtual void process(LLSD& input, std::ostream& output) = 0;

protected:
	LLAppAppearanceUtility* mApp;
};


#endif /* LL_LLBAKINGPROCESS_H */

