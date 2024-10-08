/**
 * @file lltexturectrl.cpp
 * @author Richard Nelson, James Cook
 * @brief LLTextureCtrl class implementation including related functions
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

#include "llviewerprecompiledheaders.h"

#include "lltexturectrl.h"

#include "llrender.h"
#include "llagent.h"
#include "llviewertexturelist.h"
#include "llselectmgr.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llbutton.h"
#include "lldraghandle.h"
#include "llfocusmgr.h"
#include "llfolderviewmodel.h"
#include "llinventory.h"
#include "llinventoryfunctions.h"
#include "llinventoryicon.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "lllineeditor.h"
#include "llmaterialeditor.h"
#include "llui.h"
#include "llviewerinventory.h"
#include "llviewermenufile.h"   // LLFilePickerReplyThread
#include "llpermissions.h"
#include "llpreviewtexture.h"
#include "llsaleinfo.h"
#include "llassetstorage.h"
#include "lltextbox.h"
#include "llresizehandle.h"
#include "llscrollcontainer.h"
#include "lltoolmgr.h"
#include "lltoolpipette.h"
#include "llfiltereditor.h"
#include "llwindow.h"

#include "lltool.h"
#include "llviewerwindow.h"
#include "llviewerobject.h"
#include "llviewercontrol.h"
#include "llglheaders.h"
#include "lluictrlfactory.h"
#include "lltrans.h"

#include "llradiogroup.h"
#include "llfloaterreg.h"
#include "llgltfmaterialpreviewmgr.h"
#include "lllocalbitmaps.h"
#include "lllocalgltfmaterials.h"
#include "llerror.h"

#include "llavatarappearancedefines.h"


//static
bool get_is_predefined_texture(LLUUID asset_id)
{
    if (asset_id == DEFAULT_OBJECT_TEXTURE
        || asset_id == DEFAULT_OBJECT_SPECULAR
        || asset_id == DEFAULT_OBJECT_NORMAL
        || asset_id == BLANK_OBJECT_NORMAL
        || asset_id == IMG_WHITE
        || asset_id == LLUUID(SCULPT_DEFAULT_TEXTURE))
    {
        return true;
    }
    return false;
}

LLUUID get_copy_free_item_by_asset_id(LLUUID asset_id, bool no_trans_perm)
{
    LLViewerInventoryCategory::cat_array_t cats;
    LLViewerInventoryItem::item_array_t items;
    LLAssetIDMatches asset_id_matches(asset_id);
    gInventory.collectDescendentsIf(LLUUID::null,
        cats,
        items,
        LLInventoryModel::INCLUDE_TRASH,
        asset_id_matches);

    LLUUID res;
    if (items.size())
    {
        for (S32 i = 0; i < items.size(); i++)
        {
            LLViewerInventoryItem* itemp = items[i];
            if (itemp)
            {
                LLPermissions item_permissions = itemp->getPermissions();
                if (item_permissions.allowOperationBy(PERM_COPY,
                    gAgent.getID(),
                    gAgent.getGroupID()))
                {
                    bool allow_trans = item_permissions.allowOperationBy(PERM_TRANSFER, gAgent.getID(), gAgent.getGroupID());
                    if (allow_trans != no_trans_perm)
                    {
                        return itemp->getUUID();
                    }
                    res = itemp->getUUID();
                }
            }
        }
    }
    return res;
}

bool get_can_copy_texture(LLUUID asset_id)
{
    // User is allowed to copy a texture if:
    // library asset or default texture,
    // or copy perm asset exists in user's inventory

    return get_is_predefined_texture(asset_id) || get_copy_free_item_by_asset_id(asset_id).notNull();
}

S32 LLFloaterTexturePicker::sLastPickerMode = 0;

LLFloaterTexturePicker::LLFloaterTexturePicker(
    LLView* owner,
    LLUUID image_asset_id,
    LLUUID default_image_asset_id,
    LLUUID blank_image_asset_id,
    bool tentative,
    bool allow_no_texture,
    const std::string& label,
    PermissionMask immediate_filter_perm_mask,
    PermissionMask dnd_filter_perm_mask,
    bool can_apply_immediately,
    LLUIImagePtr fallback_image,
    EPickInventoryType pick_type)
:   LLFloater(LLSD()),
    mOwner( owner ),
    mImageAssetID( image_asset_id ),
    mOriginalImageAssetID(image_asset_id),
    mFallbackImage(fallback_image),
    mDefaultImageAssetID(default_image_asset_id),
    mBlankImageAssetID(blank_image_asset_id),
    mTentative(tentative),
    mAllowNoTexture(allow_no_texture),
    mLabel(label),
    mTentativeLabel(NULL),
    mResolutionLabel(NULL),
    mActive( true ),
    mFilterEdit(NULL),
    mImmediateFilterPermMask(immediate_filter_perm_mask),
    mDnDFilterPermMask(dnd_filter_perm_mask),
    mContextConeOpacity(0.f),
    mSelectedItemPinned( false ),
    mCanApply(true),
    mCanPreview(true),
    mLimitsSet(false),
    mMaxDim(S32_MAX),
    mMinDim(0),
    mPreviewSettingChanged(false),
    mOnFloaterCommitCallback(NULL),
    mOnFloaterCloseCallback(NULL),
    mSetImageAssetIDCallback(NULL),
    mOnUpdateImageStatsCallback(NULL),
    mBakeTextureEnabled(false),
    mInventoryPickType(pick_type),
    mSelectionSource(PICKER_UNKNOWN)
{
    mCanApplyImmediately = can_apply_immediately;
    buildFromFile("floater_texture_ctrl.xml");
    setCanMinimize(false);
}

LLFloaterTexturePicker::~LLFloaterTexturePicker()
{
}

void LLFloaterTexturePicker::setImageID(const LLUUID& image_id, bool set_selection /*=true*/)
{
    if( ((mImageAssetID != image_id) || mTentative) && mActive)
    {
        mNoCopyTextureSelected = false;
        mViewModel->setDirty(); // *TODO: shouldn't we be using setValue() here?
        mImageAssetID = image_id;
        mSelectionSource = PICKER_UNKNOWN;

        if (LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(mImageAssetID))
        {
            if ( mBakeTextureEnabled && mModeSelector->getValue().asInteger() != 2)
            {
                mModeSelector->selectByValue(2);
                onModeSelect(0,this);
            }
            mSelectionSource = PICKER_BAKE;
        }
        else
        {
            if (mModeSelector->getValue().asInteger() == 2)
            {
                mModeSelector->selectByValue(0);
                onModeSelect(0,this);
            }

            LLUUID item_id;
            LLFolderView* root_folder = mInventoryPanel->getRootFolder();
            if (root_folder && root_folder->getCurSelectedItem())
            {
                LLFolderViewItem* last_selected = root_folder->getCurSelectedItem();
                LLFolderViewModelItemInventory* inv_view = static_cast<LLFolderViewModelItemInventory*>(last_selected->getViewModelItem());

                LLInventoryItem* itemp = gInventory.getItem(inv_view->getUUID());

                if (mInventoryPickType == PICK_MATERIAL
                    && mImageAssetID == BLANK_MATERIAL_ASSET_ID
                    && itemp && itemp->getAssetUUID().isNull())
                {
                    item_id = inv_view->getUUID();
                }
                else if (itemp && itemp->getAssetUUID() == mImageAssetID)
                {
                    item_id = inv_view->getUUID();
                }
            }
            if (item_id.isNull())
            {
                item_id = findItemID(mImageAssetID, false);
            }
            if (item_id.isNull())
            {
                mInventoryPanel->getRootFolder()->clearSelection();
            }
            else
            {
                LLInventoryItem* itemp = gInventory.getItem(item_id);
                if (itemp && !itemp->getPermissions().allowCopyBy(gAgent.getID()))
                {
                    // no copy texture
                    getChild<LLUICtrl>("apply_immediate_check")->setValue(false);
                    mNoCopyTextureSelected = true;
                }
                mSelectionSource = PICKER_INVENTORY;
            }

            if (set_selection)
            {
                mInventoryPanel->setSelection(item_id, TAKE_FOCUS_NO);
            }
        }
    }
}

void LLFloaterTexturePicker::setImageIDFromItem(const LLInventoryItem* itemp, bool set_selection)
{
    LLUUID asset_id = itemp->getAssetUUID();
    if (mInventoryPickType == PICK_MATERIAL && asset_id.isNull())
    {
        // If an inventory item has a null asset, consider it a valid blank material(gltf)
        asset_id = BLANK_MATERIAL_ASSET_ID;
    }
    setImageID(asset_id, set_selection);
    mSelectionSource = PICKER_INVENTORY;
}

void LLFloaterTexturePicker::setActive( bool active )
{
    if (!active && getChild<LLUICtrl>("Pipette")->getValue().asBoolean())
    {
        stopUsingPipette();
    }
    mActive = active;
}

void LLFloaterTexturePicker::setCanApplyImmediately(bool b)
{
    mCanApplyImmediately = b;

    LLUICtrl *apply_checkbox = getChild<LLUICtrl>("apply_immediate_check");
    apply_checkbox->setValue(mCanApplyImmediately && gSavedSettings.getBOOL("TextureLivePreview"));
    apply_checkbox->setEnabled(mCanApplyImmediately);
}

void LLFloaterTexturePicker::stopUsingPipette()
{
    if (LLToolMgr::getInstance()->getCurrentTool() == LLToolPipette::getInstance())
    {
        LLToolMgr::getInstance()->clearTransientTool();
    }
}

bool LLFloaterTexturePicker::updateImageStats()
{
    bool result = true;
    if (mGLTFMaterial.notNull())
    {
        S32 width = 0;
        S32 height = 0;

        bool has_texture = false;

        if (mGLTFMaterial->mBaseColorTexture)
        {
            width = llmax(width, mGLTFMaterial->mBaseColorTexture->getFullWidth());
            height = llmax(height, mGLTFMaterial->mBaseColorTexture->getFullHeight());
            has_texture = true;
        }
        if (mGLTFMaterial->mNormalTexture)
        {
            width = llmax(width, mGLTFMaterial->mNormalTexture->getFullWidth());
            height = llmax(height, mGLTFMaterial->mNormalTexture->getFullHeight());
            has_texture = true;
        }
        if (mGLTFMaterial->mMetallicRoughnessTexture)
        {
            width = llmax(width, mGLTFMaterial->mMetallicRoughnessTexture->getFullWidth());
            height = llmax(height, mGLTFMaterial->mMetallicRoughnessTexture->getFullHeight());
            has_texture = true;
        }
        if (mGLTFMaterial->mEmissiveTexture)
        {
            width = llmax(width, mGLTFMaterial->mEmissiveTexture->getFullWidth());
            height = llmax(height, mGLTFMaterial->mEmissiveTexture->getFullHeight());
            has_texture = true;
        }

        if (width > 0 && height > 0)
        {
            std::string formatted_dims = llformat("%d x %d", width, height);
            mResolutionLabel->setTextArg("[DIMENSIONS]", formatted_dims);
            if (mOnUpdateImageStatsCallback)
            {
                mOnUpdateImageStatsCallback(mTexturep);
            }
        }
        else if (has_texture)
        {
            // unknown resolution
            mResolutionLabel->setTextArg("[DIMENSIONS]", std::string("[? x ?]"));
        }
        else
        {
            // No textures - no applicable resolution (may be show some max value instead?)
            mResolutionLabel->setTextArg("[DIMENSIONS]", std::string(""));
        }
    }
    else if (mTexturep.notNull())
    {
        //RN: have we received header data for this image?
        S32 width = mTexturep->getFullWidth();
        S32 height = mTexturep->getFullHeight();
        if (width > 0 && height > 0)
        {
            if ((mLimitsSet && (width != height))
                || width < mMinDim
                || width > mMaxDim
                || height < mMinDim
                || height > mMaxDim
                )
            {
                std::string formatted_dims = llformat("%dx%d", width, height);
                mResolutionWarning->setTextArg("[TEXDIM]", formatted_dims);
                result = false;
            }
            else
            {
                std::string formatted_dims = llformat("%d x %d", width, height);
                mResolutionLabel->setTextArg("[DIMENSIONS]", formatted_dims);
            }

            if (mOnUpdateImageStatsCallback)
            {
                mOnUpdateImageStatsCallback(mTexturep);
            }
        }
        else
        {
            mResolutionLabel->setTextArg("[DIMENSIONS]", std::string("[? x ?]"));
        }
    }
    else
    {
        mResolutionLabel->setTextArg("[DIMENSIONS]", std::string(""));
    }
    mResolutionLabel->setVisible(result);
    mResolutionWarning->setVisible(!result);

    // Hide buttons and pipete to make space for mResolutionWarning
    // Hiding buttons is suboptimal, but at the moment limited to inventory thumbnails
    // may be this should be an info/warning icon with a tooltip?
    S32 index = mModeSelector->getValue().asInteger();
    if (index == 0)
    {
        mDefaultBtn->setVisible(result);
        mNoneBtn->setVisible(result);
        mBlankBtn->setVisible(result);
        mPipetteBtn->setVisible(result);
    }
    return result;
}

// virtual
bool LLFloaterTexturePicker::handleDragAndDrop(
        S32 x, S32 y, MASK mask,
        bool drop,
        EDragAndDropType cargo_type, void *cargo_data,
        EAcceptance *accept,
        std::string& tooltip_msg)
{
    bool handled = false;

    bool is_mesh = cargo_type == DAD_MESH;
    bool is_texture = cargo_type == DAD_TEXTURE;
    bool is_material = cargo_type == DAD_MATERIAL;

    bool allow_dnd = false;
    if (mInventoryPickType == PICK_MATERIAL)
    {
        allow_dnd = is_material;
    }
    else if (mInventoryPickType == PICK_TEXTURE)
    {
        allow_dnd = is_texture || is_mesh;
    }
    else
    {
        allow_dnd = is_texture || is_mesh || is_material;
    }

    if (allow_dnd)
    {
        LLInventoryItem *item = (LLInventoryItem *)cargo_data;

        bool copy = item->getPermissions().allowCopyBy(gAgent.getID());
        bool mod = item->getPermissions().allowModifyBy(gAgent.getID());
        bool xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
                                                            gAgent.getID());

        PermissionMask item_perm_mask = 0;
        if (copy) item_perm_mask |= PERM_COPY;
        if (mod)  item_perm_mask |= PERM_MODIFY;
        if (xfer) item_perm_mask |= PERM_TRANSFER;

        PermissionMask filter_perm_mask = mDnDFilterPermMask;
        if ( (item_perm_mask & filter_perm_mask) == filter_perm_mask )
        {
            if (drop)
            {
                setImageIDFromItem(item);
                commitIfImmediateSet();
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

    handled = true;
    LL_DEBUGS("UserInput") << "dragAndDrop handled by LLFloaterTexturePicker " << getName() << LL_ENDL;

    return handled;
}

bool LLFloaterTexturePicker::handleKeyHere(KEY key, MASK mask)
{
    LLFolderView* root_folder = mInventoryPanel->getRootFolder();

    if (root_folder && mFilterEdit)
    {
        if (mFilterEdit->hasFocus()
            && (key == KEY_RETURN || key == KEY_DOWN)
            && mask == MASK_NONE)
        {
            if (!root_folder->getCurSelectedItem())
            {
                LLFolderViewItem* itemp =    mInventoryPanel->getItemByID(gInventory.getRootFolderID());
                if (itemp)
                {
                    root_folder->setSelection(itemp, false, false);
                }
            }
            root_folder->scrollToShowSelection();

            // move focus to inventory proper
            mInventoryPanel->setFocus(true);

            // treat this as a user selection of the first filtered result
            commitIfImmediateSet();

            return true;
        }

        if (mInventoryPanel->hasFocus() && key == KEY_UP)
        {
            mFilterEdit->focusFirstItem(true);
        }
    }

    return LLFloater::handleKeyHere(key, mask);
}

void LLFloaterTexturePicker::onOpen(const LLSD& key)
{
    if (sLastPickerMode != 0
        && mModeSelector->selectByValue(sLastPickerMode))
    {
        changeMode();
    }
}

void LLFloaterTexturePicker::onClose(bool app_quitting)
{
    if (mOwner && mOnFloaterCloseCallback)
    {
        mOnFloaterCloseCallback();
    }
    stopUsingPipette();
    sLastPickerMode = mModeSelector->getValue().asInteger();
    // *NOTE: Vertex buffer for sphere preview is still cached
    mGLTFPreview = nullptr;
}

// virtual
bool LLFloaterTexturePicker::postBuild()
{
    LLFloater::postBuild();

    if (!mLabel.empty())
    {
        std::string pick = getString("pick title");

        setTitle(pick + mLabel);
    }
    mTentativeLabel = getChild<LLTextBox>("Multiple");

    mResolutionLabel = getChild<LLTextBox>("size_lbl");
    mResolutionWarning = getChild<LLTextBox>("over_limit_lbl");

    mPreviewWidget = getChild<LLView>("preview_widget");

    mDefaultBtn = getChild<LLButton>("Default");
    mNoneBtn = getChild<LLButton>("None");
    mBlankBtn = getChild<LLButton>("Blank");
    mPipetteBtn = getChild<LLButton>("Pipette");
    mSelectBtn = getChild<LLButton>("Select");
    mCancelBtn = getChild<LLButton>("Cancel");

    mDefaultBtn->setClickedCallback(boost::bind(LLFloaterTexturePicker::onBtnSetToDefault,this));
    mNoneBtn->setClickedCallback(boost::bind(LLFloaterTexturePicker::onBtnNone, this));
    mBlankBtn->setClickedCallback(boost::bind(LLFloaterTexturePicker::onBtnBlank, this));
    mPipetteBtn->setCommitCallback(boost::bind(&LLFloaterTexturePicker::onBtnPipette, this));
    mSelectBtn->setClickedCallback(boost::bind(LLFloaterTexturePicker::onBtnSelect, this));
    mCancelBtn->setClickedCallback(boost::bind(LLFloaterTexturePicker::onBtnCancel, this));

    mFilterEdit = getChild<LLFilterEditor>("inventory search editor");
    mFilterEdit->setCommitCallback(boost::bind(&LLFloaterTexturePicker::onFilterEdit, this, _2));

    mInventoryPanel = getChild<LLInventoryPanel>("inventory panel");

    mModeSelector = getChild<LLComboBox>("mode_selection");
    mModeSelector->setCommitCallback(onModeSelect, this);
    mModeSelector->selectByValue(0);

    if(mInventoryPanel)
    {
        // to avoid having to make an assumption about which option is
        // selected at startup, we call the same function that is triggered
        // when a texture/materials/both choice is made and let it take care
        // of setting the filters
        refreshInventoryFilter();

        mInventoryPanel->setFilterPermMask(mImmediateFilterPermMask);
        mInventoryPanel->setSelectCallback(boost::bind(&LLFloaterTexturePicker::onSelectionChange, this, _1, _2));
        mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);

        // Disable auto selecting first filtered item because it takes away
        // selection from the item set by LLTextureCtrl owning this floater.
        mInventoryPanel->getRootFolder()->setAutoSelectOverride(true);

        // Commented out to scroll to currently selected texture. See EXT-5403.
        // // store this filter as the default one
        // mInventoryPanel->getRootFolder()->getFilter().markDefault();

        // Commented out to stop opening all folders with textures
        // mInventoryPanel->openDefaultFolderForType(LLFolderType::FT_TEXTURE);

        // don't put keyboard focus on selected item, because the selection callback
        // will assume that this was user input

        if(!mImageAssetID.isNull() || mInventoryPickType == PICK_MATERIAL)
        {
            mInventoryPanel->setSelection(findItemID(mImageAssetID, false), TAKE_FOCUS_NO);
        }
    }

    childSetAction("l_add_btn", LLFloaterTexturePicker::onBtnAdd, this);
    childSetAction("l_rem_btn", LLFloaterTexturePicker::onBtnRemove, this);
    childSetAction("l_upl_btn", LLFloaterTexturePicker::onBtnUpload, this);

    mLocalScrollCtrl = getChild<LLScrollListCtrl>("l_name_list");
    mLocalScrollCtrl->setCommitCallback(onLocalScrollCommit, this);
    refreshLocalList();

    mNoCopyTextureSelected = false;

    getChild<LLUICtrl>("apply_immediate_check")->setValue(mCanApplyImmediately && gSavedSettings.getBOOL("TextureLivePreview"));
    childSetCommitCallback("apply_immediate_check", onApplyImmediateCheck, this);
    getChildView("apply_immediate_check")->setEnabled(mCanApplyImmediately);

    getChild<LLUICtrl>("Pipette")->setCommitCallback( boost::bind(&LLFloaterTexturePicker::onBtnPipette, this));
    childSetAction("Cancel", LLFloaterTexturePicker::onBtnCancel,this);
    childSetAction("Select", LLFloaterTexturePicker::onBtnSelect,this);

    mSavedFolderState.setApply(false);

    LLToolPipette::getInstance()->setToolSelectCallback(boost::bind(&LLFloaterTexturePicker::onTextureSelect, this, _1));

    getChild<LLComboBox>("l_bake_use_texture_combo_box")->setCommitCallback(onBakeTextureSelect, this);

    setBakeTextureEnabled(mInventoryPickType != PICK_MATERIAL);
    return true;
}

// virtual
void LLFloaterTexturePicker::draw()
{
    static LLCachedControl<F32> max_opacity(gSavedSettings, "PickerContextOpacity", 0.4f);
    drawConeToOwner(mContextConeOpacity, max_opacity, mOwner);

    // This is going to spam mOnUpdateImageStatsCallback,
    // either move elsewhere or fix to cause update once per image
    bool valid_dims = updateImageStats();

    // if we're inactive, gray out "apply immediate" checkbox
    mSelectBtn->setEnabled(mActive && mCanApply && valid_dims);
    mPipetteBtn->setEnabled(mActive);
    mPipetteBtn->setValue(LLToolMgr::getInstance()->getCurrentTool() == LLToolPipette::getInstance());

    //bool allow_copy = false;
    if( mOwner )
    {
        mTexturep = NULL;
        LLPointer<LLFetchedGLTFMaterial> old_material = mGLTFMaterial;
        mGLTFMaterial = NULL;
        if (mImageAssetID.notNull())
        {
            if (mInventoryPickType == PICK_MATERIAL)
            {
                mGLTFMaterial = (LLFetchedGLTFMaterial*) gGLTFMaterialList.getMaterial(mImageAssetID);
                llassert(mGLTFMaterial == nullptr || dynamic_cast<LLFetchedGLTFMaterial*>(gGLTFMaterialList.getMaterial(mImageAssetID)) != nullptr);
                if (mGLTFPreview.isNull() || mGLTFMaterial.isNull() || (old_material.notNull() && (old_material.get() != mGLTFMaterial.get())))
                {
                    // Only update the preview if needed, since gGLTFMaterialPreviewMgr does not cache the preview.
                    if (mGLTFMaterial.isNull())
                    {
                        mGLTFPreview = nullptr;
                    }
                    else
                    {
                        mGLTFPreview = gGLTFMaterialPreviewMgr.getPreview(mGLTFMaterial);
                    }
                }
                if (mGLTFPreview)
                {
                    mGLTFPreview->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
                }
            }
            else
            {
                LLPointer<LLViewerFetchedTexture> texture = NULL;
                mGLTFPreview = nullptr;

                if (LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(mImageAssetID))
                {
                    // TODO: Fix this! Picker is not warrantied to be connected to a selection
                    // LLSelectMgr shouldn't be used in texture picker
                    LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
                    if (obj)
                    {
                        LLViewerTexture* viewerTexture = obj->getBakedTextureForMagicId(mImageAssetID);
                        texture = viewerTexture ? dynamic_cast<LLViewerFetchedTexture*>(viewerTexture) : NULL;
                    }
                }

                if (texture.isNull())
                {
                    texture = LLViewerTextureManager::getFetchedTexture(mImageAssetID);
                }

                mTexturep = texture;
                mTexturep->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
            }
        }

        if (mTentativeLabel)
        {
            mTentativeLabel->setVisible( false  );
        }

        mDefaultBtn->setEnabled(mImageAssetID != mDefaultImageAssetID || mTentative);
        mBlankBtn->setEnabled((mImageAssetID != mBlankImageAssetID && mBlankImageAssetID.notNull()) || mTentative);
        mNoneBtn->setEnabled(mAllowNoTexture && (!mImageAssetID.isNull() || mTentative));

        LLFloater::draw();

        if( isMinimized() )
        {
            return;
        }

        // Border
        LLRect border = mPreviewWidget->getRect();
        gl_rect_2d( border, LLColor4::black, false );


        // Interior
        LLRect interior = border;
        interior.stretch( -1 );

        // If the floater is focused, don't apply its alpha to the texture (STORM-677).
        const F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
        LLViewerTexture* preview;
        if (mGLTFMaterial)
        {
            preview = mGLTFPreview.get();
        }
        else
        {
            preview = mTexturep.get();
        }

        if( preview )
        {
            preview->addTextureStats( (F32)(interior.getWidth() * interior.getHeight()) );
            if( preview->getComponents() == 4 )
            {
                gl_rect_2d_checkerboard( interior, alpha );
            }

            gl_draw_scaled_image( interior.mLeft, interior.mBottom, interior.getWidth(), interior.getHeight(), preview, UI_VERTEX_COLOR % alpha );
        }
        else if (!mFallbackImage.isNull())
        {
            mFallbackImage->draw(interior, UI_VERTEX_COLOR % alpha);
        }
        else
        {
            gl_rect_2d( interior, LLColor4::grey % alpha, true );

            // Draw X
            gl_draw_x(interior, LLColor4::black );
        }

        // Draw Tentative Label over the image
        if( mTentative && !mViewModel->isDirty() )
        {
            mTentativeLabel->setVisible( true );
            drawChild(mTentativeLabel);
        }

        if (mSelectedItemPinned) return;

        LLFolderView* folder_view = mInventoryPanel->getRootFolder();
        if (!folder_view) return;

        LLFolderViewFilter& filter = static_cast<LLFolderViewModelInventory*>(folder_view->getFolderViewModel())->getFilter();

        bool is_filter_active = folder_view->getViewModelItem()->getLastFilterGeneration() < filter.getCurrentGeneration() &&
                filter.isNotDefault();

        // After inventory panel filter is applied we have to update
        // constraint rect for the selected item because of folder view
        // AutoSelectOverride set to true. We force PinningSelectedItem
        // flag to false state and setting filter "dirty" to update
        // scroll container to show selected item (see LLFolderView::doIdle()).
        if (!is_filter_active && !mSelectedItemPinned)
        {
            folder_view->setPinningSelectedItem(mSelectedItemPinned);
            folder_view->getViewModelItem()->dirtyFilter();
            mSelectedItemPinned = true;
        }
    }
}

const LLUUID& LLFloaterTexturePicker::findItemID(const LLUUID& asset_id, bool copyable_only, bool ignore_library)
{
    if (asset_id.isNull())
    {
        // null asset id means, no material or texture assigned
        return LLUUID::null;
    }

    LLUUID loockup_id = asset_id;
    if (mInventoryPickType == PICK_MATERIAL && loockup_id == BLANK_MATERIAL_ASSET_ID)
    {
        // default asset id means we are looking for an inventory item with a default asset UUID (null)
        loockup_id = LLUUID::null;
    }

    LLViewerInventoryCategory::cat_array_t cats;
    LLViewerInventoryItem::item_array_t items;

    if (loockup_id.isNull())
    {
        // looking for a material with a null id, null id is shared by a lot
        // of objects as a default value, so have to filter by type as well
        LLAssetIDAndTypeMatches matches(loockup_id, LLAssetType::AT_MATERIAL);
        gInventory.collectDescendentsIf(LLUUID::null,
                                        cats,
                                        items,
                                        LLInventoryModel::INCLUDE_TRASH,
                                        matches);
    }
    else
    {
        LLAssetIDMatches asset_id_matches(loockup_id);
        gInventory.collectDescendentsIf(LLUUID::null,
                                        cats,
                                        items,
                                        LLInventoryModel::INCLUDE_TRASH,
                                        asset_id_matches);
    }


    if (items.size())
    {
        // search for copyable version first
        for (S32 i = 0; i < items.size(); i++)
        {
            LLInventoryItem* itemp = items[i];
            LLPermissions item_permissions = itemp->getPermissions();
            if (item_permissions.allowCopyBy(gAgent.getID(), gAgent.getGroupID()))
            {
                if(!ignore_library || !gInventory.isObjectDescendentOf(itemp->getUUID(),gInventory.getLibraryRootFolderID()))
                {
                    return itemp->getUUID();
                }
            }
        }
        // otherwise just return first instance, unless copyable requested
        if (copyable_only)
        {
            return LLUUID::null;
        }
        else
        {
            if(!ignore_library || !gInventory.isObjectDescendentOf(items[0]->getUUID(),gInventory.getLibraryRootFolderID()))
            {
                return items[0]->getUUID();
            }
        }
    }

    return LLUUID::null;
}

void LLFloaterTexturePicker::commitIfImmediateSet()
{
    if (!mNoCopyTextureSelected && mCanApply)
    {
        commitCallback(LLTextureCtrl::TEXTURE_CHANGE);
    }
}

void LLFloaterTexturePicker::commitCallback(LLTextureCtrl::ETexturePickOp op)
{
    if (!mOnFloaterCommitCallback)
    {
        return;
    }

    LLUUID asset_id = mImageAssetID;
    LLUUID inventory_id;
    LLUUID tracking_id;
    LLPickerSource mode = mSelectionSource;
    if (mode == PICKER_UNKNOWN)
    {
        mode = (LLPickerSource)mModeSelector->getValue().asInteger();
    }

    switch (mode)
    {
        case PICKER_INVENTORY:
            {
                LLFolderView* root_folder = mInventoryPanel->getRootFolder();
                if (root_folder && root_folder->getCurSelectedItem())
                {
                    LLFolderViewItem* last_selected = root_folder->getCurSelectedItem();
                    LLFolderViewModelItemInventory* inv_view = static_cast<LLFolderViewModelItemInventory*>(last_selected->getViewModelItem());

                    LLInventoryItem* itemp = gInventory.getItem(inv_view->getUUID());

                    if (mInventoryPickType == PICK_MATERIAL
                        && mImageAssetID == BLANK_MATERIAL_ASSET_ID
                        && itemp && itemp->getAssetUUID().isNull())
                    {
                        inventory_id = inv_view->getUUID();
                    }
                    else if (itemp && itemp->getAssetUUID() == mImageAssetID)
                    {
                        inventory_id = inv_view->getUUID();
                    }
                    else
                    {
                        // Item's asset id changed?
                        mode = PICKER_UNKNOWN; // source of id unknown
                    }
                }
                else
                {
                    // Item could have been removed from inventory
                    mode = PICKER_UNKNOWN; // source of id unknown
                }
                break;
            }
        case PICKER_LOCAL:
            {
                if (!mLocalScrollCtrl->getAllSelected().empty())
                {
                    LLSD data = mLocalScrollCtrl->getFirstSelected()->getValue();
                    tracking_id = data["id"];
                    S32 asset_type = data["type"].asInteger();

                    if (LLAssetType::AT_MATERIAL == asset_type)
                    {
                        asset_id = LLLocalGLTFMaterialMgr::getInstance()->getWorldID(tracking_id);
                    }
                    else
                    {
                        asset_id = LLLocalBitmapMgr::getInstance()->getWorldID(tracking_id);
                    }
                }
                else
                {
                    // List could have been emptied, with local image still selected
                    asset_id = mImageAssetID;
                    mode = PICKER_UNKNOWN; // source of id unknown
                }
                break;
            }
        case PICKER_BAKE:
            break;
        default:
            mode = PICKER_UNKNOWN; // source of id unknown
            break;
    }

    mOnFloaterCommitCallback(op, mode, asset_id, inventory_id, tracking_id);
}
void LLFloaterTexturePicker::commitCancel()
{
    if (!mNoCopyTextureSelected && mOnFloaterCommitCallback && mCanApply)
    {
        mOnFloaterCommitCallback(LLTextureCtrl::TEXTURE_CANCEL, PICKER_UNKNOWN, mOriginalImageAssetID, LLUUID::null, LLUUID::null);
    }
}

// static
void LLFloaterTexturePicker::onBtnSetToDefault(void* userdata)
{
    LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
    self->setCanApply(true, true);
    if (self->mOwner)
    {
        self->setImageID( self->getDefaultImageAssetID() );
    }
    self->commitIfImmediateSet();
}

// static
void LLFloaterTexturePicker::onBtnBlank(void* userdata)
{
    LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
    self->setCanApply(true, true);
    self->setImageID( self->getBlankImageAssetID() );
    self->commitIfImmediateSet();
}


// static
void LLFloaterTexturePicker::onBtnNone(void* userdata)
{
    LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
    self->setCanApply(true, true);
    self->setImageID( LLUUID::null );
    self->commitIfImmediateSet();
}

// static
void LLFloaterTexturePicker::onBtnCancel(void* userdata)
{
    LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
    self->setImageID( self->mOriginalImageAssetID );
    if (self->mOnFloaterCommitCallback)
    {
        self->mOnFloaterCommitCallback(LLTextureCtrl::TEXTURE_CANCEL, PICKER_UNKNOWN, self->mOriginalImageAssetID, LLUUID::null, LLUUID::null);
    }
    self->mViewModel->resetDirty();
    self->closeFloater();
}

// static
void LLFloaterTexturePicker::onBtnSelect(void* userdata)
{
    LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
    if (self->mViewModel->isDirty() && self->mOnFloaterCommitCallback)
    {
        // If nothing changed, don't commit.
        // ex: can overwrite multiple original textures with a single one.
        // or resubmit something thus overriding some other source of change
        self->commitCallback(LLTextureCtrl::TEXTURE_SELECT);
    }
    self->closeFloater();
}

void LLFloaterTexturePicker::onBtnPipette()
{
    bool pipette_active = getChild<LLUICtrl>("Pipette")->getValue().asBoolean();
    pipette_active = !pipette_active;
    if (pipette_active)
    {
        LLToolMgr::getInstance()->setTransientTool(LLToolPipette::getInstance());
    }
    else
    {
        LLToolMgr::getInstance()->clearTransientTool();
    }
}

void LLFloaterTexturePicker::onSelectionChange(const std::deque<LLFolderViewItem*> &items, bool user_action)
{
    if (items.size())
    {
        LLFolderViewItem* first_item = items.front();
        LLInventoryItem* itemp = gInventory.getItem(static_cast<LLFolderViewModelItemInventory*>(first_item->getViewModelItem())->getUUID());
        mNoCopyTextureSelected = false;
        if (itemp)
        {
            if (!mTextureSelectedCallback.empty())
            {
                mTextureSelectedCallback(itemp);
            }
            if (!itemp->getPermissions().allowCopyBy(gAgent.getID()))
            {
                mNoCopyTextureSelected = true;
            }
            bool was_dirty = mViewModel->isDirty();
            setImageIDFromItem(itemp, false);
            if (user_action)
            {
                mViewModel->setDirty(); // *TODO: shouldn't we be using setValue() here?
                setTentative( false );
            }
            else if (!was_dirty)
            {
                // setImageIDFromItem could have dropped the flag
                mViewModel->resetDirty();
            }

            if(!mPreviewSettingChanged)
            {
                mCanPreview = mCanApplyImmediately && gSavedSettings.getBOOL("TextureLivePreview");
            }
            else
            {
                mPreviewSettingChanged = false;
            }

            if (user_action && mCanPreview)
            {
                // only commit intentional selections, not implicit ones
                commitIfImmediateSet();
            }
        }
    }
}

// static
void LLFloaterTexturePicker::onModeSelect(LLUICtrl* ctrl, void *userdata)
{
    LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
    self->changeMode();
}

// static
void LLFloaterTexturePicker::onBtnAdd(void* userdata)
{
    LLFloaterTexturePicker* self = (LLFloaterTexturePicker*)userdata;

    if (self->mInventoryPickType == PICK_TEXTURE_MATERIAL)
    {
        LLFilePickerReplyThread::startPicker(boost::bind(&onPickerCallback, _1, self->getHandle()), LLFilePicker::FFLOAD_MATERIAL_TEXTURE, true);
    }
    else if (self->mInventoryPickType == PICK_TEXTURE)
    {
        LLFilePickerReplyThread::startPicker(boost::bind(&onPickerCallback, _1, self->getHandle()), LLFilePicker::FFLOAD_IMAGE, true);
    }
    else if (self->mInventoryPickType == PICK_MATERIAL)
    {
        LLFilePickerReplyThread::startPicker(boost::bind(&onPickerCallback, _1, self->getHandle()), LLFilePicker::FFLOAD_MATERIAL, true);
    }
}

// static
void LLFloaterTexturePicker::onBtnRemove(void* userdata)
{
    LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
    std::vector<LLScrollListItem*> selected_items = self->mLocalScrollCtrl->getAllSelected();

    if (!selected_items.empty())
    {

        for(std::vector<LLScrollListItem*>::iterator iter = selected_items.begin();
            iter != selected_items.end(); iter++)
        {
            LLScrollListItem* list_item = *iter;
            if (list_item)
            {
                LLSD data = list_item->getValue();
                LLUUID tracking_id = data["id"];
                S32 asset_type = data["type"].asInteger();

                if (LLAssetType::AT_MATERIAL == asset_type)
                {
                    LLLocalGLTFMaterialMgr::getInstance()->delUnit(tracking_id);
                }
                else
                {
                    LLLocalBitmapMgr::getInstance()->delUnit(tracking_id);
                }
            }
        }

        self->getChild<LLButton>("l_rem_btn")->setEnabled(false);
        self->getChild<LLButton>("l_upl_btn")->setEnabled(false);
        self->refreshLocalList();
    }
}

// static
void LLFloaterTexturePicker::onBtnUpload(void* userdata)
{
    LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
    std::vector<LLScrollListItem*> selected_items = self->mLocalScrollCtrl->getAllSelected();

    if (selected_items.empty())
    {
        return;
    }

    /* currently only allows uploading one by one, picks the first item from the selection list.  (not the vector!)
       in the future, it might be a good idea to check the vector size and if more than one units is selected - opt for multi-image upload. */

    LLSD data = self->mLocalScrollCtrl->getFirstSelected()->getValue();
    LLUUID tracking_id = data["id"];
    S32 asset_type = data["type"].asInteger();

    if (LLAssetType::AT_MATERIAL == asset_type)
    {
        std::string filename;
        S32 index;
        LLLocalGLTFMaterialMgr::getInstance()->getFilenameAndIndex(tracking_id, filename, index);
        if (!filename.empty())
        {
            LLMaterialEditor::loadMaterialFromFile(filename, index);
        }
    }
    else
    {
        std::string filename = LLLocalBitmapMgr::getInstance()->getFilename(tracking_id);
        if (!filename.empty())
        {
            LLFloaterReg::showInstance("upload_image", LLSD(filename));
        }
    }
}

//static
void LLFloaterTexturePicker::onLocalScrollCommit(LLUICtrl* ctrl, void* userdata)
{
    LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
    std::vector<LLScrollListItem*> selected_items = self->mLocalScrollCtrl->getAllSelected();
    bool has_selection = !selected_items.empty();

    self->getChild<LLButton>("l_rem_btn")->setEnabled(has_selection);
    self->getChild<LLButton>("l_upl_btn")->setEnabled(has_selection && (selected_items.size() < 2));
    /* since multiple-localbitmap upload is not implemented, upl button gets disabled if more than one is selected. */

    if (has_selection)
    {
        LLSD data = self->mLocalScrollCtrl->getFirstSelected()->getValue();
        LLUUID tracking_id = data["id"];
        S32 asset_type = data["type"].asInteger();
        LLUUID inworld_id;

        if (LLAssetType::AT_MATERIAL == asset_type)
        {
            inworld_id = LLLocalGLTFMaterialMgr::getInstance()->getWorldID(tracking_id);
        }
        else
        {
            inworld_id = LLLocalBitmapMgr::getInstance()->getWorldID(tracking_id);
        }

        self->mSelectionSource = PICKER_LOCAL;

        if (self->mSetImageAssetIDCallback)
        {
            self->mSetImageAssetIDCallback(inworld_id);
        }

        if (self->childGetValue("apply_immediate_check").asBoolean())
        {
            if (self->mOnFloaterCommitCallback)
            {
                self->mOnFloaterCommitCallback(LLTextureCtrl::TEXTURE_CHANGE, PICKER_LOCAL, inworld_id, LLUUID::null, tracking_id);
            }
        }
    }
}

// static
void LLFloaterTexturePicker::onApplyImmediateCheck(LLUICtrl* ctrl, void *user_data)
{
    LLFloaterTexturePicker* picker = (LLFloaterTexturePicker*)user_data;

    LLCheckBoxCtrl* check_box = (LLCheckBoxCtrl*)ctrl;
    gSavedSettings.setBOOL("TextureLivePreview", check_box->get());

    picker->commitIfImmediateSet();
}

//static
void LLFloaterTexturePicker::onBakeTextureSelect(LLUICtrl* ctrl, void *user_data)
{
    LLFloaterTexturePicker* self = (LLFloaterTexturePicker*)user_data;
    LLComboBox* combo_box = (LLComboBox*)ctrl;

    S8 type = combo_box->getValue().asInteger();

    LLUUID imageID = self->mDefaultImageAssetID;
    if (type == 0)
    {
        imageID = IMG_USE_BAKED_HEAD;
    }
    else if (type == 1)
    {
        imageID = IMG_USE_BAKED_UPPER;
    }
    else if (type == 2)
    {
        imageID = IMG_USE_BAKED_LOWER;
    }
    else if (type == 3)
    {
        imageID = IMG_USE_BAKED_EYES;
    }
    else if (type == 4)
    {
        imageID = IMG_USE_BAKED_SKIRT;
    }
    else if (type == 5)
    {
        imageID = IMG_USE_BAKED_HAIR;
    }
    else if (type == 6)
    {
        imageID = IMG_USE_BAKED_LEFTARM;
    }
    else if (type == 7)
    {
        imageID = IMG_USE_BAKED_LEFTLEG;
    }
    else if (type == 8)
    {
        imageID = IMG_USE_BAKED_AUX1;
    }
    else if (type == 9)
    {
        imageID = IMG_USE_BAKED_AUX2;
    }
    else if (type == 10)
    {
        imageID = IMG_USE_BAKED_AUX3;
    }

    self->setImageID(imageID);
    self->mViewModel->setDirty(); // *TODO: shouldn't we be using setValue() here?

    if (!self->mPreviewSettingChanged)
    {
        self->mCanPreview = self->mCanApplyImmediately && gSavedSettings.getBOOL("TextureLivePreview");
    }
    else
    {
        self->mPreviewSettingChanged = false;
    }

    if (self->mCanPreview)
    {
        // only commit intentional selections, not implicit ones
        self->commitIfImmediateSet();
    }
    self->mSelectionSource = PICKER_BAKE;
}

void LLFloaterTexturePicker::setCanApply(bool can_preview, bool can_apply, bool inworld_image)
{
    mSelectBtn->setEnabled(can_apply);
    getChildRef<LLUICtrl>("preview_disabled").setVisible(!can_preview && inworld_image);
    getChildRef<LLUICtrl>("apply_immediate_check").setVisible(can_preview);

    mCanApply = can_apply;
    mCanPreview = can_preview ? (mCanApplyImmediately && gSavedSettings.getBOOL("TextureLivePreview")) : false;
    mPreviewSettingChanged = true;
}

void LLFloaterTexturePicker::setMinDimentionsLimits(S32 min_dim)
{
    mMinDim = min_dim;
    mLimitsSet = true;

    std::string formatted_dims = llformat("%dx%d", mMinDim, mMinDim);
    mResolutionWarning->setTextArg("[MINTEXDIM]", formatted_dims);
}

void LLFloaterTexturePicker::onFilterEdit(const std::string& search_string )
{
    std::string upper_case_search_string = search_string;
    LLStringUtil::toUpper(upper_case_search_string);

    if (upper_case_search_string.empty())
    {
        if (mInventoryPanel->getFilterSubString().empty())
        {
            // current filter and new filter empty, do nothing
            return;
        }

        mSavedFolderState.setApply(true);
        mInventoryPanel->getRootFolder()->applyFunctorRecursively(mSavedFolderState);
        // add folder with current item to list of previously opened folders
        LLOpenFoldersWithSelection opener;
        mInventoryPanel->getRootFolder()->applyFunctorRecursively(opener);
        mInventoryPanel->getRootFolder()->scrollToShowSelection();

    }
    else if (mInventoryPanel->getFilterSubString().empty())
    {
        // first letter in search term, save existing folder open state
        if (!mInventoryPanel->getFilter().isNotDefault())
        {
            mSavedFolderState.setApply(false);
            mInventoryPanel->getRootFolder()->applyFunctorRecursively(mSavedFolderState);
        }
    }

    mInventoryPanel->setFilterSubString(search_string);
}

void LLFloaterTexturePicker::changeMode()
{
    int index = mModeSelector->getValue().asInteger();

    mDefaultBtn->setVisible(index == PICKER_INVENTORY);
    mBlankBtn->setVisible(index == PICKER_INVENTORY);
    mNoneBtn->setVisible(index == PICKER_INVENTORY);
    mFilterEdit->setVisible(index == PICKER_INVENTORY);
    mInventoryPanel->setVisible(index == PICKER_INVENTORY);

    getChild<LLButton>("l_add_btn")->setVisible(index == PICKER_LOCAL);
    getChild<LLButton>("l_rem_btn")->setVisible(index == PICKER_LOCAL);
    getChild<LLButton>("l_upl_btn")->setVisible(index == PICKER_LOCAL);
    getChild<LLScrollListCtrl>("l_name_list")->setVisible(index == PICKER_LOCAL);

    getChild<LLComboBox>("l_bake_use_texture_combo_box")->setVisible(index == PICKER_BAKE);
    getChild<LLCheckBoxCtrl>("hide_base_mesh_region")->setVisible(false);// index == 2);

    bool pipette_visible = (index == PICKER_INVENTORY)
        && (mInventoryPickType != PICK_MATERIAL);
    mPipetteBtn->setVisible(pipette_visible);

    if (index == PICKER_BAKE)
    {
        stopUsingPipette();

        S8 val = -1;

        LLUUID imageID = mImageAssetID;
        if (imageID == IMG_USE_BAKED_HEAD)
        {
            val = 0;
        }
        else if (imageID == IMG_USE_BAKED_UPPER)
        {
            val = 1;
        }
        else if (imageID == IMG_USE_BAKED_LOWER)
        {
            val = 2;
        }
        else if (imageID == IMG_USE_BAKED_EYES)
        {
            val = 3;
        }
        else if (imageID == IMG_USE_BAKED_SKIRT)
        {
            val = 4;
        }
        else if (imageID == IMG_USE_BAKED_HAIR)
        {
            val = 5;
        }
        else if (imageID == IMG_USE_BAKED_LEFTARM)
        {
            val = 6;
        }
        else if (imageID == IMG_USE_BAKED_LEFTLEG)
        {
            val = 7;
        }
        else if (imageID == IMG_USE_BAKED_AUX1)
        {
            val = 8;
        }
        else if (imageID == IMG_USE_BAKED_AUX2)
        {
            val = 9;
        }
        else if (imageID == IMG_USE_BAKED_AUX3)
        {
            val = 10;
        }

        getChild<LLComboBox>("l_bake_use_texture_combo_box")->setSelectedByValue(val, true);
    }
}

void LLFloaterTexturePicker::refreshLocalList()
{
    mLocalScrollCtrl->clearRows();

    if (mInventoryPickType == PICK_TEXTURE_MATERIAL)
    {
        LLLocalBitmapMgr::getInstance()->feedScrollList(mLocalScrollCtrl);
        LLLocalGLTFMaterialMgr::getInstance()->feedScrollList(mLocalScrollCtrl);
    }
    else if (mInventoryPickType == PICK_TEXTURE)
    {
        LLLocalBitmapMgr::getInstance()->feedScrollList(mLocalScrollCtrl);
    }
    else if (mInventoryPickType == PICK_MATERIAL)
    {
        LLLocalGLTFMaterialMgr::getInstance()->feedScrollList(mLocalScrollCtrl);
    }
}

void LLFloaterTexturePicker::refreshInventoryFilter()
{
    U32 filter_types = 0x0;

    if (mInventoryPickType == PICK_TEXTURE_MATERIAL)
    {
        filter_types |= 0x1 << LLInventoryType::IT_TEXTURE;
        filter_types |= 0x1 << LLInventoryType::IT_SNAPSHOT;
        filter_types |= 0x1 << LLInventoryType::IT_MATERIAL;
    }
    else if (mInventoryPickType == PICK_TEXTURE)
    {
        filter_types |= 0x1 << LLInventoryType::IT_TEXTURE;
        filter_types |= 0x1 << LLInventoryType::IT_SNAPSHOT;
    }
    else if (mInventoryPickType == PICK_MATERIAL)
    {
        filter_types |= 0x1 << LLInventoryType::IT_MATERIAL;
    }

    mInventoryPanel->setFilterTypes(filter_types);
}

void LLFloaterTexturePicker::setLocalTextureEnabled(bool enabled)
{
    mModeSelector->setEnabledByValue(1, enabled);
}

void LLFloaterTexturePicker::setBakeTextureEnabled(bool enabled)
{
    bool changed = (enabled != mBakeTextureEnabled);

    mBakeTextureEnabled = enabled;
    mModeSelector->setEnabledByValue(2, enabled);

    if (!mBakeTextureEnabled && (mModeSelector->getValue().asInteger() == 2))
    {
        mModeSelector->selectByValue(0);
    }

    if (changed && mBakeTextureEnabled && LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(mImageAssetID))
    {
        if (mModeSelector->getValue().asInteger() != 2)
        {
            mModeSelector->selectByValue(2);
        }
    }
    onModeSelect(0, this);
}

void LLFloaterTexturePicker::setInventoryPickType(EPickInventoryType type)
{
    mInventoryPickType = type;
    refreshLocalList();
    refreshInventoryFilter();

    if (mInventoryPickType == PICK_MATERIAL)
    {
        getChild<LLButton>("Pipette")->setVisible(false);
    }
    else
    {
        S32 index = mModeSelector->getValue().asInteger();
        getChild<LLButton>("Pipette")->setVisible(index == 0);
    }

    if (!mLabel.empty())
    {
        std::string pick = getString("pick title");

        setTitle(pick + mLabel);
    }
    else if(mInventoryPickType == PICK_MATERIAL)
    {
        setTitle(getString("pick_material"));
    }
    else
    {
        setTitle(getString("pick_texture"));
    }

    // refresh selection
    if (!mImageAssetID.isNull() || mInventoryPickType == PICK_MATERIAL)
    {
        mInventoryPanel->setSelection(findItemID(mImageAssetID, false), TAKE_FOCUS_NO);
    }
}

void LLFloaterTexturePicker::setImmediateFilterPermMask(PermissionMask mask)
{
    mImmediateFilterPermMask = mask;
    mInventoryPanel->setFilterPermMask(mask);
}

void LLFloaterTexturePicker::onPickerCallback(const std::vector<std::string>& filenames, LLHandle<LLFloater> handle)
{
    std::vector<std::string>::const_iterator iter = filenames.begin();
    while (iter != filenames.end())
    {
        if (!iter->empty())
        {
            std::string temp_exten = gDirUtilp->getExtension(*iter);
            if (temp_exten == "gltf" || temp_exten == "glb")
            {
                LLLocalGLTFMaterialMgr::getInstance()->addUnit(*iter);
            }
            else
            {
                LLLocalBitmapMgr::getInstance()->addUnit(*iter);
            }
        }
        iter++;
    }

    // Todo: this should referesh all pickers, not just a current one
    if (!handle.isDead())
    {
        LLFloaterTexturePicker* self = (LLFloaterTexturePicker*)handle.get();
        self->mLocalScrollCtrl->clearRows();

        if (self->mInventoryPickType == PICK_TEXTURE_MATERIAL)
        {
            LLLocalBitmapMgr::getInstance()->feedScrollList(self->mLocalScrollCtrl);
            LLLocalGLTFMaterialMgr::getInstance()->feedScrollList(self->mLocalScrollCtrl);
        }
        else if (self->mInventoryPickType == PICK_TEXTURE)
        {
            LLLocalBitmapMgr::getInstance()->feedScrollList(self->mLocalScrollCtrl);
        }
        else if (self->mInventoryPickType == PICK_MATERIAL)
        {
            LLLocalGLTFMaterialMgr::getInstance()->feedScrollList(self->mLocalScrollCtrl);
        }
    }
}

void LLFloaterTexturePicker::onTextureSelect( const LLTextureEntry& te )
{
    LLUUID inventory_item_id = findItemID(te.getID(), true);
    if (inventory_item_id.notNull())
    {
        LLToolPipette::getInstance()->setResult(true, "");
        if (mInventoryPickType == PICK_MATERIAL)
        {
            // tes have no data about material ids
            // Plus gltf materials are layered with overrides,
            // which mean that end result might have no id.
            LL_WARNS() << "tes have no data about material ids" << LL_ENDL;
        }
        else
        {
            setImageID(te.getID());
        }

        mNoCopyTextureSelected = false;
        LLInventoryItem* itemp = gInventory.getItem(inventory_item_id);

        if (itemp && !itemp->getPermissions().allowCopyBy(gAgent.getID()))
        {
            // no copy texture
            mNoCopyTextureSelected = true;
        }
        mSelectionSource = PICKER_INVENTORY;

        commitIfImmediateSet();
    }
    else
    {
        LLToolPipette::getInstance()->setResult(false, LLTrans::getString("InventoryNoTexture"));
    }
}

///////////////////////////////////////////////////////////////////////
// LLTextureCtrl

static LLDefaultChildRegistry::Register<LLTextureCtrl> r("texture_picker");

LLTextureCtrl::LLTextureCtrl(const LLTextureCtrl::Params& p)
:   LLUICtrl(p),
    mDragCallback(NULL),
    mDropCallback(NULL),
    mOnCancelCallback(NULL),
    mOnCloseCallback(NULL),
    mOnSelectCallback(NULL),
    mBorderColor( p.border_color() ),
    mAllowNoTexture( p.allow_no_texture ),
    mAllowLocalTexture( true ),
    mImmediateFilterPermMask( PERM_NONE ),
    mCanApplyImmediately( false ),
    mNeedsRawImageData( false ),
    mValid( true ),
    mShowLoadingPlaceholder( true ),
    mOpenTexPreview(false),
    mBakeTextureEnabled(true),
    mInventoryPickType(p.pick_type),
    mImageAssetID(p.image_id),
    mDefaultImageAssetID(p.default_image_id),
    mDefaultImageName(p.default_image_name),
    mFallbackImage(p.fallback_image)
{

    // Default of defaults is white image for diff tex
    //
    setBlankImageAssetID(IMG_WHITE);

    setAllowNoTexture(p.allow_no_texture);
    setCanApplyImmediately(p.can_apply_immediately);
    mCommitOnSelection = !p.no_commit_on_selection;

    LLTextBox::Params params(p.caption_text);
    params.name(p.label);
    params.rect(LLRect( 0, BTN_HEIGHT_SMALL, getRect().getWidth(), 0 ));
    params.initial_value(p.label());
    params.follows.flags(FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_BOTTOM);
    mCaption = LLUICtrlFactory::create<LLTextBox> (params);
    addChild( mCaption );

    S32 image_top = getRect().getHeight();
    S32 image_bottom = BTN_HEIGHT_SMALL;
    S32 image_middle = (image_top + image_bottom) / 2;
    S32 line_height = LLFontGL::getFontSansSerifSmall()->getLineHeight();

    LLTextBox::Params tentative_label_p(p.multiselect_text);
    tentative_label_p.name("Multiple");
    tentative_label_p.rect(LLRect (0, image_middle + line_height / 2, getRect().getWidth(), image_middle - line_height / 2 ));
    tentative_label_p.follows.flags(FOLLOWS_ALL);
    mTentativeLabel = LLUICtrlFactory::create<LLTextBox> (tentative_label_p);

    // It is no longer possible to associate a style with a textbox, so it has to be done in this fashion
    LLStyle::Params style_params;
    style_params.color = LLColor4::white;

    mTentativeLabel->setText(LLTrans::getString("multiple_textures"), style_params);
    mTentativeLabel->setHAlign(LLFontGL::HCENTER);
    addChild( mTentativeLabel );

    LLRect border_rect = getLocalRect();
    border_rect.mBottom += BTN_HEIGHT_SMALL;
    LLViewBorder::Params vbparams(p.border);
    vbparams.name("border");
    vbparams.rect(border_rect);
    mBorder = LLUICtrlFactory::create<LLViewBorder> (vbparams);
    addChild(mBorder);

    mLoadingPlaceholderString = LLTrans::getString("texture_loading");
}

LLTextureCtrl::~LLTextureCtrl()
{
    closeDependentFloater();
}

void LLTextureCtrl::setShowLoadingPlaceholder(bool showLoadingPlaceholder)
{
    mShowLoadingPlaceholder = showLoadingPlaceholder;
}

void LLTextureCtrl::setCaption(const std::string& caption)
{
    mCaption->setText( caption );
}

void LLTextureCtrl::setCanApplyImmediately(bool b)
{
    mCanApplyImmediately = b;
    LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
    if( floaterp )
    {
        floaterp->setCanApplyImmediately(b);
    }
}

void LLTextureCtrl::setCanApply(bool can_preview, bool can_apply)
{
    LLFloaterTexturePicker* floaterp = dynamic_cast<LLFloaterTexturePicker*>(mFloaterHandle.get());
    if( floaterp )
    {
        floaterp->setCanApply(can_preview, can_apply);
    }
}

void LLTextureCtrl::setImmediateFilterPermMask(PermissionMask mask)
{
    mImmediateFilterPermMask = mask;

    LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
    if (floaterp)
    {
        floaterp->setImmediateFilterPermMask(mask);
    }
}

void LLTextureCtrl::setFilterPermissionMasks(PermissionMask mask)
{
    setImmediateFilterPermMask(mask);
    setDnDFilterPermMask(mask);
}

void LLTextureCtrl::onVisibilityChange(bool new_visibility)
{
    if (!new_visibility)
    {
        // *NOTE: Vertex buffer for sphere preview is still cached
        mGLTFPreview = nullptr;
    }
    else
    {
        llassert(!mGLTFPreview);
    }
}

void LLTextureCtrl::setVisible(bool visible )
{
    if( !visible )
    {
        closeDependentFloater();
    }
    LLUICtrl::setVisible( visible );
}

void LLTextureCtrl::setEnabled( bool enabled )
{
    LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
    if( floaterp )
    {
        floaterp->setActive(enabled);
    }
    if( enabled )
    {
        std::string tooltip;
        if (floaterp) tooltip = floaterp->getString("choose_picture");
        setToolTip( tooltip );
    }
    else
    {
        setToolTip( std::string() );
        // *TODO: would be better to keep floater open and show
        // disabled state.
        closeDependentFloater();
    }

    mCaption->setEnabled( enabled );

    LLView::setEnabled( enabled );
}

void LLTextureCtrl::setValid(bool valid )
{
    mValid = valid;
    if (!valid)
    {
        LLFloaterTexturePicker* pickerp = (LLFloaterTexturePicker*)mFloaterHandle.get();
        if (pickerp)
        {
            pickerp->setActive(false);
        }
    }
}


// virtual
void LLTextureCtrl::clear()
{
    setImageAssetID(LLUUID::null);
}

void LLTextureCtrl::setLabel(const std::string& label)
{
    mLabel = label;
    mCaption->setText(label);
}

void LLTextureCtrl::showPicker(bool take_focus)
{
    // show hourglass cursor when loading inventory window
    // because inventory construction is slooow
    getWindow()->setCursor(UI_CURSOR_WAIT);
    LLFloater* floaterp = mFloaterHandle.get();

    // Show the dialog
    if( floaterp )
    {
        floaterp->openFloater();
    }
    else
    {
        floaterp = new LLFloaterTexturePicker(
            this,
            getImageAssetID(),
            getDefaultImageAssetID(),
            getBlankImageAssetID(),
            getTentative(),
            getAllowNoTexture(),
            mLabel,
            mImmediateFilterPermMask,
            mDnDFilterPermMask,
            mCanApplyImmediately,
            mFallbackImage,
            mInventoryPickType);
        mFloaterHandle = floaterp->getHandle();

        LLFloaterTexturePicker* texture_floaterp = dynamic_cast<LLFloaterTexturePicker*>(floaterp);
        if (texture_floaterp && mOnTextureSelectedCallback)
        {
            texture_floaterp->setTextureSelectedCallback(mOnTextureSelectedCallback);
        }
        if (texture_floaterp && mOnCloseCallback)
        {
            texture_floaterp->setOnFloaterCloseCallback(boost::bind(&LLTextureCtrl::onFloaterClose, this));
        }
        if (texture_floaterp)
        {
            texture_floaterp->setOnFloaterCommitCallback(boost::bind(&LLTextureCtrl::onFloaterCommit, this, _1, _2, _3, _4, _5));
        }
        if (texture_floaterp)
        {
            texture_floaterp->setSetImageAssetIDCallback(boost::bind(&LLTextureCtrl::setImageAssetID, this, _1));

            texture_floaterp->setBakeTextureEnabled(mBakeTextureEnabled && mInventoryPickType != PICK_MATERIAL);
        }

        LLFloater* root_floater = gFloaterView->getParentFloater(this);
        if (root_floater)
            root_floater->addDependentFloater(floaterp);
        floaterp->openFloater();
    }

    LLFloaterTexturePicker* picker_floater = dynamic_cast<LLFloaterTexturePicker*>(floaterp);
    if (picker_floater)
    {
        picker_floater->setLocalTextureEnabled(mAllowLocalTexture);
    }

    if (take_focus)
    {
        floaterp->setFocus(true);
    }
}


void LLTextureCtrl::closeDependentFloater()
{
    LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
    if( floaterp && floaterp->isInVisibleChain())
    {
        floaterp->setOwner(NULL);
        floaterp->setVisible(false);
        floaterp->closeFloater();
    }
}

// Allow us to download textures quickly when floater is shown
class LLTextureFetchDescendentsObserver : public LLInventoryFetchDescendentsObserver
{
public:
    virtual void done()
    {
        // We need to find textures in all folders, so get the main
        // background download going.
        LLInventoryModelBackgroundFetch::instance().start();
        gInventory.removeObserver(this);
        delete this;
    }
};

bool LLTextureCtrl::handleHover(S32 x, S32 y, MASK mask)
{
    getWindow()->setCursor(mBorder->parentPointInView(x,y) ? UI_CURSOR_HAND : UI_CURSOR_ARROW);
    return true;
}


bool LLTextureCtrl::handleMouseDown(S32 x, S32 y, MASK mask)
{
    bool handled = LLUICtrl::handleMouseDown( x, y , mask );

    if (!handled && mBorder->parentPointInView(x, y))
    {
        if (!mOpenTexPreview)
        {
            showPicker(false);
            if (mInventoryPickType == PICK_MATERIAL)
            {
                //grab materials first...
                LLInventoryModelBackgroundFetch::instance().start(gInventory.findCategoryUUIDForType(LLFolderType::FT_MATERIAL));
            }
            else
            {
                //grab textures first...
                LLInventoryModelBackgroundFetch::instance().start(gInventory.findCategoryUUIDForType(LLFolderType::FT_TEXTURE));
            }
            //...then start full inventory fetch.
            if (!LLInventoryModelBackgroundFetch::instance().inventoryFetchStarted())
            {
                LLInventoryModelBackgroundFetch::instance().start();
            }
            handled = true;
        }
        else
        {
            if (getImageAssetID().notNull())
            {
                LLPreviewTexture* preview_texture = LLFloaterReg::showTypedInstance<LLPreviewTexture>("preview_texture", getValue());
                if (preview_texture && !preview_texture->isDependent())
                {
                    LLFloater* root_floater = gFloaterView->getParentFloater(this);
                    if (root_floater)
                    {
                        root_floater->addDependentFloater(preview_texture);
                        preview_texture->hideCtrlButtons();
                    }
                }
            }
        }
    }

    return handled;
}

void LLTextureCtrl::onFloaterClose()
{
    LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();

    if (floaterp)
    {
        if (mOnCloseCallback)
        {
            mOnCloseCallback(this,LLSD());
        }
        floaterp->setOwner(NULL);
    }

    mFloaterHandle.markDead();
}

void LLTextureCtrl::onFloaterCommit(ETexturePickOp op, LLPickerSource source, const LLUUID& asset_id, const LLUUID& inv_id, const LLUUID& tracking_id)
{
    LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();

    if( floaterp && getEnabled())
    {
        if (op == TEXTURE_CANCEL)
            mViewModel->resetDirty();
        // If the "no_commit_on_selection" parameter is set
        // we get dirty only when user presses OK in the picker
        // (i.e. op == TEXTURE_SELECT) or texture changes via DnD.
        else if (mCommitOnSelection || op == TEXTURE_SELECT)
            mViewModel->setDirty(); // *TODO: shouldn't we be using setValue() here?

        if(floaterp->isDirty() || asset_id.notNull()) // mModelView->setDirty does not work.
        {
            setTentative( false );

            switch(source)
            {
                case PICKER_INVENTORY:
                    mImageItemID = inv_id;
                    mImageAssetID = asset_id;
                    mLocalTrackingID.setNull();
                    break;
                case PICKER_BAKE:
                    mImageItemID = LLUUID::null;
                    mImageAssetID = asset_id;
                    mLocalTrackingID.setNull();
                    break;
                case PICKER_LOCAL:
                    mImageItemID = LLUUID::null;
                    mImageAssetID = asset_id;
                    mLocalTrackingID = tracking_id;
                    break;
                case PICKER_UNKNOWN:
                default:
                    mImageItemID = floaterp->findItemID(asset_id, false);
                    mImageAssetID = asset_id;
                    mLocalTrackingID.setNull();
                    break;
            }

            LL_DEBUGS() << "mImageAssetID: " << mImageAssetID << ", mImageItemID: " << mImageItemID << LL_ENDL;

            if (op == TEXTURE_SELECT && mOnSelectCallback)
            {
                mOnSelectCallback(this, LLSD());
            }
            else if (op == TEXTURE_CANCEL && mOnCancelCallback)
            {
                mOnCancelCallback( this, LLSD() );
            }
            else
            {
                // If the "no_commit_on_selection" parameter is set
                // we commit only when user presses OK in the picker
                // (i.e. op == TEXTURE_SELECT) or texture changes via DnD.
                if (mCommitOnSelection || op == TEXTURE_SELECT)
                {
                    onCommit();
                }
            }
        }
    }
}

void LLTextureCtrl::setOnTextureSelectedCallback(texture_selected_callback cb)
{
    mOnTextureSelectedCallback = cb;
    LLFloaterTexturePicker* floaterp = dynamic_cast<LLFloaterTexturePicker*>(mFloaterHandle.get());
    if (floaterp)
    {
        floaterp->setTextureSelectedCallback(cb);
    }
}

void    LLTextureCtrl::setImageAssetName(const std::string& name)
{
    LLPointer<LLUIImage> imagep = LLUI::getUIImage(name);
    if(imagep)
    {
        LLViewerFetchedTexture* pTexture = dynamic_cast<LLViewerFetchedTexture*>(imagep->getImage().get());
        if(pTexture)
        {
            LLUUID id = pTexture->getID();
            setImageAssetID(id);
        }
    }
}

void LLTextureCtrl::setImageAssetID( const LLUUID& asset_id )
{
    if( mImageAssetID != asset_id )
    {
        mImageItemID.setNull();
        mImageAssetID = asset_id;
        mLocalTrackingID.setNull();
        LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
        if( floaterp && getEnabled() )
        {
            floaterp->setImageID( asset_id );
            floaterp->resetDirty();
        }
    }
}

void LLTextureCtrl::setBakeTextureEnabled(bool enabled)
{
    mBakeTextureEnabled = enabled;
    LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
    if (floaterp)
    {
        floaterp->setBakeTextureEnabled(enabled && mInventoryPickType != PICK_MATERIAL);
    }
}

void LLTextureCtrl::setInventoryPickType(EPickInventoryType type)
{
    mInventoryPickType = type;
    LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
    if (floaterp)
    {
        floaterp->setInventoryPickType(type);
    }
}

bool LLTextureCtrl::handleDragAndDrop(S32 x, S32 y, MASK mask,
                      bool drop, EDragAndDropType cargo_type, void *cargo_data,
                      EAcceptance *accept,
                      std::string& tooltip_msg)
{
    bool handled = false;

    // this downcast may be invalid - but if the second test below
    // returns true, then the cast was valid, and we can perform
    // the third test without problems.
    LLInventoryItem* item = (LLInventoryItem*)cargo_data;

    bool is_mesh = cargo_type == DAD_MESH;
    bool is_texture = cargo_type == DAD_TEXTURE;
    bool is_material = cargo_type == DAD_MATERIAL;

    bool allow_dnd = false;
    if (mInventoryPickType == PICK_MATERIAL)
    {
        allow_dnd = is_material;
    }
    else if (mInventoryPickType == PICK_TEXTURE)
    {
        allow_dnd = is_texture || is_mesh;
    }
    else
    {
        allow_dnd = is_texture || is_mesh || is_material;
    }

    if (getEnabled() && allow_dnd && allowDrop(item, cargo_type, tooltip_msg))
    {
        if (drop)
        {
            if(doDrop(item))
            {
                if (!mCommitOnSelection)
                    mViewModel->setDirty();

                // This removes the 'Multiple' overlay, since
                // there is now only one texture selected.
                setTentative( false );
                onCommit();
            }
        }

        *accept = ACCEPT_YES_SINGLE;
    }
    else
    {
        *accept = ACCEPT_NO;
    }

    handled = true;
    LL_DEBUGS("UserInput") << "dragAndDrop handled by LLTextureCtrl " << getName() << LL_ENDL;

    return handled;
}

void LLTextureCtrl::draw()
{
    mBorder->setKeyboardFocusHighlight(hasFocus());

    LLPointer<LLViewerTexture> preview = NULL;

    if (!mValid)
    {
        mTexturep = NULL;
        mGLTFMaterial = NULL;
        mGLTFPreview = NULL;
    }
    else if (!mImageAssetID.isNull())
    {
        if (LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(mImageAssetID))
        {
            LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
            if (obj)
            {
                LLViewerTexture* viewerTexture = obj->getBakedTextureForMagicId(mImageAssetID);
                mTexturep = viewerTexture ? dynamic_cast<LLViewerFetchedTexture*>(viewerTexture) : NULL;
                mGLTFMaterial = NULL;
                mGLTFPreview = NULL;

                preview = mTexturep;
            }

        }

        if (preview.isNull())
        {
            LLPointer<LLFetchedGLTFMaterial> old_material = mGLTFMaterial;
            mGLTFMaterial = NULL;
            mTexturep = NULL;
            if (mInventoryPickType == PICK_MATERIAL)
            {
                mGLTFMaterial = gGLTFMaterialList.getMaterial(mImageAssetID);
                if (mGLTFPreview.isNull() || mGLTFMaterial.isNull() || (old_material.notNull() && (old_material.get() != mGLTFMaterial.get())))
                {
                    // Only update the preview if needed, since gGLTFMaterialPreviewMgr does not cache the preview.
                    if (mGLTFMaterial.isNull())
                    {
                        mGLTFPreview = nullptr;
                    }
                    else
                    {
                        mGLTFPreview = gGLTFMaterialPreviewMgr.getPreview(mGLTFMaterial);
                    }
                }
                if (mGLTFPreview)
                {
                    mGLTFPreview->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
                }

                preview = mGLTFPreview;
            }
            else
            {
                mTexturep = LLViewerTextureManager::getFetchedTexture(mImageAssetID, FTT_DEFAULT, true, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
                mTexturep->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
                mTexturep->forceToSaveRawImage(0);

                preview = mTexturep;
            }
        }
    }
    else//mImageAssetID == LLUUID::null
    {
        mTexturep = NULL;
        mGLTFMaterial = NULL;
        mGLTFPreview = NULL;
    }

    // Border
    LLRect border( 0, getRect().getHeight(), getRect().getWidth(), BTN_HEIGHT_SMALL );
    gl_rect_2d( border, mBorderColor.get(), false );

    // Interior
    LLRect interior = border;
    interior.stretch( -1 );

    // If we're in a focused floater, don't apply the floater's alpha to the texture (STORM-677).
    const F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
    if( preview )
    {
        if( preview->getComponents() == 4 )
        {
            gl_rect_2d_checkerboard( interior, alpha );
        }

        gl_draw_scaled_image( interior.mLeft, interior.mBottom, interior.getWidth(), interior.getHeight(), preview, UI_VERTEX_COLOR % alpha);
        preview->addTextureStats( (F32)(interior.getWidth() * interior.getHeight()) );
    }
    else if (!mFallbackImage.isNull())
    {
        mFallbackImage->draw(interior, UI_VERTEX_COLOR % alpha);
    }
    else
    {
        gl_rect_2d( interior, LLColor4::grey % alpha, true );

        // Draw X
        gl_draw_x( interior, LLColor4::black );
    }

    mTentativeLabel->setVisible( getTentative() );

    // Show "Loading..." string on the top left corner while this texture is loading.
    // Using the discard level, do not show the string if the texture is almost but not
    // fully loaded.
    if (mTexturep.notNull() &&
        !mTexturep->isFullyLoaded() &&
        mShowLoadingPlaceholder)
    {
        U32 v_offset = 25;
        LLFontGL* font = LLFontGL::getFontSansSerif();

        // Don't show as loaded if the texture is almost fully loaded (i.e. discard1) unless god
        if ((mTexturep->getDiscardLevel() > 1) || gAgent.isGodlike())
        {
            font->renderUTF8(
                mLoadingPlaceholderString,
                0,
                (interior.mLeft+3),
                (interior.mTop-v_offset),
                LLColor4::white,
                LLFontGL::LEFT,
                LLFontGL::BASELINE,
                LLFontGL::DROP_SHADOW);
        }

        // Optionally show more detailed information.
        if (gSavedSettings.getBOOL("DebugAvatarRezTime"))
        {
            LLFontGL* font = LLFontGL::getFontSansSerif();
            std::string tdesc;
            // Show what % the texture has loaded (0 to 100%, 100 is highest), and what level of detail (5 to 0, 0 is best).

            v_offset += 12;
            tdesc = llformat("  PK  : %d%%", U32(mTexturep->getDownloadProgress()*100.0));
            font->renderUTF8(tdesc, 0, interior.mLeft+3, interior.mTop-v_offset,
                             LLColor4::white, LLFontGL::LEFT, LLFontGL::BASELINE, LLFontGL::DROP_SHADOW);

            v_offset += 12;
            tdesc = llformat("  LVL: %d", mTexturep->getDiscardLevel());
            font->renderUTF8(tdesc, 0, interior.mLeft+3, interior.mTop-v_offset,
                             LLColor4::white, LLFontGL::LEFT, LLFontGL::BASELINE, LLFontGL::DROP_SHADOW);

            v_offset += 12;
            tdesc = llformat("  ID  : %s...", (mImageAssetID.asString().substr(0,7)).c_str());
            font->renderUTF8(tdesc, 0, interior.mLeft+3, interior.mTop-v_offset,
                             LLColor4::white, LLFontGL::LEFT, LLFontGL::BASELINE, LLFontGL::DROP_SHADOW);
        }
    }

    LLUICtrl::draw();
}

bool LLTextureCtrl::allowDrop(LLInventoryItem* item, EDragAndDropType cargo_type, std::string& tooltip_msg)
{
    bool copy = item->getPermissions().allowCopyBy(gAgent.getID());
    bool mod = item->getPermissions().allowModifyBy(gAgent.getID());
    bool xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
                                                        gAgent.getID());

    PermissionMask item_perm_mask = 0;
    if (copy) item_perm_mask |= PERM_COPY;
    if (mod)  item_perm_mask |= PERM_MODIFY;
    if (xfer) item_perm_mask |= PERM_TRANSFER;

    PermissionMask filter_perm_mask = mImmediateFilterPermMask;
    if ( (item_perm_mask & filter_perm_mask) == filter_perm_mask )
    {
        if(mDragCallback)
        {
            return mDragCallback(this, item);
        }
        else
        {
            return true;
        }
    }
    else
    {
        PermissionMask mask = PERM_COPY | PERM_TRANSFER;
        if ((filter_perm_mask & mask) == mask
            && cargo_type == DAD_TEXTURE)
        {
            tooltip_msg.assign(LLTrans::getString("TooltipTextureRestrictedDrop"));
        }
        return false;
    }
}

bool LLTextureCtrl::doDrop(LLInventoryItem* item)
{
    // call the callback if it exists.
    if(mDropCallback)
    {
        // if it returns true, we return true, and therefore the
        // commit is called above.
        return mDropCallback(this, item);
    }

    // no callback installed, so just set the image ids and carry on.
    LLUUID asset_id = item->getAssetUUID();

    if (mInventoryPickType == PICK_MATERIAL && asset_id.isNull())
    {
        // If an inventory material has a null asset, consider it a valid blank material(gltf)
        asset_id = BLANK_MATERIAL_ASSET_ID;
    }

    setImageAssetID(asset_id);
    mImageItemID = item->getUUID();
    return true;
}

bool LLTextureCtrl::handleUnicodeCharHere(llwchar uni_char)
{
    if( ' ' == uni_char )
    {
        showPicker(true);
        return true;
    }
    return LLUICtrl::handleUnicodeCharHere(uni_char);
}

void LLTextureCtrl::setValue( const LLSD& value )
{
    setImageAssetID(value.asUUID());
}

LLSD LLTextureCtrl::getValue() const
{
    return LLSD(getImageAssetID());
}

namespace LLInitParam
{
    void TypeValues<EPickInventoryType>::declareValues()
    {
        declare("texture_material", PICK_TEXTURE_MATERIAL);
        declare("texture", PICK_TEXTURE);
        declare("material", PICK_MATERIAL);
    }
}





