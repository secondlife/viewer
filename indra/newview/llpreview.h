/** 
 * @file llpreview.h
 * @brief LLPreview class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPREVIEW_H
#define LL_LLPREVIEW_H

#include "llfloater.h"
#include "llresizehandle.h"
#include "llmap.h"
#include "lluuid.h"
#include "llviewerinventory.h"
#include "lltabcontainer.h"
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
	typedef std::map<LLUUID, LLViewHandle> handle_map_t;
	static std::map<LLUUID, LLViewHandle> sAutoOpenPreviewHandles;
};

class LLPreview : public LLFloater
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
	LLPreview(const std::string& name, const LLRect& rect, const std::string& title, const LLUUID& item_uuid, const LLUUID& object_uuid, BOOL allow_resize = FALSE, S32 min_width = 0, S32 min_height = 0, LLViewerInventoryItem* inv_item = NULL );
	virtual ~LLPreview();

	void setItemID(const LLUUID& item_id);
	void setObjectID(const LLUUID& object_id);
	void setSourceID(const LLUUID& source_id);
	LLViewerInventoryItem* getItem() const; // searches if not constructed with it

	static LLPreview* find(const LLUUID& item_uuid);
	static LLPreview*	show(const LLUUID& item_uuid, BOOL take_focus = TRUE );
	static void			hide(const LLUUID& item_uuid);
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

protected:
	virtual void onCommit();

	void addDescriptionUI();

	static void onText(LLUICtrl*, void* userdata);
	static void onRadio(LLUICtrl*, void* userdata);
	

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
	typedef std::multimap<LLUUID, LLViewHandle> preview_multimap_t;

	static preview_multimap_t sPreviewsBySource;
	static preview_map_t sInstances;
	LLUUID mNotecardInventoryID;
	LLUUID mObjectID;
	LLViewerInventoryItem* mItem;
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
