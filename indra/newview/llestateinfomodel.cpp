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
#include "llregionflags.h"
#include "message.h"

// viewer
#include "llagent.h"
#include "llfloaterregioninfo.h" // for invoice id
#include "llviewerregion.h"

#include "llcorehttputil.h"

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
    std::string url = gAgent.getRegion()->getCapability("EstateChangeInfo");

    if (url.empty())
    {
        LL_WARNS("EstateInfo") << "Unable to get URL for cap: EstateChangeInfo!!!" << LL_ENDL;
        // whoops, couldn't find the cap, so bail out
        return;
    }

    LLCoros::instance().launch("LLEstateInfoModel::commitEstateInfoCapsCoro",
        boost::bind(&LLEstateInfoModel::commitEstateInfoCapsCoro, this, url));

}

bool LLEstateInfoModel::getUseFixedSun()			const {	return getFlag(REGION_FLAGS_SUN_FIXED);				}
bool LLEstateInfoModel::getIsExternallyVisible()	const {	return getFlag(REGION_FLAGS_EXTERNALLY_VISIBLE);	}
bool LLEstateInfoModel::getAllowDirectTeleport()	const {	return getFlag(REGION_FLAGS_ALLOW_DIRECT_TELEPORT);	}
bool LLEstateInfoModel::getDenyAnonymous()			const {	return getFlag(REGION_FLAGS_DENY_ANONYMOUS); 		}
bool LLEstateInfoModel::getDenyAgeUnverified()		const {	return getFlag(REGION_FLAGS_DENY_AGEUNVERIFIED);	}
bool LLEstateInfoModel::getAllowVoiceChat()			const { return getFlag(REGION_FLAGS_ALLOW_VOICE); }
bool LLEstateInfoModel::getAllowAccessOverride()	const { return getFlag(REGION_FLAGS_ALLOW_ACCESS_OVERRIDE); }

void LLEstateInfoModel::setUseFixedSun(bool val)			{ setFlag(REGION_FLAGS_SUN_FIXED, 				val);	}
void LLEstateInfoModel::setIsExternallyVisible(bool val)	{ setFlag(REGION_FLAGS_EXTERNALLY_VISIBLE,		val);	}
void LLEstateInfoModel::setAllowDirectTeleport(bool val)	{ setFlag(REGION_FLAGS_ALLOW_DIRECT_TELEPORT,	val);	}
void LLEstateInfoModel::setDenyAnonymous(bool val)			{ setFlag(REGION_FLAGS_DENY_ANONYMOUS,			val);	}
void LLEstateInfoModel::setDenyAgeUnverified(bool val)		{ setFlag(REGION_FLAGS_DENY_AGEUNVERIFIED,		val);	}
void LLEstateInfoModel::setAllowVoiceChat(bool val)		    { setFlag(REGION_FLAGS_ALLOW_VOICE,				val);	}
void LLEstateInfoModel::setAllowAccessOverride(bool val)    { setFlag(REGION_FLAGS_ALLOW_ACCESS_OVERRIDE,   val);   }

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
	LL_DEBUGS() << getInfoDump() << LL_ENDL;

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

void LLEstateInfoModel::commitEstateInfoCapsCoro(std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("EstateChangeInfo", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD body;
    body["estate_name"] = getName();
    body["sun_hour"] = getSunHour();

    body["is_sun_fixed"] = getUseFixedSun();
    body["is_externally_visible"] = getIsExternallyVisible();
    body["allow_direct_teleport"] = getAllowDirectTeleport();
    body["deny_anonymous"] = getDenyAnonymous();
    body["deny_age_unverified"] = getDenyAgeUnverified();
    body["allow_voice_chat"] = getAllowVoiceChat();
    body["override_public_access"] = getAllowAccessOverride();

    body["invoice"] = LLFloaterRegionInfo::getLastInvoice();

    LL_DEBUGS("Windlight Sync") << "Sending estate caps: "
        << "is_sun_fixed = " << getUseFixedSun()
        << ", sun_hour = " << getSunHour() << LL_ENDL;
    LL_DEBUGS() << body << LL_ENDL;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, body);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status)
    {
        LL_INFOS() << "Committed estate info" << LL_ENDL;
        LLEstateInfoModel::instance().notifyCommit();
    }
    else
    {
        LL_WARNS() << "Failed to commit estate info " << LL_ENDL;
    }
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
    dump["override_public_access"] = getAllowAccessOverride();

	std::stringstream dump_str;
	dump_str << dump;
	return dump_str.str();
}
