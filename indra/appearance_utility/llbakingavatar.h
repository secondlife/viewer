/**
 * @file llbakingavatar.h
 * @brief Declaration of LLBakingAvatar class which is a derivation of LLAvatarAppearance
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

#ifndef LL_LLBAKINGAVATAR_H
#define LL_LLBAKINGAVATAR_H

#include "llavatarappearance.h"

class LLBakingAvatar : public LLAvatarAppearance
{
	LOG_CLASS(LLBakingAvatar);

/********************************************************************************
 **                                                                            **
 **                    INITIALIZATION
 **/
public:
	LLBakingAvatar(LLWearableData* wearable_data);
	virtual ~LLBakingAvatar();

	static void initClass(); // initializes static members

/**                    Initialization
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    STATE
 **/
public:
	virtual bool 	isSelf() const { return true; }
	virtual BOOL	isValid() const { return TRUE; }
	virtual BOOL	isUsingBakedTextures() const { return TRUE; }

/**                    State
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    SKELETON
 **/

protected:
	virtual LLAvatarJoint*	createAvatarJoint();
	virtual LLAvatarJoint*	createAvatarJoint(S32 joint_num);
	virtual LLAvatarJointMesh*	createAvatarJointMesh();

};

#endif /* LL_LLBAKINGAVATAR_H */

