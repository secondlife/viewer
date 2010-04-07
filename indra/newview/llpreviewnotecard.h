/** 
 * @file llpreviewnotecard.h
 * @brief LLPreviewNotecard class header file
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
	LLPreviewNotecard(const LLSD& key);
	virtual ~LLPreviewNotecard();
	
	bool saveItem();

	// llview
	virtual void draw();
	virtual BOOL handleKeyHere(KEY key, MASK mask);
	virtual void setEnabled( BOOL enabled );

	// llfloater
	virtual BOOL canClose();

	// llpanel
	virtual BOOL postBuild();

	// reach into the text editor, and grab the drag item
	const LLInventoryItem* getDragItem();


	// return true if there is any embedded inventory.
	bool hasEmbeddedInventory();

	// After saving a notecard, the tcp based upload system will
	// change the asset, therefore, we need to re-fetch it from the
	// asset system. :(
	void refreshFromInventory(const LLUUID& item_id = LLUUID::null);

protected:

	void updateTitleButtons();
	virtual void loadAsset();
	bool saveIfNeeded(LLInventoryItem* copyitem = NULL);

	static void onLoadComplete(LLVFS *vfs,
							   const LLUUID& asset_uuid,
							   LLAssetType::EType type,
							   void* user_data, S32 status, LLExtStat ext_status);

	static void onClickSave(void* data);

	static void onSaveComplete(const LLUUID& asset_uuid,
							   void* user_data,
							   S32 status, LLExtStat ext_status);

	bool handleSaveChangesDialog(const LLSD& notification, const LLSD& response);

protected:
	LLViewerTextEditor* mEditor;
	LLButton* mSaveBtn;

	LLUUID mAssetID;

	LLUUID mObjectID;
};


#endif // LL_LLPREVIEWNOTECARD_H
