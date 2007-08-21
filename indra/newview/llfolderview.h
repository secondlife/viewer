/** 
 * @file llfolderview.h
 * @brief Definition of the folder view collection of classes.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/**
 *
 * The folder view collection of classes provides an interface for
 * making a 'folder view' similar to the way the a single pane file
 * folder interface works.
 *
 */

#ifndef LL_LLFOLDERVIEW_H
#define LL_LLFOLDERVIEW_H

#include <vector>
#include <map>
#include <deque>

#include "lluictrl.h"
#include "v4color.h"
#include "lldarray.h"
//#include "llviewermenu.h"
#include "stdenums.h"
#include "llfontgl.h"
#include "lleditmenuhandler.h"
#include "llviewerimage.h"
#include "lldepthstack.h"
#include "lltooldraganddrop.h"

class LLMenuGL;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFolderViewEventListener
//
// This is an abstract base class that users of the folderview classes
// would use to catch the useful events emitted from the folder
// views.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


class LLFolderViewItem;
class LLFolderView;
class LLInventoryModel;
class LLScrollableContainerView;
typedef BOOL (*LLFolderSearchFunction)(LLFolderViewItem* first_item, const char *find_text, BOOL backward);

class LLFolderViewEventListener
{
public:
	virtual ~LLFolderViewEventListener( void ) {}
	virtual const LLString& getName() const = 0;
	virtual const LLString& getDisplayName() const = 0;
	virtual const LLUUID& getUUID() const = 0;
	virtual U32 getCreationDate() const = 0;	// UTC seconds
	virtual PermissionMask getPermissionMask() const = 0;
	virtual LLViewerImage* getIcon() const = 0;
	virtual LLFontGL::StyleFlags getLabelStyle() const = 0;
	virtual LLString getLabelSuffix() const = 0;
	virtual void openItem( void ) = 0;
	virtual void previewItem( void ) = 0;
	virtual void selectItem(void) = 0;
	virtual void showProperties(void) = 0;
	virtual BOOL isItemRenameable() const = 0;
	virtual BOOL renameItem(const LLString& new_name) = 0;
	virtual BOOL isItemMovable( void ) = 0;		// Can be moved to another folder
	virtual BOOL isItemRemovable( void ) = 0;	// Can be destroyed
	virtual BOOL removeItem() = 0;
	virtual void removeBatch(LLDynamicArray<LLFolderViewEventListener*>& batch) = 0;
	virtual void move( LLFolderViewEventListener* parent_listener ) = 0;
	virtual BOOL isItemCopyable() const = 0;
	virtual BOOL copyToClipboard() const = 0;
	virtual void cutToClipboard() = 0;
	virtual BOOL isClipboardPasteable() const = 0;
	virtual void pasteFromClipboard() = 0;
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags) = 0;
	virtual BOOL isUpToDate() const = 0;
	virtual BOOL hasChildren() const = 0;
	virtual LLInventoryType::EType getInventoryType() const = 0;
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, LLString action) {}

	// This method should be called when a drag begins. returns TRUE
	// if the drag can begin, otherwise FALSE.
	virtual BOOL startDrag(EDragAndDropType* type, LLUUID* id) = 0;

	// This method will be called to determine if a drop can be
	// performed, and will set drop to TRUE if a drop is
	// requested. Returns TRUE if a drop is possible/happened,
	// otherwise FALSE.
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data) = 0;

	// This method is called when the object being referenced by the
	// bridge is actually dropped. This allows for cleanup of the old
	// view, reference counting, etc.
//	virtual void dropped() = 0;

	// this method accesses the parent and arranges and sets it as
	// specified.
	void arrangeAndSet(LLFolderViewItem* focus, BOOL set_selection,
		BOOL take_keyboard_focus = TRUE);
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFolderViewListenerFunctor
//
// This simple abstract base class can be used to applied to all
// listeners in a hierarchy.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFolderViewListenerFunctor
{
public:
	virtual ~LLFolderViewListenerFunctor() {}
	virtual void operator()(LLFolderViewEventListener* listener) = 0;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFolderViewFunctor
//
// Simple abstract base class for applying a functor to folders and
// items in a folder view hierarchy. This is suboptimal for algorithms
// that only work folders or only work on items, but I'll worry about
// that later when it's determined to be too slow.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFolderViewItem;
class LLFolderViewFolder;

class LLFolderViewFunctor
{
public:
	virtual ~LLFolderViewFunctor() {}
	virtual void doFolder(LLFolderViewFolder* folder) = 0;
	virtual void doItem(LLFolderViewItem* item) = 0;
};

class LLInventoryFilter
{
public:
	typedef enum e_folder_show
	{
		SHOW_ALL_FOLDERS,
		SHOW_NON_EMPTY_FOLDERS,
		SHOW_NO_FOLDERS
	} EFolderShow;

	typedef enum e_filter_behavior
	{
		FILTER_NONE,				// nothing to do, already filtered
		FILTER_RESTART,				// restart filtering from scratch
		FILTER_LESS_RESTRICTIVE,	// existing filtered items will certainly pass this filter
		FILTER_MORE_RESTRICTIVE		// if you didn't pass the previous filter, you definitely won't pass this one
	} EFilterBehavior;

	static const U32 SO_DATE = 1;
	static const U32 SO_FOLDERS_BY_NAME = 2;
	static const U32 SO_SYSTEM_FOLDERS_TO_TOP = 4;

	LLInventoryFilter(const LLString& name);
	virtual ~LLInventoryFilter();

	void setFilterTypes(U32 types);
	U32 getFilterTypes() const { return mFilterOps.mFilterTypes; }
	
	void setFilterSubString(const LLString& string);
	const LLString getFilterSubString(BOOL trim = FALSE);

	void setFilterPermissions(PermissionMask perms);
	PermissionMask getFilterPermissions() const { return mFilterOps.mPermissions; }

	void setDateRange(U32 min_date, U32 max_date);
	void setDateRangeLastLogoff(BOOL sl);
	U32 getMinDate() const { return mFilterOps.mMinDate; }
	U32 getMaxDate() const { return mFilterOps.mMaxDate; }

	void setHoursAgo(U32 hours);
	U32 getHoursAgo() const { return mFilterOps.mHoursAgo; }

	void setShowFolderState( EFolderShow state);
	EFolderShow getShowFolderState() { return mFilterOps.mShowFolderState; }

	void setSortOrder(U32 order);
	U32 getSortOrder() { return mOrder; }

	BOOL check(LLFolderViewItem* item);
	std::string::size_type getStringMatchOffset() const;
	BOOL isActive();
	BOOL isNotDefault();
	BOOL isModified();
	BOOL isModifiedAndClear();
	BOOL isSinceLogoff();
	void clearModified() { mModified = FALSE; mFilterBehavior = FILTER_NONE; }
	const LLString getName() const { return mName; }
	LLString getFilterText();

	void setFilterCount(S32 count) { mFilterCount = count; }
	S32 getFilterCount() { return mFilterCount; }
	void decrementFilterCount() { mFilterCount--; }
	
	void markDefault();
	void resetDefault();

	BOOL isFilterWith(LLInventoryType::EType t);

	S32 getCurrentGeneration() const { return mFilterGeneration; }
	S32 getMinRequiredGeneration() const { return mMinRequiredGeneration; }
	S32 getMustPassGeneration() const { return mMustPassGeneration; }

	//RN: this is public to allow system to externally force a global refilter
	void setModified(EFilterBehavior behavior = FILTER_RESTART);

	void toLLSD(LLSD& data);
	void fromLLSD(LLSD& data);

protected:
	struct filter_ops
	{
		U32			mFilterTypes;
		U32			mMinDate;
		U32			mMaxDate;
		U32			mHoursAgo;
		EFolderShow	mShowFolderState;
		PermissionMask	mPermissions;
	};
	filter_ops		mFilterOps;
	filter_ops		mDefaultFilterOps;
	std::string::size_type	mSubStringMatchOffset;
	LLString		mFilterSubString;
	U32				mOrder;
	const LLString	mName;
	S32				mFilterGeneration;
	S32				mMustPassGeneration;
	S32				mMinRequiredGeneration;
	S32				mFilterCount;
	S32				mNextFilterGeneration;
	EFilterBehavior mFilterBehavior;

private:
	U32 mLastLogoff;
	BOOL mModified;
	BOOL mNeedTextRebuild;
	LLString mFilterText;
};

// These are grouping of inventory types.
// Order matters when sorting system folders to the top.
enum EInventorySortGroup
{ 
	SG_SYSTEM_FOLDER, 
	SG_TRASH_FOLDER, 
	SG_NORMAL_FOLDER, 
	SG_ITEM 
};

class LLInventorySort
{
public:
	LLInventorySort() 
		: mSortOrder(0) { }
	
	// Returns true if order has changed
	bool updateSort(U32 order);
	U32 getSort() { return mSortOrder; }

	bool operator()(LLFolderViewItem* a, LLFolderViewItem* b);
private:
	U32  mSortOrder;
	bool mByDate;
	bool mSystemToTop;
	bool mFoldersByName;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFolderViewItem
//
// An instance of this class represents a single item in a folder view
// such as an inventory item or a file.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFontGL;
class LLFolderViewFolder;
class LLFolderView;

class LLFolderViewItem : public LLUICtrl
{
protected:
	friend class LLFolderViewEventListener;

	static const LLFontGL*		sFont;
	static const LLFontGL*		sSmallFont;
	static LLColor4				sFgColor;
	static LLColor4				sHighlightBgColor;
	static LLColor4				sHighlightFgColor;
	static LLColor4				sFilterBGColor;
	static LLColor4				sFilterTextColor;

	LLString					mLabel;
	LLString					mSearchableLabel;
	LLString					mType;
	S32							mLabelWidth;
	U32							mCreationDate;
	LLFolderViewFolder*			mParentFolder;
	LLFolderViewEventListener*	mListener;
	BOOL						mIsSelected;
	BOOL						mIsCurSelection;
	BOOL						mSelectPending;
	LLFontGL::StyleFlags		mLabelStyle;
	LLString					mLabelSuffix;
	LLPointer<LLViewerImage>	mIcon;
	LLString					mStatusText;
	BOOL						mHasVisibleChildren;
	S32							mIndentation;
	S32							mNumDescendantsSelected;
	BOOL						mFiltered;
	S32							mLastFilterGeneration;
	std::string::size_type		mStringMatchOffset;
	F32							mControlLabelRotation;
	LLFolderView*				mRoot;
	BOOL						mDragAndDropTarget;
	LLPointer<LLViewerImage>	mArrowImage;
	LLPointer<LLViewerImage>	mBoxImage;
	
	// This function clears the currently selected item, and records
	// the specified selected item appropriately for display and use
	// in the UI. If open is TRUE, then folders are opened up along
	// the way to the selection.
	void setSelectionFromRoot(LLFolderViewItem* selection, BOOL open,		/* Flawfinder: ignore */
		BOOL take_keyboard_focus = TRUE);

	// helper function to change the selection from the root.
	void changeSelectionFromRoot(LLFolderViewItem* selection, BOOL selected);

	// helper function to change the selection from the root.
	void extendSelectionFromRoot(LLFolderViewItem* selection);

	// this is an internal method used for adding items to folders. A
	// no-op at this leve, but reimplemented in derived classes.
	virtual BOOL addItem(LLFolderViewItem*) { return FALSE; }
	virtual BOOL addFolder(LLFolderViewFolder*) { return FALSE; }

public:
	// This function is called when the folder view is dirty. It's
	// implemented here but called by derived classes when folding the
	// views.
	void arrangeFromRoot();
	void filterFromRoot( void );

	// creation_date is in UTC seconds
	LLFolderViewItem( const LLString& name, LLViewerImage* icon, S32 creation_date, LLFolderView* root, LLFolderViewEventListener* listener );
	virtual ~LLFolderViewItem( void );

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	// addToFolder() returns TRUE if it succeeds. FALSE otherwise
	enum { ARRANGE = TRUE, DO_NOT_ARRANGE = FALSE };
	virtual BOOL addToFolder(LLFolderViewFolder* folder, LLFolderView* root);

	virtual EInventorySortGroup getSortGroup();

	// Finds width and height of this object and it's children.  Also
	// makes sure that this view and it's children are the right size.
	virtual S32 arrange( S32* width, S32* height, S32 filter_generation );
	virtual S32 getItemHeight();

	// applies filters to control visibility of inventory items
	virtual void filter( LLInventoryFilter& filter);

	// updates filter serial number and optionally propagated value up to root
	S32		getLastFilterGeneration() { return mLastFilterGeneration; }

	virtual void	dirtyFilter();

	// If the selection is 'this' then note that otherwise
	// ignore. Returns TRUE if this object was affected. If open is
	// TRUE, then folders are opened up along the way to the
	// selection.
	virtual BOOL setSelection(LLFolderViewItem* selection, BOOL open,		/* Flawfinder: ignore */
		BOOL take_keyboard_focus);

	// This method is used to toggle the selection of an item. If
	// selection is 'this', then note selection, and return TRUE.
	virtual BOOL changeSelection(LLFolderViewItem* selection, BOOL selected);

	// this method is used to group select items
	virtual S32 extendSelection(LLFolderViewItem* selection, LLFolderViewItem* last_selected, LLDynamicArray<LLFolderViewItem*>& items){ return FALSE; }

	// this method is used to group select items
	virtual void recursiveDeselect(BOOL deselect_self);

	// gets multiple-element selection
	virtual BOOL getSelectionList(std::set<LLUUID> &selection){return TRUE;}

	// Returns true is this object and all of its children can be removed (deleted by user)
	virtual BOOL isRemovable();

	// Returns true is this object and all of its children can be moved
	virtual BOOL isMovable();

	// destroys this item recursively
	virtual void destroyView();

	S32 getNumSelectedDescendants() { return mNumDescendantsSelected; }

	BOOL isSelected() { return mIsSelected; }

	void setIsCurSelection(BOOL select) { mIsCurSelection = select; }

	BOOL getIsCurSelection() { return mIsCurSelection; }

	BOOL hasVisibleChildren() { return mHasVisibleChildren; }

	// Call through to the viewed object and return true if it can be
	// removed. Returns true if it's removed.
	//virtual BOOL removeRecursively(BOOL single_item);
	BOOL remove();

	// Build an appropriate context menu for the item.	Flags unused.
	void buildContextMenu(LLMenuGL& menu, U32 flags);

	// This method returns the actual name of the thing being
	// viewed. This method will ask the viewed object itself.
	const LLString& getName( void ) const;

	const LLString& getSearchableLabel( void ) const;

	// This method returns the label displayed on the view. This
	// method was primarily added to allow sorting on the folder
	// contents possible before the entire view has been constructed.
	const char* getLabel() const { return mLabel.c_str(); }

	// Used for sorting, like getLabel() above.
	virtual U32 getCreationDate() const { return mCreationDate; }
	
	LLFolderViewFolder* getParentFolder( void );
	LLFolderViewItem* getNextOpenNode( BOOL include_children = TRUE );
	LLFolderViewItem* getPreviousOpenNode( BOOL include_children = TRUE );

	LLFolderViewEventListener* getListener( void );

	// just rename the object.
	void rename(const LLString& new_name);

	// open
	virtual void open( void );		/* Flawfinder: ignore */
	virtual void preview(void);

	// Show children (unfortunate that this is called "open")
	virtual void setOpen(BOOL open = TRUE) {};

	virtual BOOL isOpen() { return FALSE; }

	LLFolderView*	getRoot();
	BOOL			isDescendantOf( const LLFolderViewFolder* potential_ancestor );
	S32				getIndentation() { return mIndentation; }

	virtual void setStatusText(const LLString& text) { mStatusText = text; }

	virtual BOOL	potentiallyVisible(); // do we know for a fact that this item has been filtered out?

	virtual BOOL	getFiltered();
	virtual BOOL	getFiltered(S32 filter_generation);
	virtual void	setFiltered(BOOL filtered, S32 filter_generation);

	// change the icon
	void setIcon(LLViewerImage* icon);

	// refresh information from the object being viewed.
	virtual void refresh();

	virtual void applyListenerFunctorRecursively(LLFolderViewListenerFunctor& functor);

	// LLView functionality
	virtual BOOL handleRightMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleHover( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseUp( S32 x, S32 y, MASK mask );
	virtual BOOL handleDoubleClick( S32 x, S32 y, MASK mask );
	virtual BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);

	//	virtual void handleDropped();
	virtual void draw();
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   LLString& tooltip_msg);
};


// function used for sorting.
typedef bool (*sort_order_f)(LLFolderViewItem* a, LLFolderViewItem* b);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFolderViewFolder
//
// An instance of an LLFolderViewFolder represents a collection of
// more folders and items. This is used to build the hierarchy of
// items found in the folder view. :)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFolderViewFolder : public LLFolderViewItem
{
public:
	typedef enum e_trash
	{
		UNKNOWN, TRASH, NOT_TRASH
	} ETrash;

protected:
	typedef std::vector<LLFolderViewItem*> items_t;
	typedef std::vector<LLFolderViewFolder*> folders_t;
	items_t mItems;
	folders_t mFolders;
	LLInventorySort	mSortFunction;

	BOOL		mIsOpen;
	BOOL		mExpanderHighlighted;
	F32			mCurHeight;
	F32			mTargetHeight;
	F32			mAutoOpenCountdown;
	U32			mSubtreeCreationDate;
	ETrash		mAmTrash;
	S32			mLastArrangeGeneration;
	S32			mLastCalculatedWidth;
	S32			mCompletedFilterGeneration;
	S32			mMostFilteredDescendantGeneration;
public:
	typedef enum e_recurse_type
	{
		RECURSE_NO,
		RECURSE_UP,
		RECURSE_DOWN,
		RECURSE_UP_DOWN
	} ERecurseType;

	LLFolderViewFolder( const LLString& name, LLViewerImage* icon,
						LLFolderView* root,
						LLFolderViewEventListener* listener );
	virtual ~LLFolderViewFolder( void );

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	virtual BOOL	potentiallyVisible();

	LLFolderViewItem* getNextFromChild( LLFolderViewItem*, BOOL include_children = TRUE );
	LLFolderViewItem* getPreviousFromChild( LLFolderViewItem*, BOOL include_children = TRUE  );

	// addToFolder() returns TRUE if it succeeds. FALSE otherwise
	virtual BOOL addToFolder(LLFolderViewFolder* folder, LLFolderView* root);

	// Finds width and height of this object and it's children.  Also
	// makes sure that this view and it's children are the right size.
	virtual S32 arrange( S32* width, S32* height, S32 filter_generation );

	BOOL needsArrange();

	// Returns the sort group (system, trash, folder) for this folder.
	virtual EInventorySortGroup getSortGroup();

	virtual void	setCompletedFilterGeneration(S32 generation, BOOL recurse_up);
	virtual S32		getCompletedFilterGeneration() { return mCompletedFilterGeneration; }

	BOOL hasFilteredDescendants(S32 filter_generation) { return mMostFilteredDescendantGeneration >= filter_generation; }
	BOOL hasFilteredDescendants();

	// applies filters to control visibility of inventory items
	virtual void filter( LLInventoryFilter& filter);
	virtual void setFiltered(BOOL filtered, S32 filter_generation);
	virtual void dirtyFilter();

	// Passes selection information on to children and record
	// selection information if necessary. Returns TRUE if this object
	// (or a child) was affected.
	virtual BOOL setSelection(LLFolderViewItem* selection, BOOL open,		/* Flawfinder: ignore */
		BOOL take_keyboard_focus);

	// This method is used to change the selection of an item. If
	// selection is 'this', then note selection as true. Returns TRUE
	// if this or a child is now selected.
	virtual BOOL changeSelection(LLFolderViewItem* selection, BOOL selected);

	// this method is used to group select items
	virtual S32 extendSelection(LLFolderViewItem* selection, LLFolderViewItem* last_selected, LLDynamicArray<LLFolderViewItem*>& items);

	virtual void recursiveDeselect(BOOL deselect_self);

	// Returns true is this object and all of its children can be removed.
	virtual BOOL isRemovable();

	// Returns true is this object and all of its children can be moved
	virtual BOOL isMovable();

	// destroys this folder, and all children
	virtual void destroyView();

	// If this folder can be removed, remove all children that can be
	// removed, return TRUE if this is empty after the operation and
	// it's viewed folder object can be removed.
	//virtual BOOL removeRecursively(BOOL single_item);
	//virtual BOOL remove();

	// remove the specified item (and any children) if
	// possible. Return TRUE if the item was deleted.
	BOOL removeItem(LLFolderViewItem* item);

	// simply remove the view (and any children) Don't bother telling
	// the listeners.
	void removeView(LLFolderViewItem* item);

	// extractItem() removes the specified item from the folder, but
	// doesn't delete it.
	void extractItem( LLFolderViewItem* item );

	// This function is called by a child that needs to be resorted.
	void resort(LLFolderViewItem* item);

	void setItemSortOrder(U32 ordering);
	void sortBy(U32);
	//BOOL (*func)(LLFolderViewItem* a, LLFolderViewItem* b));

	void setAutoOpenCountdown(F32 countdown) { mAutoOpenCountdown = countdown; }

	// folders can be opened. This will usually be called by internal
	// methods.
	virtual void toggleOpen();

	// Force a folder open or closed
	virtual void setOpen(BOOL open = TRUE);		/* Flawfinder: ignore */

	// Called when a child is refreshed.
	// don't rearrange child folder contents unless explicitly requested
	virtual void requestArrange(BOOL include_descendants = FALSE);

	// internal method which doesn't update the entire view. This
	// method was written because the list iterators destroy the state
	// of other iterations, thus, we can't arrange while iterating
	// through the children (such as when setting which is selected.
	virtual void setOpenArrangeRecursively(BOOL open, ERecurseType recurse = RECURSE_NO);		/* Flawfinder: ignore */

	// Get the current state of the folder.
	virtual BOOL isOpen() { return mIsOpen; }

	// special case if an object is dropped on the child.
	BOOL handleDragAndDropFromChild(MASK mask,
									BOOL drop,
									EDragAndDropType cargo_type,
									void* cargo_data,
									EAcceptance* accept,
									LLString& tooltip_msg);

	void applyFunctorRecursively(LLFolderViewFunctor& functor);
	virtual void applyListenerFunctorRecursively(LLFolderViewListenerFunctor& functor);

	virtual void open( void );		/* Flawfinder: ignore */
	virtual BOOL addItem(LLFolderViewItem* item); 
	virtual BOOL addFolder( LLFolderViewFolder* folder);

	// LLView functionality
	virtual BOOL handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL handleRightMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleDoubleClick( S32 x, S32 y, MASK mask );
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   LLString& tooltip_msg);
	virtual void draw();

	U32 getCreationDate() const;
	bool isTrash();
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFolderView
//
// Th LLFolderView represents the root level folder view object. It
// manages the screen region of the folder view.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLUICtrl;
class LLLineEditor;

class LLFolderView : public LLFolderViewFolder, LLEditMenuHandler
{
public:
	typedef void (*SelectCallback)(const std::deque<LLFolderViewItem*> &items, BOOL user_action, void* data);

	static F32 sAutoOpenTime;

	LLFolderView( const LLString& name, LLViewerImage* root_folder_icon, const LLRect& rect, 
					const LLUUID& source_id, LLView *parent_view );
	virtual ~LLFolderView( void );

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;
	virtual BOOL canFocusChildren() const;

	// FolderViews default to sort by name.  This will change that,
	// and resort the items if necessary.
	void setSortOrder(U32 order);
	void checkTreeResortForModelChanged();
	void setFilterPermMask(PermissionMask filter_perm_mask) { mFilter.setFilterPermissions(filter_perm_mask); }
	void setSelectCallback(SelectCallback callback, void* user_data) { mSelectCallback = callback, mUserData = user_data; }
	void setAllowMultiSelect(BOOL allow) { mAllowMultiSelect = allow; }

	LLInventoryFilter* getFilter() { return &mFilter; }
	const LLString getFilterSubString(BOOL trim = FALSE);
	U32 getFilterTypes() const { return mFilter.getFilterTypes(); }
	PermissionMask getFilterPermissions() const { return mFilter.getFilterPermissions(); }
	LLInventoryFilter::EFolderShow getShowFolderState() { return mFilter.getShowFolderState(); }
	U32 getSortOrder() const;
	BOOL isFilterModified() { return mFilter.isNotDefault(); }
	BOOL getAllowMultiSelect() { return mAllowMultiSelect; }

	// Close all folders in the view
	void closeAllFolders();
	void openFolder(const LLString& foldername);

	virtual void toggleOpen() {};
	virtual void setOpenArrangeRecursively(BOOL open, ERecurseType recurse);		/* Flawfinder: ignore */
	virtual BOOL addFolder( LLFolderViewFolder* folder);

	// Finds width and height of this object and it's children.  Also
	// makes sure that this view and it's children are the right size.
	virtual S32 arrange( S32* width, S32* height, S32 filter_generation );

	void arrangeAll() { mArrangeGeneration++; }
	S32 getArrangeGeneration() { return mArrangeGeneration; }

	// applies filters to control visibility of inventory items
	virtual void filter( LLInventoryFilter& filter);

	// get the last selected item
	virtual LLFolderViewItem* getCurSelectedItem( void );

	// Record the selected item and pass it down the hierachy.
	virtual BOOL setSelection(LLFolderViewItem* selection, BOOL open,		/* Flawfinder: ignore */
		BOOL take_keyboard_focus);

	// This method is used to toggle the selection of an item. Walks
	// children, and keeps track of selected objects.
	virtual BOOL changeSelection(LLFolderViewItem* selection, BOOL selected);

	virtual S32 extendSelection(LLFolderViewItem* selection, LLFolderViewItem* last_selected, LLDynamicArray<LLFolderViewItem*>& items);

	virtual BOOL getSelectionList(std::set<LLUUID> &selection);

	// make sure if ancestor is selected, descendents are not
	void sanitizeSelection();
	void clearSelection();
	void addToSelectionList(LLFolderViewItem* item);
	void removeFromSelectionList(LLFolderViewItem* item);

	BOOL startDrag(LLToolDragAndDrop::ESource source);
	void setDragAndDropThisFrame() { mDragAndDropThisFrame = TRUE; }

	// deletion functionality
 	void removeSelectedItems();

	// open the selected item.
	void openSelectedItems( void );
	void propertiesSelectedItems( void );

	void autoOpenItem(LLFolderViewFolder* item);
	void closeAutoOpenedFolders();
	BOOL autoOpenTest(LLFolderViewFolder* item);

	// copy & paste
	virtual void	copy();
	virtual BOOL	canCopy();

	virtual void	cut();
	virtual BOOL	canCut();

	virtual void	paste();
	virtual BOOL	canPaste();

	virtual void	doDelete();
	virtual BOOL	canDoDelete();

	// public rename functionality - can only start the process
	void startRenamingSelectedItem( void );

	// These functions were used when there was only one folderview,
	// and relied on that concept. This functionality is now handled
	// by the listeners and the lldraganddroptool.
	//LLFolderViewItem*	getMovingItem() { return mMovingItem; }
	//void setMovingItem( LLFolderViewItem* item ) { mMovingItem = item; }
	//void				dragItemIntoFolder( LLFolderViewItem* moving_item, LLFolderViewFolder* dst_folder, BOOL drop, BOOL* accept );
	//void				dragFolderIntoFolder( LLFolderViewFolder* moving_folder, LLFolderViewFolder* dst_folder, BOOL drop, BOOL* accept );

	// LLUICtrl Functionality
	/*virtual*/ void setFocus(BOOL focus);

	// LLView functionality
	///*virtual*/ BOOL handleKey( KEY key, MASK mask, BOOL called_from_parent );
	/*virtual*/ BOOL handleKeyHere( KEY key, MASK mask, BOOL called_from_parent );
	/*virtual*/ BOOL handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent);
	/*virtual*/ BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleDoubleClick( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleRightMouseDown( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleHover( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   LLString& tooltip_msg);
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	/*virtual*/ void onFocusLost();
	virtual BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual void draw();
	virtual void deleteAllChildren();

	void scrollToShowSelection();
	void scrollToShowItem(LLFolderViewItem* item);
	void setScrollContainer( LLScrollableContainerView* parent ) { mScrollContainer = parent; }
	LLRect getVisibleRect();

	BOOL search(LLFolderViewItem* first_item, const LLString &search_string, BOOL backward);
	void setShowSelectionContext(BOOL show) { mShowSelectionContext = show; }
	BOOL getShowSelectionContext();
	void setShowSingleSelection(BOOL show);
	BOOL getShowSingleSelection() { return mShowSingleSelection; }
	F32  getSelectionFadeElapsedTime() { return mMultiSelectionFadeTimer.getElapsedTimeF32(); }

	void addItemID(const LLUUID& id, LLFolderViewItem* itemp);
	void removeItemID(const LLUUID& id);
	LLFolderViewItem* getItemByID(const LLUUID& id);

	void	doIdle();						// Real idle routine
	static void idle(void* user_data);		// static glue to doIdle()

	BOOL needsAutoSelect() { return mNeedsAutoSelect && !mAutoSelectOverride; }
	BOOL needsAutoRename() { return mNeedsAutoRename; }
	void setNeedsAutoRename(BOOL val) { mNeedsAutoRename = val; }

	BOOL getDebugFilters() { return mDebugFilters; }

	// DEBUG only
	void dumpSelectionInformation();

protected:
	LLScrollableContainerView* mScrollContainer;  // NULL if this is not a child of a scroll container.

	static void commitRename( LLUICtrl* renamer, void* user_data );
	void finishRenamingItem( void );
	void revertRenamingItem( void );

protected:
	LLViewHandle					mPopupMenuHandle;
	
	typedef std::deque<LLFolderViewItem*> selected_items_t;
	selected_items_t				mSelectedItems;
	BOOL							mKeyboardSelection;
	BOOL							mAllowMultiSelect;
	BOOL							mShowFolderHierarchy;
	LLUUID							mSourceID;

	// Renaming variables and methods
	LLFolderViewItem*				mRenameItem;  // The item currently being renamed
	LLLineEditor*					mRenamer;

	BOOL							mNeedsScroll;
	LLFolderViewItem*				mLastScrollItem;
	LLCoordGL						mLastScrollOffset;
	BOOL							mNeedsAutoSelect;
	BOOL							mAutoSelectOverride;
	BOOL							mNeedsAutoRename;
	
	BOOL							mDebugFilters;
	U32								mSortOrder;
	LLDepthStack<LLFolderViewFolder>	mAutoOpenItems;
	LLFolderViewFolder*				mAutoOpenCandidate;
	LLFrameTimer					mAutoOpenTimer;
	LLFrameTimer					mSearchTimer;
	LLString						mSearchString;
	LLInventoryFilter				mFilter;
	BOOL							mShowSelectionContext;
	BOOL							mShowSingleSelection;
	LLFrameTimer					mMultiSelectionFadeTimer;
	S32								mArrangeGeneration;

	void*							mUserData;
	SelectCallback					mSelectCallback;
	BOOL							mSelectionChanged;
	S32								mMinWidth;
	std::map<LLUUID, LLFolderViewItem*> mItemMap;
	BOOL							mDragAndDropThisFrame;

};

bool sort_item_name(LLFolderViewItem* a, LLFolderViewItem* b);
bool sort_item_date(LLFolderViewItem* a, LLFolderViewItem* b);

// Flags for buildContextMenu()
const U32 SUPPRESS_OPEN_ITEM = 0x1;
const U32 FIRST_SELECTED_ITEM = 0x2;

#endif // LL_LLFOLDERVIEW_H
