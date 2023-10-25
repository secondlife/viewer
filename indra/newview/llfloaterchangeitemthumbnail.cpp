/** 
 * @file llfloaterchangeitemthumbnail.cpp
 * @brief LLFloaterChangeItemThumbnail class implementation
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterchangeitemthumbnail.h"

#include "llbutton.h"
#include "llclipboard.h"
#include "lliconctrl.h"
#include "llinventoryfunctions.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llfloaterreg.h"
#include "llfloatersimplesnapshot.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "llthumbnailctrl.h"
#include "llviewerfoldertype.h"
#include "llviewermenufile.h"
#include "llviewerobjectlist.h"
#include "llviewertexturelist.h"
#include "llwindow.h"


class LLThumbnailImagePicker : public LLFilePickerThread
{
public:
    LLThumbnailImagePicker(const LLUUID &item_id);
    LLThumbnailImagePicker(const LLUUID &item_id, const LLUUID &task_id);
    ~LLThumbnailImagePicker();
    void notify(const std::vector<std::string>& filenames) override;

private:
    LLUUID mInventoryId;
    LLUUID mTaskId;
};

LLThumbnailImagePicker::LLThumbnailImagePicker(const LLUUID &item_id)
    : LLFilePickerThread(LLFilePicker::FFLOAD_IMAGE)
    , mInventoryId(item_id)
{
}

LLThumbnailImagePicker::LLThumbnailImagePicker(const LLUUID &item_id, const LLUUID &task_id)
    : LLFilePickerThread(LLFilePicker::FFLOAD_IMAGE)
    , mInventoryId(item_id)
    , mTaskId(task_id)
{
}

LLThumbnailImagePicker::~LLThumbnailImagePicker()
{
}

void LLThumbnailImagePicker::notify(const std::vector<std::string>& filenames)
{
    if (filenames.empty())
    {
        return;
    }
    std::string file_path = filenames[0];
    if (file_path.empty())
    {
        return;
    }
    
    LLFloaterSimpleSnapshot::uploadThumbnail(file_path, mInventoryId, mTaskId);
}

LLFloaterChangeItemThumbnail::LLFloaterChangeItemThumbnail(const LLSD& key)
    : LLFloater(key)
    , mObserverInitialized(false)
    , mTooltipState(TOOLTIP_NONE)
{
}

LLFloaterChangeItemThumbnail::~LLFloaterChangeItemThumbnail()
{
    gInventory.removeObserver(this);
    removeVOInventoryListener();
}

BOOL LLFloaterChangeItemThumbnail::postBuild()
{
    mItemNameText = getChild<LLUICtrl>("item_name");
    mItemTypeIcon = getChild<LLIconCtrl>("item_type_icon");
    mThumbnailCtrl = getChild<LLThumbnailCtrl>("item_thumbnail");
    mToolTipTextBox = getChild<LLTextBox>("tooltip_text");

    LLSD tooltip_text;
    mToolTipTextBox->setValue(tooltip_text);

    LLButton *upload_local = getChild<LLButton>("upload_local");
    upload_local->setClickedCallback(onUploadLocal, (void*)this);
    upload_local->setMouseEnterCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseEnter, this, _1, _2, TOOLTIP_UPLOAD_LOCAL));
    upload_local->setMouseLeaveCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseLeave, this, _1, _2, TOOLTIP_UPLOAD_LOCAL));

    LLButton *upload_snapshot = getChild<LLButton>("upload_snapshot");
    upload_snapshot->setClickedCallback(onUploadSnapshot, (void*)this);
    upload_snapshot->setMouseEnterCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseEnter, this, _1, _2, TOOLTIP_UPLOAD_SNAPSHOT));
    upload_snapshot->setMouseLeaveCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseLeave, this, _1, _2, TOOLTIP_UPLOAD_SNAPSHOT));

    LLButton *use_texture = getChild<LLButton>("use_texture");
    use_texture->setClickedCallback(onUseTexture, (void*)this);
    use_texture->setMouseEnterCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseEnter, this, _1, _2, TOOLTIP_USE_TEXTURE));
    use_texture->setMouseLeaveCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseLeave, this, _1, _2, TOOLTIP_USE_TEXTURE));

    mCopyToClipboardBtn = getChild<LLButton>("copy_to_clipboard");
    mCopyToClipboardBtn->setClickedCallback(onCopyToClipboard, (void*)this);
    mCopyToClipboardBtn->setMouseEnterCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseEnter, this, _1, _2, TOOLTIP_COPY_TO_CLIPBOARD));
    mCopyToClipboardBtn->setMouseLeaveCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseLeave, this, _1, _2, TOOLTIP_COPY_TO_CLIPBOARD));

    mPasteFromClipboardBtn = getChild<LLButton>("paste_from_clipboard");
    mPasteFromClipboardBtn->setClickedCallback(onPasteFromClipboard, (void*)this);
    mPasteFromClipboardBtn->setMouseEnterCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseEnter, this, _1, _2, TOOLTIP_COPY_FROM_CLIPBOARD));
    mPasteFromClipboardBtn->setMouseLeaveCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseLeave, this, _1, _2, TOOLTIP_COPY_FROM_CLIPBOARD));

    mRemoveImageBtn = getChild<LLButton>("remove_image");
    mRemoveImageBtn->setClickedCallback(onRemove, (void*)this);
    mRemoveImageBtn->setMouseEnterCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseEnter, this, _1, _2, TOOLTIP_REMOVE));
    mRemoveImageBtn->setMouseLeaveCallback(boost::bind(&LLFloaterChangeItemThumbnail::onButtonMouseLeave, this, _1, _2, TOOLTIP_REMOVE));

    return LLFloater::postBuild();
}

void LLFloaterChangeItemThumbnail::onOpen(const LLSD& key)
{
    if (!key.has("item_id") && !key.isUUID())
    {
        closeFloater();
    }

    if (key.isUUID())
    {
        mItemId = key.asUUID();
    }
    else
    {
        mItemId = key["item_id"].asUUID();
        mTaskId = key["task_id"].asUUID();
    }

    refreshFromInventory();
}

void LLFloaterChangeItemThumbnail::onFocusReceived()
{
    mPasteFromClipboardBtn->setEnabled(LLClipboard::instance().hasContents());
}

void LLFloaterChangeItemThumbnail::onMouseEnter(S32 x, S32 y, MASK mask)
{
    mPasteFromClipboardBtn->setEnabled(LLClipboard::instance().hasContents());
}

BOOL LLFloaterChangeItemThumbnail::handleDragAndDrop(
    S32 x,
    S32 y,
    MASK mask,
    BOOL drop,
    EDragAndDropType cargo_type,
    void *cargo_data,
    EAcceptance *accept,
    std::string& tooltip_msg)
{
    if (cargo_type == DAD_TEXTURE)
    {
        LLInventoryItem *item = (LLInventoryItem *)cargo_data;
        if (item->getAssetUUID().notNull())
        {
            if (drop)
            {
                assignAndValidateAsset(item->getAssetUUID());
            }

            *accept = ACCEPT_YES_SINGLE;
        }
        else
        {
            *accept = ACCEPT_NO;
        }
    }
    else
    {
        *accept = ACCEPT_NO;
    }

    LL_DEBUGS("UserInput") << "dragAndDrop handled by LLFloaterChangeItemThumbnail " << getKey() << LL_ENDL;

    return TRUE;
}

void LLFloaterChangeItemThumbnail::changed(U32 mask)
{
    //LLInventoryObserver

    if (mTaskId.notNull() || mItemId.isNull())
    {
        // Task inventory or not set up yet
        return;
    }

    const std::set<LLUUID>& mChangedItemIDs = gInventory.getChangedIDs();
    std::set<LLUUID>::const_iterator it;

    for (it = mChangedItemIDs.begin(); it != mChangedItemIDs.end(); it++)
    {
        // set dirty for 'item profile panel' only if changed item is the item for which 'item profile panel' is shown (STORM-288)
        if (*it == mItemId)
        {
            // if there's a change we're interested in.
            if ((mask & (LLInventoryObserver::LABEL | LLInventoryObserver::INTERNAL | LLInventoryObserver::REMOVE)) != 0)
            {
                refreshFromInventory();
            }
        }
    }
}

void LLFloaterChangeItemThumbnail::inventoryChanged(LLViewerObject* object,
    LLInventoryObject::object_list_t* inventory,
    S32 serial_num,
    void* user_data)
{
    //LLVOInventoryListener
    refreshFromInventory();
}

LLInventoryObject* LLFloaterChangeItemThumbnail::getInventoryObject()
{
    LLInventoryObject* obj = NULL;
    if (mTaskId.isNull())
    {
        // it is in agent inventory
        if (!mObserverInitialized)
        {
            gInventory.addObserver(this);
            mObserverInitialized = true;
        }

        obj = gInventory.getObject(mItemId);
    }
    else
    {
        LLViewerObject* object = gObjectList.findObject(mTaskId);
        if (object)
        {
            if (!mObserverInitialized)
            {
                registerVOInventoryListener(object, NULL);
                mObserverInitialized = false;
            }

            obj = object->getInventoryObject(mItemId);
        }
    }
    return obj;
}

void LLFloaterChangeItemThumbnail::refreshFromInventory()
{
    LLInventoryObject* obj = getInventoryObject();
    if (!obj)
    {
        closeFloater();
    }

    if (obj)
    {
        const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
        bool in_trash = gInventory.isObjectDescendentOf(obj->getUUID(), trash_id);
        if (in_trash && obj->getUUID() != trash_id)
        {
            // Close properties when moving to trash
            // Aren't supposed to view properties from trash
            closeFloater();
        }
        else
        {
            refreshFromObject(obj);
        }
    }
    else
    {
        closeFloater();
    }
}

class LLIsOutfitTextureType : public LLInventoryCollectFunctor
{
public:
    LLIsOutfitTextureType() {}
    virtual ~LLIsOutfitTextureType() {}
    virtual bool operator()(LLInventoryCategory* cat,
        LLInventoryItem* item);
};

bool LLIsOutfitTextureType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
    return item && (item->getType() == LLAssetType::AT_TEXTURE);
}

void LLFloaterChangeItemThumbnail::refreshFromObject(LLInventoryObject* obj)
{
    LLUIImagePtr icon_img;
    LLUUID thumbnail_id = obj->getThumbnailUUID();

    LLViewerInventoryItem* item = dynamic_cast<LLViewerInventoryItem*>(obj);
    if (item)
    {
        setTitle(getString("title_item_thumbnail"));

        icon_img = LLInventoryIcon::getIcon(item->getType(), item->getInventoryType(), item->getFlags(), FALSE);
        mRemoveImageBtn->setEnabled(thumbnail_id.notNull() && ((item->getActualType() != LLAssetType::AT_TEXTURE) || (item->getAssetUUID() != thumbnail_id)));
    }
    else
    {
        LLViewerInventoryCategory* cat = dynamic_cast<LLViewerInventoryCategory*>(obj);

        if (cat)
        {
            setTitle(getString("title_folder_thumbnail"));
            icon_img = LLUI::getUIImage(LLViewerFolderType::lookupIconName(cat->getPreferredType(), true));

            if (thumbnail_id.isNull() && (cat->getPreferredType() == LLFolderType::FT_OUTFIT))
            {
                // Legacy support, check if there is an image inside

                LLInventoryModel::cat_array_t cats;
                LLInventoryModel::item_array_t items;
                // Not LLIsOfAssetType, because we allow links
                LLIsOutfitTextureType f;
                gInventory.getDirectDescendentsOf(mItemId, cats, items, f);

                if (1 == items.size())
                {
                    LLViewerInventoryItem* item = items.front();
                    if (item && item->getIsLinkType())
                    {
                        item = item->getLinkedItem();
                    }
                    if (item)
                    {
                        thumbnail_id = item->getAssetUUID();
                        if (thumbnail_id.notNull())
                        {
                            // per SL-19188, set this image as a thumbnail
                            LL_INFOS() << "Setting image " << thumbnail_id
                                       << " from outfit as a thumbnail for inventory object " << obj->getUUID()
                                       << LL_ENDL;
                            assignAndValidateAsset(thumbnail_id, true);
                        }
                    }
                }
            }

            mRemoveImageBtn->setEnabled(thumbnail_id.notNull());
        }
    }
    mItemTypeIcon->setImage(icon_img);
    mItemNameText->setValue(obj->getName());

    mThumbnailCtrl->setValue(thumbnail_id);

    mCopyToClipboardBtn->setEnabled(thumbnail_id.notNull());
    mPasteFromClipboardBtn->setEnabled(LLClipboard::instance().hasContents());

    // todo: some elements might not support setting thumbnails
    // since they already have them
    // It is unclear how system folders should function
}

void LLFloaterChangeItemThumbnail::onUploadLocal(void *userdata)
{
    LLFloaterChangeItemThumbnail *self = (LLFloaterChangeItemThumbnail*)userdata;

    (new LLThumbnailImagePicker(self->mItemId, self->mTaskId))->getFile();

    LLFloater* floaterp = self->mPickerHandle.get();
    if (floaterp)
    {
        floaterp->closeFloater();
    }
    floaterp = self->mSnapshotHandle.get();
    if (floaterp)
    {
        floaterp->closeFloater();
    }
}

void LLFloaterChangeItemThumbnail::onUploadSnapshot(void *userdata)
{
    LLFloaterChangeItemThumbnail *self = (LLFloaterChangeItemThumbnail*)userdata;

    LLFloater* floaterp = self->mSnapshotHandle.get();
    // Show the dialog
    if (floaterp)
    {
        floaterp->openFloater();
    }
    else
    {
        LLSD key;
        key["item_id"] = self->mItemId;
        key["task_id"] = self->mTaskId;
        LLFloaterSimpleSnapshot* snapshot_floater = (LLFloaterSimpleSnapshot*)LLFloaterReg::showInstance("simple_snapshot", key, true);
        if (snapshot_floater)
        {
            self->addDependentFloater(snapshot_floater);
            self->mSnapshotHandle = snapshot_floater->getHandle();
            snapshot_floater->setOwner(self);
        }
    }

    floaterp = self->mPickerHandle.get();
    if (floaterp)
    {
        floaterp->closeFloater();
    }
}

void LLFloaterChangeItemThumbnail::onUseTexture(void *userdata)
{
    LLFloaterChangeItemThumbnail *self = (LLFloaterChangeItemThumbnail*)userdata;
    LLInventoryObject* obj = self->getInventoryObject();
    if (obj)
    {
        self->showTexturePicker(obj->getThumbnailUUID());
    }

    LLFloater* floaterp = self->mSnapshotHandle.get();
    if (floaterp)
    {
        floaterp->closeFloater();
    }
}

void LLFloaterChangeItemThumbnail::onCopyToClipboard(void *userdata)
{
    LLFloaterChangeItemThumbnail *self = (LLFloaterChangeItemThumbnail*)userdata;
    LLInventoryObject* obj = self->getInventoryObject();
    if (obj)
    {
        LLClipboard::instance().reset();
        LLClipboard::instance().addToClipboard(obj->getThumbnailUUID(), LLAssetType::AT_NONE);
        self->mPasteFromClipboardBtn->setEnabled(true);
    }
}

void LLFloaterChangeItemThumbnail::onPasteFromClipboard(void *userdata)
{
    LLFloaterChangeItemThumbnail *self = (LLFloaterChangeItemThumbnail*)userdata;
    std::vector<LLUUID> objects;
    LLClipboard::instance().pasteFromClipboard(objects);
    if (objects.size() > 0)
    {
        LLUUID potential_uuid = objects[0];
        LLUUID asset_id;

        if (potential_uuid.notNull())
        {
            LLViewerInventoryItem* item = gInventory.getItem(potential_uuid);
            if (item)
            {
                // no point checking snapshot?
                if (item->getType() == LLAssetType::AT_TEXTURE)
                {
                    bool copy = item->getPermissions().allowCopyBy(gAgent.getID());
                    bool xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID());

                    if (copy && xfer)
                    {
                        asset_id = item->getAssetUUID();
                    }
                    else
                    {
                        LLNotificationsUtil::add("ThumbnailInsufficientPermissions");
                        return;
                    }
                }
            }
            else
            {
                // assume that this is a texture
                asset_id = potential_uuid;
            }
        }

        LLInventoryObject* obj = self->getInventoryObject();
        if (obj && obj->getThumbnailUUID() == asset_id)
        {
            // nothing to do
            return;
        }
        if (asset_id.notNull())
        {
            self->assignAndValidateAsset(asset_id);
        }
        // else show 'buffer has no texture' warning?
    }
}

void LLFloaterChangeItemThumbnail::onRemove(void *userdata)
{
    LLFloaterChangeItemThumbnail *self = (LLFloaterChangeItemThumbnail*)userdata;

    LLSD payload;
    payload["item_id"] = self->mItemId;
    payload["object_id"] = self->mTaskId;
    LLNotificationsUtil::add("DeleteThumbnail", LLSD(), payload, boost::bind(&LLFloaterChangeItemThumbnail::onRemovalConfirmation, _1, _2, self->getHandle()));
}

// static 
void LLFloaterChangeItemThumbnail::onRemovalConfirmation(const LLSD& notification, const LLSD& response, LLHandle<LLFloater> handle)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0 && !handle.isDead() && !handle.get()->isDead())
    {
        LLFloaterChangeItemThumbnail* self = (LLFloaterChangeItemThumbnail*)handle.get();
        self->setThumbnailId(LLUUID::null);
    }
}

struct ImageLoadedData
{
    LLUUID mThumbnailId;
    LLUUID mObjectId;
    LLHandle<LLFloater> mFloaterHandle;
    bool mSilent;
    // Keep image reference to prevent deletion on timeout
    LLPointer<LLViewerFetchedTexture> mTexturep;
};

void LLFloaterChangeItemThumbnail::assignAndValidateAsset(const LLUUID &asset_id, bool silent)
{
    LLPointer<LLViewerFetchedTexture> texturep = LLViewerTextureManager::getFetchedTexture(asset_id);
    if (texturep->isMissingAsset())
    {
        LL_WARNS() << "Attempted to assign missing asset " << asset_id << LL_ENDL;
        if (!silent)
        {
            LLNotificationsUtil::add("ThumbnailDimentionsLimit");
        }
    }
    else if (texturep->getFullWidth() == 0)
    {
        if (silent)
        {
            mExpectingAssetId = LLUUID::null;
        }
        else
        {
            // don't warn user multiple times if some textures took their time
            mExpectingAssetId = asset_id;
        }
        ImageLoadedData *data = new ImageLoadedData();
        data->mObjectId = mItemId;
        data->mThumbnailId = asset_id;
        data->mFloaterHandle = getHandle();
        data->mSilent = silent;
        data->mTexturep = texturep;

        texturep->setLoadedCallback(onImageDataLoaded,
            MAX_DISCARD_LEVEL, // Don't need full image, just size data
            FALSE,
            FALSE,
            (void*)data,
            NULL,
            FALSE);
    }
    else
    {
        if (validateAsset(asset_id))
        {
            setThumbnailId(asset_id);
        }
        else if (!silent)
        {
            LLNotificationsUtil::add("ThumbnailDimentionsLimit");
        }
    }
}
bool LLFloaterChangeItemThumbnail::validateAsset(const LLUUID &asset_id)
{
    if (asset_id.isNull())
    {
        return false;
    }

    LLPointer<LLViewerFetchedTexture> texturep = LLViewerTextureManager::findFetchedTexture(asset_id, TEX_LIST_STANDARD);

    if (!texturep)
    {
        return false;
    }

    if (texturep->isMissingAsset())
    {
        return false;
    }

    if (texturep->getFullWidth() != texturep->getFullHeight())
    {
        return false;
    }

    if (texturep->getFullWidth() > LLFloaterSimpleSnapshot::THUMBNAIL_SNAPSHOT_DIM_MAX
        || texturep->getFullHeight() > LLFloaterSimpleSnapshot::THUMBNAIL_SNAPSHOT_DIM_MAX)
    {
        return false;
    }

    if (texturep->getFullWidth() < LLFloaterSimpleSnapshot::THUMBNAIL_SNAPSHOT_DIM_MIN
        || texturep->getFullHeight() < LLFloaterSimpleSnapshot::THUMBNAIL_SNAPSHOT_DIM_MIN)
    {
        return false;
    }
    return true;
}

//static
void LLFloaterChangeItemThumbnail::onImageDataLoaded(
    BOOL success,
    LLViewerFetchedTexture *src_vi,
    LLImageRaw* src,
    LLImageRaw* aux_src,
    S32 discard_level,
    BOOL final,
    void* userdata)
{
    if (!userdata) return;

    if (!final && success) return; //not done yet

    ImageLoadedData* data = (ImageLoadedData*)userdata;

    if (success)
    {
        // Update the item, set it even if floater is dead
        if (validateAsset(data->mThumbnailId))
        {
            setThumbnailId(data->mThumbnailId, data->mObjectId);
        }
        else if (!data->mSilent)
        {
            // Should this only appear if floater is alive?
            LLNotificationsUtil::add("ThumbnailDimentionsLimit");
        }
    }

    // Update floater
    if (!data->mSilent && !data->mFloaterHandle.isDead())
    {
        LLFloaterChangeItemThumbnail* self = static_cast<LLFloaterChangeItemThumbnail*>(data->mFloaterHandle.get());
        if (self && self->mExpectingAssetId == data->mThumbnailId)
        {
            self->mExpectingAssetId = LLUUID::null;
        }
    }

    delete data;
}

//static
void LLFloaterChangeItemThumbnail::onFullImageLoaded(
    BOOL success,
    LLViewerFetchedTexture* src_vi,
    LLImageRaw* src,
    LLImageRaw* aux_src,
    S32 discard_level,
    BOOL final,
    void* userdata)
{
    if (!userdata) return;

    if (!final && success) return; //not done yet

    ImageLoadedData* data = (ImageLoadedData*)userdata;

    if (success)
    {
        if (src_vi->getFullWidth() != src_vi->getFullHeight()
            || src_vi->getFullWidth() < LLFloaterSimpleSnapshot::THUMBNAIL_SNAPSHOT_DIM_MIN)
        {
            if (!data->mSilent)
            {
                LLNotificationsUtil::add("ThumbnailDimentionsLimit");
            }
        }
        else if (src_vi->getFullWidth() > LLFloaterSimpleSnapshot::THUMBNAIL_SNAPSHOT_DIM_MAX)
        {
            LLFloaterSimpleSnapshot::uploadThumbnail(src, data->mObjectId, LLUUID::null);
        }
        else
        {
            setThumbnailId(data->mThumbnailId, data->mObjectId);
        }
    }

    delete data;
}

void LLFloaterChangeItemThumbnail::showTexturePicker(const LLUUID &thumbnail_id)
{
    // show hourglass cursor when loading inventory window
    getWindow()->setCursor(UI_CURSOR_WAIT);

    LLFloater* floaterp = mPickerHandle.get();
    // Show the dialog
    if (floaterp)
    {
        floaterp->openFloater();
    }
    else
    {
        floaterp = new LLFloaterTexturePicker(
            this,
            thumbnail_id,
            thumbnail_id,
            thumbnail_id,
            FALSE,
            TRUE,
            "SELECT PHOTO",
            PERM_NONE,
            PERM_NONE,
            FALSE,
            NULL);

        mPickerHandle = floaterp->getHandle();

        LLFloaterTexturePicker* texture_floaterp = dynamic_cast<LLFloaterTexturePicker*>(floaterp);
        if (texture_floaterp)
        {
            //texture_floaterp->setTextureSelectedCallback();
            //texture_floaterp->setOnUpdateImageStatsCallback();
            texture_floaterp->setOnFloaterCommitCallback([this](LLTextureCtrl::ETexturePickOp op, LLPickerSource, const LLUUID&, const LLUUID&, const LLUUID&)
            {
                if (op == LLTextureCtrl::TEXTURE_SELECT)
                {
                    onTexturePickerCommit();
                }
            }
            );

            texture_floaterp->setLocalTextureEnabled(FALSE);
            texture_floaterp->setBakeTextureEnabled(FALSE);
            texture_floaterp->setCanApplyImmediately(false);
            texture_floaterp->setCanApply(false, true, false /*Hide 'preview disabled'*/);
            texture_floaterp->setMinDimentionsLimits(LLFloaterSimpleSnapshot::THUMBNAIL_SNAPSHOT_DIM_MIN);

            addDependentFloater(texture_floaterp);
        }

        floaterp->openFloater();
    }
    floaterp->setFocus(TRUE);
}

void LLFloaterChangeItemThumbnail::onTexturePickerCommit()
{
    LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mPickerHandle.get();

    if (floaterp)
    {
        LLUUID asset_id = floaterp->getAssetID();

        if (asset_id.isNull())
        {
            setThumbnailId(asset_id);
            return;
        }

        LLInventoryObject* obj = getInventoryObject();
        if (obj && obj->getThumbnailUUID() == asset_id)
        {
            // nothing to do
            return;
        }

        LLPointer<LLViewerFetchedTexture> texturep = LLViewerTextureManager::findFetchedTexture(asset_id, TEX_LIST_STANDARD);
        if (!texturep)
        {
            LL_WARNS() << "Image " << asset_id << " doesn't exist" << LL_ENDL;
            return;
        }

        if (texturep->isMissingAsset())
        {
            LL_WARNS() << "Image " << asset_id << " is missing" << LL_ENDL;
            return;
        }

        if (texturep->getFullWidth() != texturep->getFullHeight())
        {
            LLNotificationsUtil::add("ThumbnailDimentionsLimit");
            return;
        }

        if (texturep->getFullWidth() < LLFloaterSimpleSnapshot::THUMBNAIL_SNAPSHOT_DIM_MIN
            && texturep->getFullWidth() > 0)
        {
            LLNotificationsUtil::add("ThumbnailDimentionsLimit");
            return;
        }

        if (texturep->getFullWidth() > LLFloaterSimpleSnapshot::THUMBNAIL_SNAPSHOT_DIM_MAX
            || texturep->getFullWidth() == 0)
        {
            if (texturep->isFullyLoaded()
                && (texturep->getCachedRawImageLevel() == 0 || texturep->getRawImageLevel() == 0)
                && (texturep->isCachedRawImageReady() || texturep->isRawImageValid()))
            {
                if (texturep->isRawImageValid())
                {
                    LLFloaterSimpleSnapshot::uploadThumbnail(texturep->getRawImage(), mItemId, mTaskId);
                }
                else
                {
                    LLFloaterSimpleSnapshot::uploadThumbnail(texturep->getCachedRawImage(), mItemId, mTaskId);
                }
            }
            else
            {
                ImageLoadedData* data = new ImageLoadedData();
                data->mObjectId = mItemId;
                data->mThumbnailId = asset_id;
                data->mFloaterHandle = getHandle();
                data->mSilent = false;
                data->mTexturep = texturep;

                texturep->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
                texturep->setMinDiscardLevel(0);
                texturep->setLoadedCallback(onFullImageLoaded,
                                            0, // Need best quality
                                            TRUE,
                                            FALSE,
                                            (void*)data,
                                            NULL,
                                            FALSE);
                texturep->forceToSaveRawImage(0);
            }
            return;
        }

        setThumbnailId(asset_id);
    }
}


void LLFloaterChangeItemThumbnail::setThumbnailId(const LLUUID &new_thumbnail_id)
{
    LLInventoryObject* obj = getInventoryObject();
    if (!obj)
    {
        return;
    }

    if (mTaskId.notNull())
    {
        LL_ERRS() << "Not implemented yet" << LL_ENDL;
        return;
    }

    setThumbnailId(new_thumbnail_id, mItemId, obj);
}

void LLFloaterChangeItemThumbnail::setThumbnailId(const LLUUID& new_thumbnail_id, const LLUUID& object_id)
{
    LLInventoryObject* obj = gInventory.getObject(object_id);
    if (!obj)
    {
        return;
    }

    setThumbnailId(new_thumbnail_id, object_id, obj);
}
void LLFloaterChangeItemThumbnail::setThumbnailId(const LLUUID& new_thumbnail_id, const LLUUID& object_id, LLInventoryObject* obj)
{
    if (obj->getThumbnailUUID() != new_thumbnail_id)
    {
        LLSD updates;
        if (new_thumbnail_id.notNull())
        {
            // At the moment server expects id as a string
            updates["thumbnail"] = LLSD().with("asset_id", new_thumbnail_id.asString());
        }
        else
        {
            // No thumbnail isntead of 'null id thumbnail'
            updates["thumbnail"] = LLSD();
        }
        LLViewerInventoryCategory* view_folder = dynamic_cast<LLViewerInventoryCategory*>(obj);
        if (view_folder)
        {
            update_inventory_category(object_id, updates, NULL);
        }
        LLViewerInventoryItem* view_item = dynamic_cast<LLViewerInventoryItem*>(obj);
        if (view_item)
        {
            update_inventory_item(object_id, updates, NULL);
        }
    }
}

void LLFloaterChangeItemThumbnail::onButtonMouseEnter(LLUICtrl* button, const LLSD& param, EToolTipState state)
{
    mTooltipState = state;

    std::string tooltip_text;
    std::string tooltip_name = "tooltip_" + button->getName();
    if (hasString(tooltip_name))
    {
        tooltip_text = getString(tooltip_name);
    }

    mToolTipTextBox->setValue(tooltip_text);
}

void LLFloaterChangeItemThumbnail::onButtonMouseLeave(LLUICtrl* button, const LLSD& param, EToolTipState state)
{
    if (mTooltipState == state)
    {
        mTooltipState = TOOLTIP_NONE;
        LLSD tooltip_text;
        mToolTipTextBox->setValue(tooltip_text);
    }
}

