/** 
 * @file llmutelist.h
 * @brief Management of list of muted players
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	
	LLMute(const LLUUID& id, const LLString& name = "", EType type = BY_NAME, U32 flags = 0) 
	: mID(id), mName(name), mType(type),mFlags(flags) { }

	// Returns name + suffix based on type
	// For example:  "James Tester (resident)"
	LLString getDisplayName() const;
	
	// Converts a UI name into just the agent or object name
	// For example: "James Tester (resident)" sets the name to "James Tester"
	// and the type to AGENT.
	void setFromDisplayName(const LLString& display_name);
	
public:
	LLUUID		mID;	// agent or object id
	LLString	mName;	// agent or object name
	EType		mType;	// needed for UI display of existing mutes
	U32			mFlags;	// flags pertaining to this mute entry
};

class LLMuteList
{
public:
	LLMuteList();
	~LLMuteList();

	void addObserver(LLMuteListObserver* observer);
	void removeObserver(LLMuteListObserver* observer);

	// Add either a normal or a BY_NAME mute, for any or all properties.
	BOOL add(const LLMute& mute, U32 flags = 0);

	// Remove both normal and legacy mutes, for any or all properties.
	BOOL remove(const LLMute& mute, U32 flags = 0);
	
	// Name is required to test against legacy text-only mutes.
	BOOL isMuted(const LLUUID& id, const LLString& name = LLString::null, U32 flags = 0) const;
	
	// Alternate (convenience) form for places we don't need to pass the name, but do need flags
	BOOL isMuted(const LLUUID& id, U32 flags) const { return isMuted(id, LLString::null, flags); };
	
	BOOL isLinden(const LLString& name) const;
	
	BOOL isLoaded() const { return mIsLoaded; }

	std::vector<LLMute> getMutes() const;
	
	// request the mute list
	void requestFromServer(const LLUUID& agent_id);

	// call this method on logout to save everything.
	void cache(const LLUUID& agent_id);

private:
	BOOL loadFromFile(const LLString& filename);
	BOOL saveToFile(const LLString& filename);

	void setLoaded();
	void notifyObservers();

	void updateAdd(const LLMute& mute);
	void updateRemove(const LLMute& mute);

	// TODO: NULL out mute_id in database
	static void processMuteListUpdate(LLMessageSystem* msg, void**);
	static void processUseCachedMuteList(LLMessageSystem* msg, void**);

	static void onFileMuteList(void** user_data, S32 code);

private:
	struct compare_by_name
	{
		bool operator()(const LLMute& a, const LLMute& b) const
		{
			return a.mName < b.mName;
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
	
	typedef std::set<LLString> string_set_t;
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

extern LLMuteList *gMuteListp;

#endif //LL_MUTELIST_H
