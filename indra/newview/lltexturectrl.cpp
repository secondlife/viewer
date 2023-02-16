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
#include "llviewermenufile.h"	// LLFilePickerReplyThread
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
#include "lllocalbitmaps.h"
#include "lllocalgltfmaterials.h"
#include "llerror.h"

#include "llavatarappearancedefines.h"


//static
bool get_is_predefined_texture(LLUUID asset_id)
{
    if (asset_id == LLUUID(gSavedSettings.getString("DefaultObjectTexture"))
        || asset_id == LLUUID(gSavedSettings.getString("UIImgWhiteUUID"))
        || asset_id == LLUUID(gSavedSettings.getString("UIImgInvisibleUUID"))
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

LLFloaterTexturePicker::LLFloaterTexturePicker(	
	LLView* owner,
	LLUUID image_asset_id,
	LLUUID default_image_asset_id,
	LLUUID blank_image_asset_id,
	BOOL tentative,
	BOOL allow_no_texture,
	const std::string& label,
	PermissionMask immediate_filter_perm_mask,
	PermissionMask dnd_filter_perm_mask,
	BOOL can_apply_immediately,
	LLUIImagePtr fallback_image)
:	LLFloater(LLSD()),
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
	mActive( TRUE ),
	mFilterEdit(NULL),
	mImmediateFilterPermMask(immediate_filter_perm_mask),
	mDnDFilterPermMask(dnd_filter_perm_mask),
	mContextConeOpacity(0.f),
	mSelectedItemPinned( FALSE ),
	mCanApply(true),
	mCanPreview(true),
	mPreviewSettingChanged(false),
	mOnFloaterCommitCallback(NULL),
	mOnFloaterCloseCallback(NULL),
	mSetImageAssetIDCallback(NULL),
	mOnUpdateImageStatsCallback(NULL),
	mBakeTextureEnabled(FALSE),
    mInventoryPickType(LLTextureCtrl::PICK_TEXTURE)
{
	mCanApplyImmediately = can_apply_immediately;
	buildFromFile("floater_texture_ctrl.xml");
	setCanMinimize(FALSE);
}

LLFloaterTexturePicker::~LLFloaterTexturePicker()
{
}

void LLFloaterTexturePicker::setImageID(const LLUUID& image_id, bool set_selection /*=true*/)
{
	if( ((mImageAssetID != image_id) || mTentative) && mActive)
	{
		mNoCopyTextureSelected = FALSE;
		mViewModel->setDirty(); // *TODO: shouldn't we be using setValue() here?
		mImageAssetID = image_id; 

		if (LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(mImageAssetID))
		{
			if ( mBakeTextureEnabled && mModeSelector->getValue().asInteger() != 2)
			{
				mModeSelector->selectByValue(2);
				onModeSelect(0,this);
			}
		}
		else
		{
			if (mModeSelector->getValue().asInteger() == 2)
			{
				mModeSelector->selectByValue(0);
				onModeSelect(0,this);
			}
			
			LLUUID item_id = findItemID(mImageAssetID, FALSE);
			if (item_id.isNull())
			{
				mInventoryPanel->getRootFolder()->clearSelection();
			}
			else
			{
				LLInventoryItem* itemp = gInventory.getItem(image_id);
				if (itemp && !itemp->getPermissions().allowCopyBy(gAgent.getID()))
				{
					// no copy texture
					getChild<LLUICtrl>("apply_immediate_check")->setValue(FALSE);
					mNoCopyTextureSelected = TRUE;
				}
			}

			if (set_selection)
			{
				mInventoryPanel->setSelection(item_id, TAKE_FOCUS_NO);
			}
		}
		

		
	}
}

void LLFloaterTexturePicker::setActive( BOOL active )					
{
	if (!active && getChild<LLUICtrl>("Pipette")->getValue().asBoolean())
	{
		stopUsingPipette();
	}
	mActive = active; 
}

void LLFloaterTexturePicker::setCanApplyImmediately(BOOL b)
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

void LLFloaterTexturePicker::updateImageStats()
{
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
		if (mTexturep->getFullWidth() > 0 && mTexturep->getFullHeight() > 0)
		{
			std::string formatted_dims = llformat("%d x %d", mTexturep->getFullWidth(),mTexturep->getFullHeight());
			mResolutionLabel->setTextArg("[DIMENSIONS]", formatted_dims);
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
}

// virtual
BOOL LLFloaterTexturePicker::handleDragAndDrop( 
		S32 x, S32 y, MASK mask,
		BOOL drop,
		EDragAndDropType cargo_type, void *cargo_data, 
		EAcceptance *accept,
		std::string& tooltip_msg)
{
	BOOL handled = FALSE;

	bool is_mesh = cargo_type == DAD_MESH;
    bool is_texture = cargo_type == DAD_TEXTURE;
    bool is_material = cargo_type == DAD_MATERIAL;

    bool allow_dnd = false;
    if (mInventoryPickType == LLTextureCtrl::PICK_MATERIAL)
    {
        allow_dnd = is_material;
    }
    else if (mInventoryPickType == LLTextureCtrl::PICK_TEXTURE)
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

		BOOL copy = item->getPermissions().allowCopyBy(gAgent.getID());
		BOOL mod = item->getPermissions().allowModifyBy(gAgent.getID());
		BOOL xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
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
				setImageID( item->getAssetUUID() );
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

	handled = TRUE;
	LL_DEBUGS("UserInput") << "dragAndDrop handled by LLFloaterTexturePicker " << getName() << LL_ENDL;

	return handled;
}

BOOL LLFloaterTexturePicker::handleKeyHere(KEY key, MASK mask)
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
					root_folder->setSelection(itemp, FALSE, FALSE);
				}
			}
			root_folder->scrollToShowSelection();
			
			// move focus to inventory proper
			mInventoryPanel->setFocus(TRUE);
			
			// treat this as a user selection of the first filtered result
			commitIfImmediateSet();
			
			return TRUE;
		}
		
		if (mInventoryPanel->hasFocus() && key == KEY_UP)
		{
			mFilterEdit->focusFirstItem(TRUE);
		}
	}

	return LLFloater::handleKeyHere(key, mask);
}

void LLFloaterTexturePicker::onClose(bool app_quitting)
{
	if (mOwner && mOnFloaterCloseCallback)
	{
		mOnFloaterCloseCallback();
	}
	stopUsingPipette();
}

// virtual
BOOL LLFloaterTexturePicker::postBuild()
{
	LLFloater::postBuild();

	if (!mLabel.empty())
	{
		std::string pick = getString("pick title");
	
		setTitle(pick + mLabel);
	}
	mTentativeLabel = getChild<LLTextBox>("Multiple");

	mResolutionLabel = getChild<LLTextBox>("size_lbl");


	childSetAction("Default",LLFloaterTexturePicker::onBtnSetToDefault,this);
	childSetAction("None", LLFloaterTexturePicker::onBtnNone,this);
	childSetAction("Blank", LLFloaterTexturePicker::onBtnBlank,this);

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
		mInventoryPanel->getRootFolder()->setAutoSelectOverride(TRUE);

		// Commented out to scroll to currently selected texture. See EXT-5403.
		// // store this filter as the default one
		// mInventoryPanel->getRootFolder()->getFilter().markDefault();

		// Commented out to stop opening all folders with textures
		// mInventoryPanel->openDefaultFolderForType(LLFolderType::FT_TEXTURE);

		// don't put keyboard focus on selected item, because the selection callback
		// will assume that this was user input

		

		if(!mImageAssetID.isNull())
		{
			mInventoryPanel->setSelection(findItemID(mImageAssetID, FALSE), TAKE_FOCUS_NO);
		}
	}

	childSetAction("l_add_btn", LLFloaterTexturePicker::onBtnAdd, this);
	childSetAction("l_rem_btn", LLFloaterTexturePicker::onBtnRemove, this);
	childSetAction("l_upl_btn", LLFloaterTexturePicker::onBtnUpload, this);

	mLocalScrollCtrl = getChild<LLScrollListCtrl>("l_name_list");
	mLocalScrollCtrl->setCommitCallback(onLocalScrollCommit, this);
    refreshLocalList();

	mNoCopyTextureSelected = FALSE;

	getChild<LLUICtrl>("apply_immediate_check")->setValue(mCanApplyImmediately && gSavedSettings.getBOOL("TextureLivePreview"));
	childSetCommitCallback("apply_immediate_check", onApplyImmediateCheck, this);
    getChildView("apply_immediate_check")->setEnabled(mCanApplyImmediately);

	getChild<LLUICtrl>("Pipette")->setCommitCallback( boost::bind(&LLFloaterTexturePicker::onBtnPipette, this));
	childSetAction("Cancel", LLFloaterTexturePicker::onBtnCancel,this);
	childSetAction("Select", LLFloaterTexturePicker::onBtnSelect,this);

	mSavedFolderState.setApply(FALSE);

	LLToolPipette::getInstance()->setToolSelectCallback(boost::bind(&LLFloaterTexturePicker::onTextureSelect, this, _1));
	
	getChild<LLComboBox>("l_bake_use_texture_combo_box")->setCommitCallback(onBakeTextureSelect, this);

	setBakeTextureEnabled(TRUE);
	return TRUE;
}

// virtual
void LLFloaterTexturePicker::draw()
{
    static LLCachedControl<F32> max_opacity(gSavedSettings, "PickerContextOpacity", 0.4f);
    drawConeToOwner(mContextConeOpacity, max_opacity, mOwner);

    // This is going to spam mOnUpdateImageStatsCallback,
    // either move elsewhere or fix to cause update once per image
	updateImageStats();

	// if we're inactive, gray out "apply immediate" checkbox
	getChildView("Select")->setEnabled(mActive && mCanApply);
	getChildView("Pipette")->setEnabled(mActive);
	getChild<LLUICtrl>("Pipette")->setValue(LLToolMgr::getInstance()->getCurrentTool() == LLToolPipette::getInstance());

	//BOOL allow_copy = FALSE;
	if( mOwner ) 
	{
		mTexturep = NULL;
        mGLTFMaterial = NULL;
        if (mImageAssetID.notNull())
        {
            if (mInventoryPickType == LLTextureCtrl::PICK_MATERIAL)
            {
                mGLTFMaterial = (LLFetchedGLTFMaterial*) gGLTFMaterialList.getMaterial(mImageAssetID);
            }
            else
            {
                LLPointer<LLViewerFetchedTexture> texture = NULL;

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
			mTentativeLabel->setVisible( FALSE  );
		}

		getChildView("Default")->setEnabled(mImageAssetID != mDefaultImageAssetID || mTentative);
		getChildView("Blank")->setEnabled((mImageAssetID != mBlankImageAssetID && mBlankImageAssetID.notNull()) || mTentative);
		getChildView("None")->setEnabled(mAllowNoTexture && (!mImageAssetID.isNull() || mTentative));

		LLFloater::draw();

		if( isMinimized() )
		{
			return;
		}

		// Border
		LLRect border = getChildView("preview_widget")->getRect();
		gl_rect_2d( border, LLColor4::black, FALSE );


		// Interior
		LLRect interior = border;
		interior.stretch( -1 ); 

		// If the floater is focused, don't apply its alpha to the texture (STORM-677).
		const F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
		if( mTexturep )
		{
			if( mTexturep->getComponents() == 4 )
			{
				gl_rect_2d_checkerboard( interior, alpha );
			}

			gl_draw_scaled_image( interior.mLeft, interior.mBottom, interior.getWidth(), interior.getHeight(), mTexturep, UI_VERTEX_COLOR % alpha );

			// Pump the priority
			mTexturep->addTextureStats( (F32)(interior.getWidth() * interior.getHeight()) );
		}
		else if (!mFallbackImage.isNull())
		{
			mFallbackImage->draw(interior, UI_VERTEX_COLOR % alpha);
		}
		else
		{
			gl_rect_2d( interior, LLColor4::grey % alpha, TRUE );

			// Draw X
			gl_draw_x(interior, LLColor4::black );
		}

		// Draw Tentative Label over the image
		if( mTentative && !mViewModel->isDirty() )
		{
			mTentativeLabel->setVisible( TRUE );
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
		// AutoSelectOverride set to TRUE. We force PinningSelectedItem
		// flag to FALSE state and setting filter "dirty" to update
		// scroll container to show selected item (see LLFolderView::doIdle()).
		if (!is_filter_active && !mSelectedItemPinned)
		{
			folder_view->setPinningSelectedItem(mSelectedItemPinned);
			folder_view->getViewModelItem()->dirtyFilter();
			mSelectedItemPinned = TRUE;
		}
	}
}

const LLUUID& LLFloaterTexturePicker::findItemID(const LLUUID& asset_id, BOOL copyable_only, BOOL ignore_library)
{
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLAssetIDMatches asset_id_matches(asset_id);
	gInventory.collectDescendentsIf(LLUUID::null,
							cats,
							items,
							LLInventoryModel::INCLUDE_TRASH,
							asset_id_matches);

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
	if (!mNoCopyTextureSelected && mOnFloaterCommitCallback && mCanApply)
	{
		mOnFloaterCommitCallback(LLTextureCtrl::TEXTURE_CHANGE, LLUUID::null);
	}
}

void LLFloaterTexturePicker::commitCancel()
{
	if (!mNoCopyTextureSelected && mOnFloaterCommitCallback && mCanApply)
	{
		mOnFloaterCommitCallback(LLTextureCtrl::TEXTURE_CANCEL, LLUUID::null);
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

/*
// static
void LLFloaterTexturePicker::onBtnRevert(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	self->setImageID( self->mOriginalImageAssetID );
	// TODO: Change this to tell the owner to cancel.  It needs to be
	// smart enough to restore multi-texture selections.
	self->mOwner->onFloaterCommit();
	self->mViewModel->resetDirty();
}*/

// static
void LLFloaterTexturePicker::onBtnCancel(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	self->setImageID( self->mOriginalImageAssetID );
	if (self->mOnFloaterCommitCallback)
	{
		self->mOnFloaterCommitCallback(LLTextureCtrl::TEXTURE_CANCEL, LLUUID::null);
	}
	self->mViewModel->resetDirty();
	self->closeFloater();
}

// static
void LLFloaterTexturePicker::onBtnSelect(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	LLUUID local_id = LLUUID::null;
	if (self->mOwner)
	{
		if (self->mLocalScrollCtrl->getVisible() && !self->mLocalScrollCtrl->getAllSelected().empty())
		{
            LLSD data = self->mLocalScrollCtrl->getFirstSelected()->getValue();
            LLUUID temp_id = data["id"];
            S32 asset_type = data["type"].asInteger();

            if (LLAssetType::AT_MATERIAL == asset_type)
            {
                local_id = LLLocalGLTFMaterialMgr::getInstance()->getWorldID(temp_id);
            }
            else
            {
                local_id = LLLocalBitmapMgr::getInstance()->getWorldID(temp_id);
            }
		}
	}
	
	if (self->mOnFloaterCommitCallback)
	{
		self->mOnFloaterCommitCallback(LLTextureCtrl::TEXTURE_SELECT, local_id);
	}
	self->closeFloater();
}

void LLFloaterTexturePicker::onBtnPipette()
{
	BOOL pipette_active = getChild<LLUICtrl>("Pipette")->getValue().asBoolean();
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

void LLFloaterTexturePicker::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	if (items.size())
	{
		LLFolderViewItem* first_item = items.front();
		LLInventoryItem* itemp = gInventory.getItem(static_cast<LLFolderViewModelItemInventory*>(first_item->getViewModelItem())->getUUID());
		mNoCopyTextureSelected = FALSE;
		if (itemp)
		{
			if (!mTextureSelectedCallback.empty())
			{
				mTextureSelectedCallback(itemp);
			}
			if (!itemp->getPermissions().allowCopyBy(gAgent.getID()))
			{
				mNoCopyTextureSelected = TRUE;
			}
			setImageID(itemp->getAssetUUID(),false);
			mViewModel->setDirty(); // *TODO: shouldn't we be using setValue() here?

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
    int index = self->mModeSelector->getValue().asInteger();

	self->getChild<LLButton>("Default")->setVisible(index == 0 ? TRUE : FALSE);
	self->getChild<LLButton>("Blank")->setVisible(index == 0 ? TRUE : FALSE);
	self->getChild<LLButton>("None")->setVisible(index == 0 ? TRUE : FALSE);
	self->getChild<LLFilterEditor>("inventory search editor")->setVisible(index == 0 ? TRUE : FALSE);
	self->getChild<LLInventoryPanel>("inventory panel")->setVisible(index == 0 ? TRUE : FALSE);

	self->getChild<LLButton>("l_add_btn")->setVisible(index == 1 ? TRUE : FALSE);
	self->getChild<LLButton>("l_rem_btn")->setVisible(index == 1 ? TRUE : FALSE);
	self->getChild<LLButton>("l_upl_btn")->setVisible(index == 1 ? TRUE : FALSE);
	self->getChild<LLScrollListCtrl>("l_name_list")->setVisible(index == 1 ? TRUE : FALSE);

	self->getChild<LLComboBox>("l_bake_use_texture_combo_box")->setVisible(index == 2 ? TRUE : FALSE);

    bool pipette_visible = (index == 0)
        && (self->mInventoryPickType != LLTextureCtrl::PICK_MATERIAL);
	self->getChild<LLButton>("Pipette")->setVisible(pipette_visible);

	if (index == 2)
	{
		self->stopUsingPipette();

		S8 val = -1;

		LLUUID imageID = self->mImageAssetID;
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


		self->getChild<LLComboBox>("l_bake_use_texture_combo_box")->setSelectedByValue(val, TRUE);
	}
}

// static
void LLFloaterTexturePicker::onBtnAdd(void* userdata)
{
    LLFloaterTexturePicker* self = (LLFloaterTexturePicker*)userdata;

    if (self->mInventoryPickType == LLTextureCtrl::PICK_TEXTURE_MATERIAL)
    {
        LLFilePickerReplyThread::startPicker(boost::bind(&onPickerCallback, _1, self->getHandle()), LLFilePicker::FFLOAD_MATERIAL_TEXTURE, true);
    }
    else if (self->mInventoryPickType == LLTextureCtrl::PICK_TEXTURE)
    {
        LLFilePickerReplyThread::startPicker(boost::bind(&onPickerCallback, _1, self->getHandle()), LLFilePicker::FFLOAD_IMAGE, true);
    }
    else if (self->mInventoryPickType == LLTextureCtrl::PICK_MATERIAL)
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
                LLSD data = self->mLocalScrollCtrl->getFirstSelected()->getValue();
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

		if (self->mSetImageAssetIDCallback)
		{
			self->mSetImageAssetIDCallback(inworld_id);
		}

		if (self->childGetValue("apply_immediate_check").asBoolean())
		{
			if (self->mOnFloaterCommitCallback)
			{
				self->mOnFloaterCommitCallback(LLTextureCtrl::TEXTURE_CHANGE, inworld_id);
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
}

void LLFloaterTexturePicker::setCanApply(bool can_preview, bool can_apply)
{
	getChildRef<LLUICtrl>("Select").setEnabled(can_apply);
	getChildRef<LLUICtrl>("preview_disabled").setVisible(!can_preview);
	getChildRef<LLUICtrl>("apply_immediate_check").setVisible(can_preview);

	mCanApply = can_apply;
	mCanPreview = can_preview ? (mCanApplyImmediately && gSavedSettings.getBOOL("TextureLivePreview")) : false;
	mPreviewSettingChanged = true;
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

		mSavedFolderState.setApply(TRUE);
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
			mSavedFolderState.setApply(FALSE);
			mInventoryPanel->getRootFolder()->applyFunctorRecursively(mSavedFolderState);
		}
	}

	mInventoryPanel->setFilterSubString(search_string);
}

void LLFloaterTexturePicker::refreshLocalList()
{
    mLocalScrollCtrl->clearRows();

    if (mInventoryPickType == LLTextureCtrl::PICK_TEXTURE_MATERIAL)
    {
        LLLocalBitmapMgr::getInstance()->feedScrollList(mLocalScrollCtrl);
        LLLocalGLTFMaterialMgr::getInstance()->feedScrollList(mLocalScrollCtrl);
    }
    else if (mInventoryPickType == LLTextureCtrl::PICK_TEXTURE)
    {
        LLLocalBitmapMgr::getInstance()->feedScrollList(mLocalScrollCtrl);
    }
    else if (mInventoryPickType == LLTextureCtrl::PICK_MATERIAL)
    {
        LLLocalGLTFMaterialMgr::getInstance()->feedScrollList(mLocalScrollCtrl);
    }
}

void LLFloaterTexturePicker::refreshInventoryFilter()
{
    U32 filter_types = 0x0;

    if (mInventoryPickType == LLTextureCtrl::PICK_TEXTURE_MATERIAL)
    {
        filter_types |= 0x1 << LLInventoryType::IT_TEXTURE;
        filter_types |= 0x1 << LLInventoryType::IT_SNAPSHOT;
        filter_types |= 0x1 << LLInventoryType::IT_MATERIAL;
    }
    else if (mInventoryPickType == LLTextureCtrl::PICK_TEXTURE)
    {
        filter_types |= 0x1 << LLInventoryType::IT_TEXTURE;
        filter_types |= 0x1 << LLInventoryType::IT_SNAPSHOT;
    }
    else if (mInventoryPickType == LLTextureCtrl::PICK_MATERIAL)
    {
        filter_types |= 0x1 << LLInventoryType::IT_MATERIAL;
    }

    mInventoryPanel->setFilterTypes(filter_types);
}

void LLFloaterTexturePicker::setLocalTextureEnabled(BOOL enabled)
{
    mModeSelector->setEnabledByValue(1, enabled);
}

void LLFloaterTexturePicker::setBakeTextureEnabled(BOOL enabled)
{
	BOOL changed = (enabled != mBakeTextureEnabled);

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

void LLFloaterTexturePicker::setInventoryPickType(LLTextureCtrl::EPickInventoryType type)
{
    mInventoryPickType = type;
    refreshLocalList();
    refreshInventoryFilter();

    if (mInventoryPickType == LLTextureCtrl::PICK_MATERIAL)
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
    else if(mInventoryPickType == LLTextureCtrl::PICK_MATERIAL)
    {
        setTitle(getString("pick_material"));
    }
    else
    {
        setTitle(getString("pick_texture"));
    }
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

        if (self->mInventoryPickType == LLTextureCtrl::PICK_TEXTURE_MATERIAL)
        {
            LLLocalBitmapMgr::getInstance()->feedScrollList(self->mLocalScrollCtrl);
            LLLocalGLTFMaterialMgr::getInstance()->feedScrollList(self->mLocalScrollCtrl);
        }
        else if (self->mInventoryPickType == LLTextureCtrl::PICK_TEXTURE)
        {
            LLLocalBitmapMgr::getInstance()->feedScrollList(self->mLocalScrollCtrl);
        }
        else if (self->mInventoryPickType == LLTextureCtrl::PICK_MATERIAL)
        {
            LLLocalGLTFMaterialMgr::getInstance()->feedScrollList(self->mLocalScrollCtrl);
        }
    }
}

void LLFloaterTexturePicker::onTextureSelect( const LLTextureEntry& te )
{
	LLUUID inventory_item_id = findItemID(te.getID(), TRUE);
	if (inventory_item_id.notNull())
	{
		LLToolPipette::getInstance()->setResult(TRUE, "");
        if (mInventoryPickType == LLTextureCtrl::PICK_MATERIAL)
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

		mNoCopyTextureSelected = FALSE;
		LLInventoryItem* itemp = gInventory.getItem(inventory_item_id);

		if (itemp && !itemp->getPermissions().allowCopyBy(gAgent.getID()))
		{
			// no copy texture
			mNoCopyTextureSelected = TRUE;
		}
		
		commitIfImmediateSet();
	}
	else
	{
		LLToolPipette::getInstance()->setResult(FALSE, LLTrans::getString("InventoryNoTexture"));
	}
}

///////////////////////////////////////////////////////////////////////
// LLTextureCtrl

static LLDefaultChildRegistry::Register<LLTextureCtrl> r("texture_picker");

LLTextureCtrl::LLTextureCtrl(const LLTextureCtrl::Params& p)
:	LLUICtrl(p),
	mDragCallback(NULL),
	mDropCallback(NULL),
	mOnCancelCallback(NULL),
	mOnCloseCallback(NULL),
	mOnSelectCallback(NULL),
	mBorderColor( p.border_color() ),
	mAllowNoTexture( p.allow_no_texture ),
	mAllowLocalTexture( TRUE ),
	mImmediateFilterPermMask( PERM_NONE ),
	mCanApplyImmediately( FALSE ),
	mNeedsRawImageData( FALSE ),
	mValid( TRUE ),
	mShowLoadingPlaceholder( TRUE ),
	mOpenTexPreview(false),
    mBakeTextureEnabled(true),
    mInventoryPickType(PICK_TEXTURE),
	mImageAssetID(p.image_id),
	mDefaultImageAssetID(p.default_image_id),
	mDefaultImageName(p.default_image_name),
	mFallbackImage(p.fallback_image)
{

	// Default of defaults is white image for diff tex
	//
	LLUUID whiteImage( gSavedSettings.getString( "UIImgWhiteUUID" ) );
	setBlankImageAssetID( whiteImage );

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

void LLTextureCtrl::setShowLoadingPlaceholder(BOOL showLoadingPlaceholder)
{
	mShowLoadingPlaceholder = showLoadingPlaceholder;
}

void LLTextureCtrl::setCaption(const std::string& caption)
{
	mCaption->setText( caption );
}

void LLTextureCtrl::setCanApplyImmediately(BOOL b)
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

void LLTextureCtrl::setVisible( BOOL visible ) 
{
	if( !visible )
	{
		closeDependentFloater();
	}
	LLUICtrl::setVisible( visible );
}

void LLTextureCtrl::setEnabled( BOOL enabled )
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

void LLTextureCtrl::setValid(BOOL valid )
{
	mValid = valid;
	if (!valid)
	{
		LLFloaterTexturePicker* pickerp = (LLFloaterTexturePicker*)mFloaterHandle.get();
		if (pickerp)
		{
			pickerp->setActive(FALSE);
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

void LLTextureCtrl::showPicker(BOOL take_focus)
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
			mFallbackImage);
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
			texture_floaterp->setOnFloaterCommitCallback(boost::bind(&LLTextureCtrl::onFloaterCommit, this, _1, _2));
			texture_floaterp->setSetImageAssetIDCallback(boost::bind(&LLTextureCtrl::setImageAssetID, this, _1));

			texture_floaterp->setBakeTextureEnabled(mBakeTextureEnabled);
            texture_floaterp->setInventoryPickType(mInventoryPickType);
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
		floaterp->setFocus(TRUE);
	}
}


void LLTextureCtrl::closeDependentFloater()
{
	LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
	if( floaterp && floaterp->isInVisibleChain())
	{
		floaterp->setOwner(NULL);
		floaterp->setVisible(FALSE);
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

BOOL LLTextureCtrl::handleHover(S32 x, S32 y, MASK mask)
{
	getWindow()->setCursor(mBorder->parentPointInView(x,y) ? UI_CURSOR_HAND : UI_CURSOR_ARROW);
	return TRUE;
}


BOOL LLTextureCtrl::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLUICtrl::handleMouseDown( x, y , mask );

	if (!handled && mBorder->parentPointInView(x, y))
	{
		if (!mOpenTexPreview)
		{
			showPicker(FALSE);
            if (mInventoryPickType == LLTextureCtrl::PICK_MATERIAL)
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
			LLInventoryModelBackgroundFetch::instance().start();
			handled = TRUE;
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

void LLTextureCtrl::onFloaterCommit(ETexturePickOp op, LLUUID id)
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
			
		if(floaterp->isDirty() || id.notNull()) // mModelView->setDirty does not work.
		{
			setTentative( FALSE );

			if (id.notNull())
			{
				mImageItemID = id;
				mImageAssetID = id;
			}
			else
			{
			    mImageItemID = floaterp->findItemID(floaterp->getAssetID(), FALSE);
			    LL_DEBUGS() << "mImageItemID: " << mImageItemID << LL_ENDL;
			    mImageAssetID = floaterp->getAssetID();
			    LL_DEBUGS() << "mImageAssetID: " << mImageAssetID << LL_ENDL;
			}

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

void	LLTextureCtrl::setImageAssetName(const std::string& name)
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
		floaterp->setBakeTextureEnabled(enabled);
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

BOOL LLTextureCtrl::handleDragAndDrop(S32 x, S32 y, MASK mask,
					  BOOL drop, EDragAndDropType cargo_type, void *cargo_data,
					  EAcceptance *accept,
					  std::string& tooltip_msg)
{
	BOOL handled = FALSE;

	// this downcast may be invalid - but if the second test below
	// returns true, then the cast was valid, and we can perform
	// the third test without problems.
	LLInventoryItem* item = (LLInventoryItem*)cargo_data; 

    bool is_mesh = cargo_type == DAD_MESH;
    bool is_texture = cargo_type == DAD_TEXTURE;
    bool is_material = cargo_type == DAD_MATERIAL;

    bool allow_dnd = false;
    if (mInventoryPickType == LLTextureCtrl::PICK_MATERIAL)
    {
        allow_dnd = is_material;
    }
    else if (mInventoryPickType == LLTextureCtrl::PICK_TEXTURE)
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
				setTentative( FALSE ); 
				onCommit();
			}
		}

		*accept = ACCEPT_YES_SINGLE;
	}
	else
	{
		*accept = ACCEPT_NO;
	}

	handled = TRUE;
	LL_DEBUGS("UserInput") << "dragAndDrop handled by LLTextureCtrl " << getName() << LL_ENDL;

	return handled;
}

void LLTextureCtrl::draw()
{
	mBorder->setKeyboardFocusHighlight(hasFocus());

	if (!mValid)
	{
		mTexturep = NULL;
	}
	else if (!mImageAssetID.isNull())
	{
		LLPointer<LLViewerFetchedTexture> texture = NULL;

		if (LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::isBakedImageId(mImageAssetID))
		{
			LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
			if (obj)
			{
				LLViewerTexture* viewerTexture = obj->getBakedTextureForMagicId(mImageAssetID);
				texture = viewerTexture ? dynamic_cast<LLViewerFetchedTexture*>(viewerTexture) : NULL;
			}
			
		}

		if (texture.isNull())
		{
			texture = LLViewerTextureManager::getFetchedTexture(mImageAssetID, FTT_DEFAULT, MIPMAP_YES, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
		}
		
		texture->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
		texture->forceToSaveRawImage(0) ;

		mTexturep = texture;
	}
	else//mImageAssetID == LLUUID::null
	{
		mTexturep = NULL; 
	}
	
	// Border
	LLRect border( 0, getRect().getHeight(), getRect().getWidth(), BTN_HEIGHT_SMALL );
	gl_rect_2d( border, mBorderColor.get(), FALSE );

	// Interior
	LLRect interior = border;
	interior.stretch( -1 ); 

	// If we're in a focused floater, don't apply the floater's alpha to the texture (STORM-677).
	const F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
	if( mTexturep )
	{
		if( mTexturep->getComponents() == 4 )
		{
			gl_rect_2d_checkerboard( interior, alpha );
		}
		
		gl_draw_scaled_image( interior.mLeft, interior.mBottom, interior.getWidth(), interior.getHeight(), mTexturep, UI_VERTEX_COLOR % alpha);
		mTexturep->addTextureStats( (F32)(interior.getWidth() * interior.getHeight()) );
	}
	else if (!mFallbackImage.isNull())
	{
		mFallbackImage->draw(interior, UI_VERTEX_COLOR % alpha);
	}
	else
	{
		gl_rect_2d( interior, LLColor4::grey % alpha, TRUE );

		// Draw X
		gl_draw_x( interior, LLColor4::black );
	}

	mTentativeLabel->setVisible( getTentative() );

	// Show "Loading..." string on the top left corner while this texture is loading.
	// Using the discard level, do not show the string if the texture is almost but not 
	// fully loaded.
	if (mTexturep.notNull() &&
		(!mTexturep->isFullyLoaded()) &&
		(mShowLoadingPlaceholder == TRUE))
	{
		U32 v_offset = 25;
		LLFontGL* font = LLFontGL::getFontSansSerif();

		// Don't show as loaded if the texture is almost fully loaded (i.e. discard1) unless god
		if ((mTexturep->getDiscardLevel() > 1) || gAgent.isGodlike())
		{
			font->renderUTF8(
				mLoadingPlaceholderString, 
				0,
				llfloor(interior.mLeft+3), 
				llfloor(interior.mTop-v_offset),
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
			font->renderUTF8(tdesc, 0, llfloor(interior.mLeft+3), llfloor(interior.mTop-v_offset),
							 LLColor4::white, LLFontGL::LEFT, LLFontGL::BASELINE, LLFontGL::DROP_SHADOW);

			v_offset += 12;
			tdesc = llformat("  LVL: %d", mTexturep->getDiscardLevel());
			font->renderUTF8(tdesc, 0, llfloor(interior.mLeft+3), llfloor(interior.mTop-v_offset),
							 LLColor4::white, LLFontGL::LEFT, LLFontGL::BASELINE, LLFontGL::DROP_SHADOW);

			v_offset += 12;
			tdesc = llformat("  ID  : %s...", (mImageAssetID.asString().substr(0,7)).c_str());
			font->renderUTF8(tdesc, 0, llfloor(interior.mLeft+3), llfloor(interior.mTop-v_offset),
							 LLColor4::white, LLFontGL::LEFT, LLFontGL::BASELINE, LLFontGL::DROP_SHADOW);
		}
	}

	LLUICtrl::draw();
}

BOOL LLTextureCtrl::allowDrop(LLInventoryItem* item, EDragAndDropType cargo_type, std::string& tooltip_msg)
{
	BOOL copy = item->getPermissions().allowCopyBy(gAgent.getID());
	BOOL mod = item->getPermissions().allowModifyBy(gAgent.getID());
	BOOL xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
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
			return TRUE;
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
		return FALSE;
	}
}

BOOL LLTextureCtrl::doDrop(LLInventoryItem* item)
{
	// call the callback if it exists.
	if(mDropCallback)
	{
		// if it returns TRUE, we return TRUE, and therefore the
		// commit is called above.
		return mDropCallback(this, item);
	}

	// no callback installed, so just set the image ids and carry on.
	setImageAssetID( item->getAssetUUID() );
	mImageItemID = item->getUUID();
	return TRUE;
}

BOOL LLTextureCtrl::handleUnicodeCharHere(llwchar uni_char)
{
	if( ' ' == uni_char )
	{
		showPicker(TRUE);
		return TRUE;
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





