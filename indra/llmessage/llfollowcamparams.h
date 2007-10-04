/** 
 * @file llfollowcamparams.h
 * @brief Follow camera parameters.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_FOLLOWCAM_PARAMS_H
#define LL_FOLLOWCAM_PARAMS_H


//Ventrella Follow Cam Script Stuff
enum EFollowCamAttributes {
	FOLLOWCAM_PITCH = 0,
	FOLLOWCAM_FOCUS_OFFSET,
	FOLLOWCAM_FOCUS_OFFSET_X, //this HAS to come after FOLLOWCAM_FOCUS_OFFSET in this list
	FOLLOWCAM_FOCUS_OFFSET_Y,
	FOLLOWCAM_FOCUS_OFFSET_Z,
	FOLLOWCAM_POSITION_LAG,	
	FOLLOWCAM_FOCUS_LAG,	
	FOLLOWCAM_DISTANCE,	
	FOLLOWCAM_BEHINDNESS_ANGLE,
	FOLLOWCAM_BEHINDNESS_LAG,
	FOLLOWCAM_POSITION_THRESHOLD,
	FOLLOWCAM_FOCUS_THRESHOLD,	
	FOLLOWCAM_ACTIVE,			
	FOLLOWCAM_POSITION,			
	FOLLOWCAM_POSITION_X, //this HAS to come after FOLLOWCAM_POSITION in this list
	FOLLOWCAM_POSITION_Y,
	FOLLOWCAM_POSITION_Z,
	FOLLOWCAM_FOCUS,
	FOLLOWCAM_FOCUS_X, //this HAS to come after FOLLOWCAM_FOCUS in this list
	FOLLOWCAM_FOCUS_Y,
	FOLLOWCAM_FOCUS_Z,
	FOLLOWCAM_POSITION_LOCKED,
	FOLLOWCAM_FOCUS_LOCKED,
	NUM_FOLLOWCAM_ATTRIBUTES
};

//end Ventrella

#endif //FOLLOWCAM_PARAMS_H
