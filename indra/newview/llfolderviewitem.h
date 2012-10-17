/** 
* @file llfolderviewitem.h
* @brief Items and folders that can appear in a hierarchical folder view
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
#ifndef LLFOLDERVIEWITEM_H
#define LLFOLDERVIEWITEM_H

#include "llview.h"
#include "lldarray.h"  // *TODO: Eliminate, forward declare
#include "lluiimage.h"

class LLFontGL;
class LLFolderView;
class LLFolderViewEventListener;
class LLFolderViewFolder;
class LLFolderViewFunctor;
class LLFolderViewItem;
class LLFolderViewListenerFunctor;
class LLInventoryFilter;
class LLMenuGL;
class LLUIImage;
class LLViewerInventoryItem;

// These are grouping of inventory types.
// Order matters when sorting system folders to the top.
enum EInventorySortGroup
{ 
	SG_SYSTEM_FOLDER, 
	SG_TRASH_FOLDER, 
	SG_NORMAL_FOLDER, 
	SG_ITEM 
};

// *TODO: do we really need one sort object per folder?
// can we just have one of these per LLFolderView ?
class LLInventorySort
{
public:
	LLInventorySort() 
		: mSortOrder(0),
		mByDate(false),
		mSystemToTop(false),
		mFoldersByName(false) { }

	// Returns true if order has changed
	bool updateSort(U32 order);
	U32 getSort() { return mSortOrder; }
	bool isByDate() { return mByDate; }

	bool operator()(const LLFolderViewItem* const& a, const LLFolderViewItem* const& b);
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

class LLFolderViewItem : public LLView
{
public:
	static void initClass();
	static void cleanupClass();

	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<LLUIImage*>					icon;
		Optional<LLUIImage*>					icon_open;  // used for folders
		Optional<LLUIImage*>					icon_overlay;  // for links
		Optional<LLFolderView*>					root;
		Mandatory<LLFolderViewEventListener*>	listener;

		Optional<LLUIImage*>					folder_arrow_image;
		Optional<S32>							folder_indentation; // pixels
		Optional<LLUIImage*>					selection_image;
		Optional<S32>							item_height; // pixels
		Optional<S32>							item_top_pad; // pixels

		Optional<S32>							creation_date; //UTC seconds

		Params();
	};

	// layout constants
	static const S32 LEFT_PAD = 5;
	// LEFT_INDENTATION is set via folder_indentation above
	static const S32 ICON_PAD = 2;
	static const S32 ICON_WIDTH = 16;
	static const S32 TEXT_PAD = 1;
	static const S32 TEXT_PAD_RIGHT = 4;
	static const S32 ARROW_SIZE = 12;
	static const S32 MAX_FOLDER_ITEM_OVERLAP = 2;
	// animation parameters
	static const F32 FOLDER_CLOSE_TIME_CONSTANT;
	static const F32 FOLDER_OPEN_TIME_CONSTANT;

	// Mostly for debugging printout purposes.
	const std::string& getSearchableLabel() { return mSearchableLabel; }
	
	BOOL isLoading() const { return mIsLoading; }

private:
	BOOL						mIsSelected;

protected:
	friend class LLUICtrlFactory;
	friend class LLFolderViewEventListener;

	LLFolderViewItem(const Params& p);

	std::string					mLabel;
	std::string					mSearchableLabel;
	S32							mLabelWidth;
	bool						mLabelWidthDirty;
	time_t						mCreationDate;
	LLFolderViewFolder*			mParentFolder;
	LLFolderViewEventListener*	mListener;
	BOOL						mIsCurSelection;
	BOOL						mSelectPending;
	LLFontGL::StyleFlags		mLabelStyle;
	std::string					mLabelSuffix;
	LLUIImagePtr				mIcon;
	std::string					mStatusText;
	LLUIImagePtr				mIconOpen;
	LLUIImagePtr				mIconOverlay;
	BOOL						mHasVisibleChildren;
	S32							mIndentation;
	S32							mItemHeight;
	BOOL						mPassedFilter;
	S32							mLastFilterGeneration;
	std::string::size_type		mStringMatchOffset;
	F32							mControlLabelRotation;
	LLFolderView*				mRoot;
	BOOL						mDragAndDropTarget;
	BOOL                        mIsLoading;
	LLTimer                     mTimeSinceRequestStart;
	bool						mShowLoadStatus;
	bool						mIsMouseOverTitle;

	// helper function to change the selection from the root.
	void changeSelectionFromRoot(LLFolderViewItem* selection, BOOL selected);

	// this is an internal method used for adding items to folders. A
	// no-op at this level, but reimplemented in derived classes.
	virtual BOOL addItem(LLFolderViewItem*) { return FALSE; }
	virtual BOOL addFolder(LLFolderViewFolder*) { return FALSE; }

	static LLFontGL* getLabelFontForStyle(U8 style);

	virtual void setCreationDate(time_t creation_date_utc)	{ mCreationDate = creation_date_utc; }

public:
	BOOL postBuild();

	// This function clears the currently selected item, and records
	// the specified selected item appropriately for display and use
	// in the UI. If open is TRUE, then folders are opened up along
	// the way to the selection.
	void setSelectionFromRoot(LLFolderViewItem* selection, BOOL openitem,
		BOOL take_keyboard_focus = TRUE);

	// This function is called when the folder view is dirty. It's
	// implemented here but called by derived classes when folding the
	// views.
	void arrangeFromRoot();
	void filterFromRoot( void );
	
	void arrangeAndSet(BOOL set_selection, BOOL take_keyboard_focus);

	virtual ~LLFolderViewItem( void );

	// addToFolder() returns TRUE if it succeeds. FALSE otherwise
	enum { ARRANGE = TRUE, DO_NOT_ARRANGE = FALSE };
	virtual BOOL addToFolder(LLFolderViewFolder* folder, LLFolderView* root);

	virtual EInventorySortGroup getSortGroup() const;

	// Finds width and height of this object and it's children.  Also
	// makes sure that this view and it's children are the right size.
	virtual S32 arrange( S32* width, S32* height, S32 filter_generation );
	virtual S32 getItemHeight();

	// applies filters to control visibility of inventory items
	virtual void filter( LLInventoryFilter& filter);

	// updates filter serial number and optionally propagated value up to root
	S32		getLastFilterGeneration() { return mLastFilterGeneration; }

	virtual void	dirtyFilter();

	// If 'selection' is 'this' then note that otherwise ignore.
	// Returns TRUE if this item ends up being selected.
	virtual BOOL setSelection(LLFolderViewItem* selection, BOOL openitem, BOOL take_keyboard_focus);

	// This method is used to set the selection state of an item.
	// If 'selection' is 'this' then note selection.
	// Returns TRUE if the selection state of this item was changed.
	virtual BOOL changeSelection(LLFolderViewItem* selection, BOOL selected);

	// this method is used to deselect this element
	void deselectItem();

	// this method is used to select this element
	virtual void selectItem();

	// gets multiple-element selection
	virtual std::set<LLUUID> getSelectionList() const;

	// Returns true is this object and all of its children can be removed (deleted by user)
	virtual BOOL isRemovable();

	// Returns true is this object and all of its children can be moved
	virtual BOOL isMovable();

	// destroys this item recursively
	virtual void destroyView();

	BOOL isSelected() const { return mIsSelected; }
	bool isInSelection() const;

	void setUnselected() { mIsSelected = FALSE; }

	void setIsCurSelection(BOOL select) { mIsCurSelection = select; }

	BOOL getIsCurSelection() { return mIsCurSelection; }

	BOOL hasVisibleChildren() { return mHasVisibleChildren; }

	void setShowLoadStatus(bool status) { mShowLoadStatus = status; }

	// Call through to the viewed object and return true if it can be
	// removed. Returns true if it's removed.
	//virtual BOOL removeRecursively(BOOL single_item);
	BOOL remove();

	// Build an appropriate context menu for the item.	Flags unused.
	void buildContextMenu(LLMenuGL& menu, U32 flags);

	// This method returns the actual name of the thing being
	// viewed. This method will ask the viewed object itself.
	const std::string& getName( void ) const;

	const std::string& getSearchableLabel( void ) const;

	// This method returns the label displayed on the view. This
	// method was primarily added to allow sorting on the folder
	// contents possible before the entire view has been constructed.
	const std::string& getLabel() const { return mLabel; }

	// Used for sorting, like getLabel() above.
	virtual time_t getCreationDate() const { return mCreationDate; }

	LLFolderViewFolder* getParentFolder( void ) { return mParentFolder; }
	const LLFolderViewFolder* getParentFolder( void ) const { return mParentFolder; }

	LLFolderViewItem* getNextOpenNode( BOOL include_children = TRUE );
	LLFolderViewItem* getPreviousOpenNode( BOOL include_children = TRUE );

	const LLFolderViewEventListener* getListener( void ) const { return mListener; }
	LLFolderViewEventListener* getListener( void ) { return mListener; }
	
	// Gets the inventory item if it exists (null otherwise)
	LLViewerInventoryItem * getInventoryItem(void);

	// just rename the object.
	void rename(const std::string& new_name);

	// open
	virtual void openItem( void );
	virtual void preview(void);

	// Show children (unfortunate that this is called "open")
	virtual void setOpen(BOOL open = TRUE) {};

	virtual BOOL isOpen() const { return FALSE; }

	virtual LLFolderView*	getRoot();
	BOOL			isDescendantOf( const LLFolderViewFolder* potential_ancestor );
	S32				getIndentation() { return mIndentation; }

	virtual BOOL	potentiallyVisible(); // do we know for a fact that this item won't be displayed?
	virtual BOOL	potentiallyFiltered(); // do we know for a fact that this item has been filtered out?

	virtual BOOL	getFiltered();
	virtual BOOL	getFiltered(S32 filter_generation);
	virtual void	setFiltered(BOOL filtered, S32 filter_generation);

	// change the icon
	void setIcon(LLUIImagePtr icon);

	// refresh information from the object being viewed.
	void refreshFromListener();
	virtual void refresh();

	virtual void applyListenerFunctorRecursively(LLFolderViewListenerFunctor& functor);

	// LLView functionality
	virtual BOOL handleRightMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleHover( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseUp( S32 x, S32 y, MASK mask );
	virtual BOOL handleDoubleClick( S32 x, S32 y, MASK mask );

	virtual void onMouseLeave(S32 x, S32 y, MASK mask);

	virtual LLView* findChildView(const std::string& name, BOOL recurse) const { return NULL; }

	//	virtual void handleDropped();
	virtual void draw();
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
		EDragAndDropType cargo_type,
		void* cargo_data,
		EAcceptance* accept,
		std::string& tooltip_msg);

private:
	static std::map<U8, LLFontGL*> sFonts; // map of styles to fonts
};


// function used for sorting.
typedef bool (*sort_order_f)(LLFolderViewItem* a, LLFolderViewItem* b);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFolderViewFolder
//
// An instance of an LLFolderViewFolder represents a collection of
// more folders and items. This is used to build the hierarchy of
// items found in the folder view.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFolderViewFolder : public LLFolderViewItem
{
protected:
	LLFolderViewFolder( const LLFolderViewItem::Params& );
	friend class LLUICtrlFactory;

public:
	typedef enum e_trash
	{
		UNKNOWN, TRASH, NOT_TRASH
	} ETrash;

	typedef std::list<LLFolderViewItem*> items_t;
	typedef std::list<LLFolderViewFolder*> folders_t;

protected:
	items_t mItems;
	folders_t mFolders;
	LLInventorySort	mSortFunction;

	BOOL		mIsOpen;
	BOOL		mExpanderHighlighted;
	F32			mCurHeight;
	F32			mTargetHeight;
	F32			mAutoOpenCountdown;
	time_t		mSubtreeCreationDate;
	mutable ETrash mAmTrash;
	S32			mLastArrangeGeneration;
	S32			mLastCalculatedWidth;
	S32			mCompletedFilterGeneration;
	S32			mMostFilteredDescendantGeneration;
	bool		mNeedsSort;
	bool		mPassedFolderFilter;

public:
	typedef enum e_recurse_type
	{
		RECURSE_NO,
		RECURSE_UP,
		RECURSE_DOWN,
		RECURSE_UP_DOWN
	} ERecurseType;


	virtual ~LLFolderViewFolder( void );

	virtual BOOL	potentiallyVisible();

	LLFolderViewItem* getNextFromChild( LLFolderViewItem*, BOOL include_children = TRUE );
	LLFolderViewItem* getPreviousFromChild( LLFolderViewItem*, BOOL include_children = TRUE  );

	// addToFolder() returns TRUE if it succeeds. FALSE otherwise
	virtual BOOL addToFolder(LLFolderViewFolder* folder, LLFolderView* root);

	// Finds width and height of this object and it's children.  Also
	// makes sure that this view and it's children are the right size.
	virtual S32 arrange( S32* width, S32* height, S32 filter_generation );

	BOOL needsArrange();
	void requestSort();

	// Returns the sort group (system, trash, folder) for this folder.
	virtual EInventorySortGroup getSortGroup() const;

	virtual void	setCompletedFilterGeneration(S32 generation, BOOL recurse_up);
	virtual S32		getCompletedFilterGeneration() { return mCompletedFilterGeneration; }

	BOOL hasFilteredDescendants(S32 filter_generation);
	BOOL hasFilteredDescendants();

	// applies filters to control visibility of inventory items
	virtual void filter( LLInventoryFilter& filter);
	virtual void setFiltered(BOOL filtered, S32 filter_generation);
	virtual BOOL getFiltered();
	virtual BOOL getFiltered(S32 filter_generation);

	virtual void dirtyFilter();
	
	// folder-specific filtering (filter status propagates top down instead of bottom up)
	void filterFolder(LLInventoryFilter& filter);
	void setFilteredFolder(bool filtered, S32 filter_generation);
	bool getFilteredFolder(S32 filter_generation);

	// Passes selection information on to children and record
	// selection information if necessary.
	// Returns TRUE if this object (or a child) ends up being selected.
	// If 'openitem' is TRUE then folders are opened up along the way to the selection.
	virtual BOOL setSelection(LLFolderViewItem* selection, BOOL openitem, BOOL take_keyboard_focus);

	// This method is used to change the selection of an item.
	// Recursively traverse all children; if 'selection' is 'this' then change
	// the select status if necessary.
	// Returns TRUE if the selection state of this folder, or of a child, was changed.
	virtual BOOL changeSelection(LLFolderViewItem* selection, BOOL selected);

	// this method is used to group select items
	void extendSelectionTo(LLFolderViewItem* selection);

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
	virtual void setOpen(BOOL openitem = TRUE);

	// Called when a child is refreshed.
	// don't rearrange child folder contents unless explicitly requested
	virtual void requestArrange(BOOL include_descendants = FALSE);

	// internal method which doesn't update the entire view. This
	// method was written because the list iterators destroy the state
	// of other iterations, thus, we can't arrange while iterating
	// through the children (such as when setting which is selected.
	virtual void setOpenArrangeRecursively(BOOL openitem, ERecurseType recurse = RECURSE_NO);

	// Get the current state of the folder.
	virtual BOOL isOpen() const { return mIsOpen; }

	// special case if an object is dropped on the child.
	BOOL handleDragAndDropFromChild(MASK mask,
		BOOL drop,
		EDragAndDropType cargo_type,
		void* cargo_data,
		EAcceptance* accept,
		std::string& tooltip_msg);

	void applyFunctorRecursively(LLFolderViewFunctor& functor);
	virtual void applyListenerFunctorRecursively(LLFolderViewListenerFunctor& functor);

	// Just apply this functor to the folder's immediate children.
	void applyFunctorToChildren(LLFolderViewFunctor& functor);

	virtual void openItem( void );
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
		std::string& tooltip_msg);
	BOOL handleDragAndDropToThisFolder(MASK mask, BOOL drop,
									   EDragAndDropType cargo_type,
									   void* cargo_data,
									   EAcceptance* accept,
									   std::string& tooltip_msg);
	virtual void draw();

	time_t getCreationDate() const;
	bool isTrash() const;

	folders_t::const_iterator getFoldersBegin() const { return mFolders.begin(); }
	folders_t::const_iterator getFoldersEnd() const { return mFolders.end(); }
	folders_t::size_type getFoldersCount() const { return mFolders.size(); }

	items_t::const_iterator getItemsBegin() const { return mItems.begin(); }
	items_t::const_iterator getItemsEnd() const { return mItems.end(); }
	items_t::size_type getItemsCount() const { return mItems.size(); }
	LLFolderViewFolder* getCommonAncestor(LLFolderViewItem* item_a, LLFolderViewItem* item_b, bool& reverse);
	void gatherChildRangeExclusive(LLFolderViewItem* start, LLFolderViewItem* end, bool reverse,  std::vector<LLFolderViewItem*>& items);
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

#endif  // LLFOLDERVIEWITEM_H
