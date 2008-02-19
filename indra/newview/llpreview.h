/** 
 * @file llpreview.h
 * @brief LLPreview class definition
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

#ifndef LL_LLPREVIEW_H
#define LL_LLPREVIEW_H

#include "llfloater.h"
#include "llresizehandle.h"
#include "llmap.h"
#include "lluuid.h"
#include "llviewerinventory.h"
#include "lltabcontainer.h"
#include "llinventorymodel.h"
#include <map>

class LLLineEditor;
class LLRadioGroup;
class LLPreview;

class LLMultiPreview : public LLMultiFloater
{
public:
	LLMultiPreview(const LLRect& rect);

	/*virtual*/void open();		/*Flawfinder: ignore*/
	/*virtual*/void tabOpen(LLFloater* opened_floater, bool from_click);
	/*virtual*/ void userSetShape(const LLRect& new_rect);

	static LLMultiPreview* getAutoOpenInstance(const LLUUID& id);
	static void setAutoOpenInstance(LLMultiPreview* previewp, const LLUUID& id);

protected:
	typedef std::map<LLUUID, LLHandle<LLFloater> > handle_map_t;
	static handle_map_t sAutoOpenPreviewHandles;
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
	// Used for XML-based construction.
	LLPreview(const std::string& name);
	LLPreview(const std::string& name, const LLRect& rect, const std::string& title, const LLUUID& item_uuid, const LLUUID& object_uuid, BOOL allow_resize = FALSE, S32 min_width = 0, S32 min_height = 0, LLPointer<LLViewerInventoryItem> inv_item = NULL );
	virtual ~LLPreview();

	void setItemID(const LLUUID& item_id);
	void setObjectID(const LLUUID& object_id);
	void setSourceID(const LLUUID& source_id);
	const LLViewerInventoryItem *getItem() const; // searches if not constructed with it

	static LLPreview* find(const LLUUID& item_uuid);
	static LLPreview*	show(const LLUUID& item_uuid, BOOL take_focus = TRUE );
	static void			hide(const LLUUID& item_uuid, BOOL no_saving = FALSE );
	static void			rename(const LLUUID& item_uuid, const std::string& new_name);
	static bool			save(const LLUUID& item_uuid, LLPointer<LLInventoryItem>* itemptr);

	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL handleHover(S32 x, S32 y, MASK mask);
	virtual void open();		/*Flawfinder: ignore*/
	virtual bool saveItem(LLPointer<LLInventoryItem>* itemptr);
 
	void setAuxItem( const LLInventoryItem* item )
	{
		if ( mAuxItem ) 
			mAuxItem->copy(item);
	}

	static void			onBtnCopyToInv(void* userdata);

	void				addKeepDiscardButtons();
	static void			onKeepBtn(void* data);
	static void			onDiscardBtn(void* data);
	/*virtual*/ void	userSetShape(const LLRect& new_rect);

	void userResized() { mUserResized = TRUE; };

	virtual void loadAsset() { mAssetStatus = PREVIEW_ASSET_LOADED; }
	virtual EAssetStatus getAssetStatus() { return mAssetStatus;}

	static LLPreview* getFirstPreviewForSource(const LLUUID& source_id);
	void setNotecardInfo(const LLUUID& notecard_inv_id, const LLUUID& object_id)
	{ mNotecardInventoryID = notecard_inv_id; mObjectID = object_id; }

	// llview
	virtual void draw();
	void refreshFromItem(const LLInventoryItem* item);
	
protected:
	virtual void onCommit();

	void addDescriptionUI();

	static void onText(LLUICtrl*, void* userdata);
	static void onRadio(LLUICtrl*, void* userdata);
	
	// for LLInventoryObserver 
	virtual void changed(U32 mask);	
	BOOL mDirty;
	virtual const char *getTitleName() const { return "Preview"; }
	
protected:
	LLUUID mItemUUID;
	LLUUID	mSourceID;

	// mObjectID will have a value if it is associated with a task in
	// the world, and will be == LLUUID::null if it's in the agent
	// inventory.
	LLUUID mObjectUUID;

	LLRect mClientRect;

	LLPointer<LLInventoryItem> mAuxItem;  // HACK!
	LLButton* mCopyToInvBtn;

	// Close without saving changes
	BOOL mForceClose;

	BOOL mUserResized;

	// When closing springs a "Want to save?" dialog, we want
	// to keep the preview open until the save completes.
	BOOL mCloseAfterSave;

	EAssetStatus mAssetStatus;

	typedef std::map<LLUUID, LLPreview*> preview_map_t;
	typedef std::multimap<LLUUID, LLHandle<LLFloater> > preview_multimap_t;

	static preview_multimap_t sPreviewsBySource;
	static preview_map_t sInstances;
	LLUUID mNotecardInventoryID;
	LLUUID mObjectID;
	LLPointer<LLViewerInventoryItem> mItem;
};


const S32 PREVIEW_BORDER = 4;
const S32 PREVIEW_PAD = 5;
const S32 PREVIEW_BUTTON_WIDTH = 100;

const S32 PREVIEW_LINE_HEIGHT = 19;
const S32 PREVIEW_CLOSE_BOX_SIZE = 16;
const S32 PREVIEW_BORDER_WIDTH = 2;
const S32 PREVIEW_RESIZE_HANDLE_SIZE = S32(RESIZE_HANDLE_WIDTH * OO_SQRT2) + PREVIEW_BORDER_WIDTH;
const S32 PREVIEW_VPAD = 2;
const S32 PREVIEW_HPAD = PREVIEW_RESIZE_HANDLE_SIZE;
const S32 PREVIEW_HEADER_SIZE = 2*PREVIEW_LINE_HEIGHT + 2 * PREVIEW_VPAD;

#endif  // LL_LLPREVIEW_H
