/** 
 * @file llinventorybridge.h
 * @brief Implementation of the Inventory-Folder-View-Bridge classes.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llfloaterproperties.h"
#include "llwearable.h"
#include "llviewercontrol.h"
#include "llcallingcard.h"

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
	
	ANIMATION_ICON_NAME,
	GESTURE_ICON_NAME,

	ICON_NAME_COUNT
};

extern const char* ICON_NAME[ICON_NAME_COUNT];

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


//helper functions
class LLShowProps 
{
public:

	static void showProperties(const LLUUID& uuid)
	{
		if(!LLFloaterProperties::show(uuid, LLUUID::null))
		{
			S32 left, top;
			gFloaterView->getNewFloaterPosition(&left, &top);
			LLRect rect = gSavedSettings.getRect("PropertiesRect");
			rect.translate( left - rect.mLeft, top - rect.mTop );
			LLFloaterProperties* floater;
			floater = new LLFloaterProperties("item properties",
											rect,
											"Inventory Item Properties",
											uuid,
											LLUUID::null);
			// keep onscreen
			gFloaterView->adjustToFitScreen(floater, FALSE);
		}
	}
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryPanelObserver
//
// Bridge to support knowing when the inventory has changed.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryPanelObserver : public LLInventoryObserver
{
public:
	LLInventoryPanelObserver(LLInventoryPanel* ip) : mIP(ip) {}
	virtual ~LLInventoryPanelObserver() {}
	virtual void changed(U32 mask) { mIP->modelChanged(mask); }
protected:
	LLInventoryPanel* mIP;
};

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
									   LLInventoryType::EType inv_type,
									   LLInventoryPanel* inventory,
									   const LLUUID& uuid,
									   U32 flags = 0x00);
	virtual ~LLInvFVBridge() {}

	virtual const LLUUID& getUUID() const { return mUUID; }

	virtual const LLString& getPrefix() { return LLString::null; }
	virtual void restoreItem() {}

	// LLFolderViewEventListener functions
	virtual const LLString& getName() const;
	virtual const LLString& getDisplayName() const;
	virtual PermissionMask getPermissionMask() const;
	virtual U32 getCreationDate() const;
	virtual LLFontGL::StyleFlags getLabelStyle() const
	{
		return LLFontGL::NORMAL;
	}
	virtual LLString getLabelSuffix() const { return LLString::null; }
	virtual void openItem() {}
	virtual void previewItem() {openItem();}
	virtual void showProperties();
	virtual BOOL isItemRenameable() const { return TRUE; }
	//virtual BOOL renameItem(const LLString& new_name) {}
	virtual BOOL isItemRemovable();
	virtual BOOL isItemMovable();
	//virtual BOOL removeItem() = 0;
	virtual void removeBatch(LLDynamicArray<LLFolderViewEventListener*>& batch);
	virtual void move(LLFolderViewEventListener* new_parent_bridge) {}
	virtual BOOL isItemCopyable() const { return FALSE; }
	virtual BOOL copyToClipboard() const { return FALSE; }
	virtual void cutToClipboard() {}
	virtual BOOL isClipboardPasteable() const;
	virtual void pasteFromClipboard() {}
	void getClipboardEntries(bool show_asset_id, std::vector<LLString> &items, 
		std::vector<LLString> &disabled_items, U32 flags);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL startDrag(EDragAndDropType* type, LLUUID* id);
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data) { return FALSE; }
	virtual LLInventoryType::EType getInventoryType() const { return mInvType; }

	// LLInvFVBridge functionality
	virtual void clearDisplayName() {}

protected:
	LLInvFVBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
		mInventoryPanel(inventory), mUUID(uuid) {}

	LLInventoryObject* getInventoryObject() const;
	BOOL isInTrash() const;
	// return true if the item is in agent inventory. if false, it
	// must be lost or in the inventory library.
	BOOL isAgentInventory() const;
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
	LLInventoryPanel* mInventoryPanel;
	LLUUID mUUID;	// item id
	LLInventoryType::EType mInvType;
};


class LLItemBridge : public LLInvFVBridge
{
public:
	LLItemBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
		LLInvFVBridge(inventory, uuid) {}

	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, LLString action);

	virtual void selectItem();
	virtual void restoreItem();

	virtual LLViewerImage* getIcon() const;
	virtual const LLString& getDisplayName() const;
	virtual LLString getLabelSuffix() const;
	virtual PermissionMask getPermissionMask() const;
	virtual U32 getCreationDate() const;
	virtual BOOL isItemRenameable() const;
	virtual BOOL renameItem(const LLString& new_name);
	virtual BOOL removeItem();
	virtual BOOL isItemCopyable() const;
	virtual BOOL copyToClipboard() const;
	virtual BOOL hasChildren() const { return FALSE; }
	virtual BOOL isUpToDate() const { return TRUE; }

	// override for LLInvFVBridge
	virtual void clearDisplayName() { mDisplayName.clear(); }

	LLViewerInventoryItem* getItem() const;

protected:
	virtual BOOL isItemPermissive() const;
	static void buildDisplayName(LLInventoryItem* item, LLString& name);
	mutable LLString mDisplayName;
};


class LLFolderBridge : public LLInvFVBridge
{
	friend class LLInvFVBridge;
public:
	BOOL dragItemIntoFolder(LLInventoryItem* inv_item,
							BOOL drop);
	BOOL dragCategoryIntoFolder(LLInventoryCategory* inv_category,
								BOOL drop);
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, LLString action);
	virtual void openItem();
	virtual BOOL isItemRenameable() const;
	virtual void selectItem();
	virtual void restoreItem();


	virtual LLViewerImage* getIcon() const;
	virtual BOOL renameItem(const LLString& new_name);
	virtual BOOL removeItem();
	virtual BOOL isClipboardPasteable() const;
	virtual void pasteFromClipboard();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL hasChildren() const;
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data);

	virtual BOOL isItemRemovable();
	virtual BOOL isItemMovable();
	virtual BOOL isUpToDate() const;

	static void createWearable(LLFolderBridge* bridge, EWearableType type);
	static void createWearable(LLUUID parent_folder_id, EWearableType type);

	LLViewerInventoryCategory* getCategory() const;

protected:
	LLFolderBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
		LLInvFVBridge(inventory, uuid) {}

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

	void modifyOutfit(BOOL append);
public:
	static LLFolderBridge* sSelf;
	static void staticFolderOptionsMenu();
	void folderOptionsMenu();
private:
	BOOL			mCallingCards;
	BOOL			mWearables;
	LLMenuGL*		mMenu;
	std::vector<LLString> mItems;
	std::vector<LLString> mDisabledItems;
};

// DEPRECATED
class LLScriptBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	LLViewerImage* getIcon() const;

protected:
	LLScriptBridge( LLInventoryPanel* inventory, const LLUUID& uuid ) :
		LLItemBridge(inventory, uuid) {}
};


class LLTextureBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual const LLString& getPrefix() { return sPrefix; }

	virtual LLViewerImage* getIcon() const;
	virtual void openItem();

protected:
	LLTextureBridge(LLInventoryPanel* inventory, const LLUUID& uuid, LLInventoryType::EType type) :
		LLItemBridge(inventory, uuid), mInvType(type) {}
	static LLString sPrefix;
	LLInventoryType::EType mInvType;
};

class LLSoundBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual const LLString& getPrefix() { return sPrefix; }

	virtual LLViewerImage* getIcon() const;
	virtual void openItem();
	virtual void previewItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	static void openSoundPreview(void*);

protected:
	LLSoundBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
		LLItemBridge(inventory, uuid) {}
	static LLString sPrefix;
};

class LLLandmarkBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual const LLString& getPrefix() { return sPrefix; }
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, LLString action);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual LLViewerImage* getIcon() const;
	virtual void openItem();

protected:
	LLLandmarkBridge(LLInventoryPanel* inventory, const LLUUID& uuid, U32 flags = 0x00) :
		LLItemBridge(inventory, uuid) 
	{
		mVisited = FALSE;
		if (flags & LLInventoryItem::II_FLAGS_LANDMARK_VISITED)
		{
			mVisited = TRUE;
		}
	}

protected:
	static LLString sPrefix;
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
	virtual const LLString& getPrefix() { return sPrefix; }

	virtual LLString getLabelSuffix() const;
	//virtual const LLString& getDisplayName() const;
	virtual LLViewerImage* getIcon() const;
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, LLString action);
	virtual void openItem();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	//virtual void renameItem(const LLString& new_name);
	//virtual BOOL removeItem();
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data);
	void refreshFolderViewItem();

protected:
	LLCallingCardBridge( LLInventoryPanel* inventory, const LLUUID& uuid );
	~LLCallingCardBridge();
	
protected:
	static LLString sPrefix;
	LLCallingCardObserver* mObserver;
};


class LLNotecardBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual const LLString& getPrefix() { return sPrefix; }

	virtual LLViewerImage* getIcon() const;
	virtual void openItem();

protected:
	LLNotecardBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
		LLItemBridge(inventory, uuid) {}

protected:
	static LLString sPrefix;
};

class LLGestureBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual const LLString& getPrefix() { return sPrefix; }

	virtual LLViewerImage* getIcon() const;

	// Only suffix for gesture items, not task items, because only
	// gestures in your inventory can be active.
	virtual LLFontGL::StyleFlags getLabelStyle() const;
	virtual LLString getLabelSuffix() const;

	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, LLString action);
	virtual void openItem();
	virtual BOOL removeItem();

	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);

protected:
	LLGestureBridge(LLInventoryPanel* inventory, const LLUUID& uuid)
	:	LLItemBridge(inventory, uuid) {}

protected:
	static LLString sPrefix;
};


class LLAnimationBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual const LLString& getPrefix() { return sPrefix; }
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, LLString action);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);

	virtual LLViewerImage* getIcon() const;
	virtual void openItem();

protected:
	LLAnimationBridge(LLInventoryPanel* inventory, const LLUUID& uuid) :
		LLItemBridge(inventory, uuid) {}

protected:
	static LLString sPrefix;
};


class LLObjectBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual const LLString& getPrefix() { return sPrefix; }

	virtual LLViewerImage*	getIcon() const;
	virtual void			performAction(LLFolderView* folder, LLInventoryModel* model, LLString action);
	virtual void			openItem();
	virtual LLFontGL::StyleFlags getLabelStyle() const;
	virtual LLString getLabelSuffix() const;
	virtual void			buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL			isItemRemovable();
	virtual BOOL renameItem(const LLString& new_name);

protected:
	LLObjectBridge(LLInventoryPanel* inventory, const LLUUID& uuid, LLInventoryType::EType type, U32 flags) :
		LLItemBridge(inventory, uuid), mInvType(type)
	{
		mAttachPt = (flags & 0xff); // low bye of inventory flags

		mIsMultiObject = ( flags & LLInventoryItem::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS ) ?  TRUE: FALSE;
	}

protected:
	static LLString sPrefix;
	static LLUUID	sContextMenuItemID;  // Only valid while the context menu is open.
	LLInventoryType::EType mInvType;
	U32 mAttachPt;
	BOOL mIsMultiObject;
};


class LLLSLTextBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual const LLString& getPrefix() { return sPrefix; }

	virtual LLViewerImage* getIcon() const;
	virtual void openItem();

protected:
	LLLSLTextBridge( LLInventoryPanel* inventory, const LLUUID& uuid ) :
		LLItemBridge(inventory, uuid) {}

protected:
	static LLString sPrefix;
};


class LLWearableBridge : public LLItemBridge
{
	friend class LLInvFVBridge;
public:
	virtual LLViewerImage* getIcon() const;
	virtual void	performAction(LLFolderView* folder, LLInventoryModel* model, LLString action);
	virtual void	openItem();
	virtual void	buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual LLFontGL::StyleFlags getLabelStyle() const;
	virtual LLString getLabelSuffix() const;
	virtual BOOL	isItemRemovable();
	virtual BOOL renameItem(const LLString& new_name);

	static void		onWearOnAvatar( void* userdata );	// Access to wearOnAvatar() from menu
	static BOOL		canWearOnAvatar( void* userdata );
	static void		onWearOnAvatarArrived( LLWearable* wearable, void* userdata );
	void			wearOnAvatar();

	static BOOL		canEditOnAvatar( void* userdata );	// Access to editOnAvatar() from menu
	static void		onEditOnAvatar( void* userdata );
	void			editOnAvatar();

	static BOOL		canRemoveFromAvatar( void* userdata );
	static void		onRemoveFromAvatar( void* userdata );
	static void		onRemoveFromAvatarArrived( LLWearable* wearable, void* userdata );

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
