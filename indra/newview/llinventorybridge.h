/** 
 * @file llinventorybridge.h
 * @brief Implementation of the Inventory-Folder-View-Bridge classes.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLINVENTORYBRIDGE_H
#define LL_LLINVENTORYBRIDGE_H

#include "llcallingcard.h"
#include "llfloaterproperties.h"
#include "llfoldervieweventlistener.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llviewercontrol.h"
#include "llwearable.h"

class LLInventoryPanel;
class LLInventoryModel;
class LLMenuGL;
class LLCallingCardObserver;
class LLViewerJointAttachment;

typedef std::vector<std::string> menuentry_vec_t;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInvFVBridge
//
// Short for Inventory-Folder-View-Bridge. This is an
// implementation class to be able to view inventory items.
//
// You'll want to call LLInvItemFVELister::createBridge() to actually create
// an instance of this class. This helps encapsulate the
// funcationality a bit. (except for folders, you can create those
// manually...)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInvFVBridge : public LLFolderViewEventListener
{
public:
	// This method is a convenience function which creates the correct
	// type of bridge based on some basic information
	static LLInvFVBridge* createBridge(LLAssetType::EType asset_type,
									   LLAssetType::EType actual_asset_type,
									   LLInventoryType::EType inv_type,
									   LLInventoryPanel* inventory,
									   LLFolderView* root,
									   const LLUUID& uuid,
									   U32 flags = 0x00);
	virtual ~LLInvFVBridge() {}

	virtual const LLUUID& getUUID() const { return mUUID; }

	virtual void restoreItem() {}
	virtual void restoreToWorld() {}

	// LLFolderViewEventListener functions
	virtual const std::string& getName() const;
	virtual const std::string& getDisplayName() const;
	virtual PermissionMask getPermissionMask() const;
	virtual LLFolderType::EType getPreferredType() const;
	virtual time_t getCreationDate() const;
	virtual LLFontGL::StyleFlags getLabelStyle() const
	{
		return LLFontGL::NORMAL;
	}
	virtual std::string getLabelSuffix() const { return LLStringUtil::null; }
	virtual void openItem() {}
	virtual void closeItem() {}
	virtual void previewItem() {openItem();}
	virtual void showProperties();
	virtual BOOL isItemRenameable() const { return TRUE; }
	//virtual BOOL renameItem(const std::string& new_name) {}
	virtual BOOL isItemRemovable() const;
	virtual BOOL isItemMovable() const;
	virtual BOOL isItemInTrash() const;
	virtual BOOL isLink() const;

	//virtual BOOL removeItem() = 0;
	virtual void removeBatch(LLDynamicArray<LLFolderViewEventListener*>& batch);
	virtual void move(LLFolderViewEventListener* new_parent_bridge) {}
	virtual BOOL isItemCopyable() const { return FALSE; }
	virtual BOOL copyToClipboard() const { return FALSE; }
	virtual void cutToClipboard();
	virtual BOOL isClipboardPasteable() const;
	virtual BOOL isClipboardPasteableAsLink() const;
	virtual void pasteFromClipboard() {}
	virtual void pasteLinkFromClipboard() {}
	void getClipboardEntries(bool show_asset_id, menuentry_vec_t &items, 
							 menuentry_vec_t &disabled_items, U32 flags);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL startDrag(EDragAndDropType* type, LLUUID* id) const;
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data) { return FALSE; }
	virtual LLInventoryType::EType getInventoryType() const { return mInvType; }

	// LLInvFVBridge functionality
	virtual void clearDisplayName() {}

	// Allow context menus to be customized for side panel.
	bool isInOutfitsSidePanel() const;

	bool canShare();

	//--------------------------------------------------------------------
	// Convenience functions for adding various common menu options.
	//--------------------------------------------------------------------
protected:
	virtual void addTrashContextMenuOptions(menuentry_vec_t &items,
											menuentry_vec_t &disabled_items);
	virtual void addDeleteContextMenuOptions(menuentry_vec_t &items,
											 menuentry_vec_t &disabled_items);
	virtual void addOpenRightClickMenuOption(menuentry_vec_t &items);
protected:
	LLInvFVBridge(LLInventoryPanel* inventory, LLFolderView* root, const LLUUID& uuid);

	LLInventoryObject* getInventoryObject() const;
	LLInventoryModel* getInventoryModel() const;
	
	BOOL isLinkedObjectInTrash() const; // Is this obj or its baseobj in the trash?
	BOOL isLinkedObjectMissing() const; // Is this a linked obj whose baseobj is not in inventory?

	BOOL isAgentInventory() const; // false if lost or in the inventory library
	BOOL isCOFFolder() const; // true if COF or descendent of.
	virtual BOOL isItemPermissive() const;
	static void changeItemParent(LLInventoryModel* model,
								 LLViewerInventoryItem* item,
								 const LLUUID& new_parent,
								 BOOL restamp);
	static void changeCategoryParent(LLInventoryModel* model,
									 LLViewerInventoryCategory* item,
									 const LLUUID& new_parent,
									 BOOL restamp);
	void removeBatchNoCheck(LLDynamicArray<LLFolderViewEventListener*>& batch);
protected:
	LLHandle<LLPanel> mInventoryPanel;
	LLFolderView* mRoot;
	const LLUUID mUUID;	// item id
	LLInventoryType::EType mInvType;
	BOOL mIsLink;
	void purgeItem(LLInventoryModel *model, const LLUUID &uuid);
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInvFVBridge
//
// This class intended to build Folder View Bridge via LLInvFVBridge::createBridge.
// It can be overridden with another way of creation necessary Inventory-Folder-View-Bridge.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryFVBridgeBuilder
{
public:
 	virtual ~LLInventoryFVBridgeBuilder() {}
	virtual LLInvFVBridge* createBridge(LLAssetType::EType asset_type,
										LLAssetType::EType actual_asset_type,
										LLInventoryType::EType inv_type,
										LLInventoryPanel* inventory,
										LLFolderView* root,
										const LLUUID& uuid,
										U32 flags = 0x00) const;
};

class LLItemBridge : public LLInvFVBridge
{
public:
	LLItemBridge(LLInventoryPanel* inventory, 
				 LLFolderView* root,
				 const LLUUID& uuid) :
		LLInvFVBridge(inventory, root, uuid) {}

	virtual void performAction(LLInventoryModel* model, std::string action);

	virtual void selectItem();
	virtual void restoreItem();
	virtual void restoreToWorld();
	virtual void gotoItem();
	virtual LLUIImagePtr getIcon() const;
	virtual const std::string& getDisplayName() const;
	virtual std::string getLabelSuffix() const;
	virtual LLFontGL::StyleFlags getLabelStyle() const;
	virtual PermissionMask getPermissionMask() const;
	virtual time_t getCreationDate() const;
	virtual BOOL isItemRenameable() const;
	virtual BOOL renameItem(const std::string& new_name);
	virtual BOOL removeItem();
	virtual BOOL isItemCopyable() const;
	virtual BOOL copyToClipboard() const;
	virtual BOOL hasChildren() const { return FALSE; }
	virtual BOOL isUpToDate() const { return TRUE; }

	// override for LLInvFVBridge
	virtual void clearDisplayName() { mDisplayName.clear(); }

	LLViewerInventoryItem* getItem() const;
	
	bool isAddAction(std::string action) const;
	bool isRemoveAction(std::string action) const;

protected:
	virtual BOOL isItemPermissive() const;
	static void buildDisplayName(LLInventoryItem* item, std::string& name);
	mutable std::string mDisplayName;

	BOOL confirmRemoveItem(const LLSD& notification, const LLSD& response);
};

class LLFolderBridge : public LLInvFVBridge
{
	friend class LLInvFVBridge;
public:
	BOOL dragItemIntoFolder(LLInventoryItem* inv_item,
							BOOL drop);
	BOOL dragCategoryIntoFolder(LLInventoryCategory* inv_category,
								BOOL drop);
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void openItem();
	virtual void closeItem();
	virtual BOOL isItemRenameable() const;
	virtual void selectItem();
	virtual void restoreItem();

	virtual LLFolderType::EType getPreferredType() const;
	virtual LLUIImagePtr getIcon() const;
	virtual LLUIImagePtr getOpenIcon() const;
	static LLUIImagePtr getIcon(LLFolderType::EType preferred_type, BOOL is_link = FALSE);

	virtual BOOL renameItem(const std::string& new_name);

	virtual BOOL removeItem();
	BOOL removeSystemFolder();
	bool removeItemResponse(const LLSD& notification, const LLSD& response);

	virtual void pasteFromClipboard();
	virtual void pasteLinkFromClipboard();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL hasChildren() const;
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data);

	virtual BOOL isItemRemovable() const;
	virtual BOOL isItemMovable() const ;
	virtual BOOL isUpToDate() const;
	virtual BOOL isItemCopyable() const;
	virtual BOOL isClipboardPasteable() const;
	virtual BOOL isClipboardPasteableAsLink() const;
	virtual BOOL copyToClipboard() const;
	
	static void createWearable(LLFolderBridge* bridge, LLWearableType::EType type);
	static void createWearable(const LLUUID &parent_folder_id, LLWearableType::EType type);

	LLViewerInventoryCategory* getCategory() const;

protected:
	LLFolderBridge(LLInventoryPanel* inventory, 
				   LLFolderView* root,
				   const LLUUID& uuid) :
		LLInvFVBridge(inventory, root, uuid),
		mCallingCards(FALSE),
		mWearables(FALSE),
		mMenu(NULL) {}

	// menu callbacks
	static void pasteClipboard(void* user_data);
	static void createNewCategory(void* user_data);

	static void createNewShirt(void* user_data);
	static void createNewPants(void* user_data);
	static void createNewShoes(void* user_data);
	static void createNewSocks(void* user_data);
	static void createNewJacket(void* user_data);
	static void createNewSkirt(void* user_data);
	static void createNewGloves(void* user_data);
	static void createNewUndershirt(void* user_data);
	static void createNewUnderpants(void* user_data);
	static void createNewShape(void* user_data);
	static void createNewSkin(void* user_data);
	static void createNewHair(void* user_data);
	static void createNewEyes(void* user_data);

	BOOL checkFolderForContentsOfType(LLInventoryModel* model, LLInventoryCollectFunctor& typeToCheck);
	BOOL areAnyContentsWorn(LLInventoryModel* model) const;

	void modifyOutfit(BOOL append);
	void determineFolderType();

	/**
	 * Returns a copy of current menu items.
	 */
	menuentry_vec_t getMenuItems() { return mItems; }

public:
	static LLFolderBridge* sSelf;
	static void staticFolderOptionsMenu();
	void folderOptionsMenu();
private:
	BOOL			mCallingCards;
	BOOL			mWearables;
	LLMenuGL*		mMenu;
	menuentry_vec_t mItems;
	menuentry_vec_t mDisabledItems;
};

class LLTextureBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual void performAction(LLInventoryModel* model, std::string action);

protected:
	LLTextureBridge(LLInventoryPanel* inventory, 
					LLFolderView* root,
					const LLUUID& uuid, 
					LLInventoryType::EType type) :
		LLItemBridge(inventory, root, uuid),
		mInvType(type) 
	{}
	bool canSaveTexture(void);
	LLInventoryType::EType mInvType;
};

class LLSoundBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual void openItem();
	virtual void previewItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	static void openSoundPreview(void*);

protected:
	LLSoundBridge(LLInventoryPanel* inventory, 
				  LLFolderView* root,
				  const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
};

class LLLandmarkBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();

protected:
	LLLandmarkBridge(LLInventoryPanel* inventory, 
					 LLFolderView* root,
					 const LLUUID& uuid, 
					 U32 flags = 0x00);
protected:
	BOOL mVisited;
};

class LLCallingCardBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual std::string getLabelSuffix() const;
	//virtual const std::string& getDisplayName() const;
	virtual LLUIImagePtr getIcon() const;
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void openItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data);
	void refreshFolderViewItem();
protected:
	LLCallingCardBridge(LLInventoryPanel* inventory, 
						LLFolderView* folder,
						const LLUUID& uuid );
	~LLCallingCardBridge();
protected:
	LLCallingCardObserver* mObserver;
};


class LLNotecardBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual void openItem();
protected:
	LLNotecardBridge(LLInventoryPanel* inventory, 
					 LLFolderView* root,
					 const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
};

class LLGestureBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	// Only suffix for gesture items, not task items, because only
	// gestures in your inventory can be active.
	virtual LLFontGL::StyleFlags getLabelStyle() const;
	virtual std::string getLabelSuffix() const;

	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void openItem();
	virtual BOOL removeItem();

	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);

	static void playGesture(const LLUUID& item_id);

protected:
	LLGestureBridge(LLInventoryPanel* inventory, 
					LLFolderView* root,
					const LLUUID& uuid)
	:	LLItemBridge(inventory, root, uuid) {}
};

class LLAnimationBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);

	virtual void openItem();

protected:
	LLAnimationBridge(LLInventoryPanel* inventory, 
					  LLFolderView* root, 
					  const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
};

class LLObjectBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual LLUIImagePtr	getIcon() const;
	virtual void			performAction(LLInventoryModel* model, std::string action);
	virtual void			openItem();
	virtual LLFontGL::StyleFlags getLabelStyle() const;
	virtual std::string getLabelSuffix() const;
	virtual void			buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL renameItem(const std::string& new_name);

	LLInventoryObject* getObject() const;
protected:
	LLObjectBridge(LLInventoryPanel* inventory, 
				   LLFolderView* root, 
				   const LLUUID& uuid, 
				   LLInventoryType::EType type, 
				   U32 flags);
protected:
	static LLUUID	sContextMenuItemID;  // Only valid while the context menu is open.
	LLInventoryType::EType mInvType;
	U32 mAttachPt;
	BOOL mIsMultiObject;
};

class LLLSLTextBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual void openItem();
protected:
	LLLSLTextBridge(LLInventoryPanel* inventory, 
					LLFolderView* root, 
					const LLUUID& uuid ) :
		LLItemBridge(inventory, root, uuid) {}
};

class LLWearableBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual LLUIImagePtr getIcon() const;
	virtual void	performAction(LLInventoryModel* model, std::string action);
	virtual void	openItem();
	virtual void	buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual std::string getLabelSuffix() const;
	virtual BOOL renameItem(const std::string& new_name);

	static void		onWearOnAvatar( void* userdata );	// Access to wearOnAvatar() from menu
	static BOOL		canWearOnAvatar( void* userdata );
	static void		onWearOnAvatarArrived( LLWearable* wearable, void* userdata );
	void			wearOnAvatar();

	static void		onWearAddOnAvatarArrived( LLWearable* wearable, void* userdata );
	void			wearAddOnAvatar();

	static BOOL		canEditOnAvatar( void* userdata );	// Access to editOnAvatar() from menu
	static void		onEditOnAvatar( void* userdata );
	void			editOnAvatar();

	static BOOL		canRemoveFromAvatar( void* userdata );
	static void		onRemoveFromAvatar( void* userdata );
	static void		onRemoveFromAvatarArrived( LLWearable* wearable, 	void* userdata );
	static void 	removeItemFromAvatar(LLViewerInventoryItem *item);
	static void 	removeAllClothesFromAvatar();
	void			removeFromAvatar();

protected:
	LLWearableBridge(LLInventoryPanel* inventory, 
					 LLFolderView* root, 
					 const LLUUID& uuid, 
					 LLAssetType::EType asset_type, 
					 LLInventoryType::EType inv_type, 
					 LLWearableType::EType wearable_type);
protected:
	LLAssetType::EType mAssetType;
	LLInventoryType::EType mInvType;
	LLWearableType::EType  mWearableType;
};

class LLLinkItemBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual const std::string& getPrefix() { return sPrefix; }
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
protected:
	LLLinkItemBridge(LLInventoryPanel* inventory, 
					 LLFolderView* root,
					 const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
protected:
	static std::string sPrefix;
};

class LLLinkFolderBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual const std::string& getPrefix() { return sPrefix; }
	virtual LLUIImagePtr getIcon() const;
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void gotoItem();
protected:
	LLLinkFolderBridge(LLInventoryPanel* inventory, 
					   LLFolderView* root,
					   const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
	const LLUUID &getFolderID() const;
protected:
	static std::string sPrefix;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInvFVBridgeAction
//
// This is an implementation class to be able to 
// perform action to view inventory items.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInvFVBridgeAction
{
public:
	// This method is a convenience function which creates the correct
	// type of bridge action based on some basic information.
	static LLInvFVBridgeAction* createAction(LLAssetType::EType asset_type,
											 const LLUUID& uuid,
											 LLInventoryModel* model);
	static void doAction(LLAssetType::EType asset_type,
						 const LLUUID& uuid, LLInventoryModel* model);
	static void doAction(const LLUUID& uuid, LLInventoryModel* model);

	virtual void doIt() {};
	virtual ~LLInvFVBridgeAction() {} // need this because of warning on OSX
protected:
	LLInvFVBridgeAction(const LLUUID& id, LLInventoryModel* model) :
		mUUID(id), mModel(model) {}
	LLViewerInventoryItem* getItem() const;
protected:
	const LLUUID& mUUID; // item id
	LLInventoryModel* mModel;
};


/************************************************************************/
/* Recent Inventory Panel related classes                               */
/************************************************************************/
class LLRecentInventoryBridgeBuilder;
/**
 * Overridden version of the Inventory-Folder-View-Bridge for Folders
 */
class LLRecentItemsFolderBridge : public LLFolderBridge
{
	friend class LLRecentInventoryBridgeBuilder;

public:
	/**
	 * Creates context menu for Folders related to Recent Inventory Panel.
	 *
	 * It uses base logic and than removes from visible items "New..." menu items.
	 */
	/*virtual*/ void buildContextMenu(LLMenuGL& menu, U32 flags);

protected:
	LLRecentItemsFolderBridge(LLInventoryType::EType type,
						 LLInventoryPanel* inventory,
						 LLFolderView* root,
						 const LLUUID& uuid) :
		LLFolderBridge(inventory, root, uuid)
	{
		mInvType = type;
	}
};

/**
 * Bridge builder to create Inventory-Folder-View-Bridge for Recent Inventory Panel
 */
class LLRecentInventoryBridgeBuilder : public LLInventoryFVBridgeBuilder
{
	/**
	 * Overrides FolderBridge for Recent Inventory Panel.
	 *
	 * It use base functionality for bridges other than FolderBridge.
	 */
	virtual LLInvFVBridge* createBridge(LLAssetType::EType asset_type,
		LLAssetType::EType actual_asset_type,
		LLInventoryType::EType inv_type,
		LLInventoryPanel* inventory,
		LLFolderView* root,
		const LLUUID& uuid,
		U32 flags = 0x00) const;

};



void wear_inventory_item_on_avatar(LLInventoryItem* item);

void rez_attachment(LLViewerInventoryItem* item, 
					LLViewerJointAttachment* attachment);

// Move items from an in-world object's "Contents" folder to a specified
// folder in agent inventory.
BOOL move_inv_category_world_to_agent(const LLUUID& object_id, 
									  const LLUUID& category_id,
									  BOOL drop,
									  void (*callback)(S32, void*) = NULL,
									  void* user_data = NULL);

// Utility function to hide all entries except those in the list
void hide_context_entries(LLMenuGL& menu, 
						  const menuentry_vec_t &entries_to_show, 
						  const menuentry_vec_t &disabled_entries);

#endif // LL_LLINVENTORYBRIDGE_H
