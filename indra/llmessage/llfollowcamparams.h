/** 
 * @file llfollowcamparams.h
 * @brief Follow camera parameters.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
