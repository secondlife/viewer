/** 
 * @file llfloatermarketplacelistings.h
 * @brief Implementation of the marketplace listings floater and panels
 * @author merov@lindenlab.com
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

#ifndef LL_LLFLOATERMARKETPLACELISTINGS_H
#define LL_LLFLOATERMARKETPLACELISTINGS_H

#include "llfloater.h"
#include "llinventoryfilter.h"
#include "llinventorypanel.h"
#include "llnotificationptr.h"

class LLInventoryCategoriesObserver;
class LLInventoryCategoryAddedObserver;
class LLTextBox;
class LLView;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLPanelMarketplaceListings
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLPanelMarketplaceListings : public LLPanel
{
public:
    LLPanelMarketplaceListings();
	BOOL postBuild();
	void draw();
	LLFolderView* getRootFolder() { return mAllPanel->getRootFolder(); }    // *TODO : Suppress and get DnD in here instead...
    
private:
    // UI callbacks
	void onViewSortMenuItemClicked(const LLSD& userdata);
	bool onViewSortMenuItemCheck(const LLSD& userdata);
	void onAddButtonClicked();
    
    LLInventoryPanel* mAllPanel;
    LLInventoryFilter::ESortOrderType mSortOrder;
    LLInventoryFilter::EFilterType mFilterType;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterMarketplaceListings
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFloaterMarketplaceListings : public LLFloater
{
public:
	LLFloaterMarketplaceListings(const LLSD& key);
	~LLFloaterMarketplaceListings();
	
	void initializeMarketPlace();
    
	// virtuals
	BOOL postBuild();
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
						   EDragAndDropType cargo_type,
						   void* cargo_data,
						   EAcceptance* accept,
						   std::string& tooltip_msg);
	
	void showNotification(const LLNotificationPtr& notification);
    
	BOOL handleHover(S32 x, S32 y, MASK mask);
	void onMouseLeave(S32 x, S32 y, MASK mask);
    
protected:
	void setup();
	void fetchContents();
    
	void importReportResults(U32 status, const LLSD& content);
	void importStatusChanged(bool inProgress);
	void initializationReportError(U32 status, const LLSD& content);
	void setStatusString(const std::string& statusString);

	void onClose(bool app_quitting);
	void onOpen(const LLSD& key);
	void onFocusReceived();
	void onChanged();
    
    bool isAccepted(EAcceptance accept);
	
	void updateView();
    
private:
    S32 getFolderCount();

	LLInventoryCategoriesObserver *		mCategoriesObserver;
	LLInventoryCategoryAddedObserver *	mCategoryAddedObserver;
		
	LLTextBox *		mInventoryStatus;
	LLView *		mInventoryInitializationInProgress;
	LLView *		mInventoryPlaceholder;
	LLTextBox *		mInventoryText;
	LLTextBox *		mInventoryTitle;

	LLUUID				mRootFolderId;
	LLPanelMarketplaceListings * mPanelListings;
};

#endif // LL_LLFLOATERMARKETPLACELISTINGS_H
