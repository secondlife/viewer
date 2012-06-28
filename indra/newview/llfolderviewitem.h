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

class LLFolderView;
class LLFolderViewModelItem;
class LLFolderViewFolder;
class LLFolderViewFunctor;
class LLFolderViewFilter;
class LLFolderViewModelInterface;

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
		Optional<LLUIImage*>						folder_arrow_image,
													selection_image;
		Optional<LLFolderView*>						root;
		Mandatory<LLFolderViewModelItem*>			listener;

		Optional<S32>								folder_indentation, // pixels
													item_height,
													item_top_pad;

		Optional<time_t>							creation_date;

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

private:
	BOOL						mIsSelected;

protected:
	friend class LLUICtrlFactory;
	friend class LLFolderViewModelItem;

	LLFolderViewItem(const Params& p);

	std::string					mLabel;
	S32							mLabelWidth;
	bool						mLabelWidthDirty;
	LLFolderViewFolder*			mParentFolder;
	LLFolderViewModelItem*		mListener;
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

	//TODO RN: create interface for string highlighting
	//std::string::size_type		mStringMatchOffset;
	F32							mControlLabelRotation;
	LLFolderView*				mRoot;
	BOOL						mDragAndDropTarget;
	bool						mIsMouseOverTitle;

	// this is an internal method used for adding items to folders. A
	// no-op at this level, but reimplemented in derived classes.
	virtual BOOL addItem(LLFolderViewItem*) { return FALSE; }
	virtual BOOL addFolder(LLFolderViewFolder*) { return FALSE; }

	static LLFontGL* getLabelFontForStyle(U8 style);

public:
	BOOL postBuild();

	virtual void openItem( void );

	void arrangeAndSet(BOOL set_selection, BOOL take_keyboard_focus);

	virtual ~LLFolderViewItem( void );

	// addToFolder() returns TRUE if it succeeds. FALSE otherwise
	virtual BOOL addToFolder(LLFolderViewFolder* folder);

	// Finds width and height of this object and it's children.  Also
	// makes sure that this view and it's children are the right size.
	virtual S32 arrange( S32* width, S32* height );
	virtual S32 getItemHeight();

	// updates filter serial number and optionally propagated value up to root
	S32		getLastFilterGeneration() const;

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
	virtual std::set<LLFolderViewItem*> getSelectionList() const;

	// Returns true is this object and all of its children can be removed (deleted by user)
	virtual BOOL isRemovable();

	// Returns true is this object and all of its children can be moved
	virtual BOOL isMovable();

	// destroys this item recursively
	virtual void destroyView();

	BOOL isSelected() const { return mIsSelected; }

	void setUnselected() { mIsSelected = FALSE; }

	void setIsCurSelection(BOOL select) { mIsCurSelection = select; }

	BOOL getIsCurSelection() { return mIsCurSelection; }

	BOOL hasVisibleChildren() { return mHasVisibleChildren; }

	// Call through to the viewed object and return true if it can be
	// removed. Returns true if it's removed.
	//virtual BOOL removeRecursively(BOOL single_item);
	BOOL remove();

	// Build an appropriate context menu for the item.	Flags unused.
	void buildContextMenu(class LLMenuGL& menu, U32 flags);

	// This method returns the actual name of the thing being
	// viewed. This method will ask the viewed object itself.
	const std::string& getName( void ) const;

	// This method returns the label displayed on the view. This
	// method was primarily added to allow sorting on the folder
	// contents possible before the entire view has been constructed.
	const std::string& getLabel() const { return mLabel; }


	LLFolderViewFolder* getParentFolder( void ) { return mParentFolder; }
	const LLFolderViewFolder* getParentFolder( void ) const { return mParentFolder; }

	void setParentFolder(LLFolderViewFolder* parent) { mParentFolder = parent; }

	LLFolderViewItem* getNextOpenNode( BOOL include_children = TRUE );
	LLFolderViewItem* getPreviousOpenNode( BOOL include_children = TRUE );

	const LLFolderViewModelItem* getViewModelItem( void ) const { return mListener; }
	LLFolderViewModelItem* getViewModelItem( void ) { return mListener; }

	const LLFolderViewModelInterface* getFolderViewModel( void ) const;
	LLFolderViewModelInterface* getFolderViewModel( void );

	// just rename the object.
	void rename(const std::string& new_name);


	// Show children (unfortunate that this is called "open")
	virtual void setOpen(BOOL open = TRUE) {};
	virtual BOOL isOpen() const { return FALSE; }

	virtual LLFolderView*	getRoot();
	virtual const LLFolderView*	getRoot() const;
	BOOL			isDescendantOf( const LLFolderViewFolder* potential_ancestor );
	S32				getIndentation() { return mIndentation; }

	virtual BOOL	passedFilter(S32 filter_generation = -1);

	// refresh information from the object being viewed.
	virtual void refresh();

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

	typedef std::list<LLFolderViewItem*> items_t;
	typedef std::list<LLFolderViewFolder*> folders_t;

protected:
	items_t mItems;
	folders_t mFolders;

	BOOL		mIsOpen;
	BOOL		mExpanderHighlighted;
	F32			mCurHeight;
	F32			mTargetHeight;
	F32			mAutoOpenCountdown;
	S32			mLastArrangeGeneration;
	S32			mLastCalculatedWidth;
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

	LLFolderViewItem* getNextFromChild( LLFolderViewItem*, BOOL include_children = TRUE );
	LLFolderViewItem* getPreviousFromChild( LLFolderViewItem*, BOOL include_children = TRUE  );

	// addToFolder() returns TRUE if it succeeds. FALSE otherwise
	virtual BOOL addToFolder(LLFolderViewFolder* folder);

	// Finds width and height of this object and it's children.  Also
	// makes sure that this view and it's children are the right size.
	virtual S32 arrange( S32* width, S32* height );

	BOOL needsArrange();

	bool descendantsPassedFilter(S32 filter_generation = -1);

	// Passes selection information on to children and record
	// selection information if necessary.
	// Returns TRUE if this object (or a child) ends up being selected.
	// If 'openitem' is TRUE then folders are opened up along the way to the selection.
	virtual BOOL setSelection(LLFolderViewItem* selection, BOOL openitem, BOOL take_keyboard_focus = TRUE);

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

	// extractItem() removes the specified item from the folder, but
	// doesn't delete it.
	virtual void extractItem( LLFolderViewItem* item );

	// This function is called by a child that needs to be resorted.
	void resort(LLFolderViewItem* item);

	void setAutoOpenCountdown(F32 countdown) { mAutoOpenCountdown = countdown; }

	// folders can be opened. This will usually be called by internal
	// methods.
	virtual void toggleOpen();

	// Force a folder open or closed
	virtual void setOpen(BOOL openitem = TRUE);

	// Called when a child is refreshed.
	virtual void requestArrange();

	virtual void requestSort();

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

	folders_t::iterator getFoldersBegin() { return mFolders.begin(); }
	folders_t::iterator getFoldersEnd() { return mFolders.end(); }
	folders_t::size_type getFoldersCount() const { return mFolders.size(); }

	items_t::const_iterator getItemsBegin() const { return mItems.begin(); }
	items_t::const_iterator getItemsEnd() const { return mItems.end(); }
	items_t::size_type getItemsCount() const { return mItems.size(); }

	LLFolderViewFolder* getCommonAncestor(LLFolderViewItem* item_a, LLFolderViewItem* item_b, bool& reverse);
	void gatherChildRangeExclusive(LLFolderViewItem* start, LLFolderViewItem* end, bool reverse,  std::vector<LLFolderViewItem*>& items);

public:
	//WARNING: do not call directly...use the appropriate LLFolderViewModel-derived class instead
	template<typename SORT_FUNC> void sortFolders(const SORT_FUNC& func) { mFolders.sort(func); }
	template<typename SORT_FUNC> void sortItems(const SORT_FUNC& func) { mItems.sort(func); }
};


#endif  // LLFOLDERVIEWITEM_H
