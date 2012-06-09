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

#include "llfloater.h"
#include "llmultifloater.h"
#include "llavatarpropertiesprocessor.h"
#include "llgroupmgr.h"

#include "llfolderviewitem.h"
#include "llfoldervieweventlistener.h"

class LLButton;
class LLLayoutPanel;
class LLLayoutStack;
class LLTabContainer;

// CHUI-137 : Temporary implementation of conversations list
class LLConversationItem;

typedef std::list<LLConversationItem*> conversations_items_list_t;
typedef std::list<LLFolderViewItem*> conversations_widgets_list_t;

// Conversation items: we hold a list of those and create an LLFolderViewItem widget for each that we tuck 
// into the mConversationsListPanel. 
class LLConversationItem : public LLFolderViewEventListener
{
public:
	LLConversationItem(std::string name);
	virtual ~LLConversationItem() {}

	// Stub those things we won't really be using in this conversation context
	virtual const std::string& getName() const { return mName; }
	virtual const std::string& getDisplayName() const { return mName; }
	virtual const LLUUID& getUUID() const { return mUUID; }
	virtual time_t getCreationDate() const { return 0; }
	virtual PermissionMask getPermissionMask() const { return PERM_ALL; }
	virtual LLFolderType::EType getPreferredType() const { return LLFolderType::FT_NONE; }
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
	virtual void removeBatch(LLDynamicArray<LLFolderViewEventListener*>& batch) { }
	virtual void move( LLFolderViewEventListener* parent_listener ) { }
	virtual BOOL isItemCopyable() const { return FALSE; }
	virtual BOOL copyToClipboard() const { return FALSE; }
	virtual BOOL cutToClipboard() const { return FALSE; }
	virtual BOOL isClipboardPasteable() const { return FALSE; }
	virtual void pasteFromClipboard() { }
	virtual void pasteLinkFromClipboard() { }
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags) { }
	virtual BOOL isUpToDate() const { return TRUE; }
	virtual BOOL hasChildren() const { return FALSE; }
	virtual LLInventoryType::EType getInventoryType() const { return LLInventoryType::IT_NONE; }
	virtual LLWearableType::EType getWearableType() const { return LLWearableType::WT_NONE; }

	// The action callbacks (or so we think...)
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void openItem( void );
	virtual void closeItem( void );
	virtual void previewItem( void );
	virtual void selectItem(void);
	virtual void showProperties(void);
	
	// This method should be called when a drag begins. returns TRUE
	// if the drag can begin, otherwise FALSE.
	virtual BOOL startDrag(EDragAndDropType* type, LLUUID* id) const { return FALSE; }
	
	// This method will be called to determine if a drop can be
	// performed, and will set drop to TRUE if a drop is
	// requested. Returns TRUE if a drop is possible/happened,
	// otherwise FALSE.
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data,
							std::string& tooltip_msg) { return FALSE; }
private:
	std::string mName;
	const LLUUID mUUID;
};
	// CHUI-137 : End

class LLIMFloaterContainer : public LLMultiFloater
{
public:
	LLIMFloaterContainer(const LLSD& seed);
	virtual ~LLIMFloaterContainer();
	
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	void onCloseFloater(LLUUID& id);

	/*virtual*/ void addFloater(LLFloater* floaterp, 
								BOOL select_added_floater, 
								LLTabContainer::eInsertionPoint insertion_point = LLTabContainer::END);
	/*virtual*/ void removeFloater(LLFloater* floaterp);

	static LLFloater* getCurrentVoiceFloater();

	static LLIMFloaterContainer* findInstance();

	static LLIMFloaterContainer* getInstance();

	virtual void setMinimized(BOOL b);

	void collapseMessagesPane(bool collapse);
	
	LLFolderViewItem* createConversationItemWidget(LLConversationItem* item);

private:
	typedef std::map<LLUUID,LLFloater*> avatarID_panel_map_t;
	avatarID_panel_map_t mSessions;
	boost::signals2::connection mNewMessageConnection;

	/*virtual*/ void computeResizeLimits(S32& new_min_width, S32& new_min_height);

	void onNewMessageReceived(const LLSD& data);

	void onExpandCollapseButtonClicked();

	void collapseConversationsPane(bool collapse);

	void updateState(bool collapse, S32 delta_width);

	LLButton* mExpandCollapseBtn;
	LLLayoutPanel* mMessagesPane;
	LLLayoutPanel* mConversationsPane;
	LLLayoutStack* mConversationsStack;
	
	// CHUI-137 : Data
	LLPanel* mConversationsListPanel;	// The widget we add list item to (title of each conversation)
	conversations_items_list_t mConversationsItems;
	conversations_widgets_list_t mConversationsWidgets;
};

#endif // LL_LLIMFLOATERCONTAINER_H
