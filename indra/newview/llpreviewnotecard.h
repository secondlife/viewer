/** 
 * @file llpreviewnotecard.h
 * @brief LLPreviewNotecard class header file
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

#ifndef LL_LLPREVIEWNOTECARD_H
#define LL_LLPREVIEWNOTECARD_H

#include "llpreview.h"
#include "llassetstorage.h"
#include "llpreviewscript.h"
#include "lliconctrl.h"
#include "llvoinventorylistener.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLPreviewNotecard
//
// This class allows us to edit notecards
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLViewerTextEditor;
class LLButton;

class LLPreviewNotecard : public LLPreview, public LLVOInventoryListener
{
public:
	LLPreviewNotecard(const LLSD& key);
	virtual ~LLPreviewNotecard();
	
	bool saveItem();
	void setObjectID(const LLUUID& object_id) override;

	// llview
	void draw() override;
	bool handleKeyHere(KEY key, MASK mask) override;
	void setEnabled(bool enabled) override;

	// llfloater
	bool canClose() override;

	// llpanel
	bool postBuild() override;

	// reach into the text editor, and grab the drag item
	const LLInventoryItem* getDragItem();


	// return true if there is any embedded inventory.
	bool hasEmbeddedInventory();

	// After saving a notecard, the tcp based upload system will
	// change the asset, therefore, we need to re-fetch it from the
	// asset system. :(
	void refreshFromInventory(const LLUUID& item_id = LLUUID::null);

	void syncExternal();

    void inventoryChanged(LLViewerObject* object,
        LLInventoryObject::object_list_t* inventory,
        S32 serial_num,
        void* user_data) override;

protected:

	void updateTitleButtons() override;
	void loadAsset() override;
	bool saveIfNeeded(LLInventoryItem* copyitem = NULL, bool sync = true);

	void deleteNotecard();

	static void onLoadComplete(const LLUUID& asset_uuid,
							   LLAssetType::EType type,
							   void* user_data, S32 status, LLExtStat ext_status);

	static void onClickSave(void* data);

	static void onClickDelete(void* data);

	static void onClickEdit(void* data);

	static void onSaveComplete(const LLUUID& asset_uuid,
							   void* user_data,
							   S32 status, LLExtStat ext_status);

	bool handleSaveChangesDialog(const LLSD& notification, const LLSD& response);
	bool handleConfirmDeleteDialog(const LLSD& notification, const LLSD& response);

    static void finishInventoryUpload(LLUUID itemId, LLUUID newAssetId, LLUUID newItemId);
    static void finishTaskUpload(LLUUID itemId, LLUUID newAssetId, LLUUID taskId);

    void openInExternalEditor();
    bool onExternalChange(const std::string& filename);
    bool loadNotecardText(const std::string& filename);
    bool writeToFile(const std::string& filename);
    std::string getTmpFileName();

protected:
	LLViewerTextEditor* mEditor;
	LLButton* mSaveBtn;

	LLUUID mAssetID;

	LLUUID mObjectID;

	LLLiveLSLFile* mLiveFile;
};


#endif // LL_LLPREVIEWNOTECARD_H
