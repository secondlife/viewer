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
#include "llfolderviewitem.h"
#include "llfolderviewmodelinventory.h"
#include "llfloater.h"
#include "llinventory.h"
#include "llinventoryfilter.h"
#include "llinventorymodel.h"
#include "llscrollcontainer.h"
#include "lluictrlfactory.h"
#include <set>

class LLInvFVBridge;
class LLInventoryFolderViewModelBuilder;
class LLInvPanelComplObserver;
class LLFolderViewModelInventory;
class LLFolderViewGroupedItemBridge;

namespace LLInitParam
{
	template<>
	struct TypeValues<LLFolderType::EType> : public TypeValuesHelper<LLFolderType::EType>
	{
		static void declareValues();
	};
}

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

	struct StartFolder : public LLInitParam::ChoiceBlock<StartFolder>
	{
		Alternative<std::string>			name;
		Alternative<LLUUID>					id;
		Alternative<LLFolderType::EType>	type;

		StartFolder()
		:	name("name"), 
			id("id"),
			type("type")
		{}
	};

	struct Params 
	:	public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<std::string>				sort_order_setting;
		Optional<LLInventoryModel*>			inventory;
		Optional<bool>						allow_multi_select;
		Optional<bool>						allow_drag;
		Optional<bool>						show_item_link_overlays;
		Optional<Filter>					filter;
		Optional<StartFolder>               start_folder;
		Optional<bool>						use_label_suffix;
		Optional<bool>						show_empty_message;
		Optional<bool>						suppress_folder_menu;
		Optional<bool>						show_root_folder;
		Optional<bool>						allow_drop_on_root;
		Optional<bool>						use_marketplace_folders;
		Optional<LLScrollContainer::Params>	scroll;
		Optional<bool>						accepts_drag_and_drop;
		Optional<LLFolderView::Params>		folder_view;
		Optional<LLFolderViewFolder::Params> folder;
		Optional<LLFolderViewItem::Params>	 item;

        // All item and folder views will be initialized on init if true (default)
        // Will initialize on visibility change otherwise.
        Optional<bool>						preinitialize_views;

		Params()
		:	sort_order_setting("sort_order_setting"),
			inventory("", &gInventory),
			allow_multi_select("allow_multi_select", true),
			allow_drag("allow_drag", true),
			show_item_link_overlays("show_item_link_overlays", false),
			suppress_folder_menu("suppress_folder_menu", false),
			filter("filter"),
			start_folder("start_folder"),
			use_label_suffix("use_label_suffix", true),
            show_empty_message("show_empty_message", true),
            show_root_folder("show_root_folder", false),
            allow_drop_on_root("allow_drop_on_root", true),
            use_marketplace_folders("use_marketplace_folders", false),
			scroll("scroll"),
			accepts_drag_and_drop("accepts_drag_and_drop"),
			folder_view("folder_view"),
			folder("folder"),
			item("item"),
			preinitialize_views("preinitialize_views", true)
		{}
	};

	struct InventoryState : public LLInitParam::Block<InventoryState>
	{
		Mandatory<LLInventoryFilter::Params> filter;
		Mandatory<LLInventorySort::Params> sort;
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
    typedef std::set<LLFolderViewItem*> selected_items_t;

	LLInventoryModel* getModel() { return mInventory; }
	LLFolderViewModelInventory& getRootViewModel() { return mInventoryViewModel; }

	// LLView methods
	/*virtual*/ void onVisibilityChange(BOOL new_visibility);
	void draw();
	/*virtual*/ BOOL handleKeyHere( KEY key, MASK mask );
	BOOL handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
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
    selected_items_t getSelectedItems() const;

	bool isSelectionRemovable();
	LLInventoryFilter& getFilter();
	const LLInventoryFilter& getFilter() const;
	void setFilterTypes(U64 filter, LLInventoryFilter::EFilterType = LLInventoryFilter::FILTERTYPE_OBJECT);
	void setFilterWorn();
	U32 getFilterObjectTypes() const;
	void setFilterPermMask(PermissionMask filter_perm_mask);
	U32 getFilterPermMask() const;
    void setFilterWearableTypes(U64 filter);
    void setFilterSettingsTypes(U64 filter);
    void setFilterSubString(const std::string& string);
	const std::string getFilterSubString();
	void setSinceLogoff(BOOL sl);
	void setHoursAgo(U32 hours);
	void setDateSearchDirection(U32 direction);
	BOOL getSinceLogoff();
	void setFilterLinks(U64 filter_links);
	void setSearchType(LLInventoryFilter::ESearchType type);
	LLInventoryFilter::ESearchType getSearchType();

	void setShowFolderState(LLInventoryFilter::EFolderShow show);
	LLInventoryFilter::EFolderShow getShowFolderState();
	// This method is called when something has changed about the inventory.
	void modelChanged(U32 mask);
	LLFolderView* getRootFolder() { return mFolderRoot.get(); }
	LLUUID getRootFolderID();
	LLScrollContainer* getScrollableContainer() { return mScroller; }
    bool getAllowDropOnRoot() { return mParams.allow_drop_on_root; }
	
	void onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action);
	
	LLHandle<LLInventoryPanel> getInventoryPanelHandle() const { return getDerivedHandle<LLInventoryPanel>(); }

	// Callbacks
	void doToSelected(const LLSD& userdata);
	void doCreate(const LLSD& userdata);
	bool beginIMSession();
	void fileUploadLocation(const LLSD& userdata);
	void purgeSelectedItems();
	bool attachObject(const LLSD& userdata);
	static void idle(void* user_data);

	void updateFolderLabel(const LLUUID& folder_id);

	// DEBUG ONLY:
	static void dumpSelectionInformation(void* user_data);

	void openSelected();
	void unSelectAll();
	
	static void onIdle(void* user_data);

	// Find whichever inventory panel is active / on top.
	// "Auto_open" determines if we open an inventory panel if none are open.
	static LLInventoryPanel *getActiveInventoryPanel(BOOL auto_open = TRUE);

	static void openInventoryPanelAndSetSelection(BOOL auto_open,
													const LLUUID& obj_id,
													BOOL main_panel = FALSE,
													BOOL take_keyboard_focus = TAKE_FOCUS_YES,
													BOOL reset_filter = FALSE);

	void addItemID(const LLUUID& id, LLFolderViewItem* itemp);
	void removeItemID(const LLUUID& id);
	LLFolderViewItem* getItemByID(const LLUUID& id);
	LLFolderViewFolder* getFolderByID(const LLUUID& id);
	void setSelectionByID(const LLUUID& obj_id, BOOL take_keyboard_focus);
	void updateSelection();

	void setSuppressOpenItemAction(bool supress_open_item) { mSuppressOpenItemAction = supress_open_item; }

	LLFolderViewModelInventory* getFolderViewModel() { return &mInventoryViewModel; }
	const LLFolderViewModelInventory* getFolderViewModel() const { return &mInventoryViewModel; }
    
    // Clean up stuff when the folder root gets deleted
    void clearFolderRoot();

    void callbackPurgeSelectedItems(const LLSD& notification, const LLSD& response);

protected:
	void openStartFolderOrMyInventory(); // open the first level of inventory
	void onItemsCompletion();			// called when selected items are complete

    LLUUID						mSelectThisID;	
	LLInventoryModel*			mInventory;
	LLInventoryObserver*		mInventoryObserver;
	LLInvPanelComplObserver*	mCompletionObserver;
	bool						mAcceptsDragAndDrop;
	bool 						mAllowMultiSelect;
	bool 						mAllowDrag;
	bool 						mShowItemLinkOverlays; // Shows link graphic over inventory item icons
	bool						mShowEmptyMessage;
	bool						mSuppressFolderMenu;
	bool						mSuppressOpenItemAction;

	LLHandle<LLFolderView>      mFolderRoot;
	LLScrollContainer*			mScroller;

	LLUUID						mPreviousSelectedFolder;

	LLFolderViewModelInventory	mInventoryViewModel;
    LLPointer<LLFolderViewGroupedItemBridge> mGroupedItemBridge;
	Params						mParams;	// stored copy of parameter block

	std::map<LLUUID, LLFolderViewItem*> mItemMap;
	/**
	 * Pointer to LLInventoryFolderViewModelBuilder.
	 *
	 * It is set in LLInventoryPanel's constructor and can be overridden in derived classes with 
	 * another implementation.
	 * Take into account it will not be deleted by LLInventoryPanel itself.
	 */
	const LLInventoryFolderViewModelBuilder* mInvFVBridgeBuilder;


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
	bool getViewsInitialized() const { return mViewsInitialized == VIEWS_INITIALIZED; }
protected:
	// Builds the UI.  Call this once the inventory is usable.
	void 				initializeViews();

	// Specific inventory colors
	static bool                 sColorSetInitialized;
	static LLUIColor			sDefaultColor;
	static LLUIColor			sDefaultHighlightColor;
	static LLUIColor			sLibraryColor;
	static LLUIColor			sLinkColor;
	
	LLFolderViewItem*			buildNewViews(const LLUUID& id);
    LLFolderViewItem*			buildNewViews(const LLUUID& id,
                                              LLInventoryObject const* objectp);
    LLFolderViewItem*			buildNewViews(const LLUUID& id,
                                              LLInventoryObject const* objectp,
                                              LLFolderViewItem *target_view);
    // if certain types are not allowed, no reason to create views
    virtual bool				typedViewsFilter(const LLUUID& id, LLInventoryObject const* objectp) { return true; }

	virtual void				itemChanged(const LLUUID& item_id, U32 mask, const LLInventoryObject* model_item);
	BOOL				getIsHiddenFolderType(LLFolderType::EType folder_type) const;
	
    virtual LLFolderView * createFolderRoot(LLUUID root_id );
	virtual LLFolderViewFolder*	createFolderViewFolder(LLInvFVBridge * bridge, bool allow_drop);
	virtual LLFolderViewItem*	createFolderViewItem(LLInvFVBridge * bridge);
private:
    // buildViewsTree does not include some checks and is meant
    // for recursive use, use buildNewViews() for first call
    LLFolderViewItem*			buildViewsTree(const LLUUID& id,
                                              const LLUUID& parent_id,
                                              LLInventoryObject const* objectp,
                                              LLFolderViewItem *target_view,
                                              LLFolderViewFolder *parent_folder_view);

    typedef enum e_views_initialization_state
    {
        VIEWS_UNINITIALIZED = 0,
        VIEWS_INITIALIZING,
        VIEWS_INITIALIZED,
    } EViewsInitializationState;

	bool						mBuildViewsOnInit;
    EViewsInitializationState	mViewsInitialized; // Whether views have been generated
};


class LLInventoryFavoriteItemsPanel : public LLInventoryPanel
{
public:
    struct Params : public LLInitParam::Block<Params, LLInventoryPanel::Params>
    {};

    void initFromParams(const Params& p);
    bool isSelectionRemovable() { return false; }
    void setSelectCallback(const boost::function<void(const std::deque<LLFolderViewItem*>& items, BOOL user_action)>& cb);

protected:
    LLInventoryFavoriteItemsPanel(const Params& params);
    ~LLInventoryFavoriteItemsPanel() { mFolderChangedSignal.disconnect(); }
    void updateFavoritesRootFolder();

    boost::signals2::connection mFolderChangedSignal;
    boost::function<void(const std::deque<LLFolderViewItem*>& items, BOOL user_action)> mSelectionCallback;
    friend class LLUICtrlFactory;
};

/************************************************************************/
/* Asset Pre-Filtered Inventory Panel related class                     */
/* Exchanges filter's flexibility for speed of generation and           */
/* improved performance                                                 */
/************************************************************************/

class LLAssetFilteredInventoryPanel : public LLInventoryPanel
{
public:
    struct Params
        : public LLInitParam::Block<Params, LLInventoryPanel::Params>
    {
        Mandatory<std::string>	filter_asset_type;

        Params() : filter_asset_type("filter_asset_type") {}
    };

    void initFromParams(const Params& p);
protected:
    LLAssetFilteredInventoryPanel(const Params& p) : LLInventoryPanel(p) {}
    friend class LLUICtrlFactory;
public:
    ~LLAssetFilteredInventoryPanel() {}

    /*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
        EDragAndDropType cargo_type,
        void* cargo_data,
        EAcceptance* accept,
        std::string& tooltip_msg) override;

protected:
    /*virtual*/ bool				typedViewsFilter(const LLUUID& id, LLInventoryObject const* objectp) override;
    /*virtual*/ void				itemChanged(const LLUUID& item_id, U32 mask, const LLInventoryObject* model_item) override;

private:
    LLAssetType::EType mAssetType;
};

#endif // LL_LLINVENTORYPANEL_H
