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
#include "llnotificationptr.h"


class LLButton;
class LLInventoryCategoriesObserver;
class LLInventoryCategoryAddedObserver;
class LLInventoryPanel;
class LLLoadingIndicator;
class LLNotification;
class LLTextBox;
class LLView;
class LLWindowShade;


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
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
						   EDragAndDropType cargo_type,
						   void* cargo_data,
						   EAcceptance* accept,
						   std::string& tooltip_msg);
	
	void showNotification(const LLSD& notify);

	BOOL handleHover(S32 x, S32 y, MASK mask);
	void onMouseLeave(S32 x, S32 y, MASK mask);

protected:
	void fetchOutboxContents();

	void importReportResults(U32 status, const LLSD& content);
	void importStatusChanged(bool inProgress);
	void initializationReportError(U32 status, const LLSD& content);
	
	void onClose(bool app_quitting);
	void onOpen(const LLSD& key);

	void onFocusReceived();

	void onImportButtonClicked();
	void onOutboxChanged();
	
	void setStatusString(const std::string& statusString);
	
	void updateFolderCount();
	void updateFolderCountStatus();
	void updateView();

private:
	LLInventoryCategoriesObserver *		mCategoriesObserver;
	LLInventoryCategoryAddedObserver *	mCategoryAddedObserver;
	
	bool			mImportBusy;
	LLButton *		mImportButton;
	
	LLTextBox *		mInventoryFolderCountText;
	LLView *		mInventoryImportInProgress;
	LLView *		mInventoryPlaceholder;
	LLTextBox *		mInventoryText;
	LLTextBox *		mInventoryTitle;
	
	LLUUID				mOutboxId;
	LLInventoryPanel *	mOutboxInventoryPanel;
	U32					mOutboxItemCount;
	LLPanel *			mOutboxTopLevelDropZone;
	
	LLWindowShade *	mWindowShade;
};

#endif // LL_LLFLOATEROUTBOX_H
