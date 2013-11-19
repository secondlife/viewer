/** 
 * @file llpreview.h
 * @brief LLPreview class definition
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

#ifndef LL_LLPREVIEW_H
#define LL_LLPREVIEW_H

#include "llmultifloater.h"
#include "llresizehandle.h"
#include "llpointer.h"
#include "lluuid.h"
#include "llinventoryobserver.h"
#include "llextendedstatus.h"
#include <map>

class LLInventoryItem;
class LLLineEditor;
class LLRadioGroup;
class LLPreview;

class LLMultiPreview : public LLMultiFloater
{
public:
	LLMultiPreview();

	/*virtual*/void onOpen(const LLSD& key);
	/*virtual*/void tabOpen(LLFloater* opened_floater, bool from_click);
	/*virtual*/ void handleReshape(const LLRect& new_rect, bool by_user = false);

};

// https://wiki.lindenlab.com/mediawiki/index.php?title=LLPreview&oldid=81373

class LLPreview : public LLFloater, LLInventoryObserver
{
public:
	typedef enum e_asset_status
	{
		PREVIEW_ASSET_ERROR,
		PREVIEW_ASSET_UNLOADED,
		PREVIEW_ASSET_LOADING,
		PREVIEW_ASSET_LOADED
	} EAssetStatus;
public:
	LLPreview(const LLSD& key );
	virtual ~LLPreview();
		
	/*virtual*/ BOOL postBuild();
	
	virtual void setObjectID(const LLUUID& object_id);
	void setItem( LLInventoryItem* item );
	
	void setAssetId(const LLUUID& asset_id);
	const LLInventoryItem* getItem() const; // searches if not constructed with it

	static void hide(const LLUUID& item_uuid, BOOL no_saving = FALSE );
	static void	dirty(const LLUUID& item_uuid);
	
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL handleHover(S32 x, S32 y, MASK mask);
	virtual void onOpen(const LLSD& key);
	
	void setAuxItem( const LLInventoryItem* item );

	static void			onBtnCopyToInv(void* userdata);

	void				addKeepDiscardButtons();
	static void			onKeepBtn(void* data);
	static void			onDiscardBtn(void* data);
	/*virtual*/ void	handleReshape(const LLRect& new_rect, bool by_user = false);

	void userResized() { mUserResized = TRUE; };

	virtual void loadAsset() { mAssetStatus = PREVIEW_ASSET_LOADED; }
	virtual EAssetStatus getAssetStatus() { return mAssetStatus;}

	static LLPreview* getFirstPreviewForSource(const LLUUID& source_id);

	// Why is this at the LLPreview level?  JC
	void setNotecardInfo(const LLUUID& notecard_inv_id, const LLUUID& object_id);

	// llview
	/*virtual*/ void draw();
	void refreshFromItem();
	
protected:
	virtual void onCommit();

	void addDescriptionUI();

	static void onText(LLUICtrl*, void* userdata);
	static void onRadio(LLUICtrl*, void* userdata);
	
	// for LLInventoryObserver 
	virtual void changed(U32 mask);	
	BOOL mDirty;
	
protected:
	LLUUID mItemUUID;

	// mObjectUUID will have a value if it is associated with a task in
	// the world, and will be == LLUUID::null if it's in the agent
	// inventory.
	LLUUID mObjectUUID;

	LLRect mClientRect;

	LLPointer<LLInventoryItem> mAuxItem;  // HACK!
	LLPointer<LLInventoryItem> mItem;  // For embedded items (Landmarks)
	LLButton* mCopyToInvBtn;

	// Close without saving changes
	BOOL mForceClose;

	BOOL mUserResized;

	// When closing springs a "Want to save?" dialog, we want
	// to keep the preview open until the save completes.
	BOOL mCloseAfterSave;

	EAssetStatus mAssetStatus;

	LLUUID mNotecardInventoryID;
	// I am unsure if this is always the same as mObjectUUID, or why it exists
	// at the LLPreview level.  JC 2009-06-24
	LLUUID mNotecardObjectID;
};


const S32 PREVIEW_BORDER = 4;
const S32 PREVIEW_PAD = 5;

const S32 PREVIEW_LINE_HEIGHT = 19;
const S32 PREVIEW_BORDER_WIDTH = 2;
const S32 PREVIEW_RESIZE_HANDLE_SIZE = S32(RESIZE_HANDLE_WIDTH * OO_SQRT2) + PREVIEW_BORDER_WIDTH;
const S32 PREVIEW_VPAD = 2;
const S32 PREVIEW_HEADER_SIZE = 2*PREVIEW_LINE_HEIGHT + 2 * PREVIEW_VPAD;

#endif  // LL_LLPREVIEW_H
