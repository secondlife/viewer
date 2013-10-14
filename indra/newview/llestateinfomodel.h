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

/**
 * Contains estate info, notifies interested parties of its changes.
 */
class LLEstateInfoModel : public LLSingleton<LLEstateInfoModel>
{
	LOG_CLASS(LLEstateInfoModel);

public:
	typedef boost::signals2::signal<void()> update_signal_t;
	boost::signals2::connection setUpdateCallback(const update_signal_t::slot_type& cb); /// the model has been externally updated
	boost::signals2::connection setCommitCallback(const update_signal_t::slot_type& cb); /// our changes have been applied

	void sendEstateInfo(); /// send estate info to the simulator

	// getters
	bool				getUseFixedSun()			const;
	bool				getIsExternallyVisible()	const;
	bool				getAllowDirectTeleport()	const;
	bool				getDenyAnonymous()			const;
	bool				getDenyAgeUnverified()		const;
	bool				getAllowVoiceChat()			const;

	const std::string&	getName()					const { return mName; }
	const LLUUID&		getOwnerID()				const { return mOwnerID; }
	U32					getID()						const { return mID; }
	F32					getSunHour()				const { return getUseFixedSun() ? mSunHour : 0.f; }

	// setters
	void setUseFixedSun(bool val);
	void setIsExternallyVisible(bool val);
	void setAllowDirectTeleport(bool val);
	void setDenyAnonymous(bool val);
	void setDenyAgeUnverified(bool val);
	void setAllowVoiceChat(bool val);

	void setSunHour(F32 sun_hour) { mSunHour = sun_hour; }

protected:
	typedef std::vector<std::string> strings_t;

	friend class LLSingleton<LLEstateInfoModel>;
	friend class LLDispatchEstateUpdateInfo;
	friend class LLEstateChangeInfoResponder;

	LLEstateInfoModel();

	/// refresh model with data from the incoming server message
	void update(const strings_t& strings);

	void notifyCommit();

private:
	bool commitEstateInfoCaps();
	void commitEstateInfoDataserver();
	inline bool getFlag(U64 flag) const;
	inline void setFlag(U64 flag, bool val);
	U64  getFlags() const { return mFlags; }
	std::string getInfoDump();

	// estate info
	std::string	mName;			/// estate name
	LLUUID		mOwnerID;		/// estate owner id
	U32			mID;			/// estate id
	U64			mFlags;			/// estate flags
	F32			mSunHour;		/// estate sun hour

	update_signal_t mUpdateSignal; /// emitted when we receive update from sim
	update_signal_t mCommitSignal; /// emitted when our update gets applied to sim
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
