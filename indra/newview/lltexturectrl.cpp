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
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llbutton.h"
#include "lldraghandle.h"
#include "llfocusmgr.h"
#include "llviewertexture.h"
#include "llfolderview.h"
#include "llfoldervieweventlistener.h"
#include "llinventory.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "llfloaterinventory.h"
#include "lllineeditor.h"
#include "llui.h"
#include "llviewerinventory.h"
#include "llpermissions.h"
#include "llsaleinfo.h"
#include "llassetstorage.h"
#include "lltextbox.h"
#include "llresizehandle.h"
#include "llscrollcontainer.h"
#include "lltoolmgr.h"
#include "lltoolpipette.h"
#include "llfiltereditor.h"

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

static const S32 HPAD = 4;
static const S32 VPAD = 4;
static const S32 LINE = 16;
static const S32 FOOTER_HEIGHT = 100;
static const S32 BORDER_PAD = HPAD;
static const S32 TEXTURE_INVENTORY_PADDING = 30;
static const F32 CONTEXT_CONE_IN_ALPHA = 0.0f;
static const F32 CONTEXT_CONE_OUT_ALPHA = 1.f;
static const F32 CONTEXT_FADE_TIME = 0.08f;

static const S32 LOCAL_TRACKING_ID_COLUMN = 1;

//static const char CURRENT_IMAGE_NAME[] = "Current Texture";
//static const char WHITE_IMAGE_NAME[] = "Blank Texture";
//static const char NO_IMAGE_NAME[] = "None";

//////////////////////////////////////////////////////////////////////////////////////////
// LLFloaterTexturePicker

class LLFloaterTexturePicker : public LLFloater
{
public:
	LLFloaterTexturePicker(
		LLTextureCtrl* owner,
		const std::string& label,
		PermissionMask immediate_filter_perm_mask,
		PermissionMask dnd_filter_perm_mask,
		PermissionMask non_immediate_filter_perm_mask,
		BOOL can_apply_immediately,
		LLUIImagePtr fallback_image_name);

	virtual ~LLFloaterTexturePicker();

	// LLView overrides
	/*virtual*/ BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask,
						BOOL drop, EDragAndDropType cargo_type, void *cargo_data, 
						EAcceptance *accept,
						std::string& tooltip_msg);
	/*virtual*/ void	draw();
	/*virtual*/ BOOL	handleKeyHere(KEY key, MASK mask);

	// LLFloater overrides
	/*virtual*/ BOOL    postBuild();
	/*virtual*/ void	onClose(bool app_settings);
	
	// New functions
	void setImageID( const LLUUID& image_asset_id);
	void updateImageStats();
	const LLUUID& getAssetID() { return mImageAssetID; }
	const LLUUID& findItemID(const LLUUID& asset_id, BOOL copyable_only);
	void			setCanApplyImmediately(BOOL b);

	void			setActive( BOOL active );

	LLTextureCtrl*	getOwner() const { return mOwner; }
	void			setOwner(LLTextureCtrl* owner) { mOwner = owner; }
	
	void			stopUsingPipette();
	PermissionMask 	getFilterPermMask();
	void updateFilterPermMask();
	void commitIfImmediateSet();
	
	void onFilterEdit(const std::string& search_string );
	
	void setCanApply(bool can_preview, bool can_apply);
	void setTextureSelectedCallback(texture_selected_callback cb) {mTextureSelectedCallback = cb;}

	static void		onBtnSetToDefault( void* userdata );
	static void		onBtnSelect( void* userdata );
	static void		onBtnCancel( void* userdata );
		   void		onBtnPipette( );
	//static void		onBtnRevert( void* userdata );
	static void		onBtnWhite( void* userdata );
	static void		onBtnNone( void* userdata );
	static void		onBtnClear( void* userdata );
		   void		onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action);
	static void		onShowFolders(LLUICtrl* ctrl, void* userdata);
	static void		onApplyImmediateCheck(LLUICtrl* ctrl, void* userdata);
		   void		onTextureSelect( const LLTextureEntry& te );

	static void		onModeSelect(LLUICtrl* ctrl, void *userdata);
	static void		onBtnAdd(void* userdata);
	static void		onBtnRemove(void* userdata);
	static void		onBtnUpload(void* userdata);
	static void		onLocalScrollCommit(LLUICtrl* ctrl, void* userdata);

protected:
	LLPointer<LLViewerTexture> mTexturep;
	LLTextureCtrl*		mOwner;

	LLUUID				mImageAssetID; // Currently selected texture
	LLUIImagePtr		mFallbackImage; // What to show if currently selected texture is null.

	LLUUID				mWhiteImageAssetID;
	LLUUID				mSpecialCurrentImageAssetID;  // Used when the asset id has no corresponding texture in the user's inventory.
	LLUUID				mOriginalImageAssetID;

	std::string			mLabel;

	LLTextBox*			mTentativeLabel;
	LLTextBox*			mResolutionLabel;

	std::string			mPendingName;
	BOOL				mActive;

	LLFilterEditor*		mFilterEdit;
	LLInventoryPanel*	mInventoryPanel;
	PermissionMask		mImmediateFilterPermMask;
	PermissionMask		mDnDFilterPermMask;
	PermissionMask		mNonImmediateFilterPermMask;
	BOOL				mCanApplyImmediately;
	BOOL				mNoCopyTextureSelected;
	F32					mContextConeOpacity;
	LLSaveFolderState	mSavedFolderState;
	BOOL				mSelectedItemPinned;
	
	LLRadioGroup*		mModeSelector;
	LLScrollListCtrl*	mLocalScrollCtrl;

private:
	bool mCanApply;
	bool mCanPreview;
	texture_selected_callback mTextureSelectedCallback;
};

LLFloaterTexturePicker::LLFloaterTexturePicker(	
	LLTextureCtrl* owner,
	const std::string& label,
	PermissionMask immediate_filter_perm_mask,
	PermissionMask dnd_filter_perm_mask,
	PermissionMask non_immediate_filter_perm_mask,
	BOOL can_apply_immediately,
	LLUIImagePtr fallback_image)
:	LLFloater(LLSD()),
	mOwner( owner ),
	mImageAssetID( owner->getImageAssetID() ),
	mFallbackImage( fallback_image ),
	mWhiteImageAssetID( gSavedSettings.getString( "UIImgWhiteUUID" ) ),
	mOriginalImageAssetID(owner->getImageAssetID()),
	mLabel(label),
	mTentativeLabel(NULL),
	mResolutionLabel(NULL),
	mActive( TRUE ),
	mFilterEdit(NULL),
	mImmediateFilterPermMask(immediate_filter_perm_mask),
	mDnDFilterPermMask(dnd_filter_perm_mask),
	mNonImmediateFilterPermMask(non_immediate_filter_perm_mask),
	mContextConeOpacity(0.f),
	mSelectedItemPinned( FALSE ),
	mCanApply(true),
	mCanPreview(true)
{
	buildFromFile("floater_texture_ctrl.xml");
	mCanApplyImmediately = can_apply_immediately;
	setCanMinimize(FALSE);
}

LLFloaterTexturePicker::~LLFloaterTexturePicker()
{
}

void LLFloaterTexturePicker::setImageID(const LLUUID& image_id)
{
	if( mImageAssetID != image_id && mActive)
	{
		mNoCopyTextureSelected = FALSE;
		mViewModel->setDirty(); // *TODO: shouldn't we be using setValue() here?
		mImageAssetID = image_id; 
		LLUUID item_id = findItemID(mImageAssetID, FALSE);
		if (item_id.isNull())
		{
			mInventoryPanel->clearSelection();
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
			mInventoryPanel->setSelection(item_id, TAKE_FOCUS_NO);
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
	if (!mCanApplyImmediately)
	{
		getChild<LLUICtrl>("apply_immediate_check")->setValue(FALSE);
	}
	updateFilterPermMask();
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
	if (mTexturep.notNull())
	{
		//RN: have we received header data for this image?
		if (mTexturep->getFullWidth() > 0 && mTexturep->getFullHeight() > 0)
		{
			std::string formatted_dims = llformat("%d x %d", mTexturep->getFullWidth(),mTexturep->getFullHeight());
			mResolutionLabel->setTextArg("[DIMENSIONS]", formatted_dims);
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

	if ((cargo_type == DAD_TEXTURE) || is_mesh)
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
		
		//PermissionMask filter_perm_mask = getFilterPermMask();  Commented out due to no-copy texture loss.
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
	lldebugst(LLERR_USER_INPUT) << "dragAndDrop handled by LLFloaterTexturePicker " << getName() << llendl;

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
				LLFolderViewItem* itemp = root_folder->getItemByID(gInventory.getRootFolderID());
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
	if (mOwner)
	{
		mOwner->onFloaterClose();
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

	mResolutionLabel = getChild<LLTextBox>("unknown");


	childSetAction("Default",LLFloaterTexturePicker::onBtnSetToDefault,this);
	childSetAction("None", LLFloaterTexturePicker::onBtnNone,this);
	childSetAction("Blank", LLFloaterTexturePicker::onBtnWhite,this);


	childSetCommitCallback("show_folders_check", onShowFolders, this);
	getChildView("show_folders_check")->setVisible( FALSE);

	mFilterEdit = getChild<LLFilterEditor>("inventory search editor");
	mFilterEdit->setCommitCallback(boost::bind(&LLFloaterTexturePicker::onFilterEdit, this, _2));

	mInventoryPanel = getChild<LLInventoryPanel>("inventory panel");

	if(mInventoryPanel)
	{
		U32 filter_types = 0x0;
		filter_types |= 0x1 << LLInventoryType::IT_TEXTURE;
		filter_types |= 0x1 << LLInventoryType::IT_SNAPSHOT;

		mInventoryPanel->setFilterTypes(filter_types);
		//mInventoryPanel->setFilterPermMask(getFilterPermMask());  //Commented out due to no-copy texture loss.
		mInventoryPanel->setFilterPermMask(mImmediateFilterPermMask);
		mInventoryPanel->setSelectCallback(boost::bind(&LLFloaterTexturePicker::onSelectionChange, this, _1, _2));
		mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);

		// Disable auto selecting first filtered item because it takes away
		// selection from the item set by LLTextureCtrl owning this floater.
		mInventoryPanel->getRootFolder()->setAutoSelectOverride(TRUE);

		// Commented out to scroll to currently selected texture. See EXT-5403.
		// // store this filter as the default one
		// mInventoryPanel->getRootFolder()->getFilter()->markDefault();

		// Commented out to stop opening all folders with textures
		// mInventoryPanel->openDefaultFolderForType(LLFolderType::FT_TEXTURE);

		// don't put keyboard focus on selected item, because the selection callback
		// will assume that this was user input
		mInventoryPanel->setSelection(findItemID(mImageAssetID, FALSE), TAKE_FOCUS_NO);
	}

	mModeSelector = getChild<LLRadioGroup>("mode_selection");
	mModeSelector->setCommitCallback(onModeSelect, this);
	mModeSelector->setSelectedIndex(0, 0);

	childSetAction("l_add_btn", LLFloaterTexturePicker::onBtnAdd, this);
	childSetAction("l_rem_btn", LLFloaterTexturePicker::onBtnRemove, this);
	childSetAction("l_upl_btn", LLFloaterTexturePicker::onBtnUpload, this);

	mLocalScrollCtrl = getChild<LLScrollListCtrl>("l_name_list");
	mLocalScrollCtrl->setCommitCallback(onLocalScrollCommit, this);
	LLLocalBitmapMgr::feedScrollList(mLocalScrollCtrl);

	mNoCopyTextureSelected = FALSE;

	getChild<LLUICtrl>("apply_immediate_check")->setValue(gSavedSettings.getBOOL("TextureLivePreview"));
	childSetCommitCallback("apply_immediate_check", onApplyImmediateCheck, this);

	if (!mCanApplyImmediately)
	{
		getChildView("show_folders_check")->setEnabled(FALSE);
	}

	getChild<LLUICtrl>("Pipette")->setCommitCallback( boost::bind(&LLFloaterTexturePicker::onBtnPipette, this));
	childSetAction("Cancel", LLFloaterTexturePicker::onBtnCancel,this);
	childSetAction("Select", LLFloaterTexturePicker::onBtnSelect,this);

	// update permission filter once UI is fully initialized
	updateFilterPermMask();
	mSavedFolderState.setApply(FALSE);

	LLToolPipette::getInstance()->setToolSelectCallback(boost::bind(&LLFloaterTexturePicker::onTextureSelect, this, _1));
	
	return TRUE;
}

// virtual
void LLFloaterTexturePicker::draw()
{
	if (mOwner)
	{
		// draw cone of context pointing back to texture swatch	
		LLRect owner_rect;
		mOwner->localRectToOtherView(mOwner->getLocalRect(), &owner_rect, this);
		LLRect local_rect = getLocalRect();
		if (gFocusMgr.childHasKeyboardFocus(this) && mOwner->isInVisibleChain() && mContextConeOpacity > 0.001f)
		{
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			LLGLEnable(GL_CULL_FACE);
			gGL.begin(LLRender::QUADS);
			{
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
				gGL.vertex2i(owner_rect.mLeft, owner_rect.mTop);
				gGL.vertex2i(owner_rect.mRight, owner_rect.mTop);
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
				gGL.vertex2i(local_rect.mRight, local_rect.mTop);
				gGL.vertex2i(local_rect.mLeft, local_rect.mTop);

				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
				gGL.vertex2i(local_rect.mLeft, local_rect.mTop);
				gGL.vertex2i(local_rect.mLeft, local_rect.mBottom);
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
				gGL.vertex2i(owner_rect.mLeft, owner_rect.mBottom);
				gGL.vertex2i(owner_rect.mLeft, owner_rect.mTop);

				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
				gGL.vertex2i(local_rect.mRight, local_rect.mBottom);
				gGL.vertex2i(local_rect.mRight, local_rect.mTop);
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
				gGL.vertex2i(owner_rect.mRight, owner_rect.mTop);
				gGL.vertex2i(owner_rect.mRight, owner_rect.mBottom);


				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
				gGL.vertex2i(local_rect.mLeft, local_rect.mBottom);
				gGL.vertex2i(local_rect.mRight, local_rect.mBottom);
				gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
				gGL.vertex2i(owner_rect.mRight, owner_rect.mBottom);
				gGL.vertex2i(owner_rect.mLeft, owner_rect.mBottom);
			}
			gGL.end();
		}
	}

	if (gFocusMgr.childHasMouseCapture(getDragHandle()))
	{
		mContextConeOpacity = lerp(mContextConeOpacity, gSavedSettings.getF32("PickerContextOpacity"), LLCriticalDamp::getInterpolant(CONTEXT_FADE_TIME));
	}
	else
	{
		mContextConeOpacity = lerp(mContextConeOpacity, 0.f, LLCriticalDamp::getInterpolant(CONTEXT_FADE_TIME));
	}

	updateImageStats();

	// if we're inactive, gray out "apply immediate" checkbox
	getChildView("show_folders_check")->setEnabled(mActive && mCanApplyImmediately && !mNoCopyTextureSelected);
	getChildView("Select")->setEnabled(mActive && mCanApply);
	getChildView("Pipette")->setEnabled(mActive);
	getChild<LLUICtrl>("Pipette")->setValue(LLToolMgr::getInstance()->getCurrentTool() == LLToolPipette::getInstance());

	//BOOL allow_copy = FALSE;
	if( mOwner ) 
	{
		mTexturep = NULL;
		if(mImageAssetID.notNull())
		{
			mTexturep = LLViewerTextureManager::getFetchedTexture(mImageAssetID, MIPMAP_YES);
			mTexturep->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
		}

		if (mTentativeLabel)
		{
			mTentativeLabel->setVisible( FALSE  );
		}

		getChildView("Default")->setEnabled(mImageAssetID != mOwner->getDefaultImageAssetID());
		getChildView("Blank")->setEnabled(mImageAssetID != mWhiteImageAssetID );
		getChildView("None")->setEnabled(mOwner->getAllowNoTexture() && !mImageAssetID.isNull() );

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
		if( mOwner->getTentative() && !mViewModel->isDirty() )
		{
			mTentativeLabel->setVisible( TRUE );
			drawChild(mTentativeLabel);
		}

		if (mSelectedItemPinned) return;

		LLFolderView* folder_view = mInventoryPanel->getRootFolder();
		if (!folder_view) return;

		LLInventoryFilter* filter = folder_view->getFilter();
		if (!filter) return;

		bool is_filter_active = folder_view->getCompletedFilterGeneration() < filter->getCurrentGeneration() &&
				filter->isNotDefault();

		// After inventory panel filter is applied we have to update
		// constraint rect for the selected item because of folder view
		// AutoSelectOverride set to TRUE. We force PinningSelectedItem
		// flag to FALSE state and setting filter "dirty" to update
		// scroll container to show selected item (see LLFolderView::doIdle()).
		if (!is_filter_active && !mSelectedItemPinned)
		{
			folder_view->setPinningSelectedItem(mSelectedItemPinned);
			folder_view->dirtyFilter();
			folder_view->arrangeFromRoot();

			mSelectedItemPinned = TRUE;
		}
	}
}

// static
/*
void LLFloaterTexturePicker::onSaveAnotherCopyDialog( S32 option, void* userdata )
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	if( 0 == option )
	{
		self->copyToInventoryFinal();
	}
}
*/

const LLUUID& LLFloaterTexturePicker::findItemID(const LLUUID& asset_id, BOOL copyable_only)
{
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLAssetIDMatches asset_id_matches(asset_id);
	gInventory.collectDescendentsIf(LLUUID::null,
							cats,
							items,
							LLInventoryModel::INCLUDE_TRASH,
							asset_id_matches);

	if (items.count())
	{
		// search for copyable version first
		for (S32 i = 0; i < items.count(); i++)
		{
			LLInventoryItem* itemp = items[i];
			LLPermissions item_permissions = itemp->getPermissions();
			if (item_permissions.allowCopyBy(gAgent.getID(), gAgent.getGroupID()))
			{
				return itemp->getUUID();
			}
		}
		// otherwise just return first instance, unless copyable requested
		if (copyable_only)
		{
			return LLUUID::null;
		}
		else
		{
			return items[0]->getUUID();
		}
	}

	return LLUUID::null;
}

PermissionMask LLFloaterTexturePicker::getFilterPermMask()
{
	bool apply_immediate = getChild<LLUICtrl>("apply_immediate_check")->getValue().asBoolean();
	return apply_immediate ? mImmediateFilterPermMask : mNonImmediateFilterPermMask;
}

void LLFloaterTexturePicker::commitIfImmediateSet()
{
	if (!mNoCopyTextureSelected && mOwner && mCanApply)
	{
		mOwner->onFloaterCommit(LLTextureCtrl::TEXTURE_CHANGE);
	}
}

// static
void LLFloaterTexturePicker::onBtnSetToDefault(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	self->setCanApply(true, true);
	if (self->mOwner)
	{
		self->setImageID( self->mOwner->getDefaultImageAssetID() );
	}
	self->commitIfImmediateSet();
}

// static
void LLFloaterTexturePicker::onBtnWhite(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	self->setCanApply(true, true);
	self->setImageID( self->mWhiteImageAssetID );
	self->commitIfImmediateSet();
}


// static
void LLFloaterTexturePicker::onBtnNone(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
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
	if (self->mOwner)
	{
		self->mOwner->onFloaterCommit(LLTextureCtrl::TEXTURE_CANCEL);
	}
	self->mViewModel->resetDirty();
	self->closeFloater();
}

// static
void LLFloaterTexturePicker::onBtnSelect(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	if (self->mOwner)
	{
		LLUUID local_id = LLUUID::null;

		if (self->mLocalScrollCtrl->getVisible() && !self->mLocalScrollCtrl->getAllSelected().empty())
		{
			LLUUID temp_id = self->mLocalScrollCtrl->getFirstSelected()->getColumn(LOCAL_TRACKING_ID_COLUMN)->getValue().asUUID();
			local_id = LLLocalBitmapMgr::getWorldID(temp_id);
		}

		self->mOwner->onFloaterCommit(LLTextureCtrl::TEXTURE_SELECT, local_id);
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
		LLInventoryItem* itemp = gInventory.getItem(first_item->getListener()->getUUID());
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
			mImageAssetID = itemp->getAssetUUID();
			mViewModel->setDirty(); // *TODO: shouldn't we be using setValue() here?
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
	bool mode = (self->mModeSelector->getSelectedIndex() == 0);

	self->getChild<LLButton>("Default")->setVisible(mode);
	self->getChild<LLButton>("Blank")->setVisible(mode);
	self->getChild<LLButton>("None")->setVisible(mode);
	self->getChild<LLButton>("Pipette")->setVisible(mode);
	self->getChild<LLFilterEditor>("inventory search editor")->setVisible(mode);
	self->getChild<LLInventoryPanel>("inventory panel")->setVisible(mode);

	/*self->getChild<LLCheckBox>("show_folders_check")->setVisible(mode);
	  no idea under which conditions the above is even shown, needs testing. */

	self->getChild<LLButton>("l_add_btn")->setVisible(!mode);
	self->getChild<LLButton>("l_rem_btn")->setVisible(!mode);
	self->getChild<LLButton>("l_upl_btn")->setVisible(!mode);
	self->getChild<LLScrollListCtrl>("l_name_list")->setVisible(!mode);
}

// static
void LLFloaterTexturePicker::onBtnAdd(void* userdata)
{
	if (LLLocalBitmapMgr::addUnit() == true)
	{
		LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
		LLLocalBitmapMgr::feedScrollList(self->mLocalScrollCtrl);
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
				LLUUID tracking_id = list_item->getColumn(LOCAL_TRACKING_ID_COLUMN)->getValue().asUUID();
				LLLocalBitmapMgr::delUnit(tracking_id);
			}
		}

		self->getChild<LLButton>("l_rem_btn")->setEnabled(false);
		self->getChild<LLButton>("l_upl_btn")->setEnabled(false);
		LLLocalBitmapMgr::feedScrollList(self->mLocalScrollCtrl);
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
	
	LLUUID tracking_id = (LLUUID)self->mLocalScrollCtrl->getSelectedItemLabel(LOCAL_TRACKING_ID_COLUMN);
	std::string filename = LLLocalBitmapMgr::getFilename(tracking_id);

	if (!filename.empty())
	{
		LLFloaterReg::showInstance("upload_image", LLSD(filename));
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
		LLUUID tracking_id = (LLUUID)self->mLocalScrollCtrl->getSelectedItemLabel(LOCAL_TRACKING_ID_COLUMN); 
		LLUUID inworld_id = LLLocalBitmapMgr::getWorldID(tracking_id);
		self->mOwner->setImageAssetID(inworld_id);

		if (self->childGetValue("apply_immediate_check").asBoolean())
		{
			self->mOwner->onFloaterCommit(LLTextureCtrl::TEXTURE_CHANGE, inworld_id);
		}
	}
}

// static
void LLFloaterTexturePicker::onShowFolders(LLUICtrl* ctrl, void *user_data)
{
	LLCheckBoxCtrl* check_box = (LLCheckBoxCtrl*)ctrl;
	LLFloaterTexturePicker* picker = (LLFloaterTexturePicker*)user_data;

	if (check_box->get())
	{
		picker->mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	}
	else
	{
		picker->mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NO_FOLDERS);
	}
}

// static
void LLFloaterTexturePicker::onApplyImmediateCheck(LLUICtrl* ctrl, void *user_data)
{
	LLFloaterTexturePicker* picker = (LLFloaterTexturePicker*)user_data;

	LLCheckBoxCtrl* check_box = (LLCheckBoxCtrl*)ctrl;
	gSavedSettings.setBOOL("TextureLivePreview", check_box->get());

	picker->updateFilterPermMask();
	picker->commitIfImmediateSet();
}

void LLFloaterTexturePicker::updateFilterPermMask()
{
	//mInventoryPanel->setFilterPermMask( getFilterPermMask() );  Commented out due to no-copy texture loss.
}

void LLFloaterTexturePicker::setCanApply(bool can_preview, bool can_apply)
{
	getChildRef<LLUICtrl>("Select").setEnabled(can_apply);
	getChildRef<LLUICtrl>("preview_disabled").setVisible(!can_preview);
	getChildRef<LLUICtrl>("apply_immediate_check").setVisible(can_preview);

	mCanApply = can_apply;
	mCanPreview = can_preview ? gSavedSettings.getBOOL("TextureLivePreview") : false;
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
		if (!mInventoryPanel->getRootFolder()->isFilterModified())
		{
			mSavedFolderState.setApply(FALSE);
			mInventoryPanel->getRootFolder()->applyFunctorRecursively(mSavedFolderState);
		}
	}

	mInventoryPanel->setFilterSubString(search_string);
}

void LLFloaterTexturePicker::onTextureSelect( const LLTextureEntry& te )
{
	LLUUID inventory_item_id = findItemID(te.getID(), TRUE);
	if (inventory_item_id.notNull())
	{
		LLToolPipette::getInstance()->setResult(TRUE, "");
		setImageID(te.getID());

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
	mOnSelectCallback(NULL),
	mBorderColor( p.border_color() ),
	mAllowNoTexture( FALSE ),
	mImmediateFilterPermMask( PERM_NONE ),
	mNonImmediateFilterPermMask( PERM_NONE ),
	mCanApplyImmediately( FALSE ),
	mNeedsRawImageData( FALSE ),
	mValid( TRUE ),
	mShowLoadingPlaceholder( TRUE ),
	mImageAssetID(p.image_id),
	mDefaultImageAssetID(p.default_image_id),
	mDefaultImageName(p.default_image_name),
	mFallbackImage(p.fallback_image)
{
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

	if( floaterp )
	{
		floaterp->setActive(enabled);
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
			mLabel,
			mImmediateFilterPermMask,
			mDnDFilterPermMask,
			mNonImmediateFilterPermMask,
			mCanApplyImmediately,
			mFallbackImage);

		mFloaterHandle = floaterp->getHandle();

		LLFloaterTexturePicker* texture_floaterp = dynamic_cast<LLFloaterTexturePicker*>(floaterp);
		if (texture_floaterp && mOnTextureSelectedCallback)
		{
			texture_floaterp->setTextureSelectedCallback(mOnTextureSelectedCallback);
		}

		LLFloater* root_floater = gFloaterView->getParentFloater(this);
		if (root_floater)
			root_floater->addDependentFloater(floaterp);
		floaterp->openFloater();
	}

	if (take_focus)
	{
		floaterp->setFocus(TRUE);
	}
}


void LLTextureCtrl::closeDependentFloater()
{
	LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
	if( floaterp )
	{
		floaterp->setOwner(NULL);
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
		showPicker(FALSE);
		//grab textures first...
		LLInventoryModelBackgroundFetch::instance().start(gInventory.findCategoryUUIDForType(LLFolderType::FT_TEXTURE));
		//...then start full inventory fetch.
		LLInventoryModelBackgroundFetch::instance().start();
		handled = TRUE;
	}

	return handled;
}

void LLTextureCtrl::onFloaterClose()
{
	LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();

	if (floaterp)
	{
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
				lldebugs << "mImageItemID: " << mImageItemID << llendl;
				mImageAssetID = floaterp->getAssetID();
				lldebugs << "mImageAssetID: " << mImageAssetID << llendl;
			}

			if (op == TEXTURE_SELECT && mOnSelectCallback)
			{
				mOnSelectCallback( this, LLSD() );
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
					onCommit();
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

	if (getEnabled() &&
		((cargo_type == DAD_TEXTURE) || is_mesh) &&
		 allowDrop(item))
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
	lldebugst(LLERR_USER_INPUT) << "dragAndDrop handled by LLTextureCtrl " << getName() << llendl;

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
		LLPointer<LLViewerFetchedTexture> texture = LLViewerTextureManager::getFetchedTexture(mImageAssetID, MIPMAP_YES,LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
		
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

	mTentativeLabel->setVisible( !mTexturep.isNull() && getTentative() );
	
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

BOOL LLTextureCtrl::allowDrop(LLInventoryItem* item)
{
	BOOL copy = item->getPermissions().allowCopyBy(gAgent.getID());
	BOOL mod = item->getPermissions().allowModifyBy(gAgent.getID());
	BOOL xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
														gAgent.getID());

	PermissionMask item_perm_mask = 0;
	if (copy) item_perm_mask |= PERM_COPY;
	if (mod)  item_perm_mask |= PERM_MODIFY;
	if (xfer) item_perm_mask |= PERM_TRANSFER;
	
//	PermissionMask filter_perm_mask = mCanApplyImmediately ?			commented out due to no-copy texture loss.
//			mImmediateFilterPermMask : mNonImmediateFilterPermMask;
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





