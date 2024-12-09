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
#include "llfolderviewmodel.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "llviewercontrol.h"
#include "llviewerwearable.h"
#include "lltooldraganddrop.h"
#include "lllandmarklist.h"
#include "llfolderviewitem.h"
#include "llsettingsbase.h"

class LLInventoryFilter;
class LLInventoryPanel;
class LLInventoryModel;
class LLMenuGL;
class LLCallingCardObserver;
class LLViewerJointAttachment;
class LLFolderView;
struct LLMoveInv;

typedef std::vector<std::string> menuentry_vec_t;
typedef std::pair<LLUUID, LLUUID> two_uuids_t;
typedef std::list<two_uuids_t> two_uuids_list_t;
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
                                       LLFolderViewModelInventory* view_model,
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
    virtual const LLUUID& getThumbnailUUID() const { return LLUUID::null; }
    virtual void clearDisplayName() { mDisplayName.clear(); }
    virtual void restoreItem() {}
    virtual void restoreToWorld() {}

    //--------------------------------------------------------------------
    // Inherited LLFolderViewModelItemInventory functions
    //--------------------------------------------------------------------
    virtual const std::string& getName() const;
    virtual const std::string& getDisplayName() const;
    const std::string& getSearchableName() const { return mSearchableName; }

    std::string getSearchableDescription() const;
    std::string getSearchableCreatorName() const;
    std::string getSearchableUUIDString() const;

    virtual PermissionMask getPermissionMask() const;
    virtual LLFolderType::EType getPreferredType() const;
    virtual time_t getCreationDate() const;
        virtual void setCreationDate(time_t creation_date_utc);
    virtual LLFontGL::StyleFlags getLabelStyle() const { return LLFontGL::NORMAL; }
    virtual std::string getLabelSuffix() const { return LLStringUtil::null; }
    virtual void openItem() {}
    virtual void closeItem() {}
    virtual void navigateToFolder(bool new_window = false, bool change_mode = false);
    virtual void showProperties();
    virtual bool isItemRenameable() const { return true; }
    virtual bool isMultiPreviewAllowed() { return true; }
    //virtual bool renameItem(const std::string& new_name) {}
    virtual bool isItemRemovable(bool check_worn = true) const;
    virtual bool isItemMovable() const;
    virtual bool isItemInTrash() const;
    virtual bool isItemInOutfits() const;
    virtual bool isLink() const;
    virtual bool isLibraryItem() const;
    //virtual bool removeItem() = 0;
    virtual void removeBatch(std::vector<LLFolderViewModelItem*>& batch);
    virtual void move(LLFolderViewModelItem* new_parent_bridge) {}
    virtual bool isItemCopyable(bool can_copy_as_link = true) const { return false; }
    virtual bool copyToClipboard() const;
    virtual bool cutToClipboard();
    virtual bool isCutToClipboard();
    virtual bool isClipboardPasteable() const;
    virtual bool isClipboardPasteableAsLink() const;
    virtual void pasteFromClipboard() {}
    virtual void pasteLinkFromClipboard() {}
    void getClipboardEntries(bool show_asset_id, menuentry_vec_t &items,
                             menuentry_vec_t &disabled_items, U32 flags);
    virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
    virtual LLToolDragAndDrop::ESource getDragSource() const;
    virtual bool startDrag(EDragAndDropType* type, LLUUID* id) const;
    virtual bool dragOrDrop(MASK mask, bool drop,
                            EDragAndDropType cargo_type,
                            void* cargo_data,
                            std::string& tooltip_msg) { return false; }
    virtual LLInventoryType::EType getInventoryType() const { return mInvType; }
    virtual LLWearableType::EType getWearableType() const { return LLWearableType::WT_NONE; }
    virtual LLSettingsType::type_e getSettingsType() const { return LLSettingsType::ST_NONE; }
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
    virtual void addMarketplaceContextMenuOptions(U32 flags,
                                             menuentry_vec_t &items,
                                             menuentry_vec_t &disabled_items);
    virtual void addLinkReplaceMenuOption(menuentry_vec_t& items,
                                          menuentry_vec_t& disabled_items);

    virtual bool canMenuDelete();
    virtual bool canMenuCut();

protected:
    LLInvFVBridge(LLInventoryPanel* inventory, LLFolderView* root, const LLUUID& uuid);

    LLInventoryModel* getInventoryModel() const;
    LLInventoryFilter* getInventoryFilter() const;

    bool isLinkedObjectInTrash() const; // Is this obj or its baseobj in the trash?
    bool isLinkedObjectMissing() const; // Is this a linked obj whose baseobj is not in inventory?

    bool isAgentInventory() const; // false if lost or in the inventory library
    bool isCOFFolder() const;       // true if COF or descendant of
    bool isInboxFolder() const;     // true if COF or descendant of   marketplace inbox

    bool isMarketplaceListingsFolder() const;     // true if descendant of Marketplace listings folder

    virtual bool isItemPermissive() const;
    static void changeItemParent(LLInventoryModel* model,
                                 LLViewerInventoryItem* item,
                                 const LLUUID& new_parent,
                                 bool restamp);
    static void changeCategoryParent(LLInventoryModel* model,
                                     LLViewerInventoryCategory* item,
                                     const LLUUID& new_parent,
                                     bool restamp);
    void removeBatchNoCheck(std::vector<LLFolderViewModelItem*>& batch);

    bool callback_cutToClipboard(const LLSD& notification, const LLSD& response);
    bool perform_cutToClipboard();

    LLHandle<LLInventoryPanel> mInventoryPanel;
    LLFolderView* mRoot;
    const LLUUID mUUID; // item id
    LLInventoryType::EType mInvType;
    bool                        mIsLink;
    LLTimer                     mTimeSinceRequestStart;
    mutable std::string         mDisplayName;
    mutable std::string         mSearchableName;

    void purgeItem(LLInventoryModel *model, const LLUUID &uuid);
    void removeObject(LLInventoryModel *model, const LLUUID &uuid);
    virtual void buildDisplayName() const {}
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryFolderViewModelBuilder
//
// This class intended to build Folder View Model via LLInvFVBridge::createBridge.
// It can be overridden with another way of creation necessary Inventory Folder View Models.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryFolderViewModelBuilder
{
public:
    LLInventoryFolderViewModelBuilder() {}
    virtual ~LLInventoryFolderViewModelBuilder() {}
    virtual LLInvFVBridge* createBridge(LLAssetType::EType asset_type,
                                        LLAssetType::EType actual_asset_type,
                                        LLInventoryType::EType inv_type,
                                        LLInventoryPanel* inventory,
                                        LLFolderViewModelInventory* view_model,
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

    typedef boost::function<void(std::string& slurl)> slurl_callback_t;

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
    virtual bool isItemRenameable() const;
    virtual bool renameItem(const std::string& new_name);
    virtual bool removeItem();
    virtual bool isItemCopyable(bool can_copy_as_link = true) const;
    virtual bool hasChildren() const { return false; }
    virtual bool isUpToDate() const { return true; }
    virtual LLUIImagePtr getIconOverlay() const;

    LLViewerInventoryItem* getItem() const;
    virtual const LLUUID& getThumbnailUUID() const;

protected:
    bool confirmRemoveItem(const LLSD& notification, const LLSD& response);
    virtual bool isItemPermissive() const;
    virtual void buildDisplayName() const;
    void doActionOnCurSelectedLandmark(LLLandmarkList::loaded_callback_t cb);

private:
    void doShowOnMap(LLLandmark* landmark);
};

class LLFolderBridge : public LLInvFVBridge
{
public:
    LLFolderBridge(LLInventoryPanel* inventory,
                   LLFolderView* root,
                   const LLUUID& uuid);

    ~LLFolderBridge();

    bool dragItemIntoFolder(LLInventoryItem* inv_item, bool drop, std::string& tooltip_msg, bool user_confirm = true, LLPointer<LLInventoryCallback> cb = NULL);
    bool dragCategoryIntoFolder(LLInventoryCategory* inv_category, bool drop, std::string& tooltip_msg, bool is_link = false, bool user_confirm = true, LLPointer<LLInventoryCallback> cb = NULL);
    void callback_dropItemIntoFolder(const LLSD& notification, const LLSD& response, LLInventoryItem* inv_item);
    void callback_dropCategoryIntoFolder(const LLSD& notification, const LLSD& response, LLInventoryCategory* inv_category);

    virtual void buildDisplayName() const;

    virtual void performAction(LLInventoryModel* model, std::string action);
    virtual void openItem();
    virtual void closeItem();
    virtual bool isItemRenameable() const;
    virtual void selectItem();
    virtual void restoreItem();

    virtual LLFolderType::EType getPreferredType() const;
    virtual LLUIImagePtr getIcon() const;
    virtual LLUIImagePtr getIconOpen() const;
    virtual LLUIImagePtr getIconOverlay() const;
    static LLUIImagePtr getIcon(LLFolderType::EType preferred_type);
    virtual std::string getLabelSuffix() const;
    virtual LLFontGL::StyleFlags getLabelStyle() const;
    virtual const LLUUID& getThumbnailUUID() const;

    void setShowDescendantsCount(bool show_count) {mShowDescendantsCount = show_count;}

    virtual bool renameItem(const std::string& new_name);

    virtual bool removeItem();
    bool removeSystemFolder();
    bool removeItemResponse(const LLSD& notification, const LLSD& response);
    void updateHierarchyCreationDate(time_t date);

    virtual void pasteFromClipboard();
    virtual void pasteLinkFromClipboard();
    virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
    virtual bool hasChildren() const;
    virtual bool dragOrDrop(MASK mask, bool drop,
                            EDragAndDropType cargo_type,
                            void* cargo_data,
                            std::string& tooltip_msg);

    virtual bool isItemRemovable(bool check_worn = true) const;
    virtual bool isItemMovable() const ;
    virtual bool isUpToDate() const;
    virtual bool isItemCopyable(bool can_copy_as_link = true) const;
    virtual bool isClipboardPasteable() const;
    virtual bool isClipboardPasteableAsLink() const;

    EInventorySortGroup getSortGroup()  const;
    virtual void update();

    static void createWearable(LLFolderBridge* bridge, LLWearableType::EType type);

    LLViewerInventoryCategory* getCategory() const;
    LLHandle<LLFolderBridge> getHandle() { mHandle.bind(this); return mHandle; }

    bool isLoading() { return mIsLoading; }

protected:
    void buildContextMenuOptions(U32 flags, menuentry_vec_t& items,   menuentry_vec_t& disabled_items);
    void buildContextMenuFolderOptions(U32 flags, menuentry_vec_t& items,   menuentry_vec_t& disabled_items);
    void addOpenFolderMenuOptions(U32 flags, menuentry_vec_t& items);

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

    bool checkFolderForContentsOfType(LLInventoryModel* model, LLInventoryCollectFunctor& typeToCheck);

    void modifyOutfit(bool append);
    void copyOutfitToClipboard();
    void determineFolderType();

    void dropToFavorites(LLInventoryItem* inv_item, LLPointer<LLInventoryCallback> cb = NULL);
    void dropToOutfit(LLInventoryItem* inv_item, bool move_is_into_current_outfit, LLPointer<LLInventoryCallback> cb = NULL);
    void dropToMyOutfits(LLInventoryCategory* inv_cat, LLPointer<LLInventoryCallback> cb = NULL);

    //--------------------------------------------------------------------
    // Messy hacks for handling folder options
    //--------------------------------------------------------------------
public:
    static LLHandle<LLFolderBridge> sSelf;
    static void staticFolderOptionsMenu();

protected:
    static void outfitFolderCreatedCallback(LLUUID cat_source_id,
                                            LLUUID cat_dest_id,
                                            LLPointer<LLInventoryCallback> cb,
                                            LLHandle<LLInventoryPanel> inventory_panel);
    void callback_pasteFromClipboard(const LLSD& notification, const LLSD& response);
    void perform_pasteFromClipboard();
    void gatherMessage(std::string& message, S32 depth, LLError::ELevel log_level);
    LLUIImagePtr getFolderIcon(bool is_open) const;

    bool                            mCallingCards;
    bool                            mWearables;
    bool                            mIsLoading;
    bool                            mShowDescendantsCount;
    LLTimer                         mTimeSinceRequestStart;
    std::string                     mMessage;
    LLRootHandle<LLFolderBridge> mHandle;

private:
    // checking if folder is cutable or deletable is expensive,
    // cache values and split check over frames
    static void onCanDeleteIdle(void* user_data);
    void initCanDeleteProcessing(LLInventoryModel* model, S32 version);
    void completeDeleteProcessing();
    bool canMenuDelete();
    bool canMenuCut();

    enum ECanDeleteState
    {
        CDS_INIT_FOLDER_CHECK,
        CDS_PROCESSING_ITEMS,
        CDS_PROCESSING_FOLDERS,
        CDS_DONE,
    };

    ECanDeleteState mCanDeleteFolderState;
    LLInventoryModel::cat_array_t mFoldersToCheck;
    LLInventoryModel::item_array_t mItemsToCheck;
    S32 mLastCheckedVersion;
    S32 mInProgressVersion;
    bool mCanDelete;
    bool mCanCut;
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
    void setFileName(const std::string& file_name) { mFileName = file_name; }
protected:
    std::string mFileName;
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
    virtual void performAction(LLInventoryModel* model, std::string action);
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
    bool mVisited;
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
    virtual bool dragOrDrop(MASK mask, bool drop,
                            EDragAndDropType cargo_type,
                            void* cargo_data,
                            std::string& tooltip_msg);
    void refreshFolderViewItem();
    void checkSearchBySuffixChanges();
protected:
    LLCallingCardObserver* mObserver;
    LLUUID mCreatorUUID;
};

class LLNotecardBridge : public LLItemBridge
{
public:
    LLNotecardBridge(LLInventoryPanel* inventory,
                     LLFolderView* root,
                     const LLUUID& uuid) :
        LLItemBridge(inventory, root, uuid) {}
    virtual void openItem();
    virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
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
    virtual bool removeItem();
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
    virtual LLUIImagePtr    getIcon() const;
    virtual void            performAction(LLInventoryModel* model, std::string action);
    virtual void            openItem();
    virtual bool isItemWearable() const { return true; }
    virtual std::string getLabelSuffix() const;
    virtual void            buildContextMenu(LLMenuGL& menu, U32 flags);
    virtual bool renameItem(const std::string& new_name);
    LLInventoryObject* getObject() const;
    LLViewerInventoryItem* getItem() const;
    LLViewerInventoryCategory* getCategory() const;
protected:
    static LLUUID sContextMenuItemID;  // Only valid while the context menu is open.
    U32 mAttachPt;
    bool mIsMultiObject;
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
    virtual void    performAction(LLInventoryModel* model, std::string action);
    virtual void    openItem();
    virtual bool isItemWearable() const { return true; }
    virtual void    buildContextMenu(LLMenuGL& menu, U32 flags);
    virtual std::string getLabelSuffix() const;
    virtual bool renameItem(const std::string& new_name);
    virtual LLWearableType::EType getWearableType() const { return mWearableType; }

    static void     onWearOnAvatar( void* userdata );   // Access to wearOnAvatar() from menu
    static bool     canWearOnAvatar( void* userdata );
    static void     onWearOnAvatarArrived( LLViewerWearable* wearable, void* userdata );
    void            wearOnAvatar();

    static void     onWearAddOnAvatarArrived( LLViewerWearable* wearable, void* userdata );
    void            wearAddOnAvatar();

    static bool     canEditOnAvatar( void* userdata );  // Access to editOnAvatar() from menu
    static void     onEditOnAvatar( void* userdata );
    void            editOnAvatar();

    static bool     canRemoveFromAvatar( void* userdata );
    void            removeFromAvatar();
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

class LLUnknownItemBridge : public LLItemBridge
{
public:
    LLUnknownItemBridge(LLInventoryPanel* inventory,
        LLFolderView* root,
        const LLUUID& uuid) :
        LLItemBridge(inventory, root, uuid) {}
    virtual LLUIImagePtr getIcon() const;
    virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
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


class LLSettingsBridge : public LLItemBridge
{
public:
    LLSettingsBridge(LLInventoryPanel* inventory,
        LLFolderView* root,
        const LLUUID& uuid,
        LLSettingsType::type_e settings_type);
    virtual LLUIImagePtr getIcon() const;
    virtual void    performAction(LLInventoryModel* model, std::string action);
    virtual void    openItem();
    virtual bool    isMultiPreviewAllowed() { return false; }
    virtual void    buildContextMenu(LLMenuGL& menu, U32 flags);
    virtual bool    renameItem(const std::string& new_name);
    virtual bool    isItemRenameable() const;
    virtual LLSettingsType::type_e getSettingsType() const { return mSettingsType; }

protected:
    bool            canUpdateRegion() const;
    bool            canUpdateParcel() const;

    LLSettingsType::type_e mSettingsType;

};

class LLMaterialBridge : public LLItemBridge
{
public:
    LLMaterialBridge(LLInventoryPanel* inventory,
                     LLFolderView* root,
                     const LLUUID& uuid) :
        LLItemBridge(inventory, root, uuid) {}
    virtual void openItem();
    virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
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
class LLRecentInventoryBridgeBuilder : public LLInventoryFolderViewModelBuilder
{
public:
    LLRecentInventoryBridgeBuilder() {}
    // Overrides FolderBridge for Recent Inventory Panel.
    // It use base functionality for bridges other than FolderBridge.
    virtual LLInvFVBridge* createBridge(LLAssetType::EType asset_type,
        LLAssetType::EType actual_asset_type,
        LLInventoryType::EType inv_type,
        LLInventoryPanel* inventory,
        LLFolderViewModelInventory* view_model,
        LLFolderView* root,
        const LLUUID& uuid,
        U32 flags = 0x00) const;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Marketplace Inventory Panel related classes
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLMarketplaceFolderBridge : public LLFolderBridge
{
public:
    // Overloads some display related methods specific to folders in a marketplace floater context
    LLMarketplaceFolderBridge(LLInventoryPanel* inventory,
                              LLFolderView* root,
                              const LLUUID& uuid);

    virtual LLUIImagePtr getIcon() const;
    virtual LLUIImagePtr getIconOpen() const;
    virtual std::string getLabelSuffix() const;
    virtual LLFontGL::StyleFlags getLabelStyle() const;

private:
    LLUIImagePtr getMarketplaceFolderIcon(bool is_open) const;
    // Those members are mutable because they are cached variablse to speed up display, not a state variables
    mutable S32 m_depth;
    mutable S32 m_stockCountCache;
};


void rez_attachment(LLViewerInventoryItem* item,
                    LLViewerJointAttachment* attachment,
                    bool replace = false);

// Move items from an in-world object's "Contents" folder to a specified
// folder in agent inventory.
bool move_inv_category_world_to_agent(const LLUUID& object_id,
                                      const LLUUID& category_id,
                                      bool drop,
                                      std::function<void(S32, void*, const LLMoveInv *)> callback = NULL,
                                      void* user_data = NULL,
                                      LLInventoryFilter* filter = NULL);

// Utility function to hide all entries except those in the list
// Can be called multiple times on the same menu (e.g. if multiple items
// are selected).  If "append" is false, then only common enabled items
// are set as enabled.
void hide_context_entries(LLMenuGL& menu,
                          const menuentry_vec_t &entries_to_show,
                          const menuentry_vec_t &disabled_entries);

// Helper functions to classify actions.
bool isAddAction(const std::string& action);
bool isRemoveAction(const std::string& action);
bool isMarketplaceCopyAction(const std::string& action);
bool isMarketplaceSendAction(const std::string& action);

class LLFolderViewGroupedItemBridge: public LLFolderViewGroupedItemModel
{
public:
    LLFolderViewGroupedItemBridge();
    virtual void groupFilterContextMenu(folder_view_item_deque& selected_items, LLMenuGL& menu);
    bool canWearSelected(const uuid_vec_t& item_ids) const;
};

struct LLMoveInv
{
    LLUUID mObjectID;
    LLUUID mCategoryID;
    two_uuids_list_t mMoveList;
    std::function<void(S32, void*, const LLMoveInv*)> mCallback;
    void* mUserData;
};

void warn_move_inventory(LLViewerObject* object, std::shared_ptr<LLMoveInv> move_inv);
bool move_task_inventory_callback(const LLSD& notification, const LLSD& response, std::shared_ptr<LLMoveInv>);

#endif // LL_LLINVENTORYBRIDGE_H
