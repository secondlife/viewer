/** 
 * @file llfloaterregionrestarting.h
 * @brief Shows countdown timer during region restart
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLFLOATERREGIONRESTARTING_H
#define LL_LLFLOATERREGIONRESTARTING_H

#include "llfloater.h"
#include "lltextbox.h"
#include "lleventtimer.h"

class LLFloaterRegionRestarting : public LLFloater,  public LLEventTimer
{
	friend class LLFloaterReg;

public:
	static void close();
	static void updateTime(S32 time);

private:
	LLFloaterRegionRestarting(const LLSD& key);
	virtual ~LLFloaterRegionRestarting();
	virtual BOOL postBuild();
	virtual bool tick();
	virtual void refresh();
	virtual void draw();
	virtual void regionChange();

	std::string mName;
	U32 mShakeIterations;
	F32 mShakeMagnitude;
	LLTimer mShakeTimer;

	boost::signals2::connection mRegionChangedConnection;

	enum
	{
		SHAKE_START,
		SHAKE_LEFT,
		SHAKE_UP,
		SHAKE_RIGHT,
		SHAKE_DOWN,
		SHAKE_DONE
	};
};

#endif // LL_LLFLOATERREGIONRESTARTING_H
