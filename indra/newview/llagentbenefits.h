/**
* @file llagentbenefits.h
*
* $LicenseInfo:firstyear=2019&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2019, Linden Research, Inc.
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

#ifndef LL_AGENTBENEFITS_H
#define LL_AGENTBENEFITS_H

#include "llsingleton.h"
#include "llsd.h"

class LLAgentBenefits: public LLSingleton<LLAgentBenefits>
{
	LLSINGLETON(LLAgentBenefits);
	~LLAgentBenefits();
	LOG_CLASS(LLAgentBenefits);

public:
	bool init(const LLSD& benefits_sd);

	S32 getAnimatedObjectLimit() const;
	S32 getAnimationUploadCost() const;
	S32 getAttachmentLimit() const;
	S32 getCreateGroupCost() const;
	S32 getGroupMembershipLimit() const;
	S32 getSoundUploadCost() const;
	S32 getTextureUploadCost() const;
	
private:
	S32 m_animated_object_limit;
	S32 m_animation_upload_cost;
	S32 m_attachment_limit;
	S32 m_create_group_cost;
	S32 m_group_membership_limit;
	S32 m_sound_upload_cost;
	S32 m_texture_upload_cost;

	bool m_initalized;
};

#endif
