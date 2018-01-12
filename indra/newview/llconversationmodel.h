/** 
 * @file llconversationmodel.h
 * @brief Implementation of conversations list
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLCONVERSATIONMODEL_H
#define LL_LLCONVERSATIONMODEL_H

#include <boost/signals2.hpp>

#include "llavatarname.h"
#include "../llui/llfolderviewitem.h"
#include "../llui/llfolderviewmodel.h"
#include "llviewerfoldertype.h"

// Implementation of conversations list

class LLConversationItem;
class LLConversationItemSession;
class LLConversationItemParticipant;

typedef std::map<LLUUID, LLConversationItem*> conversations_items_map;
typedef std::map<LLUUID, LLFolderViewItem*> conversations_widgets_map;

typedef std::vector<std::string> menuentry_vec_t;

// Conversation items: we hold a list of those and create an LLFolderViewItem widget for each  
// that we tuck into the mConversationsListPanel. 
class LLConversationItem : public LLFolderViewModelItemCommon
{
public:
	enum EConversationType
	{
		CONV_UNKNOWN         = 0,
		CONV_PARTICIPANT     = 1,
		CONV_SESSION_NEARBY  = 2,	// The order counts here as it is used to sort sessions by type
		CONV_SESSION_1_ON_1  = 3,
		CONV_SESSION_AD_HOC  = 4,
		CONV_SESSION_GROUP   = 5,
		CONV_SESSION_UNKNOWN = 6
	};
	
	LLConversationItem(std::string display_name, const LLUUID& uuid, LLFolderViewModelInterface& root_view_model);
	LLConversationItem(const LLUUID& uuid, LLFolderViewModelInterface& root_view_model);
	LLConversationItem(LLFolderViewModelInterface& root_view_model);
	virtual ~LLConversationItem();

	// Stub those things we won't really be using in this conversation context
	virtual const std::string& getName() const { return mName; }
	virtual const std::string& getDisplayName() const { return mName; }
	virtual const std::string& getSearchableName() const { return mName; }
	virtual std::string getSearchableDescription() const { return LLStringUtil::null; }
	virtual std::string getSearchableCreatorName() const { return LLStringUtil::null; }
	virtual std::string getSearchableUUIDString() const {return LLStringUtil::null;}
	virtual const LLUUID& getUUID() const { return mUUID; }
	virtual time_t getCreationDate() const { return 0; }
	virtual LLPointer<LLUIImage> getIcon() const { return NULL; }
	virtual LLPointer<LLUIImage> getOpenIcon() const { return getIcon(); }
	virtual LLFontGL::StyleFlags getLabelStyle() const { return LLFontGL::NORMAL; }
	virtual std::string getLabelSuffix() const { return LLStringUtil::null; }
	virtual BOOL isItemRenameable() const { return TRUE; }
	virtual BOOL renameItem(const std::string& new_name) { mName = new_name; mNeedsRefresh = true; return TRUE; }
	virtual BOOL isItemMovable( void ) const { return FALSE; }
	virtual BOOL isItemRemovable( void ) const { return FALSE; }
	virtual BOOL isItemInTrash( void) const { return FALSE; }
	virtual BOOL removeItem() { return FALSE; }
	virtual void removeBatch(std::vector<LLFolderViewModelItem*>& batch) { }
	virtual void move( LLFolderViewModelItem* parent_listener ) { }
	virtual BOOL isItemCopyable() const { return FALSE; }
	virtual BOOL copyToClipboard() const { return FALSE; }
	virtual BOOL cutToClipboard() { return FALSE; }
	virtual BOOL isClipboardPasteable() const { return FALSE; }
	virtual void pasteFromClipboard() { }
	virtual void pasteLinkFromClipboard() { }
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags) { }
	virtual BOOL isUpToDate() const { return TRUE; }
	virtual bool hasChildren() const { return FALSE; }

	virtual bool potentiallyVisible() { return true; }
	virtual bool filter( LLFolderViewFilter& filter) { return false; }
	virtual bool descendantsPassedFilter(S32 filter_generation = -1) { return true; }
	virtual void setPassedFilter(bool passed, S32 filter_generation, std::string::size_type string_offset = std::string::npos, std::string::size_type string_size = 0) { }
	virtual bool passedFilter(S32 filter_generation = -1) { return true; }

	// The action callbacks
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void openItem( void );
	virtual void closeItem( void );
	virtual void previewItem( void );
	virtual void selectItem(void) { } 
	virtual void showProperties(void);

	// Methods used in sorting (see LLConversationSort::operator())
	EConversationType const getType() const { return mConvType; }
	virtual const bool getTime(F64& time) const { time = mLastActiveTime; return (time > 0.1); }
	virtual const bool getDistanceToAgent(F64& distance) const { return false; }
	
	// This method will be called to determine if a drop can be
	// performed, and will set drop to TRUE if a drop is
	// requested. 
	// Returns TRUE if a drop is possible/happened, FALSE otherwise.
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data,
							std::string& tooltip_msg) { return FALSE; }
	
//	bool hasSameValues(std::string name, const LLUUID& uuid) { return ((name == mName) && (uuid == mUUID)); }
	bool hasSameValue(const LLUUID& uuid) { return (uuid == mUUID); }
	
	void resetRefresh() { mNeedsRefresh = false; }
	bool needsRefresh() { return mNeedsRefresh; }
	
	void postEvent(const std::string& event_type, LLConversationItemSession* session, LLConversationItemParticipant* participant);
	
    void buildParticipantMenuOptions(menuentry_vec_t& items, U32 flags);

	void fetchAvatarName(bool isParticipant = true);		// fetch and update the avatar name

protected:
	virtual void onAvatarNameCache(const LLAvatarName& av_name) {}

	std::string mName;	// Name of the session or the participant
	LLUUID mUUID;		// UUID of the session or the participant
	EConversationType mConvType;	// Type of conversation item
	bool mNeedsRefresh;	// Flag signaling to the view that something changed for this item
	F64  mLastActiveTime;
	bool mDisplayModeratorOptions;
	bool mDisplayGroupBanOptions;
	boost::signals2::connection mAvatarNameCacheConnection;
};	

class LLConversationItemSession : public LLConversationItem
{
public:
	LLConversationItemSession(std::string display_name, const LLUUID& uuid, LLFolderViewModelInterface& root_view_model);
	LLConversationItemSession(const LLUUID& uuid, LLFolderViewModelInterface& root_view_model);
	
	/*virtual*/ bool hasChildren() const;
    LLPointer<LLUIImage> getIcon() const { return NULL; }
	void setSessionID(const LLUUID& session_id) { mUUID = session_id; mNeedsRefresh = true; }
	void addParticipant(LLConversationItemParticipant* participant);
	void updateName(LLConversationItemParticipant* participant);
	void removeParticipant(LLConversationItemParticipant* participant);
	void removeParticipant(const LLUUID& participant_id);
	void clearParticipants();
	LLConversationItemParticipant* findParticipant(const LLUUID& participant_id);

	void setParticipantIsMuted(const LLUUID& participant_id, bool is_muted);
	void setParticipantIsModerator(const LLUUID& participant_id, bool is_moderator);
	void setTimeNow(const LLUUID& participant_id);
	void setDistance(const LLUUID& participant_id, F64 dist);
	
	bool isLoaded() { return mIsLoaded; }
	
    void buildContextMenu(LLMenuGL& menu, U32 flags);
    void addVoiceOptions(menuentry_vec_t& items);
	virtual const bool getTime(F64& time) const;

	void dumpDebugData(bool dump_children = false);

private:
	/*virtual*/ void onAvatarNameCache(const LLAvatarName& av_name);

	bool mIsLoaded;		// true if at least one participant has been added to the session, false otherwise
};

class LLConversationItemParticipant : public LLConversationItem
{
public:
	LLConversationItemParticipant(std::string display_name, const LLUUID& uuid, LLFolderViewModelInterface& root_view_model);
	LLConversationItemParticipant(const LLUUID& uuid, LLFolderViewModelInterface& root_view_model);
	
	virtual const std::string& getDisplayName() const { return mDisplayName; }

	bool isVoiceMuted();
	bool isModerator() const { return mIsModerator; }
	void muteVoice(bool mute_voice);
	void setIsModerator(bool is_moderator) { mIsModerator = is_moderator; mNeedsRefresh = true; }
	void setTimeNow() { mLastActiveTime = LLFrameTimer::getElapsedSeconds(); mNeedsRefresh = true; }
	void setDistance(F64 dist) { mDistToAgent = dist; mNeedsRefresh = true; }

    void buildContextMenu(LLMenuGL& menu, U32 flags);

	virtual const bool getDistanceToAgent(F64& dist) const { dist = mDistToAgent; return (dist >= 0.0); }

	void updateName();	// get from the cache (do *not* fetch) and update the avatar name
	LLConversationItemSession* getParentSession();

	void dumpDebugData();
	void setModeratorOptionsVisible(bool visible) { mDisplayModeratorOptions = visible; }
	void setDisplayModeratorRole(bool displayRole);
	void setGroupBanVisible(bool visible) { mDisplayGroupBanOptions = visible; }

private:
	void onAvatarNameCache(const LLAvatarName& av_name);	// callback used by fetchAvatarName
	void updateName(const LLAvatarName& av_name);

	bool mIsModerator;	         // default is false
	bool mDisplayModeratorLabel; // default is false
	std::string mDisplayName;
	F64  mDistToAgent;  // Distance to the agent. A negative (meaningless) value means the distance has not been set.
	boost::signals2::connection mAvatarNameCacheConnection;
};

// We don't want to ever filter conversations but we need to declare that class to create a conversation view model.
// We just stubb everything for the moment.
class LLConversationFilter : public LLFolderViewFilter
{
public:
		
	enum ESortOrderType
	{
		SO_NAME = 0,						// Sort by name
		SO_DATE = 0x1,						// Sort by date (most recent)
		SO_SESSION_TYPE = 0x2,				// Sort by type (valid only for sessions)
		SO_DISTANCE = 0x3,					// Sort by distance (valid only for participants in nearby chat)
	};
	// Default sort order is by type for sessions and by date for participants
	static const U32 SO_DEFAULT = (SO_SESSION_TYPE << 16) | (SO_DATE);
	
	LLConversationFilter() { mEmpty = ""; }
	~LLConversationFilter() {}
		
	bool 				check(const LLFolderViewModelItem* item) { return true; }
	bool				checkFolder(const LLFolderViewModelItem* folder) const { return true; }
	void 				setEmptyLookupMessage(const std::string& message) { }
	std::string			getEmptyLookupMessage() const { return mEmpty; }
	bool				showAllResults() const { return true; }
	std::string::size_type getStringMatchOffset(LLFolderViewModelItem* item) const { return std::string::npos; }
	std::string::size_type getFilterStringSize() const { return 0; }
		
	bool 				isActive() const { return false; }
	bool 				isModified() const { return false; }
	void 				clearModified() { }
	const std::string& 	getName() const { return mEmpty; }
	const std::string& 	getFilterText() { return mEmpty; }
	void 				setModified(EFilterModified behavior = FILTER_RESTART) { }

  	void 				resetTime(S32 timeout) { }
    bool                isTimedOut() { return false; }
   
	bool 				isDefault() const { return true; }
	bool 				isNotDefault() const { return false; }
	void 				markDefault() { }
	void 				resetDefault() { }
		
	S32 				getCurrentGeneration() const { return 0; }
	S32 				getFirstSuccessGeneration() const { return 0; }
	S32 				getFirstRequiredGeneration() const { return 0; }
private:
	std::string mEmpty;
};

class LLConversationSort
{
public:
	LLConversationSort(U32 order = LLConversationFilter::SO_DEFAULT) : mSortOrder(order) { }
	
	// 16 LSB bits used for participants, 16 MSB bits for sessions
	U32 getSortOrderSessions() const { return ((mSortOrder >> 16) & 0xFFFF); }
	U32 getSortOrderParticipants() const { return (mSortOrder & 0xFFFF); }
	void setSortOrderSessions(LLConversationFilter::ESortOrderType session) { mSortOrder = ((session & 0xFFFF) << 16) | (mSortOrder & 0xFFFF); }
	void setSortOrderParticipants(LLConversationFilter::ESortOrderType participant) { mSortOrder = (mSortOrder & 0xFFFF0000) | (participant & 0xFFFF); }

	bool operator()(const LLConversationItem* const& a, const LLConversationItem* const& b) const;
	operator U32() const { return mSortOrder; }
private:
	// Note: we're treating this value as a sort order bitmask as done in other places in the code (e.g. inventory)
	U32  mSortOrder;
};

class LLConversationViewModel
:	public LLFolderViewModel<LLConversationSort, LLConversationItem, LLConversationItem, LLConversationFilter>
{
public:
	typedef LLFolderViewModel<LLConversationSort, LLConversationItem, LLConversationItem, LLConversationFilter> base_t;
	LLConversationViewModel() 
	:	base_t(new LLConversationSort(), new LLConversationFilter())
	{}
	
	void sort(LLFolderViewFolder* folder);
	bool contentsReady() { return true; }	// *TODO : we need to check that participants names are available somewhat
	bool startDrag(std::vector<LLFolderViewModelItem*>& items) { return false; } // We do not allow drag of conversation items
	
private:
};

// Utility function to hide all entries except those in the list
// Can be called multiple times on the same menu (e.g. if multiple items
// are selected).  If "append" is false, then only common enabled items
// are set as enabled.

//(defined in inventorybridge.cpp)
//TODO: Gilbert Linden - Refactor to make this function non-global
void hide_context_entries(LLMenuGL& menu, 
    const menuentry_vec_t &entries_to_show, 
    const menuentry_vec_t &disabled_entries);

#endif // LL_LLCONVERSATIONMODEL_H
