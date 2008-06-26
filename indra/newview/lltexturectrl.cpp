/** 
 * @file lltexturectrl.cpp
 * @author Richard Nelson, James Cook
 * @brief LLTextureCtrl class implementation including related functions
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llviewerprecompiledheaders.h"

#include "lltexturectrl.h"

#include "llrender.h"
#include "llagent.h"
#include "llviewerimagelist.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llbutton.h"
#include "lldraghandle.h"
#include "llfocusmgr.h"
#include "llviewerimage.h"
#include "llfolderview.h"
#include "llinventory.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
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

#include "lltool.h"
#include "llviewerwindow.h"
#include "llviewerobject.h"
#include "llviewercontrol.h"
#include "llglheaders.h"
#include "lluictrlfactory.h"
#include "lltrans.h"


static const S32 CLOSE_BTN_WIDTH = 100;
const S32 PIPETTE_BTN_WIDTH = 32;
static const S32 HPAD = 4;
static const S32 VPAD = 4;
static const S32 LINE = 16;
static const S32 SMALL_BTN_WIDTH = 64;
static const S32 TEX_PICKER_MIN_WIDTH = 
	(HPAD +
	CLOSE_BTN_WIDTH +
	HPAD +
	CLOSE_BTN_WIDTH +
	HPAD + 
	SMALL_BTN_WIDTH +
	HPAD +
	SMALL_BTN_WIDTH +
	HPAD + 
	30 +
	RESIZE_HANDLE_WIDTH * 2);
static const S32 CLEAR_BTN_WIDTH = 50;
static const S32 TEX_PICKER_MIN_HEIGHT = 290;
static const S32 FOOTER_HEIGHT = 100;
static const S32 BORDER_PAD = HPAD;
static const S32 TEXTURE_INVENTORY_PADDING = 30;
static const F32 CONTEXT_CONE_IN_ALPHA = 0.0f;
static const F32 CONTEXT_CONE_OUT_ALPHA = 1.f;
static const F32 CONTEXT_FADE_TIME = 0.08f;

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
		const LLRect& rect,
		const std::string& label,
		PermissionMask immediate_filter_perm_mask,
		PermissionMask non_immediate_filter_perm_mask,
		BOOL can_apply_immediately);
	virtual ~LLFloaterTexturePicker();

	// LLView overrides
	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask,
						BOOL drop, EDragAndDropType cargo_type, void *cargo_data, 
						EAcceptance *accept,
						std::string& tooltip_msg);
	virtual void	draw();
	virtual BOOL	handleKeyHere(KEY key, MASK mask);

	// LLFloater overrides
	virtual void	onClose(bool app_quitting);
	virtual BOOL    postBuild();
	
	// New functions
	void setImageID( const LLUUID& image_asset_id);
	void updateImageStats();
	const LLUUID& getAssetID() { return mImageAssetID; }
	const LLUUID& findItemID(const LLUUID& asset_id, BOOL copyable_only);
	void			setCanApplyImmediately(BOOL b);

	void			setDirty( BOOL b ) { mIsDirty = b; }
	BOOL			isDirty() const { return mIsDirty; }
	void			setActive( BOOL active );

	LLTextureCtrl*	getOwner() const { return mOwner; }
	void			setOwner(LLTextureCtrl* owner) { mOwner = owner; }
	
	void			stopUsingPipette();
	PermissionMask 	getFilterPermMask();
	void updateFilterPermMask();
	void commitIfImmediateSet();

	static void		onBtnSetToDefault( void* userdata );
	static void		onBtnSelect( void* userdata );
	static void		onBtnCancel( void* userdata );
	static void		onBtnPipette( void* userdata );
	//static void		onBtnRevert( void* userdata );
	static void		onBtnWhite( void* userdata );
	static void		onBtnNone( void* userdata );
	static void		onBtnClear( void* userdata );
	static void		onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action, void* data);
	static void		onShowFolders(LLUICtrl* ctrl, void* userdata);
	static void		onApplyImmediateCheck(LLUICtrl* ctrl, void* userdata);
	static void		onSearchEdit(const std::string& search_string, void* user_data );
	static void		onTextureSelect( const LLTextureEntry& te, void *data );

protected:
	LLPointer<LLViewerImage> mTexturep;
	LLTextureCtrl*		mOwner;

	LLUUID				mImageAssetID; // Currently selected texture

	LLUUID				mWhiteImageAssetID;
	LLUUID				mSpecialCurrentImageAssetID;  // Used when the asset id has no corresponding texture in the user's inventory.
	LLUUID				mOriginalImageAssetID;

	std::string         mLabel;

	LLTextBox*			mTentativeLabel;
	LLTextBox*			mResolutionLabel;

	std::string			mPendingName;
	BOOL				mIsDirty;
	BOOL				mActive;

	LLSearchEditor*		mSearchEdit;
	LLInventoryPanel*	mInventoryPanel;
	PermissionMask		mImmediateFilterPermMask;
	PermissionMask		mNonImmediateFilterPermMask;
	BOOL				mCanApplyImmediately;
	BOOL				mNoCopyTextureSelected;
	F32					mContextConeOpacity;
	LLSaveFolderState	mSavedFolderState;
};

LLFloaterTexturePicker::LLFloaterTexturePicker(	
	LLTextureCtrl* owner,
	const LLRect& rect,
	const std::string& label,
	PermissionMask immediate_filter_perm_mask,
	PermissionMask non_immediate_filter_perm_mask,
	BOOL can_apply_immediately)
	:
	LLFloater( std::string("texture picker"),
		rect,
		std::string( "Pick: " ) + label,
		TRUE,
		TEX_PICKER_MIN_WIDTH, TEX_PICKER_MIN_HEIGHT ),
	mOwner( owner ),
	mImageAssetID( owner->getImageAssetID() ),
	mWhiteImageAssetID( gSavedSettings.getString( "UIImgWhiteUUID" ) ),
	mOriginalImageAssetID(owner->getImageAssetID()),
	mLabel(label),
	mTentativeLabel(NULL),
	mResolutionLabel(NULL),
	mIsDirty( FALSE ),
	mActive( TRUE ),
	mSearchEdit(NULL),
	mImmediateFilterPermMask(immediate_filter_perm_mask),
	mNonImmediateFilterPermMask(non_immediate_filter_perm_mask),
	mContextConeOpacity(0.f)
{
	LLUICtrlFactory::getInstance()->buildFloater(this,"floater_texture_ctrl.xml");

	mTentativeLabel = getChild<LLTextBox>("Multiple");

	mResolutionLabel = getChild<LLTextBox>("unknown");


	childSetAction("Default",LLFloaterTexturePicker::onBtnSetToDefault,this);
	childSetAction("None", LLFloaterTexturePicker::onBtnNone,this);
	childSetAction("Blank", LLFloaterTexturePicker::onBtnWhite,this);

		
	childSetCommitCallback("show_folders_check", onShowFolders, this);
	childSetVisible("show_folders_check", FALSE);
	
	mSearchEdit = getChild<LLSearchEditor>("inventory search editor");
	mSearchEdit->setSearchCallback(onSearchEdit, this);
		
	mInventoryPanel = getChild<LLInventoryPanel>("inventory panel");

	if(mInventoryPanel)
	{
		U32 filter_types = 0x0;
		filter_types |= 0x1 << LLInventoryType::IT_TEXTURE;
		filter_types |= 0x1 << LLInventoryType::IT_SNAPSHOT;

		mInventoryPanel->setFilterTypes(filter_types);
		//mInventoryPanel->setFilterPermMask(getFilterPermMask());  //Commented out due to no-copy texture loss.
		mInventoryPanel->setFilterPermMask(immediate_filter_perm_mask);
		mInventoryPanel->setSelectCallback(onSelectionChange, this);
		mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
		mInventoryPanel->setAllowMultiSelect(FALSE);

		// store this filter as the default one
		mInventoryPanel->getRootFolder()->getFilter()->markDefault();

		// Commented out to stop opening all folders with textures
		// mInventoryPanel->openDefaultFolderForType(LLAssetType::AT_TEXTURE);
		
		// don't put keyboard focus on selected item, because the selection callback
		// will assume that this was user input
		mInventoryPanel->setSelection(findItemID(mImageAssetID, FALSE), TAKE_FOCUS_NO);
	}

	mCanApplyImmediately = can_apply_immediately;
	mNoCopyTextureSelected = FALSE;
		
	childSetValue("apply_immediate_check", gSavedSettings.getBOOL("ApplyTextureImmediately"));
	childSetCommitCallback("apply_immediate_check", onApplyImmediateCheck, this);
	
	if (!can_apply_immediately)
	{
		childSetEnabled("show_folders_check", FALSE);
	}

	childSetAction("Pipette", LLFloaterTexturePicker::onBtnPipette,this);
	childSetAction("Cancel", LLFloaterTexturePicker::onBtnCancel,this);
	childSetAction("Select", LLFloaterTexturePicker::onBtnSelect,this);

	// update permission filter once UI is fully initialized
	updateFilterPermMask();

	setCanMinimize(FALSE);

	mSavedFolderState.setApply(FALSE);
}

LLFloaterTexturePicker::~LLFloaterTexturePicker()
{
}

void LLFloaterTexturePicker::setImageID(const LLUUID& image_id)
{
	if( mImageAssetID != image_id && mActive)
	{
		mNoCopyTextureSelected = FALSE;
		mIsDirty = TRUE;
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
				childSetValue("apply_immediate_check", FALSE);
				mNoCopyTextureSelected = TRUE;
			}
			mInventoryPanel->setSelection(item_id, TAKE_FOCUS_NO);
		}
	}
}

void LLFloaterTexturePicker::setActive( BOOL active )					
{
	if (!active && childGetValue("Pipette").asBoolean())
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
		childSetValue("apply_immediate_check", FALSE);
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
		if (mTexturep->getWidth(0) > 0 && mTexturep->getHeight(0) > 0)
		{
			std::string formatted_dims = llformat("%d x %d", mTexturep->getWidth(0),mTexturep->getHeight(0));
			mResolutionLabel->setTextArg("[DIMENSIONS]", formatted_dims);
		}
		else
		{
			mResolutionLabel->setTextArg("[DIMENSIONS]", std::string("[? x ?]"));
		}
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

	if (cargo_type == DAD_TEXTURE)
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
		PermissionMask filter_perm_mask = mImmediateFilterPermMask;
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

	if (root_folder && mSearchEdit)
	{
		if (mSearchEdit->hasFocus() 
			&& (key == KEY_RETURN || key == KEY_DOWN) 
			&& mask == MASK_NONE)
		{
			if (!root_folder->getCurSelectedItem())
			{
				LLFolderViewItem* itemp = root_folder->getItemByID(gAgent.getInventoryRootID());
				if (itemp)
				{
					root_folder->setSelection(itemp, FALSE, FALSE);
				}
			}
			root_folder->scrollToShowSelection();
			
			// move focus to inventory proper
			root_folder->setFocus(TRUE);
			
			// treat this as a user selection of the first filtered result
			commitIfImmediateSet();
			
			return TRUE;
		}
		
		if (root_folder->hasFocus() && key == KEY_UP)
		{
			mSearchEdit->focusFirstItem(TRUE);
		}
	}

	return LLFloater::handleKeyHere(key, mask);
}

// virtual
void LLFloaterTexturePicker::onClose(bool app_quitting)
{
	if (mOwner)
	{
		mOwner->onFloaterClose();
	}
	stopUsingPipette();
	destroy();
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
			LLGLSNoTexture no_texture;
			LLGLEnable(GL_CULL_FACE);
			gGL.begin(LLVertexBuffer::QUADS);
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
	childSetEnabled("show_folders_check", mActive && mCanApplyImmediately && !mNoCopyTextureSelected);
	childSetEnabled("Select", mActive);
	childSetEnabled("Pipette", mActive);
	childSetValue("Pipette", LLToolMgr::getInstance()->getCurrentTool() == LLToolPipette::getInstance());

	//RN: reset search bar to reflect actual search query (all caps, for example)
	mSearchEdit->setText(mInventoryPanel->getFilterSubString());

	//BOOL allow_copy = FALSE;
	if( mOwner ) 
	{
		mTexturep = NULL;
		if(mImageAssetID.notNull())
		{
			mTexturep = gImageList.getImage(mImageAssetID, MIPMAP_YES, IMMEDIATE_NO);
			mTexturep->setBoostLevel(LLViewerImage::BOOST_PREVIEW);
		}

		if (mTentativeLabel)
		{
			mTentativeLabel->setVisible( FALSE  );
		}

		childSetEnabled("Default",  mImageAssetID != mOwner->getDefaultImageAssetID());
		childSetEnabled("Blank",   mImageAssetID != mWhiteImageAssetID );
		childSetEnabled("None", mOwner->getAllowNoTexture() && !mImageAssetID.isNull() );

		LLFloater::draw();

		if( isMinimized() )
		{
			return;
		}

		// Border
		LLRect border( BORDER_PAD, 
				getRect().getHeight() - LLFLOATER_HEADER_SIZE - BORDER_PAD, 
				((TEX_PICKER_MIN_WIDTH / 2) - TEXTURE_INVENTORY_PADDING - HPAD) - BORDER_PAD,
				BORDER_PAD + FOOTER_HEIGHT + (getRect().getHeight() - TEX_PICKER_MIN_HEIGHT));
		gl_rect_2d( border, LLColor4::black, FALSE );


		// Interior
		LLRect interior = border;
		interior.stretch( -1 ); 

		if( mTexturep )
		{
			if( mTexturep->getComponents() == 4 )
			{
				gl_rect_2d_checkerboard( interior );
			}

			gl_draw_scaled_image( interior.mLeft, interior.mBottom, interior.getWidth(), interior.getHeight(), mTexturep );

			// Pump the priority
			mTexturep->addTextureStats( (F32)(interior.getWidth() * interior.getHeight()) );

			// Draw Tentative Label over the image
			if( mOwner->getTentative() && !mIsDirty )
			{
				mTentativeLabel->setVisible( TRUE );
				drawChild(mTentativeLabel);
			}
		}
		else
		{
			gl_rect_2d( interior, LLColor4::grey, TRUE );

			// Draw X
			gl_draw_x(interior, LLColor4::black );
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
	bool apply_immediate = childGetValue("apply_immediate_check").asBoolean();
	return apply_immediate ? mImmediateFilterPermMask : mNonImmediateFilterPermMask;
}

void LLFloaterTexturePicker::commitIfImmediateSet()
{
	bool apply_immediate = childGetValue("apply_immediate_check").asBoolean();
	if (!mNoCopyTextureSelected && apply_immediate && mOwner)
	{
		mOwner->onFloaterCommit(LLTextureCtrl::TEXTURE_CHANGE);
	}
}

// static
void LLFloaterTexturePicker::onBtnSetToDefault(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
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
	self->mIsDirty = FALSE;
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
	self->mIsDirty = FALSE;
	self->close();
}

// static
void LLFloaterTexturePicker::onBtnSelect(void* userdata)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;
	if (self->mOwner)
	{
		self->mOwner->onFloaterCommit(LLTextureCtrl::TEXTURE_SELECT);
	}
	self->close();
}

// static
void LLFloaterTexturePicker::onBtnPipette( void* userdata )
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*) userdata;

	if ( self)
	{
		BOOL pipette_active = self->childGetValue("Pipette").asBoolean();
		pipette_active = !pipette_active;
		if (pipette_active)
		{
			LLToolPipette::getInstance()->setSelectCallback(onTextureSelect, self);
			LLToolMgr::getInstance()->setTransientTool(LLToolPipette::getInstance());
		}
		else
		{
			LLToolMgr::getInstance()->clearTransientTool();
		}
	}

}

// static 
void LLFloaterTexturePicker::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action, void* data)
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*)data;
	if (items.size())
	{
		LLFolderViewItem* first_item = items.front();
		LLInventoryItem* itemp = gInventory.getItem(first_item->getListener()->getUUID());
		self->mNoCopyTextureSelected = FALSE;
		if (itemp)
		{
			if (!itemp->getPermissions().allowCopyBy(gAgent.getID()))
			{
				self->mNoCopyTextureSelected = TRUE;
			}
			self->mImageAssetID = itemp->getAssetUUID();
			self->mIsDirty = TRUE;
			if (user_action)
			{
				// only commit intentional selections, not implicit ones
				self->commitIfImmediateSet();
			}
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
	gSavedSettings.setBOOL("ApplyTextureImmediately", check_box->get());

	picker->updateFilterPermMask();
	picker->commitIfImmediateSet();
}

void LLFloaterTexturePicker::updateFilterPermMask()
{
	//mInventoryPanel->setFilterPermMask( getFilterPermMask() );  Commented out due to no-copy texture loss.
}

void LLFloaterTexturePicker::onSearchEdit(const std::string& search_string, void* user_data )
{
	LLFloaterTexturePicker* picker = (LLFloaterTexturePicker*)user_data;

	std::string upper_case_search_string = search_string;
	LLStringUtil::toUpper(upper_case_search_string);

	if (upper_case_search_string.empty())
	{
		if (picker->mInventoryPanel->getFilterSubString().empty())
		{
			// current filter and new filter empty, do nothing
			return;
		}

		picker->mSavedFolderState.setApply(TRUE);
		picker->mInventoryPanel->getRootFolder()->applyFunctorRecursively(picker->mSavedFolderState);
		// add folder with current item to list of previously opened folders
		LLOpenFoldersWithSelection opener;
		picker->mInventoryPanel->getRootFolder()->applyFunctorRecursively(opener);
		picker->mInventoryPanel->getRootFolder()->scrollToShowSelection();

	}
	else if (picker->mInventoryPanel->getFilterSubString().empty())
	{
		// first letter in search term, save existing folder open state
		if (!picker->mInventoryPanel->getRootFolder()->isFilterModified())
		{
			picker->mSavedFolderState.setApply(FALSE);
			picker->mInventoryPanel->getRootFolder()->applyFunctorRecursively(picker->mSavedFolderState);
		}
	}

	picker->mInventoryPanel->setFilterSubString(upper_case_search_string);
}

//static 
void LLFloaterTexturePicker::onTextureSelect( const LLTextureEntry& te, void *data )
{
	LLFloaterTexturePicker* self = (LLFloaterTexturePicker*)data;

	LLUUID inventory_item_id = self->findItemID(te.getID(), TRUE);
	if (self && inventory_item_id.notNull())
	{
		LLToolPipette::getInstance()->setResult(TRUE, "");
		self->setImageID(te.getID());

		self->mNoCopyTextureSelected = FALSE;
		LLInventoryItem* itemp = gInventory.getItem(inventory_item_id);

		if (itemp && !itemp->getPermissions().allowCopyBy(gAgent.getID()))
		{
			// no copy texture
			self->mNoCopyTextureSelected = TRUE;
		}
		
		self->commitIfImmediateSet();
	}
	else
	{
		LLToolPipette::getInstance()->setResult(FALSE, "You do not have a copy this \nof texture in your inventory");
	}
}

///////////////////////////////////////////////////////////////////////
// LLTextureCtrl

static LLRegisterWidget<LLTextureCtrl> r("texture_picker");

LLTextureCtrl::LLTextureCtrl(
	const std::string& name, 
	const LLRect &rect, 
	const std::string& label,
	const LLUUID &image_id, 
	const LLUUID &default_image_id, 
	const std::string& default_image_name )
	:	
	LLUICtrl(name, rect, TRUE, NULL, NULL, FOLLOWS_LEFT | FOLLOWS_TOP),
	mDragCallback(NULL),
	mDropCallback(NULL),
	mOnCancelCallback(NULL),
	mOnSelectCallback(NULL),
	mBorderColor( gColors.getColor("DefaultHighlightLight") ),
	mImageAssetID( image_id ),
	mDefaultImageAssetID( default_image_id ),
	mDefaultImageName( default_image_name ),
	mLabel( label ),
	mAllowNoTexture( FALSE ),
	mImmediateFilterPermMask( PERM_NONE ),
	mNonImmediateFilterPermMask( PERM_NONE ),
	mCanApplyImmediately( FALSE ),
	mNeedsRawImageData( FALSE ),
	mValid( TRUE ),
	mDirty( FALSE ),
	mShowLoadingPlaceholder( FALSE )
{
	mCaption = new LLTextBox( label, 
		LLRect( 0, BTN_HEIGHT_SMALL, getRect().getWidth(), 0 ),
		label,
		LLFontGL::sSansSerifSmall );
	mCaption->setFollows( FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_BOTTOM );
	addChild( mCaption );

	S32 image_top = getRect().getHeight();
	S32 image_bottom = BTN_HEIGHT_SMALL;
	S32 image_middle = (image_top + image_bottom) / 2;
	S32 line_height = llround(LLFontGL::sSansSerifSmall->getLineHeight());

	mTentativeLabel = new LLTextBox( std::string("Multiple"), 
		LLRect( 
			0, image_middle + line_height / 2,
			getRect().getWidth(), image_middle - line_height / 2 ),
		std::string("Multiple"),
		LLFontGL::sSansSerifSmall );
	mTentativeLabel->setHAlign( LLFontGL::HCENTER );
	mTentativeLabel->setFollowsAll();
	addChild( mTentativeLabel );

	LLRect border_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
	border_rect.mBottom += BTN_HEIGHT_SMALL;
	mBorder = new LLViewBorder(std::string("border"), border_rect, LLViewBorder::BEVEL_IN);
	mBorder->setFollowsAll();
	addChild(mBorder);

	setEnabled(TRUE); // for the tooltip
	mLoadingPlaceholderString = LLTrans::getString("texture_loading");
}


LLTextureCtrl::~LLTextureCtrl()
{
	closeFloater();
}

// virtual
LLXMLNodePtr LLTextureCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	node->createChild("label", TRUE)->setStringValue(getLabel());

	node->createChild("default_image_name", TRUE)->setStringValue(getDefaultImageName());

	node->createChild("allow_no_texture", TRUE)->setBoolValue(mAllowNoTexture);

	node->createChild("can_apply_immediately", TRUE)->setBoolValue(mCanApplyImmediately );

	return node;
}

LLView* LLTextureCtrl::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	std::string name("texture_picker");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent);

	std::string label;
	node->getAttributeString("label", label);

	std::string image_id("");
	node->getAttributeString("image", image_id);

	std::string default_image_id("");
	node->getAttributeString("default_image", default_image_id);

	std::string default_image_name("Default");
	node->getAttributeString("default_image_name", default_image_name);

	BOOL allow_no_texture = FALSE;
	node->getAttributeBOOL("allow_no_texture", allow_no_texture);

	BOOL can_apply_immediately = FALSE;
	node->getAttributeBOOL("can_apply_immediately", can_apply_immediately);

	if (label.empty())
	{
		label.assign(node->getValue());
	}

	LLTextureCtrl* texture_picker = new LLTextureCtrl(
									name, 
									rect,
									label,
									LLUUID(image_id),
									LLUUID(default_image_id), 
									default_image_name );
	texture_picker->setAllowNoTexture(allow_no_texture);
	texture_picker->setCanApplyImmediately(can_apply_immediately);

	texture_picker->initFromXML(node, parent);

	return texture_picker;
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

void LLTextureCtrl::setVisible( BOOL visible ) 
{
	if( !visible )
	{
		closeFloater();
	}
	LLUICtrl::setVisible( visible );
}

void LLTextureCtrl::setEnabled( BOOL enabled )
{
	LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
	if( enabled )
	{
		std::string tooltip;
		if (floaterp) tooltip = floaterp->getUIString("choose_picture");
		setToolTip( tooltip );
	}
	else
	{
		setToolTip( std::string() );
		// *TODO: would be better to keep floater open and show
		// disabled state.
		closeFloater();
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
BOOL	LLTextureCtrl::isDirty() const		
{ 
	return mDirty;	
}

// virtual 
void	LLTextureCtrl::resetDirty()
{ 
	mDirty = FALSE;	
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
	LLFloater* floaterp = mFloaterHandle.get();

	// Show the dialog
	if( floaterp )
	{
		floaterp->open( );		/* Flawfinder: ignore */
	}
	else
	{
		if( !mLastFloaterLeftTop.mX && !mLastFloaterLeftTop.mY )
		{
			gFloaterView->getNewFloaterPosition(&mLastFloaterLeftTop.mX, &mLastFloaterLeftTop.mY);
		}
		LLRect rect = gSavedSettings.getRect("TexturePickerRect");
		rect.translate( mLastFloaterLeftTop.mX - rect.mLeft, mLastFloaterLeftTop.mY - rect.mTop );

		floaterp = new LLFloaterTexturePicker(
			this,
			rect,
			mLabel,
			mImmediateFilterPermMask,
			mNonImmediateFilterPermMask,
			mCanApplyImmediately);
		mFloaterHandle = floaterp->getHandle();

		gFloaterView->getParentFloater(this)->addDependentFloater(floaterp);
		floaterp->open();		/* Flawfinder: ignore */
	}

	if (take_focus)
	{
		floaterp->setFocus(TRUE);
	}
}


void LLTextureCtrl::closeFloater()
{
	LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();
	if( floaterp )
	{
		floaterp->setOwner(NULL);
		floaterp->close();
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
		gInventory.startBackgroundFetch();
		gInventory.removeObserver(this);
		delete this;
	}
};

BOOL LLTextureCtrl::handleHover(S32 x, S32 y, MASK mask)
{
	getWindow()->setCursor(UI_CURSOR_HAND);
	return TRUE;
}


BOOL LLTextureCtrl::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLUICtrl::handleMouseDown( x, y , mask );
	if( handled )
	{
		showPicker(FALSE);

		//grab textures first...
		gInventory.startBackgroundFetch(gInventory.findCategoryUUIDForType(LLAssetType::AT_TEXTURE));
		//...then start full inventory fetch.
		gInventory.startBackgroundFetch();
	}
	return handled;
}

void LLTextureCtrl::onFloaterClose()
{
	LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();

	if (floaterp)
	{
		floaterp->setOwner(NULL);
		mLastFloaterLeftTop.set( floaterp->getRect().mLeft, floaterp->getRect().mTop );
	}

	mFloaterHandle.markDead();
}

void LLTextureCtrl::onFloaterCommit(ETexturePickOp op)
{
	LLFloaterTexturePicker* floaterp = (LLFloaterTexturePicker*)mFloaterHandle.get();

	if( floaterp && getEnabled())
	{
		mDirty = (op != TEXTURE_CANCEL);
		if( floaterp->isDirty() )
		{
			setTentative( FALSE );
			mImageItemID = floaterp->findItemID(floaterp->getAssetID(), FALSE);
			lldebugs << "mImageItemID: " << mImageItemID << llendl;
			mImageAssetID = floaterp->getAssetID();
			lldebugs << "mImageAssetID: " << mImageAssetID << llendl;
			if (op == TEXTURE_SELECT && mOnSelectCallback)
			{
				mOnSelectCallback(this, mCallbackUserData);
			}
			else if (op == TEXTURE_CANCEL && mOnCancelCallback)
			{
				mOnCancelCallback(this, mCallbackUserData);
			}
			else
			{
				onCommit();
			}
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
			floaterp->setDirty( FALSE );
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
	if (getEnabled() && (cargo_type == DAD_TEXTURE) && allowDrop(item))
	{
		if (drop)
		{
			if(doDrop(item))
			{
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

	if (mImageAssetID.isNull() || !mValid)
	{
		mTexturep = NULL;
	}
	else
	{
		mTexturep = gImageList.getImage(mImageAssetID, MIPMAP_YES, IMMEDIATE_NO);
		mTexturep->setBoostLevel(LLViewerImage::BOOST_PREVIEW);
	}
	
	// Border
	LLRect border( 0, getRect().getHeight(), getRect().getWidth(), BTN_HEIGHT_SMALL );
	gl_rect_2d( border, mBorderColor, FALSE );

	// Interior
	LLRect interior = border;
	interior.stretch( -1 ); 

	if( mTexturep )
	{
		if( mTexturep->getComponents() == 4 )
		{
			gl_rect_2d_checkerboard( interior );
		}
		
		gl_draw_scaled_image( interior.mLeft, interior.mBottom, interior.getWidth(), interior.getHeight(), mTexturep);
		mTexturep->addTextureStats( (F32)(interior.getWidth() * interior.getHeight()) );
	}
	else
	{
		gl_rect_2d( interior, LLColor4::grey, TRUE );

		// Draw X
		gl_draw_x( interior, LLColor4::black );
	}

	mTentativeLabel->setVisible( !mTexturep.isNull() && getTentative() );
	
	if ( mTexturep.notNull() &&
		 (mShowLoadingPlaceholder == TRUE) && 
		 (mTexturep->getDiscardLevel() != 1) &&
		 (mTexturep->getDiscardLevel() != 0))
	{
		LLFontGL* font = LLFontGL::sSansSerifBig;
		font->renderUTF8(
			mLoadingPlaceholderString, 0,
			llfloor(interior.mLeft+10), 
			llfloor(interior.mTop-20),
			LLColor4::white,
			LLFontGL::LEFT,
			LLFontGL::BASELINE,
			LLFontGL::DROP_SHADOW);
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
			return mDragCallback(this, item, mCallbackUserData);
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
		return mDropCallback(this, item, mCallbackUserData);
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


/////////////////////////////////////////////////////////////////////////////////
// LLToolTexEyedropper

class LLToolTexEyedropper : public LLTool 
{
public:
	LLToolTexEyedropper( void (*callback)(const LLUUID& obj_id, const LLUUID& image_id, void* userdata ), void* userdata );
	virtual ~LLToolTexEyedropper();
	virtual BOOL		handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL		handleHover(S32 x, S32 y, MASK mask);

protected:
	void				(*mCallback)(const LLUUID& obj_id, const LLUUID& image_id, void* userdata );
	void*				mCallbackUserData;
};


LLToolTexEyedropper::LLToolTexEyedropper( 
	void (*callback)(const LLUUID& obj_id, const LLUUID& image_id, void* userdata ),
	void* userdata )
	: LLTool(std::string("texeyedropper")),
	  mCallback( callback ),
	  mCallbackUserData( userdata )
{
}

LLToolTexEyedropper::~LLToolTexEyedropper()
{
}


BOOL LLToolTexEyedropper::handleMouseDown(S32 x, S32 y, MASK mask)
{
	LLViewerObject* hit_obj	= gViewerWindow->lastObjectHit();
	if (hit_obj && 
		!hit_obj->isAvatar())
	{
		if( (0 <= gLastHitObjectFace) && (gLastHitObjectFace < hit_obj->getNumTEs()) )
		{
			LLViewerImage* image = hit_obj->getTEImage( gLastHitObjectFace );
			if( image )
			{
				if( mCallback )
				{
					mCallback( hit_obj->getID(), image->getID(), mCallbackUserData );
				}
			}
		}
	}
	return TRUE;
}

BOOL LLToolTexEyedropper::handleHover(S32 x, S32 y, MASK mask)
{
	lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolTexEyedropper" << llendl;
	gViewerWindow->getWindow()->setCursor(UI_CURSOR_CROSS);  // TODO: better cursor
	return TRUE;
}



