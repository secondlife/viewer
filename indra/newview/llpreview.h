/** 
 * @file llpreview.h
 * @brief LLPreview class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLPREVIEW_H
#define LL_LLPREVIEW_H

#include "llmultifloater.h"
#include "llresizehandle.h"
#include "llpointer.h"
#include "lluuid.h"
#include "llinventoryobserver.h"
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
