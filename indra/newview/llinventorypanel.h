/** 
 * @file llinventorypanel.h
 * @brief LLInventoryPanel
 * class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLINVENTORYPANEL_H
#define LL_LLINVENTORYPANEL_H

#include "llassetstorage.h"
#include "lldarray.h"
#include "llfloater.h"
#include "llinventory.h"
#include "llinventoryfilter.h"
#include "llfolderview.h"
#include "llinventorymodel.h"
#include "lluictrlfactory.h"
#include <set>

class LLFolderViewItem;
class LLInventoryFilter;
class LLInventoryModel;
class LLInvFVBridge;
class LLInventoryFVBridgeBuilder;
class LLMenuBarGL;
class LLCheckBoxCtrl;
class LLSpinCtrl;
class LLScrollContainer;
class LLTextBox;
class LLIconCtrl;
class LLSaveFolderState;
class LLFilterEditor;
class LLTabContainer;

class LLInventoryPanel : public LLPanel
{
public:
	static const std::string DEFAULT_SORT_ORDER;
	static const std::string RECENTITEMS_SORT_ORDER;
	static const std::string INHERIT_SORT_ORDER;

	struct Filter : public LLInitParam::Block<Filter>
	{
		Optional<U32>			sort_order;
		Optional<U32>			types;
		Optional<std::string>	search_string;

		Filter()
		:	sort_order("sort_order"),
			types("types", 0xffffffff),
			search_string("search_string")
		{}
	};

	struct Params 
	:	public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<std::string>				sort_order_setting;
		Optional<LLInventoryModel*>			inventory;
		Optional<bool>						allow_multi_select;
		Optional<Filter>					filter;
		Optional<std::string>               start_folder;

		Params()
		:	sort_order_setting("sort_order_setting"),
			inventory("", &gInventory),
			allow_multi_select("allow_multi_select", true),
			filter("filter"),
			start_folder("start_folder")
		{}
	};

protected:
	LLInventoryPanel(const Params&);
	friend class LLUICtrlFactory;

public:
	virtual ~LLInventoryPanel();

	LLInventoryModel* getModel() { return mInventory; }

	BOOL postBuild();

	// LLView methods
	void draw();
	BOOL handleHover(S32 x, S32 y, MASK mask);
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
	// LLUICtrl methods
	 /*virtual*/ void onFocusLost();
	 /*virtual*/ void onFocusReceived();

	// Call this method to set the selection.
	void openAllFolders();
	void setSelection(const LLUUID& obj_id, BOOL take_keyboard_focus);
	void setSelectCallback(const LLFolderView::signal_t::slot_type& cb) { if (mFolders) mFolders->setSelectCallback(cb); }
	void clearSelection();
	LLInventoryFilter* getFilter();
	void setFilterTypes(U64 filter, BOOL filter_for_categories = FALSE); // if filter_for_categories is true, operate on folder preferred asset type
	U32 getFilterTypes() const { return mFolders->getFilterTypes(); }
	void setFilterPermMask(PermissionMask filter_perm_mask);
	U32 getFilterPermMask() const { return mFolders->getFilterPermissions(); }
	void setFilterSubString(const std::string& string);
	const std::string getFilterSubString() { return mFolders->getFilterSubString(); }
	void setSortOrder(U32 order);
	U32 getSortOrder() { return mFolders->getSortOrder(); }
	void setSinceLogoff(BOOL sl);
	void setHoursAgo(U32 hours);
	BOOL getSinceLogoff();
	
	void setShowFolderState(LLInventoryFilter::EFolderShow show);
	LLInventoryFilter::EFolderShow getShowFolderState();
	void setAllowMultiSelect(BOOL allow) { mFolders->setAllowMultiSelect(allow); }
	// This method is called when something has changed about the inventory.
	void modelChanged(U32 mask);
	LLFolderView* getRootFolder() { return mFolders; }
	LLScrollContainer* getScrollableContainer() { return mScroller; }
	
	void onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action);
	
	// Callbacks
	void doToSelected(const LLSD& userdata);
	void doCreate(const LLSD& userdata);
	bool beginIMSession();
	bool attachObject(const LLSD& userdata);
	
	// DEBUG ONLY:
	static void dumpSelectionInformation(void* user_data);

	void openSelected();
	void unSelectAll()	{ mFolders->setSelection(NULL, FALSE, FALSE); }
	
private:
	// Destroys the old views, and regenerates them based on the
	// start folder ID.
	void generateViews();

	// Given the id and the parent, build all of the folder views.
	void rebuildViewsFor(const LLUUID& id);
	virtual void buildNewViews(const LLUUID& id); // made virtual to support derived classes. EXT-719
protected:
	void defaultOpenInventory(); // open the first level of inventory

	LLInventoryModel*			mInventory;
	LLInventoryObserver*		mInventoryObserver;
	BOOL 						mAllowMultiSelect;
	std::string					mSortOrderSetting;

//private: // Can not make these private - needed by llinventorysubtreepanel
	LLFolderView*				mFolders;
	std::string                 mStartFolderString;

	/**
	 * Contains UUID of Inventory item from which hierarchy should be built.
	 * Can be set with the "start_folder" xml property.
	 * Default is LLUUID::null that means total Inventory hierarchy.
	 */
	LLUUID						mStartFolderID;
	LLScrollContainer*			mScroller;
	bool						mHasInventoryConnection;

	/**
	 * Flag specified if default inventory hierarchy should be created in postBuild()
	 */
	bool						mBuildDefaultHierarchy;

	LLUUID						mRootInventoryItemUUID;

	/**
	 * Pointer to LLInventoryFVBridgeBuilder.
	 *
	 * It is set in LLInventoryPanel's constructor and can be overridden in derived classes with 
	 * another implementation.
	 * Take into account it will not be deleted by LLInventoryPanel itself.
	 */
	const LLInventoryFVBridgeBuilder* mInvFVBridgeBuilder;

};

#endif // LL_LLINVENTORYPANEL_H
