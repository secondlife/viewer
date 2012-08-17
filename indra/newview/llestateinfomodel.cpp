/** 
 * @file llestateinfomodel.cpp
 * @brief Estate info model
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llestateinfomodel.h"

// libs
#include "llhttpclient.h"
#include "llregionflags.h"
#include "message.h"

// viewer
#include "llagent.h"
#include "llfloaterregioninfo.h" // for invoice id
#include "llviewerregion.h"

LLEstateInfoModel::LLEstateInfoModel()
:	mID(0)
,	mFlags(0)
,	mSunHour(0)
{
}

boost::signals2::connection LLEstateInfoModel::setUpdateCallback(const update_signal_t::slot_type& cb)
{
	return mUpdateSignal.connect(cb);
}

boost::signals2::connection LLEstateInfoModel::setCommitCallback(const update_signal_t::slot_type& cb)
{
	return mCommitSignal.connect(cb);
}

void LLEstateInfoModel::sendEstateInfo()
{
	if (!commitEstateInfoCaps())
	{
		// the caps method failed, try the old way
		LLFloaterRegionInfo::nextInvoice();
		commitEstateInfoDataserver();
	}
}

bool LLEstateInfoModel::getUseFixedSun()			const {	return getFlag(REGION_FLAGS_SUN_FIXED);				}
bool LLEstateInfoModel::getIsExternallyVisible()	const {	return getFlag(REGION_FLAGS_EXTERNALLY_VISIBLE);	}
bool LLEstateInfoModel::getAllowDirectTeleport()	const {	return getFlag(REGION_FLAGS_ALLOW_DIRECT_TELEPORT);	}
bool LLEstateInfoModel::getDenyAnonymous()			const {	return getFlag(REGION_FLAGS_DENY_ANONYMOUS); 		}
bool LLEstateInfoModel::getDenyAgeUnverified()		const {	return getFlag(REGION_FLAGS_DENY_AGEUNVERIFIED);	}
bool LLEstateInfoModel::getAllowVoiceChat()			const {	return getFlag(REGION_FLAGS_ALLOW_VOICE);			}

void LLEstateInfoModel::setUseFixedSun(bool val)			{ setFlag(REGION_FLAGS_SUN_FIXED, 				val);	}
void LLEstateInfoModel::setIsExternallyVisible(bool val)	{ setFlag(REGION_FLAGS_EXTERNALLY_VISIBLE,		val);	}
void LLEstateInfoModel::setAllowDirectTeleport(bool val)	{ setFlag(REGION_FLAGS_ALLOW_DIRECT_TELEPORT,	val);	}
void LLEstateInfoModel::setDenyAnonymous(bool val)			{ setFlag(REGION_FLAGS_DENY_ANONYMOUS,			val);	}
void LLEstateInfoModel::setDenyAgeUnverified(bool val)		{ setFlag(REGION_FLAGS_DENY_AGEUNVERIFIED,		val);	}
void LLEstateInfoModel::setAllowVoiceChat(bool val)			{ setFlag(REGION_FLAGS_ALLOW_VOICE,				val);	}

void LLEstateInfoModel::update(const strings_t& strings)
{
	// NOTE: LLDispatcher extracts strings with an extra \0 at the
	// end.  If we pass the std::string direct to the UI/renderer
	// it draws with a weird character at the end of the string.
	mName		= strings[0].c_str();
	mOwnerID	= LLUUID(strings[1].c_str());
	mID			= strtoul(strings[2].c_str(), NULL, 10);
	mFlags		= strtoul(strings[3].c_str(), NULL, 10);
	mSunHour	= ((F32)(strtod(strings[4].c_str(), NULL)))/1024.0f;

	LL_DEBUGS("Windlight Sync") << "Received estate info: "
		<< "is_sun_fixed = " << getUseFixedSun()
		<< ", sun_hour = " << getSunHour() << LL_ENDL;
	lldebugs << getInfoDump() << llendl;

	// Update region owner.
	LLViewerRegion* regionp = gAgent.getRegion();
	regionp->setOwner(mOwnerID);

	// Let interested parties know that estate info has been updated.
	mUpdateSignal();
}

void LLEstateInfoModel::notifyCommit()
{
	mCommitSignal();
}

//== PRIVATE STUFF ============================================================

class LLEstateChangeInfoResponder : public LLHTTPClient::Responder
{
public:

	// if we get a normal response, handle it here
	virtual void result(const LLSD& content)
	{
		llinfos << "Committed estate info" << llendl;
		LLEstateInfoModel::instance().notifyCommit();
	}

	// if we get an error response
	virtual void error(U32 status, const std::string& reason)
	{
		llwarns << "Failed to commit estate info (" << status << "): " << reason << llendl;
	}
};

// tries to send estate info using a cap; returns true if it succeeded
bool LLEstateInfoModel::commitEstateInfoCaps()
{
	std::string url = gAgent.getRegion()->getCapability("EstateChangeInfo");

	if (url.empty())
	{
		// whoops, couldn't find the cap, so bail out
		return false;
	}

	LLSD body;
	body["estate_name"          ] = getName();
	body["sun_hour"             ] = getSunHour();

	body["is_sun_fixed"         ] = getUseFixedSun();
	body["is_externally_visible"] = getIsExternallyVisible();
	body["allow_direct_teleport"] = getAllowDirectTeleport();
	body["deny_anonymous"       ] = getDenyAnonymous();
	body["deny_age_unverified"  ] = getDenyAgeUnverified();
	body["allow_voice_chat"     ] = getAllowVoiceChat();

	body["invoice"              ] = LLFloaterRegionInfo::getLastInvoice();

	LL_DEBUGS("Windlight Sync") << "Sending estate caps: "
		<< "is_sun_fixed = " << getUseFixedSun()
		<< ", sun_hour = " << getSunHour() << LL_ENDL;
	lldebugs << body << LL_ENDL;

	// we use a responder so that we can re-get the data after committing to the database
	LLHTTPClient::post(url, body, new LLEstateChangeInfoResponder);
    return true;
}

/* This is the old way of doing things, is deprecated, and should be
   deleted when the dataserver model can be removed */
// key = "estatechangeinfo"
// strings[0] = str(estate_id) (added by simulator before relay - not here)
// strings[1] = estate_name
// strings[2] = str(estate_flags)
// strings[3] = str((S32)(sun_hour * 1024.f))
void LLEstateInfoModel::commitEstateInfoDataserver()
{
	LL_DEBUGS("Windlight Sync") << "Sending estate info: "
		<< "is_sun_fixed = " << getUseFixedSun()
		<< ", sun_hour = " << getSunHour() << LL_ENDL;
	lldebugs << getInfoDump() << LL_ENDL;

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("EstateOwnerMessage");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used

	msg->nextBlock("MethodData");
	msg->addString("Method", "estatechangeinfo");
	msg->addUUID("Invoice", LLFloaterRegionInfo::getLastInvoice());

	msg->nextBlock("ParamList");
	msg->addString("Parameter", getName());

	msg->nextBlock("ParamList");
	msg->addString("Parameter", llformat("%u", getFlags()));

	msg->nextBlock("ParamList");
	msg->addString("Parameter", llformat("%d", (S32) (getSunHour() * 1024.0f)));

	gAgent.sendMessage();
}

std::string LLEstateInfoModel::getInfoDump()
{
	LLSD dump;
	dump["estate_name"          ] = getName();
	dump["sun_hour"             ] = getSunHour();

	dump["is_sun_fixed"         ] = getUseFixedSun();
	dump["is_externally_visible"] = getIsExternallyVisible();
	dump["allow_direct_teleport"] = getAllowDirectTeleport();
	dump["deny_anonymous"       ] = getDenyAnonymous();
	dump["deny_age_unverified"  ] = getDenyAgeUnverified();
	dump["allow_voice_chat"     ] = getAllowVoiceChat();

	std::stringstream dump_str;
	dump_str << dump;
	return dump_str.str();
}
