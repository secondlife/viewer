/** 
 * @file llmutelist.h
 * @brief Management of list of muted players
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#ifndef LL_MUTELIST_H
#define LL_MUTELIST_H

#include "llstring.h"
#include "lluuid.h"

class LLViewerObject;
class LLMessageSystem;
class LLMuteListObserver;

// An entry in the mute list.
class LLMute
{
public:
	// Legacy mutes are BY_NAME and have null UUID.
	enum EType { BY_NAME = 0, AGENT = 1, OBJECT = 2, GROUP = 3, COUNT = 4 };
	
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

	// Returns name + suffix based on type
	// For example:  "James Tester (resident)"
	std::string getDisplayName() const;
	
	// Converts a UI name into just the agent or object name
	// For example: "James Tester (resident)" sets the name to "James Tester"
	// and the type to AGENT.
	void setFromDisplayName(const std::string& display_name);
	
public:
	LLUUID		mID;	// agent or object id
	std::string	mName;	// agent or object name
	EType		mType;	// needed for UI display of existing mutes
	U32			mFlags;	// flags pertaining to this mute entry
};

class LLMuteList : public LLSingleton<LLMuteList>
{
public:
	// reasons for auto-unmuting a resident
	enum EAutoReason 
	{ 
		AR_IM = 0,			// agent IMed a muted resident
		AR_MONEY = 1,			// agent paid L$ to a muted resident
		AR_INVENTORY = 2,	// agent offered inventory to a muted resident
		AR_COUNT			// enum count
	};

	LLMuteList();
	~LLMuteList();

	// Implemented locally so that we can perform some delayed initialization. 
	// Callers should be careful to call this one and not LLSingleton<LLMuteList>::getInstance()
	// which would circumvent that mechanism. -MG
	static LLMuteList* getInstance();

	void addObserver(LLMuteListObserver* observer);
	void removeObserver(LLMuteListObserver* observer);

	// Add either a normal or a BY_NAME mute, for any or all properties.
	BOOL add(const LLMute& mute, U32 flags = 0);

	// Remove both normal and legacy mutes, for any or all properties.
	BOOL remove(const LLMute& mute, U32 flags = 0);
	BOOL autoRemove(const LLUUID& agent_id, const EAutoReason reason, const std::string& first_name = LLStringUtil::null, const std::string& last_name = LLStringUtil::null);
	
	// Name is required to test against legacy text-only mutes.
	BOOL isMuted(const LLUUID& id, const std::string& name = LLStringUtil::null, U32 flags = 0) const;
	
	// Alternate (convenience) form for places we don't need to pass the name, but do need flags
	BOOL isMuted(const LLUUID& id, U32 flags) const { return isMuted(id, LLStringUtil::null, flags); };
	
	BOOL isLinden(const std::string& name) const;
	
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

	void updateAdd(const LLMute& mute);
	void updateRemove(const LLMute& mute);

	// TODO: NULL out mute_id in database
	static void processMuteListUpdate(LLMessageSystem* msg, void**);
	static void processUseCachedMuteList(LLMessageSystem* msg, void**);

	static void onFileMuteList(void** user_data, S32 code, LLExtStat ext_status);

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
};


#endif //LL_MUTELIST_H
