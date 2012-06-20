/** 
 * @file llinventorybridge.h
 * @brief Implementation of the Inventory-Folder-View-Bridge classes.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLINVENTORYBRIDGE_H
#define LL_LLINVENTORYBRIDGE_H

#include "llcallingcard.h"
#include "llfloaterproperties.h"
#include "llfolderviewmodel.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "llviewercontrol.h"
#include "llwearable.h"

class LLInventoryFilter;
class LLInventoryPanel;
class LLInventoryModel;
class LLMenuGL;
class LLCallingCardObserver;
class LLViewerJointAttachment;
class LLFolderView;

typedef std::vector<std::string> menuentry_vec_t;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInvFVBridge
//
// Short for Inventory-Folder-View-Bridge. This is an
// implementation class to be able to view inventory items.
//
// You'll want to call LLInvItemFVELister::createBridge() to actually create
// an instance of this class. This helps encapsulate the
// functionality a bit. (except for folders, you can create those
// manually...)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInvFVBridge : public LLFolderViewModelItemInventory
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

	bool canShare() const;
	bool canListOnMarketplace() const;
	bool canListOnMarketplaceNow() const;

	//--------------------------------------------------------------------
	// LLInvFVBridge functionality
	//--------------------------------------------------------------------
	virtual const LLUUID& getUUID() const { return mUUID; }
	virtual void clearDisplayName() {}
	virtual void restoreItem() {}
	virtual void restoreToWorld() {}

	//--------------------------------------------------------------------
	// Inherited LLFolderViewModelItemInventory functions
	//--------------------------------------------------------------------
	virtual const std::string& getName() const;
	virtual const std::string& getDisplayName() const;
	virtual PermissionMask getPermissionMask() const;
	virtual LLFolderType::EType getPreferredType() const;
	virtual time_t getCreationDate() const;
        virtual void setCreationDate(time_t creation_date_utc);
	virtual LLFontGL::StyleFlags getLabelStyle() const { return LLFontGL::NORMAL; }
	virtual std::string getLabelSuffix() const { return LLStringUtil::null; }
	virtual void openItem() {}
	virtual void closeItem() {}
	virtual void showProperties();
	virtual BOOL isItemRenameable() const { return TRUE; }
	//virtual BOOL renameItem(const std::string& new_name) {}
	virtual BOOL isItemRemovable() const;
	virtual BOOL isItemMovable() const;
	virtual BOOL isItemInTrash() const;
	virtual BOOL isLink() const;
	//virtual BOOL removeItem() = 0;
	virtual void removeBatch(std::vector<LLFolderViewModelItem*>& batch);
	virtual void move(LLFolderViewModelItem* new_parent_bridge) {}
	virtual BOOL isItemCopyable() const { return FALSE; }
	virtual BOOL copyToClipboard() const;
	virtual BOOL cutToClipboard() const;
	virtual BOOL isClipboardPasteable() const;
	virtual BOOL isClipboardPasteableAsLink() const;
	virtual void pasteFromClipboard() {}
	virtual void pasteLinkFromClipboard() {}
	void getClipboardEntries(bool show_asset_id, menuentry_vec_t &items, 
							 menuentry_vec_t &disabled_items, U32 flags);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
        virtual LLToolDragAndDrop::ESource getDragSource() const;
	virtual BOOL startDrag(EDragAndDropType* type, LLUUID* id) const;
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data,
							std::string& tooltip_msg) { return FALSE; }
	virtual LLInventoryType::EType getInventoryType() const { return mInvType; }
	virtual LLWearableType::EType getWearableType() const { return LLWearableType::WT_NONE; }
        EInventorySortGroup getSortGroup()  const { return SG_ITEM; }
	virtual LLInventoryObject* getInventoryObject() const;


	//--------------------------------------------------------------------
	// Convenience functions for adding various common menu options.
	//--------------------------------------------------------------------
protected:
	virtual void addTrashContextMenuOptions(menuentry_vec_t &items,
											menuentry_vec_t &disabled_items);
	virtual void addDeleteContextMenuOptions(menuentry_vec_t &items,
											 menuentry_vec_t &disabled_items);
	virtual void addOpenRightClickMenuOption(menuentry_vec_t &items);
	virtual void addOutboxContextMenuOptions(U32 flags,
											 menuentry_vec_t &items,
											 menuentry_vec_t &disabled_items);
protected:
	LLInvFVBridge(LLInventoryPanel* inventory, LLFolderView* root, const LLUUID& uuid);

	LLInventoryModel* getInventoryModel() const;
	LLInventoryFilter* getInventoryFilter() const;
	
	BOOL isLinkedObjectInTrash() const; // Is this obj or its baseobj in the trash?
	BOOL isLinkedObjectMissing() const; // Is this a linked obj whose baseobj is not in inventory?

	BOOL isAgentInventory() const; // false if lost or in the inventory library
	BOOL isCOFFolder() const;       // true if COF or descendant of
	BOOL isInboxFolder() const;     // true if COF or descendant of   marketplace inbox
	BOOL isOutboxFolder() const;    // true if COF or descendant of   marketplace outbox
	BOOL isOutboxFolderDirectParent() const;
	const LLUUID getOutboxFolder() const;

	virtual BOOL isItemPermissive() const;
	static void changeItemParent(LLInventoryModel* model,
								 LLViewerInventoryItem* item,
								 const LLUUID& new_parent,
								 BOOL restamp);
	static void changeCategoryParent(LLInventoryModel* model,
									 LLViewerInventoryCategory* item,
									 const LLUUID& new_parent,
									 BOOL restamp);
	void removeBatchNoCheck(std::vector<LLFolderViewModelItem*>& batch);
protected:
	LLHandle<LLInventoryPanel>	mInventoryPanel;
	LLFolderView*				mRoot;
	const LLUUID				mUUID;	// item id
	LLInventoryType::EType		mInvType;
	bool						mIsLink;
	LLTimer						mTimeSinceRequestStart;
	mutable std::string			mDisplayName;

	void purgeItem(LLInventoryModel *model, const LLUUID &uuid);
	virtual void buildDisplayName() const {}
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInvFVBridgeBuilder
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
	virtual std::string getLabelSuffix() const;
	virtual LLFontGL::StyleFlags getLabelStyle() const;
	virtual PermissionMask getPermissionMask() const;
	virtual time_t getCreationDate() const;
	virtual BOOL isItemRenameable() const;
	virtual BOOL renameItem(const std::string& new_name);
	virtual BOOL removeItem();
	virtual BOOL isItemCopyable() const;
	virtual bool hasChildren() const { return FALSE; }
	virtual BOOL isUpToDate() const { return TRUE; }

	/*virtual*/ void clearDisplayName() { mDisplayName.clear(); }

	LLViewerInventoryItem* getItem() const;

protected:
	BOOL confirmRemoveItem(const LLSD& notification, const LLSD& response);
	virtual BOOL isItemPermissive() const;
	virtual void buildDisplayName() const;

};

class LLFolderBridge : public LLInvFVBridge
{
public:
	LLFolderBridge(LLInventoryPanel* inventory, 
				   LLFolderView* root,
				   const LLUUID& uuid) 
        :       LLInvFVBridge(inventory, root, uuid),
		mCallingCards(FALSE),
		mWearables(FALSE),
		mIsLoading(false)
	{}
		
	BOOL dragItemIntoFolder(LLInventoryItem* inv_item, BOOL drop, std::string& tooltip_msg);
	BOOL dragCategoryIntoFolder(LLInventoryCategory* inv_category, BOOL drop, std::string& tooltip_msg);

    virtual void buildDisplayName() const;

	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void openItem();
	virtual void closeItem();
	virtual BOOL isItemRenameable() const;
	virtual void selectItem();
	virtual void restoreItem();

	virtual LLFolderType::EType getPreferredType() const;
	virtual LLUIImagePtr getIcon() const;
	virtual LLUIImagePtr getOpenIcon() const;
	static LLUIImagePtr getIcon(LLFolderType::EType preferred_type);

	virtual BOOL renameItem(const std::string& new_name);

	virtual BOOL removeItem();
	BOOL removeSystemFolder();
	bool removeItemResponse(const LLSD& notification, const LLSD& response);

	virtual void pasteFromClipboard();
	virtual void pasteLinkFromClipboard();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual bool hasChildren() const;
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data,
							std::string& tooltip_msg);

	virtual BOOL isItemRemovable() const;
	virtual BOOL isItemMovable() const ;
	virtual BOOL isUpToDate() const;
	virtual BOOL isItemCopyable() const;
	virtual BOOL isClipboardPasteable() const;
	virtual BOOL isClipboardPasteableAsLink() const;
	
	EInventorySortGroup getSortGroup()  const;
	virtual void update();

	static void createWearable(LLFolderBridge* bridge, LLWearableType::EType type);

	LLViewerInventoryCategory* getCategory() const;
	LLHandle<LLFolderBridge> getHandle() { mHandle.bind(this); return mHandle; }

	bool isLoading() { return mIsLoading; }

protected:
	void buildContextMenuOptions(U32 flags, menuentry_vec_t& items,   menuentry_vec_t& disabled_items);
	void buildContextMenuFolderOptions(U32 flags, menuentry_vec_t& items,   menuentry_vec_t& disabled_items);

	//--------------------------------------------------------------------
	// Menu callbacks
	//--------------------------------------------------------------------
	static void pasteClipboard(void* user_data);
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

	void modifyOutfit(BOOL append);
	void determineFolderType();

	void dropToFavorites(LLInventoryItem* inv_item);
	void dropToOutfit(LLInventoryItem* inv_item, BOOL move_is_into_current_outfit);

	//--------------------------------------------------------------------
	// Messy hacks for handling folder options
	//--------------------------------------------------------------------
public:
	static LLHandle<LLFolderBridge> sSelf;
	static void staticFolderOptionsMenu();

private:

	bool							mCallingCards;
	bool							mWearables;
	bool							mIsLoading;
	LLTimer							mTimeSinceRequestStart;
	mutable std::string				mDisplayName;
	LLRootHandle<LLFolderBridge> mHandle;
};

class LLTextureBridge : public LLItemBridge
{
public:
	LLTextureBridge(LLInventoryPanel* inventory, 
					LLFolderView* root,
					const LLUUID& uuid, 
					LLInventoryType::EType type) :
		LLItemBridge(inventory, root, uuid)
	{
		mInvType = type;
	}
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual void performAction(LLInventoryModel* model, std::string action);
	bool canSaveTexture(void);
};

class LLSoundBridge : public LLItemBridge
{
public:
 	LLSoundBridge(LLInventoryPanel* inventory, 
				  LLFolderView* root,
				  const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
	virtual void openItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	static void openSoundPreview(void*);
};

class LLLandmarkBridge : public LLItemBridge
{
public:
 	LLLandmarkBridge(LLInventoryPanel* inventory, 
					 LLFolderView* root,
					 const LLUUID& uuid, 
					 U32 flags = 0x00);
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
protected:
	BOOL mVisited;
};

class LLCallingCardBridge : public LLItemBridge
{
public:
	LLCallingCardBridge(LLInventoryPanel* inventory, 
						LLFolderView* folder,
						const LLUUID& uuid );
	~LLCallingCardBridge();
	virtual std::string getLabelSuffix() const;
	//virtual const std::string& getDisplayName() const;
	virtual LLUIImagePtr getIcon() const;
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void openItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data,
							std::string& tooltip_msg);
	void refreshFolderViewItem();
protected:
	LLCallingCardObserver* mObserver;
};

class LLNotecardBridge : public LLItemBridge
{
public:
	LLNotecardBridge(LLInventoryPanel* inventory, 
					 LLFolderView* root,
					 const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
	virtual void openItem();
};

class LLGestureBridge : public LLItemBridge
{
public:
	LLGestureBridge(LLInventoryPanel* inventory, 
					LLFolderView* root,
					const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
	// Only suffix for gesture items, not task items, because only
	// gestures in your inventory can be active.
	virtual LLFontGL::StyleFlags getLabelStyle() const;
	virtual std::string getLabelSuffix() const;
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void openItem();
	virtual BOOL removeItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	static void playGesture(const LLUUID& item_id);
};

class LLAnimationBridge : public LLItemBridge
{
public:
	LLAnimationBridge(LLInventoryPanel* inventory, 
					  LLFolderView* root, 
					  const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual void openItem();
};

class LLObjectBridge : public LLItemBridge
{
public:
	LLObjectBridge(LLInventoryPanel* inventory, 
				   LLFolderView* root, 
				   const LLUUID& uuid, 
				   LLInventoryType::EType type, 
				   U32 flags);
	virtual LLUIImagePtr	getIcon() const;
	virtual void			performAction(LLInventoryModel* model, std::string action);
	virtual void			openItem();
	virtual std::string getLabelSuffix() const;
	virtual void			buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL renameItem(const std::string& new_name);
	LLInventoryObject* getObject() const;
protected:
	static LLUUID sContextMenuItemID;  // Only valid while the context menu is open.
	U32 mAttachPt;
	BOOL mIsMultiObject;
};

class LLLSLTextBridge : public LLItemBridge
{
public:
	LLLSLTextBridge(LLInventoryPanel* inventory, 
					LLFolderView* root, 
					const LLUUID& uuid ) :
		LLItemBridge(inventory, root, uuid) {}
	virtual void openItem();
};

class LLWearableBridge : public LLItemBridge
{
public:
	LLWearableBridge(LLInventoryPanel* inventory, 
					 LLFolderView* root, 
					 const LLUUID& uuid, 
					 LLAssetType::EType asset_type, 
					 LLInventoryType::EType inv_type, 
					 LLWearableType::EType wearable_type);
	virtual LLUIImagePtr getIcon() const;
	virtual void	performAction(LLInventoryModel* model, std::string action);
	virtual void	openItem();
	virtual void	buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual std::string getLabelSuffix() const;
	virtual BOOL renameItem(const std::string& new_name);
	virtual LLWearableType::EType getWearableType() const { return mWearableType; }

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
	LLAssetType::EType mAssetType;
	LLWearableType::EType  mWearableType;
};

class LLLinkItemBridge : public LLItemBridge
{
public:
	LLLinkItemBridge(LLInventoryPanel* inventory, 
					 LLFolderView* root,
					 const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
	virtual const std::string& getPrefix() { return sPrefix; }
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
protected:
	static std::string sPrefix;
};

class LLLinkFolderBridge : public LLItemBridge
{
public:
	LLLinkFolderBridge(LLInventoryPanel* inventory, 
					   LLFolderView* root,
					   const LLUUID& uuid) :
		LLItemBridge(inventory, root, uuid) {}
	virtual const std::string& getPrefix() { return sPrefix; }
	virtual LLUIImagePtr getIcon() const;
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void gotoItem();
protected:
	const LLUUID &getFolderID() const;
	static std::string sPrefix;
};


class LLMeshBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);

protected:
	LLMeshBridge(LLInventoryPanel* inventory, 
		     LLFolderView* root,
		     const LLUUID& uuid) :
                       LLItemBridge(inventory, root, uuid) {}
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

class LLMeshBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void	doIt() ;
	virtual ~LLMeshBridgeAction(){}
protected:
	LLMeshBridgeAction(const LLUUID& id,LLInventoryModel* model):LLInvFVBridgeAction(id,model){}

};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Recent Inventory Panel related classes
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Overridden version of the Inventory-Folder-View-Bridge for Folders
class LLRecentItemsFolderBridge : public LLFolderBridge
{
	friend class LLInvFVBridgeAction;
public:
	// Creates context menu for Folders related to Recent Inventory Panel.
	// Uses base logic and than removes from visible items "New..." menu items.
	LLRecentItemsFolderBridge(LLInventoryType::EType type,
							  LLInventoryPanel* inventory,
							  LLFolderView* root,
							  const LLUUID& uuid) :
		LLFolderBridge(inventory, root, uuid)
	{
		mInvType = type;
	}
	/*virtual*/ void buildContextMenu(LLMenuGL& menu, U32 flags);
};

// Bridge builder to create Inventory-Folder-View-Bridge for Recent Inventory Panel
class LLRecentInventoryBridgeBuilder : public LLInventoryFVBridgeBuilder
{
public:
	// Overrides FolderBridge for Recent Inventory Panel.
	// It use base functionality for bridges other than FolderBridge.
	virtual LLInvFVBridge* createBridge(LLAssetType::EType asset_type,
		LLAssetType::EType actual_asset_type,
		LLInventoryType::EType inv_type,
		LLInventoryPanel* inventory,
		LLFolderView* root,
		const LLUUID& uuid,
		U32 flags = 0x00) const;
};

void rez_attachment(LLViewerInventoryItem* item, 
					LLViewerJointAttachment* attachment,
					bool replace = false);

// Move items from an in-world object's "Contents" folder to a specified
// folder in agent inventory.
BOOL move_inv_category_world_to_agent(const LLUUID& object_id, 
									  const LLUUID& category_id,
									  BOOL drop,
									  void (*callback)(S32, void*) = NULL,
									  void* user_data = NULL,
									  LLInventoryFilter* filter = NULL);

// Utility function to hide all entries except those in the list
// Can be called multiple times on the same menu (e.g. if multiple items
// are selected).  If "append" is false, then only common enabled items
// are set as enabled.
void hide_context_entries(LLMenuGL& menu, 
						  const menuentry_vec_t &entries_to_show, 
						  const menuentry_vec_t &disabled_entries);

#endif // LL_LLINVENTORYBRIDGE_H
