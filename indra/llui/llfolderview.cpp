/** 
 * @file llfolderview.cpp
 * @brief Implementation of the folder view collection of classes.
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

#include "linden_common.h"

#include "llfolderview.h"
#include "llfolderviewmodel.h"
#include "llclipboard.h" // *TODO: remove this once hack below gone.
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llpanel.h"
#include "llscrollcontainer.h" // hack to allow scrolling
#include "lltextbox.h"
#include "lltrans.h"
#include "llui.h"
#include "lluictrlfactory.h"

// Linden library includes
#include "lldbstrings.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llgl.h" 
#include "llrender.h"

// Third-party library includes
#include <algorithm>

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

const S32 RENAME_HEIGHT_PAD = 1;
const S32 AUTO_OPEN_STACK_DEPTH = 16;

const S32 MINIMUM_RENAMER_WIDTH = 80;

// *TODO: move in params in xml if necessary. Requires modification of LLFolderView & LLInventoryPanel Params.
const S32 STATUS_TEXT_HPAD = 6;
const S32 STATUS_TEXT_VPAD = 8;

enum {
	SIGNAL_NO_KEYBOARD_FOCUS = 1,
	SIGNAL_KEYBOARD_FOCUS = 2
};

F32 LLFolderView::sAutoOpenTime = 1.f;

//---------------------------------------------------------------------------

// Tells all folders in a folderview to close themselves
// For efficiency, calls setOpenArrangeRecursively().
// The calling function must then call:
//	LLFolderView* root = getRoot();
//	if( root )
//	{
//		root->arrange( NULL, NULL );
//		root->scrollToShowSelection();
//	}
// to patch things up.
class LLCloseAllFoldersFunctor : public LLFolderViewFunctor
{
public:
	LLCloseAllFoldersFunctor(BOOL close) { mOpen = !close; }
	virtual ~LLCloseAllFoldersFunctor() {}
	virtual void doFolder(LLFolderViewFolder* folder);
	virtual void doItem(LLFolderViewItem* item);

	BOOL mOpen;
};


void LLCloseAllFoldersFunctor::doFolder(LLFolderViewFolder* folder)
{
	folder->setOpenArrangeRecursively(mOpen);
}

// Do nothing.
void LLCloseAllFoldersFunctor::doItem(LLFolderViewItem* item)
{ }

///----------------------------------------------------------------------------
/// Class LLFolderViewScrollContainer
///----------------------------------------------------------------------------

// virtual
const LLRect LLFolderViewScrollContainer::getScrolledViewRect() const
{
	LLRect rect = LLRect::null;
	if (mScrolledView)
	{
		LLFolderView* folder_view = dynamic_cast<LLFolderView*>(mScrolledView);
		if (folder_view)
		{
			S32 height = folder_view->getRect().getHeight();

			rect = mScrolledView->getRect();
			rect.setLeftTopAndSize(rect.mLeft, rect.mTop, rect.getWidth(), height);
		}
	}

	return rect;
}

LLFolderViewScrollContainer::LLFolderViewScrollContainer(const LLScrollContainer::Params& p)
:	LLScrollContainer(p)
{}

///----------------------------------------------------------------------------
/// Class LLFolderView
///----------------------------------------------------------------------------
LLFolderView::Params::Params()
:	title("title"),
	use_label_suffix("use_label_suffix"),
	allow_multiselect("allow_multiselect", true),
	show_empty_message("show_empty_message", true),
	use_ellipses("use_ellipses", false),
    options_menu("options_menu", "")
{
	folder_indentation = -4;
}


// Default constructor
LLFolderView::LLFolderView(const Params& p)
:	LLFolderViewFolder(p),
	mScrollContainer( NULL ),
	mPopupMenuHandle(),
	mAllowMultiSelect(p.allow_multiselect),
	mShowEmptyMessage(p.show_empty_message),
	mShowFolderHierarchy(FALSE),
	mRenameItem( NULL ),
	mNeedsScroll( FALSE ),
	mUseLabelSuffix(p.use_label_suffix),
	mPinningSelectedItem(FALSE),
	mNeedsAutoSelect( FALSE ),
	mAutoSelectOverride(FALSE),
	mNeedsAutoRename(FALSE),
	mShowSelectionContext(FALSE),
	mShowSingleSelection(FALSE),
	mArrangeGeneration(0),
	mSignalSelectCallback(0),
	mMinWidth(0),
	mDragAndDropThisFrame(FALSE),
	mCallbackRegistrar(NULL),
	mUseEllipses(p.use_ellipses),
	mDraggingOverItem(NULL),
	mStatusTextBox(NULL),
	mShowItemLinkOverlays(p.show_item_link_overlays),
	mViewModel(p.view_model),
    mGroupedItemModel(p.grouped_item_model)
{
	claimMem(mViewModel);
    LLPanel* panel = p.parent_panel;
    mParentPanel = panel->getHandle();
	mViewModel->setFolderView(this);
	mRoot = this;

	LLRect rect = p.rect;
	LLRect new_rect(rect.mLeft, rect.mBottom + getRect().getHeight(), rect.mLeft + getRect().getWidth(), rect.mBottom);
	setRect( rect );
	reshape(rect.getWidth(), rect.getHeight());
	mAutoOpenItems.setDepth(AUTO_OPEN_STACK_DEPTH);
	mAutoOpenCandidate = NULL;
	mAutoOpenTimer.stop();
	mKeyboardSelection = FALSE;
	mIndentation = 	getParentFolder() ? getParentFolder()->getIndentation() + mLocalIndentation : 0;  

	//clear label
	// go ahead and render root folder as usual
	// just make sure the label ("Inventory Folder") never shows up
	mLabel = LLStringUtil::null;

	// Escape is handled by reverting the rename, not commiting it (default behavior)
	LLLineEditor::Params params;
	params.name("ren");
	params.rect(rect);
	params.font(getLabelFontForStyle(LLFontGL::NORMAL));
	params.max_length.bytes(DB_INV_ITEM_NAME_STR_LEN);
	params.commit_callback.function(boost::bind(&LLFolderView::commitRename, this, _2));
	params.prevalidate_callback(&LLTextValidate::validateASCIIPrintableNoPipe);
	params.commit_on_focus_lost(true);
	params.visible(false);
	mRenamer = LLUICtrlFactory::create<LLLineEditor> (params);
	addChild(mRenamer);

	// Textbox
	LLTextBox::Params text_p;
	LLFontGL* font = getLabelFontForStyle(mLabelStyle);
    //mIconPad, mTextPad are set in folder_view_item.xml
	LLRect new_r = LLRect(rect.mLeft + mIconPad,
			      rect.mTop - mTextPad,
			      rect.mRight,
			      rect.mTop - mTextPad - font->getLineHeight());
	text_p.rect(new_r);
	text_p.name(std::string(p.name));
	text_p.font(font);
	text_p.visible(false);
	text_p.parse_urls(true);
	text_p.wrap(true); // allow multiline text. See EXT-7564, EXT-7047
	// set text padding the same as in People panel. EXT-7047, EXT-4837
	text_p.h_pad(STATUS_TEXT_HPAD);
	text_p.v_pad(STATUS_TEXT_VPAD);
	mStatusTextBox = LLUICtrlFactory::create<LLTextBox> (text_p);
	mStatusTextBox->setFollowsLeft();
	mStatusTextBox->setFollowsTop();
	addChild(mStatusTextBox);


	// make the popup menu available
	llassert(LLMenuGL::sMenuContainer != NULL);
	LLMenuGL* menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>(p.options_menu, LLMenuGL::sMenuContainer, LLMenuHolderGL::child_registry_t::instance());
	if (!menu)
	{
		menu = LLUICtrlFactory::getDefaultWidget<LLMenuGL>("inventory_menu");
	}
	menu->setBackgroundColor(LLUIColorTable::instance().getColor("MenuPopupBgColor"));
	mPopupMenuHandle = menu->getHandle();

	mViewModelItem->openItem();
}

// Destroys the object
LLFolderView::~LLFolderView( void )
{
	closeRenamer();

	// The release focus call can potentially call the
	// scrollcontainer, which can potentially be called with a partly
	// destroyed scollcontainer. Just null it out here, and no worries
	// about calling into the invalid scroll container.
	// Same with the renamer.
	mScrollContainer = NULL;
	mRenameItem = NULL;
	mRenamer = NULL;
	mStatusTextBox = NULL;

	if (mPopupMenuHandle.get()) mPopupMenuHandle.get()->die();

	mAutoOpenItems.removeAllNodes();
	clearSelection();
	mItems.clear();
	mFolders.clear();

	//mViewModel->setFolderView(NULL);
	mViewModel = NULL;
}

BOOL LLFolderView::canFocusChildren() const
{
	return FALSE;
}

void LLFolderView::addFolder( LLFolderViewFolder* folder)
{
	LLFolderViewFolder::addFolder(folder);
}

void LLFolderView::closeAllFolders()
{
	// Close all the folders
	setOpenArrangeRecursively(FALSE, LLFolderViewFolder::RECURSE_DOWN);
	arrangeAll();
}

void LLFolderView::openTopLevelFolders()
{
	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		(*fit)->setOpen(TRUE);
	}
}

// This view grows and shrinks to enclose all of its children items and folders.
// *width should be 0
// conform show folder state works
S32 LLFolderView::arrange( S32* unused_width, S32* unused_height )
	{
	mMinWidth = 0;
	S32 target_height;

	LLFolderViewFolder::arrange(&mMinWidth, &target_height);

	LLRect scroll_rect = (mScrollContainer ? mScrollContainer->getContentWindowRect() : LLRect());
	reshape( llmax(scroll_rect.getWidth(), mMinWidth), ll_round(mCurHeight) );

	LLRect new_scroll_rect = (mScrollContainer ? mScrollContainer->getContentWindowRect() : LLRect());
	if (new_scroll_rect.getWidth() != scroll_rect.getWidth())
	{
		reshape( llmax(scroll_rect.getWidth(), mMinWidth), ll_round(mCurHeight) );
	}

	// move item renamer text field to item's new position
	updateRenamerPosition();

	return ll_round(mTargetHeight);
}

static LLTrace::BlockTimerStatHandle FTM_FILTER("Filter Folder View");

void LLFolderView::filter( LLFolderViewFilter& filter )
{
	LL_RECORD_BLOCK_TIME(FTM_FILTER);
    filter.resetTime(llclamp(LLUI::sSettingGroups["config"]->getS32(mParentPanel.get()->getVisible() ? "FilterItemsMaxTimePerFrameVisible" : "FilterItemsMaxTimePerFrameUnvisible"), 1, 100));

    // Note: we filter the model, not the view
	getViewModelItem()->filter(filter);
}

void LLFolderView::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLRect scroll_rect;
	if (mScrollContainer)
	{
		LLView::reshape(width, height, called_from_parent);
		scroll_rect = mScrollContainer->getContentWindowRect();
	}
	width  = llmax(mMinWidth, scroll_rect.getWidth());
	height = llmax(ll_round(mCurHeight), scroll_rect.getHeight());

	// Restrict width within scroll container's width
	if (mUseEllipses && mScrollContainer)
	{
		width = scroll_rect.getWidth();
	}
	LLView::reshape(width, height, called_from_parent);
	mReshapeSignal(mSelectedItems, FALSE);
}

void LLFolderView::addToSelectionList(LLFolderViewItem* item)
{
	if (item->isSelected())
	{
		removeFromSelectionList(item);
	}
	if (mSelectedItems.size())
	{
		mSelectedItems.back()->setIsCurSelection(FALSE);
	}
	item->setIsCurSelection(TRUE);
	mSelectedItems.push_back(item);
}

void LLFolderView::removeFromSelectionList(LLFolderViewItem* item)
{
	if (mSelectedItems.size())
	{
		mSelectedItems.back()->setIsCurSelection(FALSE);
	}

	selected_items_t::iterator item_iter;
	for (item_iter = mSelectedItems.begin(); item_iter != mSelectedItems.end();)
	{
		if (*item_iter == item)
		{
			item_iter = mSelectedItems.erase(item_iter);
		}
		else
		{
			++item_iter;
		}
	}
	if (mSelectedItems.size())
	{
		mSelectedItems.back()->setIsCurSelection(TRUE);
	}
}

LLFolderViewItem* LLFolderView::getCurSelectedItem( void )
{
	if(mSelectedItems.size())
	{
		LLFolderViewItem* itemp = mSelectedItems.back();
		llassert(itemp->getIsCurSelection());
		return itemp;
	}
	return NULL;
}

LLFolderView::selected_items_t& LLFolderView::getSelectedItems( void )
{
    return mSelectedItems;
}

// Record the selected item and pass it down the hierachy.
BOOL LLFolderView::setSelection(LLFolderViewItem* selection, BOOL openitem,
								BOOL take_keyboard_focus)
{
	mSignalSelectCallback = take_keyboard_focus ? SIGNAL_KEYBOARD_FOCUS : SIGNAL_NO_KEYBOARD_FOCUS;

	if( selection == this )
	{
		return FALSE;
	}

	if( selection && take_keyboard_focus)
	{
		mParentPanel.get()->setFocus(TRUE);
	}

	// clear selection down here because change of keyboard focus can potentially
	// affect selection
	clearSelection();

	if(selection)
	{
		addToSelectionList(selection);
	}

	BOOL rv = LLFolderViewFolder::setSelection(selection, openitem, take_keyboard_focus);
	if(openitem && selection)
	{
		selection->getParentFolder()->requestArrange();
	}

	llassert(mSelectedItems.size() <= 1);

	return rv;
}

BOOL LLFolderView::changeSelection(LLFolderViewItem* selection, BOOL selected)
{
	BOOL rv = FALSE;

	// can't select root folder
	if(!selection || selection == this)
	{
		return FALSE;
	}

	if (!mAllowMultiSelect)
	{
		clearSelection();
	}

	selected_items_t::iterator item_iter;
	for (item_iter = mSelectedItems.begin(); item_iter != mSelectedItems.end(); ++item_iter)
	{
		if (*item_iter == selection)
		{
			break;
		}
	}

	BOOL on_list = (item_iter != mSelectedItems.end());

	if(selected && !on_list)
	{
		addToSelectionList(selection);
	}
	if(!selected && on_list)
	{
		removeFromSelectionList(selection);
	}

	rv = LLFolderViewFolder::changeSelection(selection, selected);

	mSignalSelectCallback = SIGNAL_KEYBOARD_FOCUS;
	
	return rv;
}

static LLTrace::BlockTimerStatHandle FTM_SANITIZE_SELECTION("Sanitize Selection");
void LLFolderView::sanitizeSelection()
{
	LL_RECORD_BLOCK_TIME(FTM_SANITIZE_SELECTION);
	// store off current item in case it is automatically deselected
	// and we want to preserve context
	LLFolderViewItem* original_selected_item = getCurSelectedItem();

	std::vector<LLFolderViewItem*> items_to_remove;
	selected_items_t::iterator item_iter;
	for (item_iter = mSelectedItems.begin(); item_iter != mSelectedItems.end(); ++item_iter)
	{
		LLFolderViewItem* item = *item_iter;

		// ensure that each ancestor is open and potentially passes filtering
		BOOL visible = false;
		if(item->getViewModelItem() != NULL)
		{
			visible = item->getViewModelItem()->potentiallyVisible(); // initialize from filter state for this item
		}
		// modify with parent open and filters states
		LLFolderViewFolder* parent_folder = item->getParentFolder();
		// Move up through parent folders and see what's visible
				while(parent_folder)
				{
			visible = visible && parent_folder->isOpen() && parent_folder->getViewModelItem()->potentiallyVisible();
					parent_folder = parent_folder->getParentFolder();
				}

		//  deselect item if any ancestor is closed or didn't pass filter requirements.
		if (!visible)
		{
			items_to_remove.push_back(item);
		}

		// disallow nested selections (i.e. folder items plus one or more ancestors)
		// could check cached mum selections count and only iterate if there are any
		// but that may be a premature optimization.
		selected_items_t::iterator other_item_iter;
		for (other_item_iter = mSelectedItems.begin(); other_item_iter != mSelectedItems.end(); ++other_item_iter)
		{
			LLFolderViewItem* other_item = *other_item_iter;
			for( parent_folder = other_item->getParentFolder(); parent_folder; parent_folder = parent_folder->getParentFolder())
			{
				if (parent_folder == item)
				{
					// this is a descendent of the current folder, remove from list
					items_to_remove.push_back(other_item);
					break;
				}
			}
		}

		// Don't allow invisible items (such as root folders) to be selected.
		if (item == getRoot())
		{
			items_to_remove.push_back(item);
		}
	}

	std::vector<LLFolderViewItem*>::iterator item_it;
	for (item_it = items_to_remove.begin(); item_it != items_to_remove.end(); ++item_it )
	{
		changeSelection(*item_it, FALSE); // toggle selection (also removes from list)
	}

	// if nothing selected after prior constraints...
	if (mSelectedItems.empty())
	{
		// ...select first available parent of original selection
		LLFolderViewItem* new_selection = NULL;
		if (original_selected_item)
		{
			for(LLFolderViewFolder* parent_folder = original_selected_item->getParentFolder();
				parent_folder;
				parent_folder = parent_folder->getParentFolder())
			{
				if (parent_folder->getViewModelItem() && parent_folder->getViewModelItem()->potentiallyVisible())
				{
					// give initial selection to first ancestor folder that potentially passes the filter
					if (!new_selection)
					{
						new_selection = parent_folder;
					}

					// if any ancestor folder of original item is closed, move the selection up 
					// to the highest closed
					if (!parent_folder->isOpen())
					{	
						new_selection = parent_folder;
					}
				}
			}
		}
		else
		{
			new_selection = NULL;
		}

		if (new_selection)
		{
			setSelection(new_selection, FALSE, FALSE);
		}
	}
}

void LLFolderView::clearSelection()
{
	for (selected_items_t::const_iterator item_it = mSelectedItems.begin(); 
		 item_it != mSelectedItems.end(); 
		 ++item_it)
	{
		(*item_it)->setUnselected();
	}

	mSelectedItems.clear();
}

std::set<LLFolderViewItem*> LLFolderView::getSelectionList() const
{
	std::set<LLFolderViewItem*> selection;
	std::copy(mSelectedItems.begin(), mSelectedItems.end(), std::inserter(selection, selection.begin()));
	return selection;
}

bool LLFolderView::startDrag()
{
	std::vector<LLFolderViewModelItem*> selected_items;
	selected_items_t::iterator item_it;

	if (!mSelectedItems.empty())
	{
		for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
		{
			selected_items.push_back((*item_it)->getViewModelItem());
		}

		return getFolderViewModel()->startDrag(selected_items);
	}
	return false;
}

void LLFolderView::commitRename( const LLSD& data )
{
	finishRenamingItem();
	arrange( NULL, NULL );

}

void LLFolderView::draw()
{
	//LLFontGL* font = getLabelFontForStyle(mLabelStyle);

	// if cursor has moved off of me during drag and drop
	// close all auto opened folders
	if (!mDragAndDropThisFrame)
	{
		closeAutoOpenedFolders();
	}

	if (mSearchTimer.getElapsedTimeF32() > LLUI::sSettingGroups["config"]->getF32("TypeAheadTimeout") || !mSearchString.size())
	{
		mSearchString.clear();
	}

	if (hasVisibleChildren())
	{
		mStatusTextBox->setVisible( FALSE );
	}
	else if (mShowEmptyMessage)
	{
		mStatusTextBox->setValue(getFolderViewModel()->getStatusText());
		mStatusTextBox->setVisible( TRUE );
		
		// firstly reshape message textbox with current size. This is necessary to
		// LLTextBox::getTextPixelHeight works properly
		const LLRect local_rect = getLocalRect();
		mStatusTextBox->setShape(local_rect);

		// get preferable text height...
		S32 pixel_height = mStatusTextBox->getTextPixelHeight();
		bool height_changed = (local_rect.getHeight() < pixel_height);
		if (height_changed)
		{
			// ... if it does not match current height, lets rearrange current view.
			// This will indirectly call ::arrange and reshape of the status textbox.
			// We should call this method to also notify parent about required rect.
			// See EXT-7564, EXT-7047.
			S32 height = 0;
			S32 width = 0;
			S32 total_height = arrange( &width, &height );
			notifyParent(LLSD().with("action", "size_changes").with("height", total_height));

			LLUI::popMatrix();
			LLUI::pushMatrix();
			LLUI::translate((F32)getRect().mLeft, (F32)getRect().mBottom);
		}
	}

	if (mRenameItem && mRenamer && mRenamer->getVisible() && !getVisibleRect().overlaps(mRenamer->getRect()))
	{
		// renamer is not connected to the item we are renaming in any form so manage it manually
		// TODO: consider stopping on any scroll action instead of when out of visible area
		finishRenamingItem();
	}

	// skip over LLFolderViewFolder::draw since we don't want the folder icon, label, 
	// and arrow for the root folder
	LLView::draw();

	mDragAndDropThisFrame = FALSE;
}

void LLFolderView::finishRenamingItem( void )
{
	if(!mRenamer)
	{
		return;
	}
	if( mRenameItem )
	{
		mRenameItem->rename( mRenamer->getText() );
	}

	closeRenamer();

	// This is moved to an inventory observer in llinventorybridge.cpp, to handle updating after operation completed in AISv3 (SH-4611).
	// List is re-sorted alphabetically, so scroll to make sure the selected item is visible.
	//scrollToShowSelection();
}

void LLFolderView::closeRenamer( void )
{
	if (mRenamer && mRenamer->getVisible())
	{
		// Triggers onRenamerLost() that actually closes the renamer.
		LLUI::removePopup(mRenamer);
	}
}

void LLFolderView::removeSelectedItems()
{
	if(getVisible() && getEnabled())
	{
		// just in case we're removing the renaming item.
		mRenameItem = NULL;

		// create a temporary structure which we will use to remove
		// items, since the removal will futz with internal data
		// structures.
		std::vector<LLFolderViewItem*> items;
		S32 count = mSelectedItems.size();
		if(count <= 0) return;
		LLFolderViewItem* item = NULL;
		selected_items_t::iterator item_it;
		for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
		{
			item = *item_it;
			if (item && item->isRemovable())
			{
				items.push_back(item);
			}
			else
			{
				LL_INFOS() << "Cannot delete " << item->getName() << LL_ENDL;
				return;
			}
		}

		// iterate through the new container.
		count = items.size();
		LLUUID new_selection_id;
		LLFolderViewItem* item_to_select = getNextUnselectedItem();

		if(count == 1)
		{
			LLFolderViewItem* item_to_delete = items[0];
			LLFolderViewFolder* parent = item_to_delete->getParentFolder();
			if(parent)
			{
				if (item_to_delete->remove())
				{
					// change selection on successful delete
					setSelection(item_to_select, item_to_select ? item_to_select->isOpen() : false, mParentPanel.get()->hasFocus());
				}
			}
			arrangeAll();
		}
		else if (count > 1)
		{
			std::vector<LLFolderViewModelItem*> listeners;
			LLFolderViewModelItem* listener;

			setSelection(item_to_select, item_to_select ? item_to_select->isOpen() : false, mParentPanel.get()->hasFocus());

			listeners.reserve(count);
			for(S32 i = 0; i < count; ++i)
			{
				listener = items[i]->getViewModelItem();
				if(listener && (std::find(listeners.begin(), listeners.end(), listener) == listeners.end()))
				{
					listeners.push_back(listener);
				}
			}
			listener = static_cast<LLFolderViewModelItem*>(listeners.at(0));
			if(listener)
			{
				listener->removeBatch(listeners);
			}
		}
		arrangeAll();
		scrollToShowSelection();
	}
}

void LLFolderView::autoOpenItem( LLFolderViewFolder* item )
{
	if ((mAutoOpenItems.check() == item) || 
		(mAutoOpenItems.getDepth() >= (U32)AUTO_OPEN_STACK_DEPTH) ||
		item->isOpen())
	{
		return;
	}

	// close auto-opened folders
	LLFolderViewFolder* close_item = mAutoOpenItems.check();
	while (close_item && close_item != item->getParentFolder())
	{
		mAutoOpenItems.pop();
		close_item->setOpenArrangeRecursively(FALSE);
		close_item = mAutoOpenItems.check();
	}

	item->requestArrange();

	mAutoOpenItems.push(item);
	
	item->setOpen(TRUE);
	LLRect content_rect = (mScrollContainer ? mScrollContainer->getContentWindowRect() : LLRect());
	LLRect constraint_rect(0,content_rect.getHeight(), content_rect.getWidth(), 0);
	scrollToShowItem(item, constraint_rect);
}

void LLFolderView::closeAutoOpenedFolders()
{
	while (mAutoOpenItems.check())
	{
		LLFolderViewFolder* close_item = mAutoOpenItems.pop();
		close_item->setOpen(FALSE);
	}

	if (mAutoOpenCandidate)
	{
		mAutoOpenCandidate->setAutoOpenCountdown(0.f);
	}
	mAutoOpenCandidate = NULL;
	mAutoOpenTimer.stop();
}

BOOL LLFolderView::autoOpenTest(LLFolderViewFolder* folder)
{
	if (folder && mAutoOpenCandidate == folder)
	{
		if (mAutoOpenTimer.getStarted())
		{
			if (!mAutoOpenCandidate->isOpen())
			{
				mAutoOpenCandidate->setAutoOpenCountdown(clamp_rescale(mAutoOpenTimer.getElapsedTimeF32(), 0.f, sAutoOpenTime, 0.f, 1.f));
			}
			if (mAutoOpenTimer.getElapsedTimeF32() > sAutoOpenTime)
			{
				autoOpenItem(folder);
				mAutoOpenTimer.stop();
				return TRUE;
			}
		}
		return FALSE;
	}

	// otherwise new candidate, restart timer
	if (mAutoOpenCandidate)
	{
		mAutoOpenCandidate->setAutoOpenCountdown(0.f);
	}
	mAutoOpenCandidate = folder;
	mAutoOpenTimer.start();
	return FALSE;
}

BOOL LLFolderView::canCopy() const
{
	if (!(getVisible() && getEnabled() && (mSelectedItems.size() > 0)))
	{
		return FALSE;
	}
	
	for (selected_items_t::const_iterator selected_it = mSelectedItems.begin(); selected_it != mSelectedItems.end(); ++selected_it)
	{
		const LLFolderViewItem* item = *selected_it;
		if (!item->getViewModelItem()->isItemCopyable())
		{
			return FALSE;
		}
	}
	return TRUE;
}

// copy selected item
void LLFolderView::copy()
{
	// *NOTE: total hack to clear the inventory clipboard
	LLClipboard::instance().reset();
	S32 count = mSelectedItems.size();
	if(getVisible() && getEnabled() && (count > 0))
	{
		LLFolderViewModelItem* listener = NULL;
		selected_items_t::iterator item_it;
		for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
		{
			listener = (*item_it)->getViewModelItem();
			if(listener)
			{
				listener->copyToClipboard();
			}
		}
	}
	mSearchString.clear();
}

BOOL LLFolderView::canCut() const
{
	if (!(getVisible() && getEnabled() && (mSelectedItems.size() > 0)))
	{
		return FALSE;
	}
	
	for (selected_items_t::const_iterator selected_it = mSelectedItems.begin(); selected_it != mSelectedItems.end(); ++selected_it)
	{
		const LLFolderViewItem* item = *selected_it;
		const LLFolderViewModelItem* listener = item->getViewModelItem();

		if (!listener || !listener->isItemRemovable())
		{
			return FALSE;
		}
	}
	return TRUE;
}

void LLFolderView::cut()
{
	// clear the inventory clipboard
	LLClipboard::instance().reset();
	if(getVisible() && getEnabled() && (mSelectedItems.size() > 0))
	{
		// Find out which item will be selected once the selection will be cut
		LLFolderViewItem* item_to_select = getNextUnselectedItem();
		
		// Get the selection: removeItem() modified mSelectedItems and makes iterating on it unwise
		std::set<LLFolderViewItem*> inventory_selected = getSelectionList();

		// Move each item to the clipboard and out of their folder
		for (std::set<LLFolderViewItem*>::iterator item_it = inventory_selected.begin(); item_it != inventory_selected.end(); ++item_it)
		{
			LLFolderViewItem* item_to_cut = *item_it;
			LLFolderViewModelItem* listener = item_to_cut->getViewModelItem();
			if (listener)
			{
				listener->cutToClipboard();
			}
		}
		
		// Update the selection
		setSelection(item_to_select, item_to_select ? item_to_select->isOpen() : false, mParentPanel.get()->hasFocus());
	}
	mSearchString.clear();
}

BOOL LLFolderView::canPaste() const
{
	if (mSelectedItems.empty())
	{
		return FALSE;
	}

	if(getVisible() && getEnabled())
	{
		for (selected_items_t::const_iterator item_it = mSelectedItems.begin();
			 item_it != mSelectedItems.end(); ++item_it)
		{
			// *TODO: only check folders and parent folders of items
			const LLFolderViewItem* item = (*item_it);
			const LLFolderViewModelItem* listener = item->getViewModelItem();
			if(!listener || !listener->isClipboardPasteable())
			{
				const LLFolderViewFolder* folderp = item->getParentFolder();
				listener = folderp->getViewModelItem();
				if (!listener || !listener->isClipboardPasteable())
				{
					return FALSE;
				}
			}
		}
		return TRUE;
	}
	return FALSE;
}

// paste selected item
void LLFolderView::paste()
{
	if(getVisible() && getEnabled())
	{
		// find set of unique folders to paste into
		std::set<LLFolderViewFolder*> folder_set;

		selected_items_t::iterator selected_it;
		for (selected_it = mSelectedItems.begin(); selected_it != mSelectedItems.end(); ++selected_it)
		{
			LLFolderViewItem* item = *selected_it;
			LLFolderViewFolder* folder = dynamic_cast<LLFolderViewFolder*>(item);
			if (folder == NULL)
			{
				folder = item->getParentFolder();
			}
			folder_set.insert(folder);
		}

		std::set<LLFolderViewFolder*>::iterator set_iter;
		for(set_iter = folder_set.begin(); set_iter != folder_set.end(); ++set_iter)
		{
			LLFolderViewModelItem* listener = (*set_iter)->getViewModelItem();
			if(listener && listener->isClipboardPasteable())
			{
				listener->pasteFromClipboard();
			}
		}
	}
	mSearchString.clear();
}

// public rename functionality - can only start the process
void LLFolderView::startRenamingSelectedItem( void )
{
	// make sure selection is visible
	scrollToShowSelection();

	S32 count = mSelectedItems.size();
	LLFolderViewItem* item = NULL;
	if(count > 0)
	{
		item = mSelectedItems.front();
	}
	if(getVisible() && getEnabled() && (count == 1) && item && item->getViewModelItem() &&
	   item->getViewModelItem()->isItemRenameable())
	{
		mRenameItem = item;

		updateRenamerPosition();


		mRenamer->setText(item->getName());
		mRenamer->selectAll();
		mRenamer->setVisible( TRUE );
		// set focus will fail unless item is visible
		mRenamer->setFocus( TRUE );
		mRenamer->setTopLostCallback(boost::bind(&LLFolderView::onRenamerLost, this));
		LLUI::addPopup(mRenamer);
	}
}

BOOL LLFolderView::handleKeyHere( KEY key, MASK mask )
{
	BOOL handled = FALSE;

	// SL-51858: Key presses are not being passed to the Popup menu.
	// A proper fix is non-trivial so instead just close the menu.
	LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
	if (menu && menu->isOpen())
	{
		LLMenuGL::sMenuContainer->hideMenus();
	}

	switch( key )
	{
	case KEY_F2:
		mSearchString.clear();
		startRenamingSelectedItem();
		handled = TRUE;
		break;

	case KEY_RETURN:
		if (mask == MASK_NONE)
		{
			if( mRenameItem && mRenamer->getVisible() )
			{
				finishRenamingItem();
				mSearchString.clear();
				handled = TRUE;
			}
		}
		break;

	case KEY_ESCAPE:
		if( mRenameItem && mRenamer->getVisible() )
		{
			closeRenamer();
			handled = TRUE;
		}
		mSearchString.clear();
		break;

	case KEY_PAGE_UP:
		mSearchString.clear();
		if (mScrollContainer)
		{
		mScrollContainer->pageUp(30);
		}
		handled = TRUE;
		break;

	case KEY_PAGE_DOWN:
		mSearchString.clear();
		if (mScrollContainer)
		{
		mScrollContainer->pageDown(30);
		}
		handled = TRUE;
		break;

	case KEY_HOME:
		mSearchString.clear();
		if (mScrollContainer)
		{
		mScrollContainer->goToTop();
		}
		handled = TRUE;
		break;

	case KEY_END:
		mSearchString.clear();
		if (mScrollContainer)
		{
		mScrollContainer->goToBottom();
		}
		break;

	case KEY_DOWN:
		if((mSelectedItems.size() > 0) && mScrollContainer)
		{
			LLFolderViewItem* last_selected = getCurSelectedItem();
			BOOL shift_select = mask & MASK_SHIFT;
			// don't shift select down to children of folders (they are implicitly selected through parent)
			LLFolderViewItem* next = last_selected->getNextOpenNode(!shift_select);

			if (!mKeyboardSelection || (!shift_select && (!next || next == last_selected)))
			{
				setSelection(last_selected, FALSE, TRUE);
				mKeyboardSelection = TRUE;
			}

			if (shift_select)
			{
				if (next)
				{
					if (next->isSelected())
					{
						// shrink selection
						changeSelection(last_selected, FALSE);
					}
					else if (last_selected->getParentFolder() == next->getParentFolder())
					{
						// grow selection
						changeSelection(next, TRUE);
					}
				}
			}
			else
			{
				if( next )
				{
					if (next == last_selected)
					{
						//special case for LLAccordionCtrl
						if(notifyParent(LLSD().with("action","select_next")) > 0 )//message was processed
						{
							clearSelection();
							return TRUE;
						}
						return FALSE;
					}
					setSelection( next, FALSE, TRUE );
				}
				else
				{
					//special case for LLAccordionCtrl
					if(notifyParent(LLSD().with("action","select_next")) > 0 )//message was processed
					{
						clearSelection();
						return TRUE;
					}
					return FALSE;
				}
			}
			scrollToShowSelection();
			mSearchString.clear();
			handled = TRUE;
		}
		break;

	case KEY_UP:
		if((mSelectedItems.size() > 0) && mScrollContainer)
		{
			LLFolderViewItem* last_selected = mSelectedItems.back();
			BOOL shift_select = mask & MASK_SHIFT;
			// don't shift select down to children of folders (they are implicitly selected through parent)
			LLFolderViewItem* prev = last_selected->getPreviousOpenNode(!shift_select);

			if (!mKeyboardSelection || (!shift_select && prev == this))
			{
				setSelection(last_selected, FALSE, TRUE);
				mKeyboardSelection = TRUE;
			}

			if (shift_select)
			{
				if (prev)
				{
					if (prev->isSelected())
					{
						// shrink selection
						changeSelection(last_selected, FALSE);
					}
					else if (last_selected->getParentFolder() == prev->getParentFolder())
					{
						// grow selection
						changeSelection(prev, TRUE);
					}
				}
			}
			else
			{
				if( prev )
				{
					if (prev == this)
					{
						// If case we are in accordion tab notify parent to go to the previous accordion
						if(notifyParent(LLSD().with("action","select_prev")) > 0 )//message was processed
						{
							clearSelection();
							return TRUE;
						}

						return FALSE;
					}
					setSelection( prev, FALSE, TRUE );
				}
			}
			scrollToShowSelection();
			mSearchString.clear();

			handled = TRUE;
		}
		break;

	case KEY_RIGHT:
		if(mSelectedItems.size())
		{
			LLFolderViewItem* last_selected = getCurSelectedItem();
			last_selected->setOpen( TRUE );
			mSearchString.clear();
			handled = TRUE;
		}
		break;

	case KEY_LEFT:
		if(mSelectedItems.size())
		{
			LLFolderViewItem* last_selected = getCurSelectedItem();
			LLFolderViewItem* parent_folder = last_selected->getParentFolder();
			if (!last_selected->isOpen() && parent_folder && parent_folder->getParentFolder())
			{
				setSelection(parent_folder, FALSE, TRUE);
			}
			else
			{
				last_selected->setOpen( FALSE );
			}
			mSearchString.clear();
			scrollToShowSelection();
			handled = TRUE;
		}
		break;
	}

	return handled;
}


BOOL LLFolderView::handleUnicodeCharHere(llwchar uni_char)
{
	if ((uni_char < 0x20) || (uni_char == 0x7F)) // Control character or DEL
	{
		return FALSE;
	}

	if (uni_char > 0x7f)
	{
		LL_WARNS() << "LLFolderView::handleUnicodeCharHere - Don't handle non-ascii yet, aborting" << LL_ENDL;
		return FALSE;
	}

	BOOL handled = FALSE;
	if (mParentPanel.get()->hasFocus())
	{
		// SL-51858: Key presses are not being passed to the Popup menu.
		// A proper fix is non-trivial so instead just close the menu.
		LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
		if (menu && menu->isOpen())
		{
			LLMenuGL::sMenuContainer->hideMenus();
		}

		//do text search
		if (mSearchTimer.getElapsedTimeF32() > LLUI::sSettingGroups["config"]->getF32("TypeAheadTimeout"))
		{
			mSearchString.clear();
		}
		mSearchTimer.reset();
		if (mSearchString.size() < 128)
		{
			mSearchString += uni_char;
		}
		search(getCurSelectedItem(), mSearchString, FALSE);

		handled = TRUE;
	}

	return handled;
}


BOOL LLFolderView::handleMouseDown( S32 x, S32 y, MASK mask )
{
	mKeyboardSelection = FALSE;
	mSearchString.clear();

	mParentPanel.get()->setFocus(TRUE);

	LLEditMenuHandler::gEditMenuHandler = this;

	return LLView::handleMouseDown( x, y, mask );
}

BOOL LLFolderView::search(LLFolderViewItem* first_item, const std::string &search_string, BOOL backward)
{
	// get first selected item
	LLFolderViewItem* search_item = first_item;

	// make sure search string is upper case
	std::string upper_case_string = search_string;
	LLStringUtil::toUpper(upper_case_string);

	// if nothing selected, select first item in folder
	if (!search_item)
	{
		// start from first item
		search_item = getNextFromChild(NULL);
	}

	// search over all open nodes for first substring match (with wrapping)
	BOOL found = FALSE;
	LLFolderViewItem* original_search_item = search_item;
	do
	{
		// wrap at end
		if (!search_item)
		{
			if (backward)
			{
				search_item = getPreviousFromChild(NULL);
			}
			else
			{
				search_item = getNextFromChild(NULL);
			}
			if (!search_item || search_item == original_search_item)
			{
				break;
			}
		}

		std::string current_item_label(search_item->getViewModelItem()->getSearchableName());
		LLStringUtil::toUpper(current_item_label);
		S32 search_string_length = llmin(upper_case_string.size(), current_item_label.size());
		if (!current_item_label.compare(0, search_string_length, upper_case_string))
		{
			found = TRUE;
			break;
		}
		if (backward)
		{
			search_item = search_item->getPreviousOpenNode();
		}
		else
		{
			search_item = search_item->getNextOpenNode();
		}

	} while(search_item != original_search_item);
	

	if (found)
	{
		setSelection(search_item, FALSE, TRUE);
		scrollToShowSelection();
	}

	return found;
}

BOOL LLFolderView::handleDoubleClick( S32 x, S32 y, MASK mask )
{
	// skip LLFolderViewFolder::handleDoubleClick()
	return LLView::handleDoubleClick( x, y, mask );
}

BOOL LLFolderView::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
	// all user operations move keyboard focus to inventory
	// this way, we know when to stop auto-updating a search
	mParentPanel.get()->setFocus(TRUE);

	BOOL handled = childrenHandleRightMouseDown(x, y, mask) != NULL;
	S32 count = mSelectedItems.size();
	LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
	if (   handled
		&& ( count > 0 && (hasVisibleChildren()) ) // show menu only if selected items are visible
		&& menu )
	{
		if (mCallbackRegistrar)
        {
			mCallbackRegistrar->pushScope();
        }

		updateMenuOptions(menu);
	   
		menu->updateParent(LLMenuGL::sMenuContainer);
		LLMenuGL::showPopup(this, menu, x, y);
		if (mCallbackRegistrar)
        {
			mCallbackRegistrar->popScope();
	}
	}
	else
	{
		if (menu && menu->getVisible())
		{
			menu->setVisible(FALSE);
		}
		setSelection(NULL, FALSE, TRUE);
	}
	return handled;
}

// Add "--no options--" if the menu is completely blank.
BOOL LLFolderView::addNoOptions(LLMenuGL* menu) const
{
	const std::string nooptions_str = "--no options--";
	LLView *nooptions_item = NULL;
	
	const LLView::child_list_t *list = menu->getChildList();
	for (LLView::child_list_t::const_iterator itor = list->begin(); 
		 itor != list->end(); 
		 ++itor)
	{
		LLView *menu_item = (*itor);
		if (menu_item->getVisible())
		{
			return FALSE;
		}
		std::string name = menu_item->getName();
		if (menu_item->getName() == nooptions_str)
		{
			nooptions_item = menu_item;
		}
	}
	if (nooptions_item)
	{
		nooptions_item->setVisible(TRUE);
		nooptions_item->setEnabled(FALSE);
		return TRUE;
	}
	return FALSE;
}

BOOL LLFolderView::handleHover( S32 x, S32 y, MASK mask )
{
	return LLView::handleHover( x, y, mask );
}

BOOL LLFolderView::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data, 
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	mDragAndDropThisFrame = TRUE;
	// have children handle it first
	BOOL handled = LLView::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data,
											 accept, tooltip_msg);

	// when drop is not handled by child, it should be handled
	// by the folder which is the hierarchy root.
	if (!handled)
	{
			handled = LLFolderViewFolder::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
		}

	return handled;
}

void LLFolderView::deleteAllChildren()
{
	closeRenamer();
	if (mPopupMenuHandle.get()) mPopupMenuHandle.get()->die();
	mPopupMenuHandle = LLHandle<LLView>();
	mScrollContainer = NULL;
	mRenameItem = NULL;
	mRenamer = NULL;
	mStatusTextBox = NULL;
	
	clearSelection();
	LLView::deleteAllChildren();
}

void LLFolderView::scrollToShowSelection()
{
	if ( mSelectedItems.size() )
	{
		mNeedsScroll = TRUE;
	}
}

// If the parent is scroll container, scroll it to make the selection
// is maximally visible.
void LLFolderView::scrollToShowItem(LLFolderViewItem* item, const LLRect& constraint_rect)
{
	if (!mScrollContainer) return;

	// don't scroll to items when mouse is being used to scroll/drag and drop
	if (gFocusMgr.childHasMouseCapture(mScrollContainer))
	{
		mNeedsScroll = FALSE;
		return;
	}

	// if item exists and is in visible portion of parent folder...
	if(item)
	{
		LLRect local_rect = item->getLocalRect();
		S32 icon_height = mIcon.isNull() ? 0 : mIcon->getHeight(); 
		S32 label_height = getLabelFontForStyle(mLabelStyle)->getLineHeight(); 
		// when navigating with keyboard, only move top of opened folder on screen, otherwise show whole folder
		S32 max_height_to_show = item->isOpen() && mScrollContainer->hasFocus() ? (llmax( icon_height, label_height ) + item->getIconPad()) : local_rect.getHeight();
		
		// get portion of item that we want to see...
		LLRect item_local_rect = LLRect(item->getIndentation(), 
										local_rect.getHeight(), 
                                        //+40 is supposed to include few first characters
										llmin(item->getLabelXPos() - item->getIndentation() + 40, local_rect.getWidth()), 
										llmax(0, local_rect.getHeight() - max_height_to_show));

		LLRect item_doc_rect;

		item->localRectToOtherView(item_local_rect, &item_doc_rect, this); 

		mScrollContainer->scrollToShowRect( item_doc_rect, constraint_rect );

	}
}

LLRect LLFolderView::getVisibleRect()
{
	S32 visible_height = (mScrollContainer ? mScrollContainer->getRect().getHeight() : 0);
	S32 visible_width  = (mScrollContainer ? mScrollContainer->getRect().getWidth()  : 0);
	LLRect visible_rect;
	visible_rect.setLeftTopAndSize(-getRect().mLeft, visible_height - getRect().mBottom, visible_width, visible_height);
	return visible_rect;
}

BOOL LLFolderView::getShowSelectionContext()
{
	if (mShowSelectionContext)
	{
		return TRUE;
	}
	LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
	if (menu && menu->getVisible())
	{
		return TRUE;
	}
	return FALSE;
}

void LLFolderView::setShowSingleSelection(bool show)
{
	if (show != mShowSingleSelection)
	{
		mMultiSelectionFadeTimer.reset();
		mShowSingleSelection = show;
	}
}

static LLTrace::BlockTimerStatHandle FTM_AUTO_SELECT("Open and Select");
static LLTrace::BlockTimerStatHandle FTM_INVENTORY("Inventory");

// Main idle routine
void LLFolderView::update()
{
	// If this is associated with the user's inventory, don't do anything
	// until that inventory is loaded up.
	LL_RECORD_BLOCK_TIME(FTM_INVENTORY);
    
    // If there's no model, the view is in suspended state (being deleted) and shouldn't be updated
    if (getFolderViewModel() == NULL)
    {
        return;
    }

	LLFolderViewFilter& filter_object = getFolderViewModel()->getFilter();

	if (filter_object.isModified() && filter_object.isNotDefault() && mParentPanel.get()->getVisible())
	{
		mNeedsAutoSelect = TRUE;
	}
    
	// Filter to determine visibility before arranging
	filter(filter_object);
    
	// Clear the modified setting on the filter only if the filter finished after running the filter process
	// Note: if the filter count has timed out, that means the filter halted before completing the entire set of items
    if (filter_object.isModified() && (!filter_object.isTimedOut()))
	{
		filter_object.clearModified();
	}

	// automatically show matching items, and select first one if we had a selection
	if (mNeedsAutoSelect)
	{
		LL_RECORD_BLOCK_TIME(FTM_AUTO_SELECT);
		// select new item only if a filtered item not currently selected and there was a selection
		LLFolderViewItem* selected_itemp = mSelectedItems.empty() ? NULL : mSelectedItems.back();
		if (!mAutoSelectOverride && selected_itemp && !selected_itemp->getViewModelItem()->potentiallyVisible())
		{
			// these are named variables to get around gcc not binding non-const references to rvalues
			// and functor application is inherently non-const to allow for stateful functors
			LLSelectFirstFilteredItem functor;
			applyFunctorRecursively(functor);
		}

		// Open filtered folders for folder views with mAutoSelectOverride=TRUE.
		// Used by LLPlacesFolderView.
		if (filter_object.showAllResults())
		{
			// these are named variables to get around gcc not binding non-const references to rvalues
			// and functor application is inherently non-const to allow for stateful functors
			LLOpenFilteredFolders functor;
			applyFunctorRecursively(functor);
		}

		scrollToShowSelection();
	}

	BOOL filter_finished = mViewModel->contentsReady()
							&& (getViewModelItem()->passedFilter()
								|| ( getViewModelItem()->getLastFilterGeneration() >= filter_object.getFirstSuccessGeneration()
									&& !filter_object.isModified()));
	if (filter_finished 
		|| gFocusMgr.childHasKeyboardFocus(mParentPanel.get())
		|| gFocusMgr.childHasMouseCapture(mParentPanel.get()))
	{
		// finishing the filter process, giving focus to the folder view, or dragging the scrollbar all stop the auto select process
		mNeedsAutoSelect = FALSE;
	}

  BOOL is_visible = isInVisibleChain();

  //Puts folders/items in proper positions
  // arrange() takes the model filter flag into account and call sort() if necessary (CHUI-849)
  // It also handles the open/close folder animation
  if ( is_visible )
  {
    sanitizeSelection();
    if( needsArrange() )
    {
      S32 height = 0;
      S32 width = 0;
      S32 total_height = arrange( &width, &height );
      notifyParent(LLSD().with("action", "size_changes").with("height", total_height));
    }
  }

	// during filtering process, try to pin selected item's location on screen
	// this will happen when searching your inventory and when new items arrive
	if (!filter_finished)
	{
		// calculate rectangle to pin item to at start of animated rearrange
		if (!mPinningSelectedItem && !mSelectedItems.empty())
		{
			// lets pin it!
			mPinningSelectedItem = TRUE;

      //Computes visible area 
			const LLRect visible_content_rect = (mScrollContainer ? mScrollContainer->getVisibleContentRect() : LLRect());
			LLFolderViewItem* selected_item = mSelectedItems.back();

      //Computes location of selected content, content outside visible area will be scrolled to using below code
			LLRect item_rect;
			selected_item->localRectToOtherView(selected_item->getLocalRect(), &item_rect, this);
			
      //Computes intersected region of the selected content and visible area
      LLRect overlap_rect(item_rect);
      overlap_rect.intersectWith(visible_content_rect);

      //Don't scroll when the selected content exists within the visible area
			if (overlap_rect.getHeight() >= selected_item->getItemHeight())
			{
				// then attempt to keep it in same place on screen
				mScrollConstraintRect = item_rect;
				mScrollConstraintRect.translate(-visible_content_rect.mLeft, -visible_content_rect.mBottom);
			}
      //Scroll because the selected content is outside the visible area
			else
			{
				// otherwise we just want it onscreen somewhere
				LLRect content_rect = (mScrollContainer ? mScrollContainer->getContentWindowRect() : LLRect());
				mScrollConstraintRect.setOriginAndSize(0, 0, content_rect.getWidth(), content_rect.getHeight());
			}
		}
	}
	else
	{
		// stop pinning selected item after folders stop rearranging
		if (!needsArrange())
		{
			mPinningSelectedItem = FALSE;
		}
	}

	LLRect constraint_rect;
	if (mPinningSelectedItem)
	{
		// use last known constraint rect for pinned item
		constraint_rect = mScrollConstraintRect;
	}
	else
	{
		// during normal use (page up/page down, etc), just try to fit item on screen
		LLRect content_rect = (mScrollContainer ? mScrollContainer->getContentWindowRect() : LLRect());
		constraint_rect.setOriginAndSize(0, 0, content_rect.getWidth(), content_rect.getHeight());
	}

	if (mSelectedItems.size() && mNeedsScroll)
	{
		scrollToShowItem(mSelectedItems.back(), constraint_rect);
		// continue scrolling until animated layout change is done
		if (filter_finished
			&& (!needsArrange() || !is_visible))
		{
			mNeedsScroll = FALSE;
		}
	}

	if (mSignalSelectCallback)
	{
		//RN: we use keyboard focus as a proxy for user-explicit actions
		BOOL take_keyboard_focus = (mSignalSelectCallback == SIGNAL_KEYBOARD_FOCUS);
		mSelectSignal(mSelectedItems, take_keyboard_focus);
	}
	mSignalSelectCallback = FALSE;
}

void LLFolderView::dumpSelectionInformation()
{
	LL_INFOS() << "LLFolderView::dumpSelectionInformation()" << LL_NEWLINE
				<< "****************************************" << LL_ENDL;
	selected_items_t::iterator item_it;
	for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
	{
		LL_INFOS() << "  " << (*item_it)->getName() << LL_ENDL;
	}
	LL_INFOS() << "****************************************" << LL_ENDL;
}

void LLFolderView::updateRenamerPosition()
{
	if(mRenameItem)
	{
		// See also LLFolderViewItem::draw()
		S32 x = mRenameItem->getLabelXPos();
		S32 y = mRenameItem->getRect().getHeight() - mRenameItem->getItemHeight() - RENAME_HEIGHT_PAD;
		mRenameItem->localPointToScreen( x, y, &x, &y );
		screenPointToLocal( x, y, &x, &y );
		mRenamer->setOrigin( x, y );

		LLRect scroller_rect(0, 0, (S32)LLUI::getWindowSize().mV[VX], 0);
		if (mScrollContainer)
		{
			scroller_rect = mScrollContainer->getContentWindowRect();
		}

		S32 width = llmax(llmin(mRenameItem->getRect().getWidth() - x, scroller_rect.getWidth() - x - getRect().mLeft), MINIMUM_RENAMER_WIDTH);
		S32 height = mRenameItem->getItemHeight() - RENAME_HEIGHT_PAD;
		mRenamer->reshape( width, height, TRUE );
	}
}

// Update visibility and availability (i.e. enabled/disabled) of context menu items.
void LLFolderView::updateMenuOptions(LLMenuGL* menu)
{
	const LLView::child_list_t *list = menu->getChildList();

	LLView::child_list_t::const_iterator menu_itor;
	for (menu_itor = list->begin(); menu_itor != list->end(); ++menu_itor)
	{
		(*menu_itor)->setVisible(FALSE);
		(*menu_itor)->pushVisible(TRUE);
		(*menu_itor)->setEnabled(TRUE);
	}

	// Successively filter out invalid options
	U32 multi_select_flag = (mSelectedItems.size() > 1 ? ITEM_IN_MULTI_SELECTION : 0x0);
	U32 flags = multi_select_flag | FIRST_SELECTED_ITEM;
	for (selected_items_t::iterator item_itor = mSelectedItems.begin();
			item_itor != mSelectedItems.end();
			++item_itor)
	{
		LLFolderViewItem* selected_item = (*item_itor);
		selected_item->buildContextMenu(*menu, flags);
		flags = multi_select_flag;
	}

	// This adds a check for restrictions based on the entire
	// selection set - for example, any one wearable may not push you
	// over the limit, but all wearables together still might.
    if (getFolderViewGroupedItemModel())
    {
        getFolderViewGroupedItemModel()->groupFilterContextMenu(mSelectedItems,*menu);
    }

	addNoOptions(menu);
}

// Refresh the context menu (that is already shown).
void LLFolderView::updateMenu()
{
	LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
	if (menu && menu->getVisible())
	{
		updateMenuOptions(menu);
		menu->needsArrange(); // update menu height if needed
	}
}

bool LLFolderView::selectFirstItem()
{
	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();++iter)
	{
		LLFolderViewFolder* folder = (*iter );
		if (folder->getVisible())
		{
			LLFolderViewItem* itemp = folder->getNextFromChild(0,true);
			if(itemp)
				setSelection(itemp,FALSE,TRUE);
			return true;	
		}
		
	}
	for(items_t::iterator iit = mItems.begin();
		iit != mItems.end(); ++iit)
	{
		LLFolderViewItem* itemp = (*iit);
		if (itemp->getVisible())
		{
			setSelection(itemp,FALSE,TRUE);
			return true;	
		}
	}
	return false;
}
bool LLFolderView::selectLastItem()
{
	for(items_t::reverse_iterator iit = mItems.rbegin();
		iit != mItems.rend(); ++iit)
	{
		LLFolderViewItem* itemp = (*iit);
		if (itemp->getVisible())
		{
			setSelection(itemp,FALSE,TRUE);
			return true;	
		}
	}
	for (folders_t::reverse_iterator iter = mFolders.rbegin();
		 iter != mFolders.rend();++iter)
	{
		LLFolderViewFolder* folder = (*iter);
		if (folder->getVisible())
		{
			LLFolderViewItem* itemp = folder->getPreviousFromChild(0,true);
			if(itemp)
				setSelection(itemp,FALSE,TRUE);
			return true;	
		}
	}
	return false;
}


S32	LLFolderView::notify(const LLSD& info) 
{
	if(info.has("action"))
	{
		std::string str_action = info["action"];
		if(str_action == "select_first")
		{
			setFocus(true);
			selectFirstItem();
			scrollToShowSelection();
			return 1;

		}
		else if(str_action == "select_last")
		{
			setFocus(true);
			selectLastItem();
			scrollToShowSelection();
			return 1;
		}
	}
	return 0;
}


///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------

void LLFolderView::onRenamerLost()
{
	if (mRenamer && mRenamer->getVisible())
	{
		mRenamer->setVisible(FALSE);

		// will commit current name (which could be same as original name)
		mRenamer->setFocus(FALSE);
	}

	if( mRenameItem )
	{
		setSelection( mRenameItem, TRUE );
		mRenameItem = NULL;
	}
}

LLFolderViewItem* LLFolderView::getNextUnselectedItem()
{
	LLFolderViewItem* last_item = *mSelectedItems.rbegin();
	LLFolderViewItem* new_selection = last_item->getNextOpenNode(FALSE);
	while(new_selection && new_selection->isSelected())
	{
		new_selection = new_selection->getNextOpenNode(FALSE);
	}
	if (!new_selection)
	{
		new_selection = last_item->getPreviousOpenNode(FALSE);
		while (new_selection && (new_selection->isInSelection()))
		{
			new_selection = new_selection->getPreviousOpenNode(FALSE);
		}
	}
	return new_selection;
}

S32 LLFolderView::getItemHeight()
{
	if(!hasVisibleChildren())
{
		//We need to display status textbox, let's reserve some place for it
		return llmax(0, mStatusTextBox->getTextPixelHeight());
}
	return 0;
}
