/** 
 * @file llpreviewnotecard.h
 * @brief LLPreviewNotecard class header file
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPREVIEWNOTECARD_H
#define LL_LLPREVIEWNOTECARD_H

#include "llpreview.h"
#include "llassetstorage.h"
#include "lliconctrl.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLPreviewNotecard
//
// This class allows us to edit notecards
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLViewerTextEditor;
class LLButton;

class LLPreviewNotecard : public LLPreview
{
public:
	LLPreviewNotecard(const std::string& name, const LLRect& rect, const std::string& title,
					  const LLUUID& item_id,
					  const LLUUID& object_id = LLUUID::null,
					  const LLUUID& asset_id = LLUUID::null,
					  BOOL show_keep_discard = FALSE);

	// llpreview	
	virtual bool saveItem(LLPointer<LLInventoryItem>* itemptr);

	// llview
	virtual void draw();
	virtual BOOL handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	virtual void setEnabled( BOOL enabled );
	virtual void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	// llfloater
	virtual BOOL canClose();

	// llpanel
	virtual BOOL	postBuild();

	// reach into the text editor, and grab the drag item
	const LLInventoryItem* getDragItem();


	// return true if there is any embedded inventory.
	bool hasEmbeddedInventory();

	// After saving a notecard, the tcp based upload system will
	// change the asset, therefore, we need to re-fetch it from the
	// asset system. :(
	void refreshFromInventory();

protected:

	virtual void loadAsset();
	bool saveIfNeeded(LLInventoryItem* copyitem = NULL);

	static LLPreviewNotecard* getInstance(const LLUUID& uuid);

	static void onLoadComplete(LLVFS *vfs,
							   const LLUUID& asset_uuid,
							   LLAssetType::EType type,
							   void* user_data, S32 status);

	static void onClickSave(void* data);

	static void onSaveComplete(const LLUUID& asset_uuid,
							   void* user_data,
							   S32 status);

	static void handleSaveChangesDialog(S32 option, void* userdata);

protected:
	LLViewerTextEditor* mEditor;
	LLButton* mSaveBtn;

	LLUUID mAssetID;

	LLUUID mNotecardItemID;
	LLUUID mObjectID;
};


#endif // LL_LLPREVIEWNOTECARD_H
