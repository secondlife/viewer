/**
 * @file lluiavatar.cpp
 * @brief Implementation for special dummy avatar used in some UI views
 *
 * $LicenseInfo:firstyear=2017&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2017, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "lluiavatar.h"
#include "llagent.h" //  Get state values from here
#include "llviewerobjectlist.h"
#include "pipeline.h"
#include "llanimationstates.h"
#include "llviewercontrol.h"
#include "llmeshrepository.h"
#include "llviewerregion.h"

LLUIAvatar::LLUIAvatar(const LLUUID& id, const LLPCode pcode, LLViewerRegion* regionp) :
    LLVOAvatar(id, pcode, regionp)
{
    mIsDummy = TRUE;
    mIsUIAvatar = true;
}

// virtual
LLUIAvatar::~LLUIAvatar()
{
}

// virtual
void LLUIAvatar::initInstance()
{
    LLVOAvatar::initInstance();

    createDrawable( &gPipeline );
	setPositionAgent(LLVector3::zero);
	slamPosition();
	updateJointLODs();
	updateGeometry(mDrawable);

    mInitFlags |= 1<<3;
}
