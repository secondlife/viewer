/** 
 * @file llfloateroutbox.h
 * @brief LLFloaterOutbox
 * class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
 * ABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLFLOATEROUTBOX_H
#define LL_LLFLOATEROUTBOX_H

#include "llfloater.h"
#include "llfoldertype.h"


class LLButton;
class LLInventoryCategoriesObserver;
class LLInventoryCategoryAddedObserver;
class LLInventoryPanel;
class LLLoadingIndicator;
class LLTextBox;
class LLView;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterOutbox
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFloaterOutbox : public LLFloater
{
public:
	LLFloaterOutbox(const LLSD& key);
	~LLFloaterOutbox();
	
	void setupOutbox(const LLUUID& outboxId);

	// virtuals
	BOOL postBuild();
	void onOpen(const LLSD& key);
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
						   EDragAndDropType cargo_type,
						   void* cargo_data,
						   EAcceptance* accept,
						   std::string& tooltip_msg);

protected:
	void importReportResults(U32 status, const LLSD& content);
	void importStatusChanged(bool inProgress);
	
	void onImportButtonClicked();
	void onOutboxChanged();
	
	void updateView();

private:
	LLInventoryCategoriesObserver *		mCategoriesObserver;
	LLInventoryCategoryAddedObserver *	mCategoryAddedObserver;

	LLUUID				mOutboxId;
	LLInventoryPanel *	mOutboxInventoryPanel;
	U32					mOutboxItemCount;
	
	LLView *				mInventoryDisablePanel;
	LLTextBox *				mInventoryFolderCountText;
	LLView *				mInventoryImportInProgress;
	LLView *				mInventoryPlaceholder;
	LLTextBox *				mInventoryText;
	LLTextBox *				mInventoryTitle;
	
	LLButton *		mImportButton;
};

#endif // LL_LLFLOATEROUTBOX_H
