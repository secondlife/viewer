/** 
 * @file llfloaterbeacons.h
 * @brief Front-end to LLPipeline controls for highlighting various kinds of objects.
 * @author Coco 
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLFLOATERBEACONS_H
#define LL_LLFLOATERBEACONS_H

#include "llfloater.h"

class LLFloaterBeacons : public LLFloater
{
	friend class LLFloaterReg;
	
public:

	bool postBuild() override;
	
	// Needed to make the floater visibility toggle the beacons.
	// Too bad we can't just add control_name="BeaconAlwaysOn" to the XML.
	void onClickUICheck(LLUICtrl *ctrl);

private:
	LLFloaterBeacons(const LLSD& seed);
};

#endif
