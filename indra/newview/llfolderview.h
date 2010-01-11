/** 
 * @file llfolderview.h
 * @brief Definition of the folder view collection of classes.
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

/**
 *
 * The folder view collection of classes provides an interface for
 * making a 'folder view' similar to the way the a single pane file
 * folder interface works.
 *
 */

#ifndef LL_LLFOLDERVIEW_H
#define LL_LLFOLDERVIEW_H

#include "llfolderviewitem.h"	// because LLFolderView is-a LLFolderViewFolder

#include "lluictrl.h"
#include "v4color.h"
#include "lldarray.h"
#include "stdenums.h"
#include "lldepthstack.h"
#include "lleditmenuhandler.h"
#include "llfontgl.h"
#include "lltooldraganddrop.h"
#include "llviewertexture.h"

class LLFolderViewEventListener;
class LLFolderViewFolder;
class LLFolderViewItem;
class LLInventoryModel;
class LLPanel;
class LLLineEditor;
class LLMenuGL;
class LLScrollContainer;
class LLUICtrl;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFolderViewFunctor
//
// Simple abstract base class for applying a functor to folders and
// items in a folder view hierarchy. This is suboptimal for algorithms
// that only work folders or only work on items, but I'll worry about
// that later when it's determined to be too slow.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLFolderViewFunctor
{
public:
	virtual ~LLFolderViewFunctor() {}
	virtual void doFolder(LLFolderViewFolder* folder) = 0;
	virtual void doItem(LLFolderViewItem* item) = 0;
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFolderView
//
// Th LLFolderView represents the root level folder view object. It
// manages the screen region of the folder view.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFolderView : public LLFolderViewFolder, public LLEditMenuHandler
{
public:
	struct Params : public LLInitParam::Block<Params, LLFolderViewFolder::Params>
	{
		Mandatory<LLPanel*> parent_panel;
		Optional<LLUUID>	task_id;
	};
	LLFolderView(const Params&);
	virtual ~LLFolderView( void );

	virtual BOOL canFocusChildren() const;

	virtual LLFolderView*	getRoot() { return this; }

	// FolderViews default to sort by name.  This will change that,
	// and resort the items if necessary.
	void setSortOrder(U32 order);
	void checkTreeResortForModelChanged();
	void setFilterPermMask(PermissionMask filter_perm_mask);
	void setAllowMultiSelect(BOOL allow) { mAllowMultiSelect = allow; }
	
	typedef boost::signals2::signal<void (const std::deque<LLFolderViewItem*>& items, BOOL user_action)> signal_t;
	void setSelectCallback(const signal_t::slot_type& cb) { mSelectSignal.connect(cb); }
	void setReshapeCallback(const signal_t::slot_type& cb) { mReshapeSignal.connect(cb); }
	
	// filter is never null
	LLInventoryFilter* getFilter();
	const std::string getFilterSubString(BOOL trim = FALSE);
	U32 getFilterObjectTypes() const;
	PermissionMask getFilterPermissions() const;
	// JAMESDEBUG use getFilter()->getShowFolderState();
	//LLInventoryFilter::EFolderShow getShowFolderState();
	U32 getSortOrder() const;
	BOOL isFilterModified();
	BOOL getAllowMultiSelect();

	// Close all folders in the view
	void closeAllFolders();
	void openFolder(const std::string& foldername);
	void openTopLevelFolders();

	virtual void toggleOpen() {};
	virtual void setOpenArrangeRecursively(BOOL openitem, ERecurseType recurse);
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
	virtual BOOL setSelection(LLFolderViewItem* selection, BOOL openitem,
		BOOL take_keyboard_focus);
	
	// Used by menu callbacks
	void setSelectionByID(const LLUUID& obj_id, BOOL take_keyboard_focus);
	
	// Called once a frame to update the selection if mSelectThisID has been set
	void updateSelection();	
	
	// This method is used to toggle the selection of an item. Walks
	// children, and keeps track of selected objects.
	virtual BOOL changeSelection(LLFolderViewItem* selection, BOOL selected);

	virtual S32 extendSelection(LLFolderViewItem* selection, LLFolderViewItem* last_selected, LLDynamicArray<LLFolderViewItem*>& items);

	virtual BOOL getSelectionList(std::set<LLUUID> &selection) const;

	// make sure if ancestor is selected, descendents are not
	void sanitizeSelection();
	void clearSelection();
	void addToSelectionList(LLFolderViewItem* item);
	void removeFromSelectionList(LLFolderViewItem* item);

	BOOL startDrag(LLToolDragAndDrop::ESource source);
	void setDragAndDropThisFrame() { mDragAndDropThisFrame = TRUE; }
	void setDraggingOverItem(LLFolderViewItem* item) { mDraggingOverItem = item; }
	LLFolderViewItem* getDraggingOverItem() { return mDraggingOverItem; }


	// deletion functionality
 	void removeSelectedItems();

	// open the selected item.
	void openSelectedItems( void );
	void propertiesSelectedItems( void );

	// change the folder type
	void changeType(LLInventoryModel *model, LLFolderType::EType new_folder_type);

	void autoOpenItem(LLFolderViewFolder* item);
	void closeAutoOpenedFolders();
	BOOL autoOpenTest(LLFolderViewFolder* item);

	// copy & paste
	virtual void	copy();
	virtual BOOL	canCopy() const;

	virtual void	cut();
	virtual BOOL	canCut() const;

	virtual void	paste();
	virtual BOOL	canPaste() const;

	virtual void	doDelete();
	virtual BOOL	canDoDelete() const;

	// public rename functionality - can only start the process
	void startRenamingSelectedItem( void );

	// These functions were used when there was only one folderview,
	// and relied on that concept. This functionality is now handled
	// by the listeners and the lldraganddroptool.
	//LLFolderViewItem*	getMovingItem() { return mMovingItem; }
	//void setMovingItem( LLFolderViewItem* item ) { mMovingItem = item; }
	//void				dragItemIntoFolder( LLFolderViewItem* moving_item, LLFolderViewFolder* dst_folder, BOOL drop, BOOL* accept );
	//void				dragFolderIntoFolder( LLFolderViewFolder* moving_folder, LLFolderViewFolder* dst_folder, BOOL drop, BOOL* accept );

	// LLView functionality
	///*virtual*/ BOOL handleKey( KEY key, MASK mask, BOOL called_from_parent );
	/*virtual*/ BOOL handleKeyHere( KEY key, MASK mask );
	/*virtual*/ BOOL handleUnicodeCharHere(llwchar uni_char);
	/*virtual*/ BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleDoubleClick( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleRightMouseDown( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleHover( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	virtual void draw();
	virtual void deleteAllChildren();

	void scrollToShowSelection();
	void scrollToShowItem(LLFolderViewItem* item, const LLRect& constraint_rect);
	void setScrollContainer( LLScrollContainer* parent ) { mScrollContainer = parent; }
	LLRect getVisibleRect();

	BOOL search(LLFolderViewItem* first_item, const std::string &search_string, BOOL backward);
	void setShowSelectionContext(BOOL show) { mShowSelectionContext = show; }
	BOOL getShowSelectionContext();
	void setShowSingleSelection(BOOL show);
	BOOL getShowSingleSelection() { return mShowSingleSelection; }
	F32  getSelectionFadeElapsedTime() { return mMultiSelectionFadeTimer.getElapsedTimeF32(); }
	void setUseEllipses(bool use_ellipses) { mUseEllipses = use_ellipses; }
	bool getUseEllipses() { return mUseEllipses; }

	void addItemID(const LLUUID& id, LLFolderViewItem* itemp);
	void removeItemID(const LLUUID& id);
	LLFolderViewItem* getItemByID(const LLUUID& id);
	LLFolderViewFolder* getFolderByID(const LLUUID& id);
	
	bool doToSelected(LLInventoryModel* model, const LLSD& userdata);
	
	void	doIdle();						// Real idle routine
	static void idle(void* user_data);		// static glue to doIdle()

	BOOL needsAutoSelect() { return mNeedsAutoSelect && !mAutoSelectOverride; }
	BOOL needsAutoRename() { return mNeedsAutoRename; }
	void setNeedsAutoRename(BOOL val) { mNeedsAutoRename = val; }

	void setCallbackRegistrar(LLUICtrl::CommitCallbackRegistry::ScopedRegistrar* registrar) { mCallbackRegistrar = registrar; }

	BOOL getDebugFilters() { return mDebugFilters; }

	LLPanel* getParentPanel() { return mParentPanel; }
	// DEBUG only
	void dumpSelectionInformation();
	
private:
	void updateRenamerPosition();

protected:
	LLScrollContainer* mScrollContainer;  // NULL if this is not a child of a scroll container.

	void commitRename( const LLSD& data );
	static void onRenamerLost( LLFocusableElement* renamer);

	void finishRenamingItem( void );
	void closeRenamer( void );
	
protected:
	LLHandle<LLView>					mPopupMenuHandle;
	
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
	BOOL							mPinningSelectedItem;
	LLRect							mScrollConstraintRect;
	BOOL							mNeedsAutoSelect;
	BOOL							mAutoSelectOverride;
	BOOL							mNeedsAutoRename;
	
	BOOL							mDebugFilters;
	U32								mSortOrder;
	LLDepthStack<LLFolderViewFolder>	mAutoOpenItems;
	LLFolderViewFolder*				mAutoOpenCandidate;
	LLFrameTimer					mAutoOpenTimer;
	LLFrameTimer					mSearchTimer;
	std::string						mSearchString;
	LLInventoryFilter*				mFilter;
	BOOL							mShowSelectionContext;
	BOOL							mShowSingleSelection;
	LLFrameTimer					mMultiSelectionFadeTimer;
	S32								mArrangeGeneration;

	signal_t						mSelectSignal;
	signal_t						mReshapeSignal;
	S32								mSignalSelectCallback;
	S32								mMinWidth;
	std::map<LLUUID, LLFolderViewItem*> mItemMap;
	BOOL							mDragAndDropThisFrame;
	
	LLUUID							mSelectThisID; // if non null, select this item
	
	LLPanel*				mParentPanel;

	/**
	 * Is used to determine if we need to cut text In LLFolderViewItem to avoid horizontal scroll.
	 * NOTE: For now it uses only to cut LLFolderViewItem::mLabel text to be used for Landmarks in Places Panel.
	 */
	bool							mUseEllipses; // See EXT-719

	/**
	 * Contains item under mouse pointer while dragging
	 */
	LLFolderViewItem*				mDraggingOverItem; // See EXT-719

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar* mCallbackRegistrar;
	
public:
	static F32 sAutoOpenTime;
};

bool sort_item_name(LLFolderViewItem* a, LLFolderViewItem* b);
bool sort_item_date(LLFolderViewItem* a, LLFolderViewItem* b);

// Flags for buildContextMenu()
const U32 SUPPRESS_OPEN_ITEM = 0x1;
const U32 FIRST_SELECTED_ITEM = 0x2;

#endif // LL_LLFOLDERVIEW_H
