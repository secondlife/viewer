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

enum EInventoryIcon
{
	TEXTURE_ICON_NAME,
	SOUND_ICON_NAME,
	CALLINGCARD_ONLINE_ICON_NAME,
	CALLINGCARD_OFFLINE_ICON_NAME,
	LANDMARK_ICON_NAME,
	LANDMARK_VISITED_ICON_NAME,
	SCRIPT_ICON_NAME,
	CLOTHING_ICON_NAME,
	OBJECT_ICON_NAME,
	OBJECT_MULTI_ICON_NAME,
	NOTECARD_ICON_NAME,
	BODYPART_ICON_NAME,
	SNAPSHOT_ICON_NAME,

	BODYPART_SHAPE_ICON_NAME,
	BODYPART_SKIN_ICON_NAME,
	BODYPART_HAIR_ICON_NAME,
	BODYPART_EYES_ICON_NAME,
	CLOTHING_SHIRT_ICON_NAME,
	CLOTHING_PANTS_ICON_NAME,
	CLOTHING_SHOES_ICON_NAME,
	CLOTHING_SOCKS_ICON_NAME,
	CLOTHING_JACKET_ICON_NAME,
	CLOTHING_GLOVES_ICON_NAME,
	CLOTHING_UNDERSHIRT_ICON_NAME,
	CLOTHING_UNDERPANTS_ICON_NAME,
	CLOTHING_SKIRT_ICON_NAME,
	CLOTHING_ALPHA_ICON_NAME,
	CLOTHING_TATTOO_ICON_NAME,
	
	ANIMATION_ICON_NAME,
	GESTURE_ICON_NAME,

	LINKITEM_ICON_NAME,
	LINKFOLDER_ICON_NAME,

	ICON_NAME_COUNT
};

extern std::string ICON_NAME[ICON_NAME_COUNT];

typedef std::pair<LLUUID, LLUUID> two_uuids_t;
typedef std::list<two_uuids_t> two_uuids_list_t;
typedef std::pair<LLUUID, two_uuids_list_t> uuid_move_list_t;

struct LLMoveInv
{
	LLUUID mObjectID;
	LLUUID mCategoryID;
	two_uuids_list_t mMoveList;
	void (*mCallback)(S32, void*);
	void* mUserData;
};

struct LLAttachmentRezAction
{
	LLUUID	mItemID;
	S32		mAttachPt;
};

const std::string safe_inv_type_lookup(LLInventoryType::EType inv_type);
void hide_context_entries(LLMenuGL& menu, 
						const std::vector<std::string> &entries_to_show,
						const std::vector<std::string> &disabled_entries);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInvFVBridge (& it's derived classes)
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
	virtual BOOL isItemRemovable();
	virtual BOOL isItemMovable() const;
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
	void getClipboardEntries(bool show_asset_id, std::vector<std::string> &items, 
		std::vector<std::string> &disabled_items, U32 flags);
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

protected:
	LLInvFVBridge(LLInventoryPanel* inventory, const LLUUID& uuid);

	LLInventoryObject* getInventoryObject() const;
	LLInventoryModel* getInventoryModel() const;
	
	BOOL isInTrash() const;
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
	const LLUUID mUUID;	// item id
	LLInventoryType::EType mInvType;
	void purgeItem(LLInventoryModel *model, const LLUUID &uuid);
};

/**
 * This class intended to build Folder View Bridge via LLInvFVBridge::createBridge.
 * It can be overridden with another way of creation necessary Inventory-Folder-View-Bridge.
 */
class LLInventoryFVBridgeBuilder
{
public:
 	virtual ~LLInventoryFVBridgeBuilder(){}
	virtual LLInvFVBridge* createBridge(LLAssetType::EType asset_type,
										LLAssetType::EType actual_asset_type,
										LLInventoryType::EType inv_type,
										LLInventoryPanel* inventory,
										const LLUUID& uuid,
										U32 flags = 0x00) const;
};


class LLItemBridge : public LLInvFVBridge
{
public:
	LLItemBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
		LLInvFVBridge(inventory, uuid) {}

	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, std::string action);

	virtual void selectItem();
	virtual void restoreItem();
	virtual void restoreToWorld();
	virtual void gotoItem(LLFolderView *folder);
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
};


class LLFolderBridge : public LLInvFVBridge
{
	friend class LLInvFVBridge;
public:
	BOOL dragItemIntoFolder(LLInventoryItem* inv_item,
							BOOL drop);
	BOOL dragCategoryIntoFolder(LLInventoryCategory* inv_category,
								BOOL drop);
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, std::string action);
	virtual void openItem();
	virtual void closeItem();
	virtual BOOL isItemRenameable() const;
	virtual void selectItem();
	virtual void restoreItem();

	virtual LLFolderType::EType getPreferredType() const;
	virtual LLUIImagePtr getIcon() const;
	static LLUIImagePtr getIcon(LLFolderType::EType preferred_type);

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

	virtual BOOL isItemRemovable();
	virtual BOOL isItemMovable() const ;
	virtual BOOL isUpToDate() const;
	virtual BOOL isItemCopyable() const;
	virtual BOOL isClipboardPasteable() const;
	virtual BOOL isClipboardPasteableAsLink() const;
	virtual BOOL copyToClipboard() const;
	
	static void createWearable(LLFolderBridge* bridge, EWearableType type);
	static void createWearable(const LLUUID &parent_folder_id, EWearableType type);

	LLViewerInventoryCategory* getCategory() const;

protected:
	LLFolderBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
		LLInvFVBridge(inventory, uuid), mCallingCards(FALSE), mWearables(FALSE) {}

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

public:
	static LLFolderBridge* sSelf;
	static void staticFolderOptionsMenu();
	void folderOptionsMenu();
private:
	BOOL			mCallingCards;
	BOOL			mWearables;
	LLMenuGL*		mMenu;
	std::vector<std::string> mItems;
	std::vector<std::string> mDisabledItems;
};

// DEPRECATED
class LLScriptBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	LLUIImagePtr getIcon() const;

protected:
	LLScriptBridge( LLInventoryPanel* inventory, const LLUUID& uuid ) :
		LLItemBridge(inventory, uuid) {}
};


class LLTextureBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, std::string action);

protected:
	LLTextureBridge(LLInventoryPanel* inventory, const LLUUID& uuid, LLInventoryType::EType type) :
		LLItemBridge(inventory, uuid), mInvType(type) {}
	bool canSaveTexture(void);
	LLInventoryType::EType mInvType;
};

class LLSoundBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
	virtual void previewItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	static void openSoundPreview(void*);

protected:
	LLSoundBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
		LLItemBridge(inventory, uuid) {}
};

class LLLandmarkBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, std::string action);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();

protected:
	LLLandmarkBridge(LLInventoryPanel* inventory, const LLUUID& uuid, U32 flags = 0x00);

protected:
	BOOL mVisited;
};

class LLCallingCardBridge;

class LLCallingCardObserver : public LLFriendObserver
{
public:
	LLCallingCardObserver(LLCallingCardBridge* bridge) : mBridgep(bridge) {}
	virtual ~LLCallingCardObserver() { mBridgep = NULL; }
	virtual void changed(U32 mask);

protected:
	LLCallingCardBridge* mBridgep;
};

class LLCallingCardBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual std::string getLabelSuffix() const;
	//virtual const std::string& getDisplayName() const;
	virtual LLUIImagePtr getIcon() const;
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, std::string action);
	virtual void openItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	//virtual void renameItem(const std::string& new_name);
	//virtual BOOL removeItem();
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data);
	void refreshFolderViewItem();
	BOOL removeItem();

protected:
	LLCallingCardBridge( LLInventoryPanel* inventory, const LLUUID& uuid );
	~LLCallingCardBridge();
	
protected:
	LLCallingCardObserver* mObserver;
};


class LLNotecardBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();

protected:
	LLNotecardBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
		LLItemBridge(inventory, uuid) {}
};

class LLGestureBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual LLUIImagePtr getIcon() const;

	// Only suffix for gesture items, not task items, because only
	// gestures in your inventory can be active.
	virtual LLFontGL::StyleFlags getLabelStyle() const;
	virtual std::string getLabelSuffix() const;

	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, std::string action);
	virtual void openItem();
	virtual BOOL removeItem();

	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);

protected:
	LLGestureBridge(LLInventoryPanel* inventory, const LLUUID& uuid)
	:	LLItemBridge(inventory, uuid) {}
};


class LLAnimationBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, std::string action);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);

	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();

protected:
	LLAnimationBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
		LLItemBridge(inventory, uuid) {}
};


class LLObjectBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual LLUIImagePtr	getIcon() const;
	virtual void			performAction(LLFolderView* folder, LLInventoryModel* model, std::string action);
	virtual void			openItem();
	virtual LLFontGL::StyleFlags getLabelStyle() const;
	virtual std::string getLabelSuffix() const;
	virtual void			buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL renameItem(const std::string& new_name);

	LLInventoryObject* getObject() const;

protected:
	LLObjectBridge(LLInventoryPanel* inventory, const LLUUID& uuid, LLInventoryType::EType type, U32 flags);

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
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();

protected:
	LLLSLTextBridge( LLInventoryPanel* inventory, const LLUUID& uuid ) :
		LLItemBridge(inventory, uuid) {}
};


class LLWearableBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual LLUIImagePtr getIcon() const;
	virtual void	performAction(LLFolderView* folder, LLInventoryModel* model, std::string action);
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
	void			removeFromAvatar();

protected:
	LLWearableBridge(LLInventoryPanel* inventory, const LLUUID& uuid, LLAssetType::EType asset_type, LLInventoryType::EType inv_type, EWearableType  wearable_type) :
		LLItemBridge(inventory, uuid),
		mAssetType( asset_type ),
		mInvType(inv_type),
		mWearableType(wearable_type)
		{}

protected:
	LLAssetType::EType mAssetType;
	LLInventoryType::EType mInvType;
	EWearableType  mWearableType;
};

class LLLinkItemBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual const std::string& getPrefix() { return sPrefix; }

	virtual LLUIImagePtr getIcon() const;
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);

protected:
	LLLinkItemBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
		LLItemBridge(inventory, uuid) {}

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
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, std::string action);
	virtual void gotoItem(LLFolderView *folder);

protected:
	LLLinkFolderBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
		LLItemBridge(inventory, uuid) {}
	const LLUUID &getFolderID() const;

protected:
	static std::string sPrefix;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInvFVBridgeAction (& its derived classes)
//
// This is an implementation class to be able to 
// perform action to view inventory items.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInvFVBridgeAction
{
public:
	// This method is a convenience function which creates the correct
	// type of bridge action based on some basic information
	static LLInvFVBridgeAction* createAction(LLAssetType::EType asset_type,
											 const LLUUID& uuid,LLInventoryModel* model);

	static void doAction(LLAssetType::EType asset_type,
						 const LLUUID& uuid, LLInventoryModel* model);
	static void doAction(const LLUUID& uuid, LLInventoryModel* model);

	virtual void doIt() {  };
	virtual ~LLInvFVBridgeAction(){}//need this because of warning on OSX
protected:
	LLInvFVBridgeAction(const LLUUID& id,LLInventoryModel* model):mUUID(id),mModel(model){}

	LLViewerInventoryItem* getItem() const;
protected:
	const LLUUID& mUUID;	// item id
	LLInventoryModel* mModel;

};



class LLTextureBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void	doIt() ;
	virtual ~LLTextureBridgeAction(){}
protected:
	LLTextureBridgeAction(const LLUUID& id,LLInventoryModel* model):LLInvFVBridgeAction(id,model){}

};


class LLSoundBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void	doIt() ;
	virtual ~LLSoundBridgeAction(){}
protected:
	LLSoundBridgeAction(const LLUUID& id,LLInventoryModel* model):LLInvFVBridgeAction(id,model){}

};


class LLLandmarkBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void	doIt() ;
	virtual ~LLLandmarkBridgeAction(){}
protected:
	LLLandmarkBridgeAction(const LLUUID& id,LLInventoryModel* model):LLInvFVBridgeAction(id,model){}

};


class LLCallingCardBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void	doIt() ;
	virtual ~LLCallingCardBridgeAction(){}
protected:
	LLCallingCardBridgeAction(const LLUUID& id,LLInventoryModel* model):LLInvFVBridgeAction(id,model){}

};


class LLNotecardBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void	doIt() ;
	virtual ~LLNotecardBridgeAction(){}
protected:
	LLNotecardBridgeAction(const LLUUID& id,LLInventoryModel* model):LLInvFVBridgeAction(id,model){}

};


class LLGestureBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void	doIt() ;
	virtual ~LLGestureBridgeAction(){}
protected:
	LLGestureBridgeAction(const LLUUID& id,LLInventoryModel* model):LLInvFVBridgeAction(id,model){}

};


class LLAnimationBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void	doIt() ;
	virtual ~LLAnimationBridgeAction(){}
protected:
	LLAnimationBridgeAction(const LLUUID& id,LLInventoryModel* model):LLInvFVBridgeAction(id,model){}

};


class LLObjectBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void	doIt() ;
	virtual ~LLObjectBridgeAction(){}
protected:
	LLObjectBridgeAction(const LLUUID& id,LLInventoryModel* model):LLInvFVBridgeAction(id,model){}

};


class LLLSLTextBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void	doIt() ;
	virtual ~LLLSLTextBridgeAction(){}
protected:
	LLLSLTextBridgeAction(const LLUUID& id,LLInventoryModel* model):LLInvFVBridgeAction(id,model){}

};


class LLWearableBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void	doIt();
	virtual ~LLWearableBridgeAction(){}
protected:
	LLWearableBridgeAction(const LLUUID& id,LLInventoryModel* model):LLInvFVBridgeAction(id,model){}


	BOOL isInTrash() const;
	// return true if the item is in agent inventory. if false, it
	// must be lost or in the inventory library.
	BOOL isAgentInventory() const;

	void wearOnAvatar();

};

void wear_inventory_item_on_avatar(LLInventoryItem* item);

class LLViewerJointAttachment;
void rez_attachment(LLViewerInventoryItem* item, LLViewerJointAttachment* attachment);

// Move items from an in-world object's "Contents" folder to a specified
// folder in agent inventory.
BOOL move_inv_category_world_to_agent(const LLUUID& object_id, 
									  const LLUUID& category_id,
									  BOOL drop,
									  void (*callback)(S32, void*) = NULL,
									  void* user_data = NULL);



void teleport_via_landmark(const LLUUID& asset_id);

// Utility function to hide all entries except those in the list
void hide_context_entries(LLMenuGL& menu, 
		const std::vector<std::string> &entries_to_show, 
		const std::vector<std::string> &disabled_entries);

#endif // LL_LLINVENTORYBRIDGE_H
