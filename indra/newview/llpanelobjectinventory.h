/** 
 * @file llpanelobjectinventory.h
 * @brief LLPanelObjectInventory class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLPANELOBJECTINVENTORY_H
#define LL_LLPANELOBJECTINVENTORY_H

#include "llvoinventorylistener.h"
#include "llpanel.h"
#include "llinventorypanel.h" // for LLFolderViewModelInventory

#include "llinventory.h"

class LLScrollContainer;
class LLFolderView;
class LLFolderViewFolder;
class LLViewerObject;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLPanelObjectInventory
//
// This class represents the panel used to view and control a
// particular task's inventory.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLPanelObjectInventory : public LLPanel, public LLVOInventoryListener
{
public:
    struct Params : public LLInitParam::Block<Params, LLPanel::Params>
    {
        Optional<bool> show_root_folder;

        Params()
            : show_root_folder("show_root_folder", true)
        {}
    };

	LLPanelObjectInventory(const Params&);
	virtual ~LLPanelObjectInventory();
	
	virtual BOOL postBuild();

	LLFolderViewModelInventory& getRootViewModel() { return mInventoryViewModel; }

	void doToSelected(const LLSD& userdata);
	
	void refresh();
	const LLUUID& getTaskUUID() { return mTaskUUID;}
	void clearInventoryTask();
	void removeSelectedItem();
	void startRenamingSelectedItem();

	LLFolderView* getRootFolder() const { return mFolders; }

	virtual void draw();
	virtual void deleteAllChildren();
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, EDragAndDropType cargo_type, void *cargo_data, EAcceptance *accept, std::string& tooltip_msg);
	
	/*virtual*/ void onFocusLost();
	/*virtual*/ void onFocusReceived();
	
	static void idle(void* user_data);

protected:
	void reset();
	/*virtual*/ void inventoryChanged(LLViewerObject* object,
								 LLInventoryObject::object_list_t* inventory,
								 S32 serial_num,
								 void* user_data);
	void updateInventory();
	void createFolderViews(LLInventoryObject* inventory_root, LLInventoryObject::object_list_t& contents);
	void createViewsForCategory(LLInventoryObject::object_list_t* inventory,
								LLInventoryObject* parent,
								LLFolderViewFolder* folder);
	void clearContents();
	LLFolderViewItem* getItemByID(const LLUUID& id);

	void addItemID( const LLUUID& id, LLFolderViewItem*   itemp );
	void removeItemID(const LLUUID& id);
	void clearItemIDs();

	BOOL			handleKeyHere( KEY key, MASK mask );
	BOOL			isSelectionRemovable();

private:
	std::map<LLUUID, LLFolderViewItem*> mItemMap;

	LLScrollContainer* mScroller;
	LLFolderView* mFolders;
	
	LLUUID mTaskUUID;
	LLUUID mAttachmentUUID;
	BOOL mHaveInventory; // 'Loading' label and used for initial request
	BOOL mIsInventoryEmpty; // 'Empty' label
	BOOL mInventoryNeedsUpdate; // for idle, set on changed callback
	LLFolderViewModelInventory	mInventoryViewModel;	
    bool mShowRootFolder;
};

#endif // LL_LLPANELOBJECTINVENTORY_H
