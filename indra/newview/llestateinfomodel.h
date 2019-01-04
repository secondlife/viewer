/** 
 * @file llestateinfomodel.h
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

#ifndef LL_LLESTATEINFOMODEL_H
#define LL_LLESTATEINFOMODEL_H

class LLMessageSystem;

#include "llsingleton.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "lldispatcher.h"
#include "llregionflags.h"

class LLViewerRegion;
/**
 * Contains estate info, notifies interested parties of its changes.
 */
class LLEstateInfoModel : public LLSingleton<LLEstateInfoModel>
{
    LLSINGLETON_C11(LLEstateInfoModel);
	LOG_CLASS(LLEstateInfoModel);

public:
    typedef std::vector<std::string>            strings_t;
	typedef boost::signals2::signal<void()>     update_signal_t;
    typedef boost::signals2::signal<void(U32)>  update_flaged_signal_t;
    typedef boost::signals2::connection         connection_t;

    connection_t            setUpdateCallback(const update_signal_t::slot_type& cb); /// the model has been externally updated
    connection_t            setUpdateAccessCallback(const update_flaged_signal_t::slot_type& cb);
    connection_t            setUpdateExperienceCallback(const update_signal_t::slot_type& cb);
    connection_t            setCommitCallback(const update_signal_t::slot_type& cb); /// our changes have been applied

    void                    setRegion(LLViewerRegion* region);
    void                    clearRegion();
	void                    sendEstateInfo(); /// send estate info to the simulator

	// getters
    bool                    getUseFixedSun() const              { return getFlag(REGION_FLAGS_SUN_FIXED); }
    bool                    getIsExternallyVisible() const      { return getFlag(REGION_FLAGS_EXTERNALLY_VISIBLE); }
    bool                    getAllowDirectTeleport() const      { return getFlag(REGION_FLAGS_ALLOW_DIRECT_TELEPORT); }
    bool                    getDenyAnonymous() const            { return getFlag(REGION_FLAGS_DENY_ANONYMOUS); }
    bool                    getDenyAgeUnverified() const        { return getFlag(REGION_FLAGS_DENY_AGEUNVERIFIED); }
    bool                    getAllowVoiceChat() const           { return getFlag(REGION_FLAGS_ALLOW_VOICE); }
    bool                    getAllowAccessOverride() const      { return getFlag(REGION_FLAGS_ALLOW_ACCESS_OVERRIDE); }
    bool                    getAllowEnvironmentOverride() const { return getFlag(REGION_FLAGS_ALLOW_ENVIRONMENT_OVERRIDE); }

	const std::string&	    getName()					const { return mName; }
	const LLUUID&		    getOwnerID()				const { return mOwnerID; }
	U32					    getID()						const { return mID; }
	F32					    getSunHour()				const { return getUseFixedSun() ? mSunHour : 0.f; }

	// setters
    void                    setUseFixedSun(bool val)                { setFlag(REGION_FLAGS_SUN_FIXED, val); }
    void                    setIsExternallyVisible(bool val)        { setFlag(REGION_FLAGS_EXTERNALLY_VISIBLE, val); }
    void                    setAllowDirectTeleport(bool val)        { setFlag(REGION_FLAGS_ALLOW_DIRECT_TELEPORT, val); }
    void                    setDenyAnonymous(bool val)              { setFlag(REGION_FLAGS_DENY_ANONYMOUS, val); }
    void                    setDenyAgeUnverified(bool val)          { setFlag(REGION_FLAGS_DENY_AGEUNVERIFIED, val); }
    void                    setAllowVoiceChat(bool val)             { setFlag(REGION_FLAGS_ALLOW_VOICE, val); }
    void                    setAllowAccessOverride(bool val)        { setFlag(REGION_FLAGS_ALLOW_ACCESS_OVERRIDE, val); }
    void                    setAllowEnvironmentOverride(bool val)   { setFlag(REGION_FLAGS_ALLOW_ENVIRONMENT_OVERRIDE, val); }

	void                    setSunHour(F32 sun_hour) { mSunHour = sun_hour; }

    const uuid_set_t &      getAllowedAgents() const                { return mAllowedAgents; }
    const uuid_set_t &      getAllowedGroups() const                { return mAllowedGroups; }
    const uuid_set_t &      getBannedAgents() const                 { return mBannedAgents; }
    const uuid_set_t &      getEstateManagers() const               { return mEstateManagers; }

    const uuid_set_t &      getAllowedExperiences() const           { return mExperienceAllowed; }
    const uuid_set_t &      getTrustedExperiences() const           { return mExperienceTrusted; }
    const uuid_set_t &      getBlockedExperiences() const           { return mExperienceBlocked; }

    void                    sendEstateOwnerMessage(const std::string& request, const strings_t& strings);

    //---------------------------------------------------------------------
    /// refresh model with data from the incoming server message
    void                    updateEstateInfo(const strings_t& strings);
    void                    updateAccessInfo(const strings_t& strings);
    void                    updateExperienceInfo(const strings_t& strings);

    const LLUUID &          getLastInvoice()    { return mRequestInvoice; }
    const LLUUID &          nextInvoice()       { mRequestInvoice.generate(); return mRequestInvoice; }

protected:

	void                    notifyCommit();

    virtual void            initSingleton() override;

private:
	bool                    commitEstateInfoCaps();
	void                    commitEstateInfoDataserver();
	inline bool             getFlag(U64 flag) const;
	inline void             setFlag(U64 flag, bool val);
	U64                     getFlags() const { return mFlags; }
	std::string             getInfoDump();

	// estate info
	std::string	            mName;			/// estate name
	LLUUID		            mOwnerID;		/// estate owner id
	U32			            mID;			/// estate id
	U64			            mFlags;			/// estate flags
	F32			            mSunHour;		/// estate sun hour

    uuid_set_t              mAllowedAgents;
    uuid_set_t              mAllowedGroups;
    uuid_set_t              mBannedAgents;
    uuid_set_t              mEstateManagers;

    uuid_set_t              mExperienceAllowed;
    uuid_set_t              mExperienceTrusted;
    uuid_set_t              mExperienceBlocked;

	update_signal_t         mUpdateSignal; /// emitted when we receive update from sim
    update_flaged_signal_t  mUpdateAccess;
    update_signal_t         mUpdateExperience;
	update_signal_t         mCommitSignal; /// emitted when our update gets applied to sim

    LLDispatcher            mDispatch;
    LLUUID                  mRequestInvoice;
    LLViewerRegion*         mRegion;

    void                    commitEstateInfoCapsCoro(std::string url);

    static void             processEstateOwnerRequest(LLMessageSystem* msg, void**);

};

inline bool LLEstateInfoModel::getFlag(U64 flag) const
{
	return ((mFlags & flag) != 0);
}

inline void LLEstateInfoModel::setFlag(U64 flag, bool val)
{
	if (val)
	{
		mFlags |= flag;
	}
	else
	{
		mFlags &= ~flag;
	}
}


#endif // LL_LLESTATEINFOMODEL_H
