/** 
 * @file llavatarpropertiesprocessor.cpp
 * @brief LLAvatarPropertiesProcessor class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"	    
	    
#include "llavatarpropertiesprocessor.h"
#include "message.h"
#include "llagent.h"

void LLAvatarPropertiesProcessor::addObserver(const LLUUID& avatar_id, LLAvatarPropertiesObserver* observer)
{
	// Check if that observer is alredy in mObservers for that avatar_id
	observer_multimap_t::iterator it;

	// IAN BUG this should update the observer's UUID if this is a dupe - sent to PE
	it = mObservers.find(avatar_id);
	while (it != mObservers.end())
	{
		if (it->second == observer)
		{
			return;
		}
		else
		{
			++it;
		}
	}

	mObservers.insert(std::pair<LLUUID, LLAvatarPropertiesObserver*>(avatar_id, observer));
}

void LLAvatarPropertiesProcessor::removeObserver(const LLUUID& avatar_id, LLAvatarPropertiesObserver* observer)
{
	if (!observer)
	{
		return;
	}

	observer_multimap_t::iterator it;
	it = mObservers.find(avatar_id);
	while (it != mObservers.end())
	{
		if (it->second == observer)
		{
			mObservers.erase(it);
			break;
		}
		else
		{
			++it;
		}
	}
}


// IAN BUG - this is in no way linked to observers... problem?
void LLAvatarPropertiesProcessor::sendAvatarPropertiesRequest(const LLUUID& avatar_id)
{
	lldebugs << "LLAvatarPropertiesProcessor::sendAvatarPropertiesRequest()" << llendl; 
	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_AvatarPropertiesRequest);
	msg->nextBlockFast( _PREHASH_AgentData);
	msg->addUUIDFast(   _PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(   _PREHASH_AvatarID, avatar_id);
	gAgent.sendReliableMessage();

}

// static
void LLAvatarPropertiesProcessor::processAvatarPropertiesReply(LLMessageSystem *msg, void**)
{
	LLAvatarData avatar_data;

	msg->getUUIDFast(	_PREHASH_AgentData,		_PREHASH_AgentID, 	avatar_data.agent_id);
	msg->getUUIDFast(	_PREHASH_AgentData,		_PREHASH_AvatarID, 	avatar_data.avatar_id);
	msg->getUUIDFast(  	_PREHASH_PropertiesData,	_PREHASH_ImageID,	avatar_data.image_id);
	msg->getUUIDFast(  	_PREHASH_PropertiesData,	_PREHASH_FLImageID,	avatar_data.fl_image_id);
	msg->getUUIDFast(	_PREHASH_PropertiesData,	_PREHASH_PartnerID,	avatar_data.partner_id);
	msg->getStringFast(	_PREHASH_PropertiesData,	_PREHASH_AboutText,	avatar_data.about_text);
	msg->getStringFast(	_PREHASH_PropertiesData,	_PREHASH_FLAboutText,	avatar_data.fl_about_text);
	msg->getStringFast(	_PREHASH_PropertiesData,	_PREHASH_BornOn,	avatar_data.born_on);
	msg->getString(		"PropertiesData",		"ProfileURL",		avatar_data.profile_url);
	msg->getU32Fast(	_PREHASH_PropertiesData,	_PREHASH_Flags,		avatar_data.flags);

	avatar_data.caption_index = 0;

	S32 charter_member_size = 0;
	charter_member_size = msg->getSize("PropertiesData", "CharterMember");
	if(1 == charter_member_size)
	{
		msg->getBinaryData("PropertiesData", "CharterMember", &avatar_data.caption_index, 1);
	}
	else if(1 < charter_member_size)
	{
		msg->getString("PropertiesData", "CharterMember", avatar_data.caption_text);
	}

	LLAvatarPropertiesProcessor::observer_multimap_t observers = LLAvatarPropertiesProcessor::getInstance()->mObservers;

	observer_multimap_t::iterator oi = observers.find(avatar_data.avatar_id);
	observer_multimap_t::iterator end = observers.upper_bound(avatar_data.avatar_id);
	for (; oi != end; ++oi)
	{
		oi->second->processAvatarProperties(avatar_data);
	}
}

