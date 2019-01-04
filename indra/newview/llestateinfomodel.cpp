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
#include "llviewerregion.h"
#include "llcorehttputil.h"

//=========================================================================
namespace 
{
	class LLDispatchEstateUpdateInfo : public LLDispatchHandler
	{
	public:
	    LLDispatchEstateUpdateInfo() {}
	    virtual ~LLDispatchEstateUpdateInfo() {}
	    virtual bool operator()(const LLDispatcher* dispatcher, const std::string& key, 
            const LLUUID& invoice, const sparam_t& strings)
	    {
	        // key = "estateupdateinfo"
	        // strings[0] = estate name
	        // strings[1] = str(owner_id)
	        // strings[2] = str(estate_id)
	        // strings[3] = str(estate_flags)
	        // strings[4] = str((S32)(sun_hour * 1024))
	        // strings[5] = str(parent_estate_id)
	        // strings[6] = str(covenant_id)
	        // strings[7] = str(covenant_timestamp)
	        // strings[8] = str(send_to_agent_only)
	        // strings[9] = str(abuse_email_addr)
	
	        LL_DEBUGS("ESTATEINFOM") << "Received estate update" << LL_ENDL;
	
	        // Update estate info model.
	        // This will call LLPanelEstateInfo::refreshFromEstate().
	        // *TODO: Move estate message handling stuff to llestateinfomodel.cpp.
	        LLEstateInfoModel::instance().updateEstateInfo(strings);
	
	        return true;
	    }
	};
	
	class LLDispatchSetEstateAccess : public LLDispatchHandler
	{
	public:
	    LLDispatchSetEstateAccess() {}
	    virtual ~LLDispatchSetEstateAccess() {}
	    virtual bool operator()(
	        const LLDispatcher* dispatcher, const std::string& key,
	        const LLUUID& invoice, const sparam_t& strings)
	    {
	        // key = "setaccess"
	        // strings[0] = str(estate_id)
	        // strings[1] = str(packed_access_lists)
	        // strings[2] = str(num allowed agent ids)
	        // strings[3] = str(num allowed group ids)
	        // strings[4] = str(num banned agent ids)
	        // strings[5] = str(num estate manager agent ids)
	        // strings[6] = bin(uuid)
	        // strings[7] = bin(uuid)
	        // strings[8] = bin(uuid)
	        // ...

            LLEstateInfoModel::instance().updateAccessInfo(strings);
	
	        return true;
	    }
	
	};

    class LLDispatchSetEstateExperience : public LLDispatchHandler
    {
    public:
        virtual bool operator()(const LLDispatcher* dispatcher, const std::string& key,
                const LLUUID& invoice, const sparam_t& strings)
        {
            // key = "setexperience"
            // strings[0] = str(estate_id)
            // strings[1] = str(send_to_agent_only)
            // strings[2] = str(num blocked)
            // strings[3] = str(num trusted)
            // strings[4] = str(num allowed)
            // strings[8] = bin(uuid) ...
            // ...

            LLEstateInfoModel::instance().updateExperienceInfo(strings);

            return true;
        }

    };

}

//=========================================================================
LLEstateInfoModel::LLEstateInfoModel():	
    mID(0),	
    mFlags(0),	
    mSunHour(0),
    mRegion(nullptr)
{
}

boost::signals2::connection LLEstateInfoModel::setUpdateCallback(const update_signal_t::slot_type& cb)
{
	return mUpdateSignal.connect(cb);
}

boost::signals2::connection LLEstateInfoModel::setUpdateAccessCallback(const update_flaged_signal_t::slot_type& cb)
{
    return mUpdateAccess.connect(cb);
}

boost::signals2::connection LLEstateInfoModel::setUpdateExperienceCallback(const update_signal_t::slot_type& cb)
{
    return mUpdateExperience.connect(cb);
}

boost::signals2::connection LLEstateInfoModel::setCommitCallback(const update_signal_t::slot_type& cb)
{
	return mCommitSignal.connect(cb);
}

void LLEstateInfoModel::setRegion(LLViewerRegion* region)
{
    if (region != mRegion)
    {
        mRegion = region;

        if (mRegion)
        {
	        nextInvoice();
	        sendEstateOwnerMessage("getinfo", strings_t());
        }
    }
}


void LLEstateInfoModel::clearRegion()
{
    mRegion = nullptr;
}

void LLEstateInfoModel::sendEstateInfo()
{
	if (!commitEstateInfoCaps())
	{
		// the caps method failed, try the old way
		nextInvoice();
		commitEstateInfoDataserver();
	}
}

void LLEstateInfoModel::updateEstateInfo(const strings_t& strings)
{
	// NOTE: LLDispatcher extracts strings with an extra \0 at the
	// end.  If we pass the std::string direct to the UI/renderer
	// it draws with a weird character at the end of the string.
	mName		= strings[0].c_str();
	mOwnerID	= LLUUID(strings[1].c_str());
	mID			= strtoul(strings[2].c_str(), NULL, 10);
	mFlags		= strtoul(strings[3].c_str(), NULL, 10);
	mSunHour	= ((F32)(strtod(strings[4].c_str(), NULL)))/1024.0f;

    LL_DEBUGS("ESTATEINFOM") << "Received estate info: "
		<< "is_sun_fixed = " << getUseFixedSun()
		<< ", sun_hour = " << getSunHour() << LL_ENDL;
    LL_DEBUGS("ESTATEINFOM") << getInfoDump() << LL_ENDL;

	// Update region owner.
	LLViewerRegion* regionp = gAgent.getRegion();
	regionp->setOwner(mOwnerID);

	// Let interested parties know that estate info has been updated.
	mUpdateSignal();
}

void LLEstateInfoModel::updateAccessInfo(const strings_t& strings)
{
    S32 index = 1;	// skip estate_id
    U32 access_flags = strtoul(strings[index++].c_str(), NULL, 10);
    S32 num_allowed_agents = strtol(strings[index++].c_str(), NULL, 10);
    S32 num_allowed_groups = strtol(strings[index++].c_str(), NULL, 10);
    S32 num_banned_agents = strtol(strings[index++].c_str(), NULL, 10);
    S32 num_estate_managers = strtol(strings[index++].c_str(), NULL, 10);

    // sanity ckecks
    if (num_allowed_agents > 0
        && !(access_flags & ESTATE_ACCESS_ALLOWED_AGENTS))
    {
        LL_WARNS("ESTATEINFOM") << "non-zero count for allowed agents, but no corresponding flag" << LL_ENDL;
    }
    if (num_allowed_groups > 0
        && !(access_flags & ESTATE_ACCESS_ALLOWED_GROUPS))
    {
        LL_WARNS("ESTATEINFOM") << "non-zero count for allowed groups, but no corresponding flag" << LL_ENDL;
    }
    if (num_banned_agents > 0
        && !(access_flags & ESTATE_ACCESS_BANNED_AGENTS))
    {
        LL_WARNS("ESTATEINFOM") << "non-zero count for banned agents, but no corresponding flag" << LL_ENDL;
    }
    if (num_estate_managers > 0
        && !(access_flags & ESTATE_ACCESS_MANAGERS))
    {
        LL_WARNS("ESTATEINFOM") << "non-zero count for managers, but no corresponding flag" << LL_ENDL;
    }

    // grab the UUID's out of the string fields
    if (access_flags & ESTATE_ACCESS_ALLOWED_AGENTS)
    {
        mAllowedAgents.clear();

        for (S32 i = 0; i < num_allowed_agents && i < ESTATE_MAX_ACCESS_IDS; i++)
        {
            LLUUID id;
            memcpy(id.mData, strings[index++].data(), UUID_BYTES);		/* Flawfinder: ignore */
            mAllowedAgents.insert(id);
        }
    }

    if (access_flags & ESTATE_ACCESS_ALLOWED_GROUPS)
    {
        mAllowedGroups.clear();

        for (S32 i = 0; i < num_allowed_groups && i < ESTATE_MAX_GROUP_IDS; i++)
        {
            LLUUID id;
            memcpy(id.mData, strings[index++].data(), UUID_BYTES);		/* Flawfinder: ignore */
            mAllowedGroups.insert(id);
        }
    }

    if (access_flags & ESTATE_ACCESS_BANNED_AGENTS)
    {
        mBannedAgents.clear();

        for (S32 i = 0; i < num_banned_agents && i < ESTATE_MAX_ACCESS_IDS; i++)
        {
            LLUUID id;
            memcpy(id.mData, strings[index++].data(), UUID_BYTES);		/* Flawfinder: ignore */
            mBannedAgents.insert(id);
        }
    }

    if (access_flags & ESTATE_ACCESS_MANAGERS)
    {
        mEstateManagers.clear();

        // There should be only ESTATE_MAX_MANAGERS people in the list, but if the database gets more (SL-46107) don't 
        // truncate the list unless it's really big.  Go ahead and show the extras so the user doesn't get confused, 
        // and they can still remove them.
        for (S32 i = 0; i < num_estate_managers && i < (ESTATE_MAX_MANAGERS * 4); i++)
        {
            LLUUID id;
            memcpy(id.mData, strings[index++].data(), UUID_BYTES);		/* Flawfinder: ignore */
            mEstateManagers.insert(id);
        }
    }

    // Update the buttons which may change based on the list contents but also needs to account for general access features.
    mUpdateAccess(access_flags);
}

void LLEstateInfoModel::updateExperienceInfo(const strings_t& strings)
{
    strings_t::const_iterator it = strings.begin();
    ++it; // U32 estate_id = strtol((*it).c_str(), NULL, 10);
    ++it; // U32 send_to_agent_only = strtoul((*(++it)).c_str(), NULL, 10);

    LLUUID id;
    S32 num_blocked = strtol((*(it++)).c_str(), NULL, 10);
    S32 num_trusted = strtol((*(it++)).c_str(), NULL, 10);
    S32 num_allowed = strtol((*(it++)).c_str(), NULL, 10);

    mExperienceAllowed.clear();
    mExperienceTrusted.clear();
    mExperienceBlocked.clear();

    while (num_blocked-- > 0)
    {
        memcpy(id.mData, (*(it++)).data(), UUID_BYTES);
        mExperienceBlocked.insert(id);
    }

    while (num_trusted-- > 0)
    {
        memcpy(id.mData, (*(it++)).data(), UUID_BYTES);
        mExperienceTrusted.insert(id);
    }

    while (num_allowed-- > 0)
    {
        memcpy(id.mData, (*(it++)).data(), UUID_BYTES);
        mExperienceAllowed.insert(id);
    }

    mUpdateExperience();
}

void LLEstateInfoModel::notifyCommit()
{
	mCommitSignal();
}

void LLEstateInfoModel::initSingleton()
{
    gMessageSystem->setHandlerFunc("EstateOwnerMessage", &processEstateOwnerRequest);

    //	name.assign("setowner");
    //	static LLDispatchSetEstateOwner set_owner;
    //	dispatch.addHandler(name, &set_owner);

    static LLDispatchEstateUpdateInfo estate_update_info;
    mDispatch.addHandler("estateupdateinfo", &estate_update_info);

    static LLDispatchSetEstateAccess set_access;
    mDispatch.addHandler("setaccess", &set_access);

    static LLDispatchSetEstateExperience set_experience;
    mDispatch.addHandler("setexperience", &set_experience);

}

void LLEstateInfoModel::sendEstateOwnerMessage(const std::string& request, const strings_t& strings)
{
    if (!mRegion)
    {
        LL_WARNS("ESTATEINFOM") << "No selected region." << LL_ENDL;
        return;
    }
    LLMessageSystem* msg(gMessageSystem);
    LLUUID invoice(LLEstateInfoModel::instance().getLastInvoice());

    LL_INFOS() << "Sending estate request '" << request << "'" << LL_ENDL;
    msg->newMessage("EstateOwnerMessage");
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used
    msg->nextBlock("MethodData");
    msg->addString("Method", request);
    msg->addUUID("Invoice", invoice);
    if (strings.empty())
    {
        msg->nextBlock("ParamList");
        msg->addString("Parameter", NULL);
    }
    else
    {
        strings_t::const_iterator it = strings.begin();
        strings_t::const_iterator end = strings.end();
        for (; it != end; ++it)
        {
            msg->nextBlock("ParamList");
            msg->addString("Parameter", *it);
        }
    }
    msg->sendReliable(mRegion->getHost());
}

//== PRIVATE STUFF ============================================================

// tries to send estate info using a cap; returns true if it succeeded
bool LLEstateInfoModel::commitEstateInfoCaps()
{
    if (!mRegion)
    {
        LL_WARNS("ESTATEINFOM") << "Attempt to update estate caps with no anchor region! Don't do that!" << LL_ENDL;
        return false;
    }
    std::string url = mRegion->getCapability("EstateChangeInfo");

	if (url.empty())
	{
        LL_WARNS("ESTATEINFOM") << "No EstateChangeInfo cap from region." << LL_ENDL;
		// whoops, couldn't find the cap, so bail out
		return false;
	}

    LLCoros::instance().launch("LLEstateInfoModel::commitEstateInfoCapsCoro",
        boost::bind(&LLEstateInfoModel::commitEstateInfoCapsCoro, this, url));

    return true;
}

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
    body["override_environment"] = getAllowEnvironmentOverride();

    body["invoice"] = getLastInvoice();

    LL_DEBUGS("ESTATEINFOM") << "Sending estate caps: "
        << "is_sun_fixed = " << getUseFixedSun()
        << ", sun_hour = " << getSunHour() << LL_ENDL;
    LL_DEBUGS("ESTATEINFOM") << body << LL_ENDL;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, body);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status)
    {
        LL_INFOS("ESTATEINFOM") << "Committed estate info" << LL_ENDL;
        LLEstateInfoModel::instance().notifyCommit();
    }
    else
    {
        LL_WARNS("ESTATEINFOM") << "Failed to commit estate info " << LL_ENDL;
    }
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
    if (!mRegion)
    {
        LL_WARNS("ESTATEINFOM") << "No selected region." << LL_ENDL;
        return;
    }
    LL_DEBUGS("ESTATEINFOM") << "Sending estate info: "
		<< "is_sun_fixed = " << getUseFixedSun()
		<< ", sun_hour = " << getSunHour() << LL_ENDL;
    LL_DEBUGS("ESTATEINFOM") << getInfoDump() << LL_ENDL;

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("EstateOwnerMessage");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used

	msg->nextBlock("MethodData");
	msg->addString("Method", "estatechangeinfo");
	msg->addUUID("Invoice", getLastInvoice());

	msg->nextBlock("ParamList");
	msg->addString("Parameter", getName());

	msg->nextBlock("ParamList");
	msg->addString("Parameter", llformat("%u", getFlags()));

	msg->nextBlock("ParamList");
	msg->addString("Parameter", llformat("%d", (S32) (getSunHour() * 1024.0f)));

    msg->sendReliable(mRegion->getHost());
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
    dump["override_environment"]  = getAllowEnvironmentOverride();

	std::stringstream dump_str;
	dump_str << dump;
	return dump_str.str();
}

// static
void LLEstateInfoModel::processEstateOwnerRequest(LLMessageSystem* msg, void**)
{
    // unpack the message
    std::string request;
    LLUUID invoice;
    LLDispatcher::sparam_t strings;
    LLDispatcher::unpackMessage(msg, request, invoice, strings);
    if (invoice != LLEstateInfoModel::instance().getLastInvoice())
    {
        LL_WARNS("ESTATEINFOM") << "Mismatched Estate message: " << request << LL_ENDL;
        return;
    }

    //dispatch the message
    LLEstateInfoModel::instance().mDispatch.dispatch(request, invoice, strings);
}

