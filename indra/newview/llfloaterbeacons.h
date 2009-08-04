/** 
 * @file llfloaterbeacons.h
 * @brief Front-end to LLPipeline controls for highlighting various kinds of objects.
 * @author Coco 
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLFLOATERBEACONS_H
#define LL_LLFLOATERBEACONS_H

#include "llfloater.h"

class LLFloaterBeacons : public LLFloater
{
	friend class LLFloaterReg;
	
public:

	/*virtual*/ BOOL postBuild();
	
	// Needed to make the floater visibility toggle the beacons.
	// Too bad we can't just add control_name="BeaconAlwaysOn" to the XML.
	void onClickUICheck(LLUICtrl *ctrl);

private:
	LLFloaterBeacons(const LLSD& seed);
};

#endif
