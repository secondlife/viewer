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
#include "llmodaldialog.h"
#include "lltexteditor.h"

class LLInventoryCategoriesObserver;
class LLInventoryCategoryAddedObserver;
class LLTextBox;
class LLView;
class LLFilterEditor;

class LLFloaterMarketplaceListings;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLPanelMarketplaceListings
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLPanelMarketplaceListings : public LLPanel
{
public:
    LLPanelMarketplaceListings();
	BOOL postBuild();
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
						   EDragAndDropType cargo_type,
						   void* cargo_data,
						   EAcceptance* accept,
						   std::string& tooltip_msg);
	void draw();
	LLFolderView* getRootFolder() { return mRootFolder; }
    bool allowDropOnRoot();
    
    void buildAllPanels();
    
private:
    LLInventoryPanel* buildInventoryPanel(const std::string& childname, const std::string& filename);

    // UI callbacks
	void onViewSortMenuItemClicked(const LLSD& userdata);
	bool onViewSortMenuItemCheck(const LLSD& userdata);
	void onAddButtonClicked();
    void onAuditButtonClicked();
	void onSelectionChange(LLInventoryPanel *panel, const std::deque<LLFolderViewItem*>& items, BOOL user_action);
    void onTabChange();
    void onFilterEdit(const std::string& search_string);
    
    void setSortOrder(U32 sort_order);
    
    LLFolderView*     mRootFolder;
    LLButton*         mAuditBtn;
	LLFilterEditor*	  mFilterEditor;
	std::string		  mFilterSubString;
    bool              mFilterListingFoldersOnly;
    U32               mSortOrder;
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
	void setRootFolder();
	void setPanels();
	void fetchContents();
    
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

	LLUUID			mRootFolderId;
	LLPanelMarketplaceListings * mPanelListings;
    bool            mPanelListingsSet;
};

//-----------------------------------------------------------------------------
// LLFloaterAssociateListing
//-----------------------------------------------------------------------------
class LLFloaterAssociateListing : public LLFloater
{
	friend class LLFloaterReg;
public:
	virtual BOOL postBuild();
	virtual BOOL handleKeyHere(KEY key, MASK mask);
    
	static LLFloaterAssociateListing* show(const LLUUID& folder_id);
    
private:
	LLFloaterAssociateListing(const LLSD& key);
	virtual ~LLFloaterAssociateListing();
    
	// UI Callbacks
	void apply(BOOL user_confirm = TRUE);
	void cancel();
    void callback_apply(const LLSD& notification, const LLSD& response);
    
	LLUUID mUUID;
};

//-----------------------------------------------------------------------------
// LLFloaterMarketplaceValidation
//-----------------------------------------------------------------------------
// Note: The key is the UUID of the folder to validate. Validates the whole
// marketplace listings content if UUID is null.
// Note: For the moment, we just display the validation text. Eventually, we should
// get the validation triggered on the server and display the html report.
// *TODO : morph into an html/text window using the pattern in llfloatertos

class LLFloaterMarketplaceValidation : public LLFloater
{
public:
	LLFloaterMarketplaceValidation(const LLSD& key);
	virtual ~LLFloaterMarketplaceValidation();
    
	virtual BOOL postBuild();
	virtual void draw();
	virtual void onOpen(const LLSD& key);
    
    void clearMessages();
    void appendMessage(std::string& message, S32 depth, LLError::ELevel log_level);
	static void	onOK( void* userdata );
    
private:
    struct Message {
        LLError::ELevel mErrorLevel;
        std::string mMessage;
    };
	typedef std::vector<Message> message_list_t;
    
    void handleCurrentListing();

    message_list_t mCurrentListingMessages;
    LLError::ELevel mCurrentListingErrorLevel;

    message_list_t mMessages;

    LLTextEditor*	mEditor;
};

//-----------------------------------------------------------------------------
// LLFloaterItemProperties
//-----------------------------------------------------------------------------

class LLFloaterItemProperties : public LLFloater
{
public:
	LLFloaterItemProperties(const LLSD& key);
	virtual ~LLFloaterItemProperties();
    
	BOOL postBuild();
	virtual void onOpen(const LLSD& key);
    
private:
};

#endif // LL_LLFLOATERMARKETPLACELISTINGS_H
