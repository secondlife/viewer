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

#include "llfolderviewitem.h"
#include "llfolderviewmodel.h"

// Implementation of conversations list

class LLConversationItem;
class LLConversationItemSession;
class LLConversationItemParticipant;

typedef std::map<LLUUID, LLConversationItem*> conversations_items_map;
typedef std::map<LLUUID, LLFolderViewItem*> conversations_widgets_map;

// Conversation items: we hold a list of those and create an LLFolderViewItem widget for each  
// that we tuck into the mConversationsListPanel. 
class LLConversationItem : public LLFolderViewModelItemCommon
{
public:
	LLConversationItem(std::string display_name, const LLUUID& uuid, LLFolderViewModelInterface& root_view_model);
	LLConversationItem(const LLUUID& uuid, LLFolderViewModelInterface& root_view_model);
	LLConversationItem(LLFolderViewModelInterface& root_view_model);
	virtual ~LLConversationItem() {}

	// Stub those things we won't really be using in this conversation context
	virtual const std::string& getName() const { return mName; }
	virtual const std::string& getDisplayName() const { return mName; }
	virtual const std::string& getSearchableName() const { return mName; }
	virtual const LLUUID& getUUID() const { return mUUID; }
	virtual time_t getCreationDate() const { return 0; }
	virtual LLPointer<LLUIImage> getIcon() const { return NULL; }
	virtual LLPointer<LLUIImage> getOpenIcon() const { return getIcon(); }
	virtual LLFontGL::StyleFlags getLabelStyle() const { return LLFontGL::NORMAL; }
	virtual std::string getLabelSuffix() const { return LLStringUtil::null; }
	virtual BOOL isItemRenameable() const { return FALSE; }
	virtual BOOL renameItem(const std::string& new_name) { return FALSE; }
	virtual BOOL isItemMovable( void ) const { return FALSE; }
	virtual BOOL isItemRemovable( void ) const { return FALSE; }
	virtual BOOL isItemInTrash( void) const { return FALSE; }
	virtual BOOL removeItem() { return FALSE; }
	virtual void removeBatch(std::vector<LLFolderViewModelItem*>& batch) { }
	virtual void move( LLFolderViewModelItem* parent_listener ) { }
	virtual BOOL isItemCopyable() const { return FALSE; }
	virtual BOOL copyToClipboard() const { return FALSE; }
	virtual BOOL cutToClipboard() const { return FALSE; }
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

protected:
	std::string mName;	// Name of the session or the participant
	LLUUID mUUID;		// UUID of the session or the participant
};

class LLConversationItemSession : public LLConversationItem
{
public:
	LLConversationItemSession(std::string display_name, const LLUUID& uuid, LLFolderViewModelInterface& root_view_model);
	LLConversationItemSession(const LLUUID& uuid, LLFolderViewModelInterface& root_view_model);
	virtual ~LLConversationItemSession() {}
	
	void setSessionID(const LLUUID& session_id) { mUUID = session_id; }
	void addParticipant(LLConversationItemParticipant* participant);
	void removeParticipant(LLConversationItemParticipant* participant);
	void removeParticipant(const LLUUID& participant_id);
	void clearParticipants();
	LLConversationItemParticipant* findParticipant(const LLUUID& participant_id);

	void setParticipantIsMuted(const LLUUID& participant_id, bool is_muted);
	void setParticipantIsModerator(const LLUUID& participant_id, bool is_moderator);
	
	bool isLoaded() { return mIsLoaded; }
	
	void dumpDebugData();

private:
	bool mIsLoaded;		// true if at least one participant has been added to the session, false otherwise
};

class LLConversationItemParticipant : public LLConversationItem
{
public:
	LLConversationItemParticipant(std::string display_name, const LLUUID& uuid, LLFolderViewModelInterface& root_view_model);
	LLConversationItemParticipant(const LLUUID& uuid, LLFolderViewModelInterface& root_view_model);
	virtual ~LLConversationItemParticipant() {}
	
	bool isMuted() { return mIsMuted; }
	bool isModerator() {return mIsModerator; }
	void setIsMuted(bool is_muted) { mIsMuted = is_muted; }
	void setIsModerator(bool is_moderator) { mIsModerator = is_moderator; }
	
	void dumpDebugData();

private:
	bool mIsMuted;		// default is false
	bool mIsModerator;	// default is false
};

// We don't want to ever filter conversations but we need to declare that class to create a conversation view model.
// We just stubb everything for the moment.
class LLConversationFilter : public LLFolderViewFilter
{
public:
		
	enum ESortOrderType
	{
		SO_NAME = 0,						// Sort inventory by name
		SO_DATE = 0x1,						// Sort inventory by date
	};

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
		
	void 				setFilterCount(S32 count) { }
	S32 				getFilterCount() const { return 0; }
	void 				decrementFilterCount() { }
		
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
	LLConversationSort(U32 order = 0)
	:	mSortOrder(order),
	mByDate(false),
	mByName(false)
	{
		mByDate = (order & LLConversationFilter::SO_DATE);
		mByName = (order & LLConversationFilter::SO_NAME);
	}
	
	bool isByDate() const { return mByDate; }
	U32 getSortOrder() const { return mSortOrder; }
	
	bool operator()(const LLConversationItem* const& a, const LLConversationItem* const& b) const;
private:
	U32  mSortOrder;
	bool mByDate;
	bool mByName;
};

class LLConversationViewModel
:	public LLFolderViewModel<LLConversationSort, LLConversationItem, LLConversationItem, LLConversationFilter>
{
public:
	typedef LLFolderViewModel<LLConversationSort, LLConversationItem, LLConversationItem, LLConversationFilter> base_t;
	
	void sort(LLFolderViewFolder* folder) { } // *TODO : implement conversation sort
	bool contentsReady() { return true; }	// *TODO : we need to check that participants names are available somewhat
	bool startDrag(std::vector<LLFolderViewModelItem*>& items) { return false; } // We do not allow drag of conversation items
	
private:
};

#endif // LL_LLCONVERSATIONMODEL_H
