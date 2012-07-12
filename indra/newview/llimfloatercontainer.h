/** 
 * @file llimfloatercontainer.h
 * @brief Multifloater containing active IM sessions in separate tab container tabs
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

#ifndef LL_LLIMFLOATERCONTAINER_H
#define LL_LLIMFLOATERCONTAINER_H

#include <map>
#include <vector>

#include "llimview.h"
#include "llfloater.h"
#include "llmultifloater.h"
#include "llavatarpropertiesprocessor.h"
#include "llgroupmgr.h"

#include "llfolderviewitem.h"
#include "llfolderviewmodel.h"

class LLButton;
class LLLayoutPanel;
class LLLayoutStack;
class LLTabContainer;
class LLIMFloaterContainer;

// CHUI-137 : Temporary implementation of conversations list
class LLConversationItem;

typedef std::map<LLFloater*, LLConversationItem*> conversations_items_map;
typedef std::map<LLFloater*, LLFolderViewItem*> conversations_widgets_map;

// Conversation items: we hold a list of those and create an LLFolderViewItem widget for each  
// that we tuck into the mConversationsListPanel. 
class LLConversationItem : public LLFolderViewModelItemCommon
{
public:
	LLConversationItem(std::string name, const LLUUID& uuid, LLFloater* floaterp, LLIMFloaterContainer* containerp);
	LLConversationItem();
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
	virtual bool filter( LLFolderViewFilter& filter) { return true; }
	virtual bool descendantsPassedFilter(S32 filter_generation = -1) { return true; }
	virtual void setPassedFilter(bool passed, bool passed_folder, S32 filter_generation) { }
	virtual bool passedFilter(S32 filter_generation = -1) { return true; }

	// The action callbacks
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void openItem( void );
	virtual void closeItem( void );
	virtual void previewItem( void );
	virtual void selectItem(void);
	virtual void showProperties(void);

	void setVisibleIfDetached(BOOL visible);
	
	// This method will be called to determine if a drop can be
	// performed, and will set drop to TRUE if a drop is
	// requested. 
	// Returns TRUE if a drop is possible/happened, FALSE otherwise.
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data,
							std::string& tooltip_msg) { return FALSE; }
	
	bool hasSameValues(std::string name, const LLUUID& uuid) { return ((name == mName) && (uuid == mUUID)); }
	bool hasSameValue(const LLUUID& uuid) { return (uuid == mUUID); }
private:
	std::string mName;
	const LLUUID mUUID;
    LLFloater* mFloater;
    LLIMFloaterContainer* mContainer;
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

// CHUI-137 : End

class LLIMFloaterContainer
	: public LLMultiFloater
	, public LLIMSessionObserver
{
public:
	LLIMFloaterContainer(const LLSD& seed);
	virtual ~LLIMFloaterContainer();
	
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void draw();
	/*virtual*/ void setVisible(BOOL visible);
	void onCloseFloater(LLUUID& id);

	/*virtual*/ void addFloater(LLFloater* floaterp, 
								BOOL select_added_floater, 
								LLTabContainer::eInsertionPoint insertion_point = LLTabContainer::END);

	/*virtual*/ void tabClose();

	static LLFloater* getCurrentVoiceFloater();

	static LLIMFloaterContainer* findInstance();

	static LLIMFloaterContainer* getInstance();

	virtual void setMinimized(BOOL b);

	void collapseMessagesPane(bool collapse);
	

	// LLIMSessionObserver observe triggers
	/*virtual*/ void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id) {};
	/*virtual*/ void sessionVoiceOrIMStarted(const LLUUID& session_id);
	/*virtual*/ void sessionRemoved(const LLUUID& session_id);
	/*virtual*/ void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id) {};

private:
	typedef std::map<LLUUID,LLFloater*> avatarID_panel_map_t;
	avatarID_panel_map_t mSessions;
	boost::signals2::connection mNewMessageConnection;

	/*virtual*/ void computeResizeLimits(S32& new_min_width, S32& new_min_height);

	void onNewMessageReceived(const LLSD& data);

	void onExpandCollapseButtonClicked();

	void collapseConversationsPane(bool collapse);

	void updateState(bool collapse, S32 delta_width);

	void onAddButtonClicked();
	void onAvatarPicked(const uuid_vec_t& ids);

	LLButton* mExpandCollapseBtn;
	LLLayoutPanel* mMessagesPane;
	LLLayoutPanel* mConversationsPane;
	LLLayoutStack* mConversationsStack;
	
	// Conversation list implementation
public:
	void removeConversationListItem(LLFloater* floaterp, bool change_focus = true);
	void addConversationListItem(std::string name, const LLUUID& uuid, LLFloater* floaterp);
	LLFloater* findConversationItem(LLUUID& uuid);
private:
	LLFolderViewItem* createConversationItemWidget(LLConversationItem* item);

	// Conversation list data
	LLPanel* mConversationsListPanel;	// This is the main widget we add conversation widget to
	conversations_items_map mConversationsItems;
	conversations_widgets_map mConversationsWidgets;
	LLConversationViewModel mConversationViewModel;
	LLFolderView* mConversationsRoot;
};

#endif // LL_LLIMFLOATERCONTAINER_H
