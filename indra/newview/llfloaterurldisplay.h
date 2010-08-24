/** 
 * @file llfloaterurldisplay.h
 * @brief LLFloaterURLDisplay class implementation
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

#ifndef LL_LLFLOATERURLDISPLAY_H
#define LL_LLFLOATERURLDISPLAY_H

#include "llfloater.h"
#include "v3math.h"

class LLPanelPlace;
class LLSD;
class LLUUID;

class LLFloaterURLDisplay : public LLFloater
{
	friend class LLFloaterReg;
public:

	void displayParcelInfo(U64 region_handle, const LLVector3& pos);
	void setSnapshotDisplay(const LLUUID& snapshot_id);
	void setName(const std::string& name);
	void setLocationString(const std::string& name);

	static void* createPlaceDetail(void* userdata);

private:
	LLFloaterURLDisplay(const LLSD& sd);
	virtual ~LLFloaterURLDisplay();

	LLVector3		mRegionPosition;
	U64				mRegionHandle;
	LLPanelPlace*	mPlacePanel;
};

#endif // LL_LLFLOATERURLDISPLAY_H
