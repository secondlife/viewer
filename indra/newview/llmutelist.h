/** 
 * @file llmutelist.h
 * @brief Management of list of muted players
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_MUTELIST_H
#define LL_MUTELIST_H

#include "llstring.h"
#include "lluuid.h"
#include "llextendedstatus.h"

class LLViewerObject;
class LLMessageSystem;
class LLMuteListObserver;

// An entry in the mute list.
class LLMute
{
public:
	// Legacy mutes are BY_NAME and have null UUID.
	// EXTERNAL mutes are only processed through an external system (e.g. Voice) and not stored.
	enum EType { BY_NAME = 0, AGENT = 1, OBJECT = 2, GROUP = 3, EXTERNAL = 4, COUNT = 5 };
	
	// Bits in the mute flags.  For backwards compatibility (since any mute list entries that were created before the flags existed
	// will have a flags field of 0), some of the flags are "inverted".
	// Note that it's possible, through flags, to completely disable an entry in the mute list.  The code should detect this case
	// and remove the mute list entry instead.
	enum 
	{
		flagTextChat		= 0x00000001,		// If set, don't mute user's text chat
		flagVoiceChat		= 0x00000002,		// If set, don't mute user's voice chat
		flagParticles		= 0x00000004,		// If set, don't mute user's particles
		flagObjectSounds 	= 0x00000008,		// If set, mute user's object sounds
		
		flagAll				= 0x0000000F		// Mask of all currently defined flags
	};
	
	LLMute(const LLUUID& id, const std::string& name = std::string(), EType type = BY_NAME, U32 flags = 0);

	// Returns localized type name of muted item
	std::string getDisplayType() const;
	
public:
	LLUUID		mID;	// agent or object id
	std::string	mName;	// agent or object name, does not store last name "Resident"
	EType		mType;	// needed for UI display of existing mutes
	U32			mFlags;	// flags pertaining to this mute entry
};

class LLMuteList : public LLSingleton<LLMuteList>
{
	LLSINGLETON(LLMuteList);
	~LLMuteList();
	/*virtual*/ void cleanupSingleton();
public:
	// reasons for auto-unmuting a resident
	enum EAutoReason 
	{ 
		AR_IM = 0,			// agent IMed a muted resident
		AR_MONEY = 1,			// agent paid L$ to a muted resident
		AR_INVENTORY = 2,	// agent offered inventory to a muted resident
		AR_COUNT			// enum count
	};


	void addObserver(LLMuteListObserver* observer);
	void removeObserver(LLMuteListObserver* observer);

	// Add either a normal or a BY_NAME mute, for any or all properties.
	BOOL add(const LLMute& mute, U32 flags = 0);

	// Remove both normal and legacy mutes, for any or all properties.
	BOOL remove(const LLMute& mute, U32 flags = 0);
	BOOL autoRemove(const LLUUID& agent_id, const EAutoReason reason);
	
	// Name is required to test against legacy text-only mutes.
	BOOL isMuted(const LLUUID& id, const std::string& name = LLStringUtil::null, U32 flags = 0) const;

	// Workaround for username-based mute search, a lot of string conversions so use cautiously
    // Expects lower case username
	BOOL isMuted(const std::string& username, U32 flags = 0) const;

	// Alternate (convenience) form for places we don't need to pass the name, but do need flags
	BOOL isMuted(const LLUUID& id, U32 flags) const { return isMuted(id, LLStringUtil::null, flags); };
	
	static bool isLinden(const std::string& name);
	
	BOOL isLoaded() const { return mIsLoaded; }

	std::vector<LLMute> getMutes() const;
	
	// request the mute list
	void requestFromServer(const LLUUID& agent_id);

	// call this method on logout to save everything.
	void cache(const LLUUID& agent_id);

private:
	BOOL loadFromFile(const std::string& filename);
	BOOL saveToFile(const std::string& filename);

	void setLoaded();
	void notifyObservers();
	void notifyObserversDetailed(const LLMute &mute);

	void updateAdd(const LLMute& mute);
	void updateRemove(const LLMute& mute);

	// TODO: NULL out mute_id in database
	static void processMuteListUpdate(LLMessageSystem* msg, void**);
	static void processUseCachedMuteList(LLMessageSystem* msg, void**);

	static void onFileMuteList(void** user_data, S32 code, LLExtStat ext_status);
	void onAccountNameChanged(const LLUUID& id, const std::string& username);

private:
	struct compare_by_name
	{
		bool operator()(const LLMute& a, const LLMute& b) const
		{
			std::string name1 = a.mName;
			std::string name2 = b.mName;

			LLStringUtil::toUpper(name1);
			LLStringUtil::toUpper(name2);

			return name1 < name2;
		}
	};
	struct compare_by_id
	{
		bool operator()(const LLMute& a, const LLMute& b) const
		{
			return a.mID < b.mID;
		}
	};
	typedef std::set<LLMute, compare_by_id> mute_set_t;
	mute_set_t mMutes;
	typedef std::map<LLUUID, std::string> pending_names_t;
	pending_names_t mPendingAgentNameUpdates;

	typedef std::set<std::string> string_set_t;
	string_set_t mLegacyMutes;
	
	typedef std::set<LLMuteListObserver*> observer_set_t;
	observer_set_t mObservers;

	BOOL mIsLoaded;

	friend class LLDispatchEmptyMuteList;
};

class LLMuteListObserver
{
public:
	virtual ~LLMuteListObserver() { }
	virtual void onChange() = 0;
	virtual void onChangeDetailed(const LLMute& ) { }
};

class LLRenderMuteList : public LLSingleton<LLRenderMuteList>
{
    LLSINGLETON(LLRenderMuteList);
public:
	bool loadFromFile();
	bool saveToFile();
	S32 getSavedVisualMuteSetting(const LLUUID& agent_id);
	void saveVisualMuteSetting(const LLUUID& agent_id, S32 setting);

	S32 getVisualMuteDate(const LLUUID& agent_id);

	void addObserver(LLMuteListObserver* observer);
	void removeObserver(LLMuteListObserver* observer);

	std::map<LLUUID, S32> sVisuallyMuteSettingsMap;
	std::map<LLUUID, S32> sVisuallyMuteDateMap;

private:
	void notifyObservers();
	typedef std::set<LLMuteListObserver*> observer_set_t;
	observer_set_t mObservers;
};


#endif //LL_MUTELIST_H
