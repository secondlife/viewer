/** 
 * @file llinventorypanel.h
 * @brief LLInventoryPanel
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

#ifndef LL_LLINVENTORYPANEL_H
#define LL_LLINVENTORYPANEL_H

#include "llassetstorage.h"
#include "lldarray.h"
#include "llfolderviewitem.h"
#include "llfloater.h"
#include "llinventory.h"
#include "llinventoryfilter.h"
#include "llinventorymodel.h"
#include "llscrollcontainer.h"
#include "lluictrlfactory.h"
#include <set>

class LLInvFVBridge;
class LLInventoryFVBridgeBuilder;
class LLInvPanelComplObserver;

class LLFolderViewModelItemInventory
	:	public LLFolderViewModelItemCommon
{
public:
	virtual const LLUUID& getUUID() const = 0;
	virtual time_t getCreationDate() const = 0;	// UTC seconds
	virtual void setCreationDate(time_t creation_date_utc) = 0;
	virtual PermissionMask getPermissionMask() const = 0;
	virtual LLFolderType::EType getPreferredType() const = 0;
	virtual void showProperties(void) = 0;
	virtual BOOL isItemInTrash( void) const { return FALSE; } // TODO: make   into pure virtual.
	virtual BOOL isUpToDate() const = 0;
	virtual bool hasChildren() const = 0;
	virtual LLInventoryType::EType getInventoryType() const = 0;
	virtual void performAction(LLInventoryModel* model, std::string action)   = 0;
	virtual LLWearableType::EType getWearableType() const = 0;
	virtual EInventorySortGroup getSortGroup() const = 0;
	virtual LLInventoryObject* getInventoryObject() const = 0;
};

class LLInventorySort
{
public:
	LLInventorySort(U32 order)
		:	mSortOrder(0),
		mByDate(false),
		mSystemToTop(false),
		mFoldersByName(false)
	{
		mSortOrder = order;
		mByDate = (order & LLInventoryFilter::SO_DATE);
		mSystemToTop = (order & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP);
		mFoldersByName = (order & LLInventoryFilter::SO_FOLDERS_BY_NAME);
	}

	bool isByDate() { return mByDate; }
	U32 getSortOrder() { return mSortOrder; }

	bool operator()(const LLFolderViewModelItemInventory* const& a, const LLFolderViewModelItemInventory* const& b);
private:
	U32  mSortOrder;
	bool mByDate;
	bool mSystemToTop;
	bool mFoldersByName;
};

class LLFolderViewModelInventory
	:	public LLFolderViewModel<LLInventorySort,   LLFolderViewModelItemInventory, LLFolderViewModelItemInventory,   LLInventoryFilter>
{
	typedef LLFolderViewModel<LLInventorySort,   LLFolderViewModelItemInventory, LLFolderViewModelItemInventory,   LLInventoryFilter> base_t;

	virtual ~LLFolderViewModelInventory() {}

	void sort(LLFolderViewFolder* folder);
	void requestSort(LLFolderViewFolder* folder);
};


class LLInventoryPanel : public LLPanel
{
	//--------------------------------------------------------------------
	// Data
	//--------------------------------------------------------------------
public:
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
		Optional<bool>						show_item_link_overlays;
		Optional<Filter>					filter;
		Optional<std::string>               start_folder;
		Optional<bool>						use_label_suffix;
		Optional<bool>						show_empty_message;
		Optional<LLScrollContainer::Params>	scroll;
		Optional<bool>						accepts_drag_and_drop;

		Params()
		:	sort_order_setting("sort_order_setting"),
			inventory("", &gInventory),
			allow_multi_select("allow_multi_select", true),
			show_item_link_overlays("show_item_link_overlays", false),
			filter("filter"),
			start_folder("start_folder"),
			use_label_suffix("use_label_suffix", true),
			show_empty_message("show_empty_message", true),
			scroll("scroll"),
			accepts_drag_and_drop("accepts_drag_and_drop")
		{}
	};

	//--------------------------------------------------------------------
	// Initialization
	//--------------------------------------------------------------------
protected:
	LLInventoryPanel(const Params&);
	void initFromParams(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual ~LLInventoryPanel();

public:
	LLInventoryModel* getModel() { return mInventory; }

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

	// LLBadgeHolder methods
	bool addBadge(LLBadge * badge);

	// Call this method to set the selection.
	void openAllFolders();
	void setSelection(const LLUUID& obj_id, BOOL take_keyboard_focus);
	void setSelectCallback(const boost::function<void (const std::deque<LLFolderViewItem*>& items, BOOL user_action)>& cb);
	void clearSelection();
	LLInventoryFilter* getFilter();
	const LLInventoryFilter* getFilter() const;
	void setFilterTypes(U64 filter, LLInventoryFilter::EFilterType = LLInventoryFilter::FILTERTYPE_OBJECT);
	U32 getFilterObjectTypes() const;
	void setFilterPermMask(PermissionMask filter_perm_mask);
	U32 getFilterPermMask() const;
	void setFilterWearableTypes(U64 filter);
	void setFilterSubString(const std::string& string);
	const std::string getFilterSubString();
	void setSinceLogoff(BOOL sl);
	void setHoursAgo(U32 hours);
	BOOL getSinceLogoff();
	void setFilterLinks(U64 filter_links);

	void setShowFolderState(LLInventoryFilter::EFolderShow show);
	LLInventoryFilter::EFolderShow getShowFolderState();
	// This method is called when something has changed about the inventory.
	void modelChanged(U32 mask);
	LLFolderView* getRootFolder();
	LLScrollContainer* getScrollableContainer() { return mScroller; }
	
	void onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action);
	
	LLHandle<LLInventoryPanel> getInventoryPanelHandle() const { return getDerivedHandle<LLInventoryPanel>(); }

	// Callbacks
	void doToSelected(const LLSD& userdata);
	void doCreate(const LLSD& userdata);
	bool beginIMSession();
	bool attachObject(const LLSD& userdata);
	
	// DEBUG ONLY:
	static void dumpSelectionInformation(void* user_data);

	void openSelected();
	void unSelectAll();
	
	static void onIdle(void* user_data);

	// Find whichever inventory panel is active / on top.
	// "Auto_open" determines if we open an inventory panel if none are open.
	static LLInventoryPanel *getActiveInventoryPanel(BOOL auto_open = TRUE);
	
	static void openInventoryPanelAndSetSelection(BOOL auto_open, const LLUUID& obj_id);

	void addItemID(const LLUUID& id, LLFolderViewItem* itemp);
	void removeItemID(const LLUUID& id);
	LLFolderViewItem* getItemByID(const LLUUID& id);
	LLFolderViewFolder* getFolderByID(const LLUUID& id);
	void setSelectionByID(const LLUUID& obj_id, BOOL take_keyboard_focus);
	void updateSelection();
	 	
	LLFolderViewModelInventory* getFolderViewModel() { return &mViewModel; }
	const LLFolderViewModelInventory* getFolderViewModel() const { return   &mViewModel; }

protected:
	void openStartFolderOrMyInventory(); // open the first level of inventory
	void onItemsCompletion();			// called when selected items are complete

        LLUUID						mSelectThisID;	
	LLInventoryModel*			mInventory;
	LLInventoryObserver*		mInventoryObserver;
	LLInvPanelComplObserver*	mCompletionObserver;
	BOOL						mAcceptsDragAndDrop;
	BOOL 						mAllowMultiSelect;
	BOOL 						mShowItemLinkOverlays; // Shows link graphic over inventory item icons
	BOOL						mShowEmptyMessage;

	LLFolderView*				mFolderRoot;
	LLScrollContainer*			mScroller;

	LLFolderViewModelInventory	mViewModel;
	
	std::map<LLUUID, LLFolderViewItem*> mItemMap;
	/**
	 * Pointer to LLInventoryFVBridgeBuilder.
	 *
	 * It is set in LLInventoryPanel's constructor and can be overridden in derived classes with 
	 * another implementation.
	 * Take into account it will not be deleted by LLInventoryPanel itself.
	 */
	const LLInventoryFVBridgeBuilder* mInvFVBridgeBuilder;


	//--------------------------------------------------------------------
	// Sorting
	//--------------------------------------------------------------------
public:
	static const std::string DEFAULT_SORT_ORDER;
	static const std::string RECENTITEMS_SORT_ORDER;
	static const std::string INHERIT_SORT_ORDER;
	
	void setSortOrder(U32 order);
	U32 getSortOrder() const;

private:
	std::string					mSortOrderSetting;
	int							mClipboardState;

	//--------------------------------------------------------------------
	// Hidden folders
	//--------------------------------------------------------------------
public:
	void addHideFolderType(LLFolderType::EType folder_type);

public:
	BOOL 				getIsViewsInitialized() const { return mViewsInitialized; }
	const LLUUID&		getRootFolderID() const;
protected:
	// Builds the UI.  Call this once the inventory is usable.
	void 				initializeViews();
	LLFolderViewItem*	rebuildViewsFor(const LLUUID& id); // Given the id and the parent, build all of the folder views.

	virtual void		buildFolderView(const LLInventoryPanel::Params& params);
	LLFolderViewItem*	buildNewViews(const LLUUID& id);
	BOOL				getIsHiddenFolderType(LLFolderType::EType folder_type) const;
	
	virtual LLFolderView*		createFolderView(LLInvFVBridge * bridge, bool useLabelSuffix);
	virtual LLFolderViewFolder*	createFolderViewFolder(LLInvFVBridge * bridge);
	virtual LLFolderViewItem*	createFolderViewItem(LLInvFVBridge * bridge);
private:
	BOOL				mBuildDefaultHierarchy; // default inventory hierarchy should be created in postBuild()
	BOOL				mViewsInitialized; // Views have been generated
	// UUID of category from which hierarchy should be built.  Set with the 
	// "start_folder" xml property.  Default is LLUUID::null that means total Inventory hierarchy. 
	LLUUID				mStartFolderID;
};

#endif // LL_LLINVENTORYPANEL_H
