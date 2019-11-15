/**
* @file llagentbenefits.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llagentbenefits.h"

LLAgentBenefits::LLAgentBenefits():
	m_initalized(false),
	m_animated_object_limit(-1),
	m_animation_upload_cost(-1),
	m_attachment_limit(-1),
	m_group_membership_limit(-1),
	m_sound_upload_cost(-1),
	m_texture_upload_cost(-1)
{
}

LLAgentBenefits::~LLAgentBenefits()
{
}

// This could be extended to a template scheme or otherwise modified
// to support other types, if and when needed. Currently all fields
// the viewer cares about are integer.
bool get_required_S32(const LLSD& sd, const LLSD::String& key, S32& value)
{
	value = -1;
	if (sd.has(key))
	{
		value = sd[key].asInteger();
		return true;
	}

	LL_WARNS("Benefits") << "Missing required benefit field " << key << LL_ENDL;
	return false;
}

bool LLAgentBenefits::init(const LLSD& benefits_sd)
{
	LL_DEBUGS("Benefits") << "initializing benefits from " << benefits_sd << LL_ENDL;

	if (!get_required_S32(benefits_sd, "animated_object_limit", m_animated_object_limit))
	{
		return false;
	}
	if (!get_required_S32(benefits_sd, "animation_upload_cost", m_animation_upload_cost))
	{
		return false;
	}
	if (!get_required_S32(benefits_sd, "attachment_limit", m_attachment_limit))
	{
		return false;
	}
	if (!get_required_S32(benefits_sd, "create_group_cost", m_create_group_cost))
	{
		return false;
	}
	if (!get_required_S32(benefits_sd, "group_membership_limit", m_group_membership_limit))
	{
		return false;
	}
	if (!get_required_S32(benefits_sd, "sound_upload_cost", m_sound_upload_cost))
	{
		return false;
	}
	if (!get_required_S32(benefits_sd, "texture_upload_cost", m_texture_upload_cost))
	{
		return false;
	}

	// FIXME PREMIUM - either use this field or get rid of it
	m_initalized = true;
	return true;
}

S32 LLAgentBenefits::getAnimatedObjectLimit() const
{
	return m_animated_object_limit;
}

S32 LLAgentBenefits::getAnimationUploadCost() const
{
	return m_animation_upload_cost;
}

S32 LLAgentBenefits::getAttachmentLimit() const
{
	return m_attachment_limit;
}

S32 LLAgentBenefits::getCreateGroupCost() const
{
	return m_create_group_cost;
}

S32 LLAgentBenefits::getGroupMembershipLimit() const
{
	return m_group_membership_limit;
}

S32 LLAgentBenefits::getSoundUploadCost() const
{
	return m_sound_upload_cost;
}

S32 LLAgentBenefits::getTextureUploadCost() const
{
	return m_texture_upload_cost;
}
