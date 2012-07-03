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

#include "llviewerprecompiledheaders.h"

#include "llfolderview.h"
#include "llfolderview.h"

#include "llcallbacklist.h"
#include "llinventorybridge.h"
#include "llclipboard.h" // *TODO: remove this once hack below gone.
#include "llinventorypanel.h"
#include "llfoldertype.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llpanel.h"
#include "llpreview.h"
#include "llscrollcontainer.h" // hack to allow scrolling
#include "lltooldraganddrop.h"
#include "lltrans.h"
#include "llui.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llviewerjointattachment.h"
#include "llviewermenu.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llviewerfoldertype.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llfloaterproperties.h"
#include "llnotificationsutil.h"

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

const S32 RENAME_WIDTH_PAD = 4;
const S32 RENAME_HEIGHT_PAD = 1;
const S32 AUTO_OPEN_STACK_DEPTH = 16;
const S32 MIN_ITEM_WIDTH_VISIBLE = LLFolderViewItem::ICON_WIDTH
			+ LLFolderViewItem::ICON_PAD 
			+ LLFolderViewItem::ARROW_SIZE 
			+ LLFolderViewItem::TEXT_PAD 
			+ /*first few characters*/ 40;
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
:	task_id("task_id"),
	title("title"),
	use_label_suffix("use_label_suffix"),
	allow_multiselect("allow_multiselect", true),
	show_empty_message("show_empty_message", true),
	use_ellipses("use_ellipses", false)
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
	mSourceID(p.task_id),
	mRenameItem( NULL ),
	mNeedsScroll( FALSE ),
	mUseLabelSuffix(p.use_label_suffix),
	mPinningSelectedItem(FALSE),
	mNeedsAutoSelect( FALSE ),
	mAutoSelectOverride(FALSE),
	mNeedsAutoRename(FALSE),
	mSortOrder(LLInventoryFilter::SO_FOLDERS_BY_NAME),	// This gets overridden by a pref immediately
	mShowSelectionContext(FALSE),
	mShowSingleSelection(FALSE),
	mArrangeGeneration(0),
	mSignalSelectCallback(0),
	mMinWidth(0),
	mDragAndDropThisFrame(FALSE),
	mCallbackRegistrar(NULL),
	mParentPanel(p.parent_panel),
	mUseEllipses(p.use_ellipses),
	mDraggingOverItem(NULL),
	mStatusTextBox(NULL),
	mShowItemLinkOverlays(p.show_item_link_overlays),
	mViewModel(p.view_model)
{
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
	mIndentation = p.folder_indentation;
	gIdleCallbacks.addFunction(idle, this);

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
	LLRect new_r = LLRect(rect.mLeft + ICON_PAD,
			      rect.mTop - TEXT_PAD,
			      rect.mRight,
			      rect.mTop - TEXT_PAD - font->getLineHeight());
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
	//addChild(mStatusTextBox);


	// make the popup menu available
	LLMenuGL* menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_inventory.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
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

	mAutoOpenItems.removeAllNodes();
	gIdleCallbacks.deleteFunction(idle, this);

	if (mPopupMenuHandle.get()) mPopupMenuHandle.get()->die();

	mAutoOpenItems.removeAllNodes();
	clearSelection();
	mItems.clear();
	mFolders.clear();

	delete mViewModel;
	mViewModel = NULL;
}

BOOL LLFolderView::canFocusChildren() const
{
	return FALSE;
}

BOOL LLFolderView::addFolder( LLFolderViewFolder* folder)
{
	LLFolderViewFolder::addFolder(folder);

	mFolders.remove(folder);
	// enforce sort order of My Inventory followed by Library
	if (((LLFolderViewModelItemInventory*)folder->getViewModelItem())->getUUID() == gInventory.getLibraryRootFolderID())
	{
		mFolders.push_back(folder);
	}
	else
	{
		mFolders.insert(mFolders.begin(), folder);
	}

	return TRUE;
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

	LLRect scroll_rect = mScrollContainer->getContentWindowRect();
	reshape( llmax(scroll_rect.getWidth(), mMinWidth), llround(mCurHeight) );

	LLRect new_scroll_rect = mScrollContainer->getContentWindowRect();
	if (new_scroll_rect.getWidth() != scroll_rect.getWidth())
	{
		reshape( llmax(scroll_rect.getWidth(), mMinWidth), llround(mCurHeight) );
	}

	// move item renamer text field to item's new position
	updateRenamerPosition();

	return llround(mTargetHeight);
}

static LLFastTimer::DeclareTimer FTM_FILTER("Filter Folder View");

void LLFolderView::filter( LLFolderViewFilter& filter )
{
	LLFastTimer t2(FTM_FILTER);
	filter.setFilterCount(llclamp(gSavedSettings.getS32("FilterItemsPerFrame"), 1, 5000));

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
	width = llmax(mMinWidth, scroll_rect.getWidth());
	height = llmax(llround(mCurHeight), scroll_rect.getHeight());

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
		mParentPanel->setFocus(TRUE);
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

static LLFastTimer::DeclareTimer FTM_SANITIZE_SELECTION("Sanitize Selection");
void LLFolderView::sanitizeSelection()
{
	LLFastTimer _(FTM_SANITIZE_SELECTION);
	// store off current item in case it is automatically deselected
	// and we want to preserve context
	LLFolderViewItem* original_selected_item = getCurSelectedItem();

	std::vector<LLFolderViewItem*> items_to_remove;
	selected_items_t::iterator item_iter;
	for (item_iter = mSelectedItems.begin(); item_iter != mSelectedItems.end(); ++item_iter)
	{
		LLFolderViewItem* item = *item_iter;

		// ensure that each ancestor is open and potentially passes filtering
		BOOL visible = item->getViewModelItem()->potentiallyVisible(); // initialize from filter state for this item
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
				if (parent_folder->getViewModelItem()->potentiallyVisible())
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

BOOL LLFolderView::startDrag(LLToolDragAndDrop::ESource source)
{
	std::vector<EDragAndDropType> types;
	uuid_vec_t cargo_ids;
	selected_items_t::iterator item_it;
	BOOL can_drag = TRUE;
	if (!mSelectedItems.empty())
	{
		for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
		{
			EDragAndDropType type = DAD_NONE;
			LLUUID id = LLUUID::null;
			can_drag = can_drag && (*item_it)->getViewModelItem()->startDrag(&type, &id);

			types.push_back(type);
			cargo_ids.push_back(id);
		}

		LLToolDragAndDrop::getInstance()->beginMultiDrag(types, cargo_ids, source, mSourceID); 
	}
	return can_drag;
}

void LLFolderView::commitRename( const LLSD& data )
{
	finishRenamingItem();
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

	// while dragging, update selection rendering to reflect single/multi drag status
	if (LLToolDragAndDrop::getInstance()->hasMouseCapture())
	{
		EAcceptance last_accept = LLToolDragAndDrop::getInstance()->getLastAccept();
		if (last_accept == ACCEPT_YES_SINGLE || last_accept == ACCEPT_YES_COPY_SINGLE)
		{
			setShowSingleSelection(TRUE);
		}
		else
		{
			setShowSingleSelection(FALSE);
		}
	}
	else
	{
		setShowSingleSelection(FALSE);
	}


	if (mSearchTimer.getElapsedTimeF32() > gSavedSettings.getF32("TypeAheadTimeout") || !mSearchString.size())
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
		bool height_changed = local_rect.getHeight() != pixel_height;
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

	// List is re-sorted alphabetically, so scroll to make sure the selected item is visible.
	scrollToShowSelection();
}

void LLFolderView::closeRenamer( void )
{
	if (mRenamer && mRenamer->getVisible())
	{
		// Triggers onRenamerLost() that actually closes the renamer.
		gViewerWindow->removePopup(mRenamer);
	}
}

void LLFolderView::removeSelectedItems( void )
{
	if (mSelectedItems.empty()) return;
	LLSD args;
	args["QUESTION"] = LLTrans::getString(mSelectedItems.size() > 1 ? "DeleteItems" :  "DeleteItem");
	LLNotificationsUtil::add("DeleteItems", args, LLSD(), boost::bind(&LLFolderView::onItemsRemovalConfirmation, this, _1, _2));
}

bool isDescendantOfASelectedItem(LLFolderViewItem* item, const std::vector<LLFolderViewItem*>& selectedItems)
{
	LLFolderViewItem* item_parent = dynamic_cast<LLFolderViewItem*>(item->getParent());

	if (item_parent)
	{
		for(std::vector<LLFolderViewItem*>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it)
		{
			const LLFolderViewItem* const selected_item = (*it);

			LLFolderViewItem* parent = item_parent;

			while (parent)
			{
				if (selected_item == parent)
				{
					return true;
				}

				parent = dynamic_cast<LLFolderViewItem*>(parent->getParent());
			}
		}
	}

	return false;
}

// static
void LLFolderView::removeCutItems()
{
	// There's no item in "cut" mode on the clipboard -> exit
	if (!LLClipboard::instance().isCutMode())
		return;

	// Get the list of clipboard item uuids and iterate through them
	LLDynamicArray<LLUUID> objects;
	LLClipboard::instance().pasteFromClipboard(objects);
	for (LLDynamicArray<LLUUID>::const_iterator iter = objects.begin();
		 iter != objects.end();
		 ++iter)
	{
		gInventory.removeObject(*iter);
	}
}

void LLFolderView::onItemsRemovalConfirmation(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0) return; // canceled

	if(getVisible() && getEnabled())
	{
		// just in case we're removing the renaming item.
		mRenameItem = NULL;

		// create a temporary structure which we will use to remove
		// items, since the removal will futz with internal data
		// structures.
		std::vector<LLFolderViewItem*> items;
		S32 count = mSelectedItems.size();
		if(count == 0) return;
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
				llinfos << "Cannot delete " << item->getName() << llendl;
				return;
			}
		}

		// iterate through the new container.
		count = items.size();
		LLUUID new_selection_id;
		if(count == 1)
		{
			LLFolderViewItem* item_to_delete = items[0];
			LLFolderViewFolder* parent = item_to_delete->getParentFolder();
			LLFolderViewItem* new_selection = item_to_delete->getNextOpenNode(FALSE);
			if (!new_selection)
			{
				new_selection = item_to_delete->getPreviousOpenNode(FALSE);
			}
			if(parent)
			{
				if (item_to_delete->remove())
				{
					// change selection on successful delete
					if (new_selection)
					{
						getRoot()->setSelection(new_selection, new_selection->isOpen(), mParentPanel->hasFocus());
					}
					else
					{
						getRoot()->setSelection(NULL, mParentPanel->hasFocus());
					}
				}
			}
			arrangeAll();
		}
		else if (count > 1)
		{
			LLDynamicArray<LLFolderViewModelItem*> listeners;
			LLFolderViewModelItem* listener;
			LLFolderViewItem* last_item = items[count - 1];
			LLFolderViewItem* new_selection = last_item->getNextOpenNode(FALSE);
			while(new_selection && new_selection->isSelected())
			{
				new_selection = new_selection->getNextOpenNode(FALSE);
			}
			if (!new_selection)
			{
				new_selection = last_item->getPreviousOpenNode(FALSE);
				while (new_selection && (new_selection->isSelected() || isDescendantOfASelectedItem(new_selection, items)))
				{
					new_selection = new_selection->getPreviousOpenNode(FALSE);
				}
			}
			if (new_selection)
			{
				getRoot()->setSelection(new_selection, new_selection->isOpen(), mParentPanel->hasFocus());
			}
			else
			{
				getRoot()->setSelection(NULL, mParentPanel->hasFocus());
			}

			for(S32 i = 0; i < count; ++i)
			{
				listener = items[i]->getViewModelItem();
				if(listener && (listeners.find(listener) == LLDynamicArray<LLFolderViewModelItem*>::FAIL))
				{
					listeners.put(listener);
				}
			}
			listener = static_cast<LLFolderViewModelItem*>(listeners.get(0));
			if(listener)
			{
				listener->removeBatch(listeners);
			}
		}
		arrangeAll();
		scrollToShowSelection();
	}
}

// open the selected item.
void LLFolderView::openSelectedItems( void )
{
	if(getVisible() && getEnabled())
	{
		if (mSelectedItems.size() == 1)
		{
			mSelectedItems.front()->openItem();
		}
		else
		{
			LLMultiPreview* multi_previewp = new LLMultiPreview();
			LLMultiProperties* multi_propertiesp = new LLMultiProperties();

			selected_items_t::iterator item_it;
			for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
			{
				// IT_{OBJECT,ATTACHMENT} creates LLProperties
				// floaters; others create LLPreviews.  Put
				// each one in the right type of container.
				LLFolderViewModelItemInventory* listener = static_cast<LLFolderViewModelItemInventory*>((*item_it)->getViewModelItem());
				bool is_prop = listener && (listener->getInventoryType() == LLInventoryType::IT_OBJECT || listener->getInventoryType() == LLInventoryType::IT_ATTACHMENT);
				if (is_prop)
					LLFloater::setFloaterHost(multi_propertiesp);
				else
					LLFloater::setFloaterHost(multi_previewp);
				listener->openItem();
			}

			LLFloater::setFloaterHost(NULL);
			// *NOTE: LLMulti* will safely auto-delete when open'd
			// without any children.
			multi_previewp->openFloater(LLSD());
			multi_propertiesp->openFloater(LLSD());
		}
	}
}

void LLFolderView::propertiesSelectedItems( void )
{
	//TODO RN: get working again
	//if(getVisible() && getEnabled())
	//{
	//	if (mSelectedItems.size() == 1)
	//	{
	//		LLFolderViewItem* folder_item = mSelectedItems.front();
	//		if(!folder_item) return;
	//		folder_item->getViewModelItem()->showProperties();
	//	}
	//	else
	//	{
	//		LLMultiProperties* multi_propertiesp = new LLMultiProperties();

	//		LLFloater::setFloaterHost(multi_propertiesp);

	//		selected_items_t::iterator item_it;
	//		for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
	//		{
	//			(*item_it)->getViewModelItem()->showProperties();
	//		}

	//		LLFloater::setFloaterHost(NULL);
	//		multi_propertiesp->openFloater(LLSD());
	//	}
	//}
}

void LLFolderView::changeType(LLInventoryModel *model, LLFolderType::EType new_folder_type)
{
	LLFolderBridge *folder_bridge = LLFolderBridge::sSelf.get();

	if (!folder_bridge) return;
	LLViewerInventoryCategory *cat = folder_bridge->getCategory();
	if (!cat) return;
	cat->changeType(new_folder_type);
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
	LLRect content_rect = mScrollContainer->getContentWindowRect();
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
				listener->cutToClipboard();
			}
		}
		LLFolderView::removeCutItems();
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
				item = item->getParentFolder();
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
		gViewerWindow->addPopup(mRenamer);
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

	LLView *item = NULL;
	if (getChildCount() > 0)
	{
		item = *(getChildList()->begin());
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
			else
			{
				LLFolderView::openSelectedItems();
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
		mScrollContainer->pageUp(30);
		handled = TRUE;
		break;

	case KEY_PAGE_DOWN:
		mSearchString.clear();
		mScrollContainer->pageDown(30);
		handled = TRUE;
		break;

	case KEY_HOME:
		mSearchString.clear();
		mScrollContainer->goToTop();
		handled = TRUE;
		break;

	case KEY_END:
		mSearchString.clear();
		mScrollContainer->goToBottom();
		break;

	case KEY_DOWN:
		if((mSelectedItems.size() > 0) && mScrollContainer)
		{
			LLFolderViewItem* last_selected = getCurSelectedItem();

			if (!mKeyboardSelection)
			{
				setSelection(last_selected, FALSE, TRUE);
				mKeyboardSelection = TRUE;
			}

			LLFolderViewItem* next = NULL;
			if (mask & MASK_SHIFT)
			{
				// don't shift select down to children of folders (they are implicitly selected through parent)
				next = last_selected->getNextOpenNode(FALSE);
				if (next)
				{
					if (next->isSelected())
					{
						// shrink selection
						getRoot()->changeSelection(last_selected, FALSE);
					}
					else if (last_selected->getParentFolder() == next->getParentFolder())
					{
						// grow selection
						getRoot()->changeSelection(next, TRUE);
					}
				}
			}
			else
			{
				next = last_selected->getNextOpenNode();
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

			if (!mKeyboardSelection)
			{
				setSelection(last_selected, FALSE, TRUE);
				mKeyboardSelection = TRUE;
			}

			LLFolderViewItem* prev = NULL;
			if (mask & MASK_SHIFT)
			{
				// don't shift select down to children of folders (they are implicitly selected through parent)
				prev = last_selected->getPreviousOpenNode(FALSE);
				if (prev)
				{
					if (prev->isSelected())
					{
						// shrink selection
						getRoot()->changeSelection(last_selected, FALSE);
					}
					else if (last_selected->getParentFolder() == prev->getParentFolder())
					{
						// grow selection
						getRoot()->changeSelection(prev, TRUE);
					}
				}
			}
			else
			{
				prev = last_selected->getPreviousOpenNode();
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

	if (!handled && mParentPanel->hasFocus())
	{
		if (key == KEY_BACKSPACE)
		{
			mSearchTimer.reset();
			if (mSearchString.size())
			{
				mSearchString.erase(mSearchString.size() - 1, 1);
			}
			search(getCurSelectedItem(), mSearchString, FALSE);
			handled = TRUE;
		}
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
		llwarns << "LLFolderView::handleUnicodeCharHere - Don't handle non-ascii yet, aborting" << llendl;
		return FALSE;
	}

	BOOL handled = FALSE;
	if (mParentPanel->hasFocus())
	{
		// SL-51858: Key presses are not being passed to the Popup menu.
		// A proper fix is non-trivial so instead just close the menu.
		LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
		if (menu && menu->isOpen())
		{
			LLMenuGL::sMenuContainer->hideMenus();
		}

		//do text search
		if (mSearchTimer.getElapsedTimeF32() > gSavedSettings.getF32("TypeAheadTimeout"))
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


BOOL LLFolderView::canDoDelete() const
{
	if (mSelectedItems.size() == 0) return FALSE;

	for (selected_items_t::const_iterator item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
	{
		if (!(*item_it)->getViewModelItem()->isItemRemovable())
		{
			return FALSE;
		}
	}
	return TRUE;
}

void LLFolderView::doDelete()
{
	if(mSelectedItems.size() > 0)
	{				
		removeSelectedItems();
	}
}


BOOL LLFolderView::handleMouseDown( S32 x, S32 y, MASK mask )
{
	mKeyboardSelection = FALSE;
	mSearchString.clear();

	mParentPanel->setFocus(TRUE);

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

		const std::string current_item_label(search_item->getViewModelItem()->getSearchableName());
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
	mParentPanel->setFocus(TRUE);

	BOOL handled = childrenHandleRightMouseDown(x, y, mask) != NULL;
	S32 count = mSelectedItems.size();
	LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
	if (   handled
		&& ( count > 0 && (hasVisibleChildren()) ) // show menu only if selected items are visible
		&& menu )
	{
		if (mCallbackRegistrar)
			mCallbackRegistrar->pushScope();

		updateMenuOptions(menu);
	   
		menu->updateParent(LLMenuGL::sMenuContainer);
		LLMenuGL::showPopup(this, menu, x, y);
		if (mCallbackRegistrar)
			mCallbackRegistrar->popScope();
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
		LLRect item_scrolled_rect; // item position relative to display area of scroller
		LLRect visible_doc_rect = mScrollContainer->getVisibleContentRect();
		
		S32 icon_height = mIcon.isNull() ? 0 : mIcon->getHeight(); 
		S32 label_height = getLabelFontForStyle(mLabelStyle)->getLineHeight(); 
		// when navigating with keyboard, only move top of opened folder on screen, otherwise show whole folder
		S32 max_height_to_show = item->isOpen() && mScrollContainer->hasFocus() ? (llmax( icon_height, label_height ) + ICON_PAD) : local_rect.getHeight(); 
		
		// get portion of item that we want to see...
		LLRect item_local_rect = LLRect(item->getIndentation(), 
										local_rect.getHeight(), 
										llmin(MIN_ITEM_WIDTH_VISIBLE, local_rect.getWidth()), 
										llmax(0, local_rect.getHeight() - max_height_to_show));

		LLRect item_doc_rect;

		item->localRectToOtherView(item_local_rect, &item_doc_rect, this); 

		mScrollContainer->scrollToShowRect( item_doc_rect, constraint_rect );

	}
}

LLRect LLFolderView::getVisibleRect()
{
	S32 visible_height = mScrollContainer->getRect().getHeight();
	S32 visible_width = mScrollContainer->getRect().getWidth();
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

void LLFolderView::setShowSingleSelection(BOOL show)
{
	if (show != mShowSingleSelection)
	{
		mMultiSelectionFadeTimer.reset();
		mShowSingleSelection = show;
	}
}

bool LLFolderView::doToSelected(LLInventoryModel* model, const LLSD& userdata)
{
	std::string action = userdata.asString();
	
	if ("rename" == action)
	{
		startRenamingSelectedItem();
		return true;
	}
	if ("delete" == action)
	{
		removeSelectedItems();
		return true;
	}
	if (("copy" == action) || ("cut" == action))
	{	
		// Clear the clipboard before we start adding things on it
		LLClipboard::instance().reset();
	}

	static const std::string change_folder_string = "change_folder_type_";
	if (action.length() > change_folder_string.length() && 
		(action.compare(0,change_folder_string.length(),"change_folder_type_") == 0))
	{
		LLFolderType::EType new_folder_type = LLViewerFolderType::lookupTypeFromXUIName(action.substr(change_folder_string.length()));
		changeType(model, new_folder_type);
		return true;
	}


	std::set<LLFolderViewItem*> selected_items = getSelectionList();

	LLMultiPreview* multi_previewp = NULL;
	LLMultiProperties* multi_propertiesp = NULL;

	if (("task_open" == action  || "open" == action) && selected_items.size() > 1)
	{
		multi_previewp = new LLMultiPreview();
		gFloaterView->addChild(multi_previewp);

		LLFloater::setFloaterHost(multi_previewp);
	
	}
	else if (("task_properties" == action || "properties" == action) && selected_items.size() > 1)
	{
		multi_propertiesp = new LLMultiProperties();
		gFloaterView->addChild(multi_propertiesp);

		LLFloater::setFloaterHost(multi_propertiesp);
	}

	std::set<LLFolderViewItem*>::iterator set_iter;

	for (set_iter = selected_items.begin(); set_iter != selected_items.end(); ++set_iter)
	{
		LLFolderViewItem* folder_item = *set_iter;
		if(!folder_item) continue;
		LLInvFVBridge* bridge = (LLInvFVBridge*)folder_item->getViewModelItem();
		if(!bridge) continue;
		bridge->performAction(model, action);
	}

	LLFloater::setFloaterHost(NULL);
	if (multi_previewp)
	{
		multi_previewp->openFloater(LLSD());
	}
	else if (multi_propertiesp)
	{
		multi_propertiesp->openFloater(LLSD());
	}

	return true;
}

static LLFastTimer::DeclareTimer FTM_AUTO_SELECT("Open and Select");
static LLFastTimer::DeclareTimer FTM_INVENTORY("Inventory");

// Main idle routine
void LLFolderView::doIdle()
{
	// If this is associated with the user's inventory, don't do anything
	// until that inventory is loaded up.
	const LLInventoryPanel *inventory_panel = dynamic_cast<LLInventoryPanel*>(mParentPanel);
	if (inventory_panel && !inventory_panel->getIsViewsInitialized())
	{
		return;
	}
	
	LLFastTimer t2(FTM_INVENTORY);

	if (getFolderViewModel()->getFilter()->isModified() && getFolderViewModel()->getFilter()->isNotDefault())
	{
		mNeedsAutoSelect = TRUE;
	}
	getFolderViewModel()->getFilter()->clearModified();

	// filter to determine visibility before arranging
	filter(*(getFolderViewModel()->getFilter()));

	// automatically show matching items, and select first one if we had a selection
	if (mNeedsAutoSelect)
	{
		LLFastTimer t3(FTM_AUTO_SELECT);
		// select new item only if a filtered item not currently selected
		LLFolderViewItem* selected_itemp = mSelectedItems.empty() ? NULL : mSelectedItems.back();
		if (!mAutoSelectOverride && (!selected_itemp || !selected_itemp->getViewModelItem()->potentiallyVisible()))
		{
			// these are named variables to get around gcc not binding non-const references to rvalues
			// and functor application is inherently non-const to allow for stateful functors
			LLSelectFirstFilteredItem functor;
			applyFunctorRecursively(functor);
		}

		// Open filtered folders for folder views with mAutoSelectOverride=TRUE.
		// Used by LLPlacesFolderView.
		if (getFolderViewModel()->getFilter()->showAllResults())
		{
			// these are named variables to get around gcc not binding non-const references to rvalues
			// and functor application is inherently non-const to allow for stateful functors
			LLOpenFilteredFolders functor;
			applyFunctorRecursively(functor);
		}

		scrollToShowSelection();
	}

	BOOL filter_finished = getViewModelItem()->passedFilter()
						&& mViewModel->contentsReady();
	if (filter_finished 
		|| gFocusMgr.childHasKeyboardFocus(inventory_panel) 
		|| gFocusMgr.childHasMouseCapture(inventory_panel))
	{
		// finishing the filter process, giving focus to the folder view, or dragging the scrollbar all stop the auto select process
		mNeedsAutoSelect = FALSE;
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

			LLRect visible_content_rect = mScrollContainer->getVisibleContentRect();
			LLFolderViewItem* selected_item = mSelectedItems.back();

			LLRect item_rect;
			selected_item->localRectToOtherView(selected_item->getLocalRect(), &item_rect, this);
			// if item is visible in scrolled region
			if (visible_content_rect.overlaps(item_rect))
			{
				// then attempt to keep it in same place on screen
				mScrollConstraintRect = item_rect;
				mScrollConstraintRect.translate(-visible_content_rect.mLeft, -visible_content_rect.mBottom);
			}
			else
			{
				// otherwise we just want it onscreen somewhere
				LLRect content_rect = mScrollContainer->getContentWindowRect();
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
		LLRect content_rect = mScrollContainer->getContentWindowRect();
		constraint_rect.setOriginAndSize(0, 0, content_rect.getWidth(), content_rect.getHeight());
	}


	BOOL is_visible = isInVisibleChain();

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


//static
void LLFolderView::idle(void* user_data)
{
	LLFolderView* self = (LLFolderView*)user_data;
	if ( self )
	{	// Do the real idle 
		self->doIdle();
	}
}

void LLFolderView::dumpSelectionInformation()
{
	llinfos << "LLFolderView::dumpSelectionInformation()" << llendl;
	llinfos << "****************************************" << llendl;
	selected_items_t::iterator item_it;
	for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
	{
		llinfos << "  " << (*item_it)->getName() << llendl;
	}
	llinfos << "****************************************" << llendl;
}

void LLFolderView::updateRenamerPosition()
{
	if(mRenameItem)
	{
		// See also LLFolderViewItem::draw()
		S32 x = ARROW_SIZE + TEXT_PAD + ICON_WIDTH + ICON_PAD + mRenameItem->getIndentation();
		S32 y = mRenameItem->getRect().getHeight() - mRenameItem->getItemHeight() - RENAME_HEIGHT_PAD;
		mRenameItem->localPointToScreen( x, y, &x, &y );
		screenPointToLocal( x, y, &x, &y );
		mRenamer->setOrigin( x, y );

		LLRect scroller_rect(0, 0, gViewerWindow->getWindowWidthScaled(), 0);
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

	U32 flags = FIRST_SELECTED_ITEM;
	for (selected_items_t::iterator item_itor = mSelectedItems.begin();
			item_itor != mSelectedItems.end();
			++item_itor)
	{
		LLFolderViewItem* selected_item = (*item_itor);
		selected_item->buildContextMenu(*menu, flags);
		flags = 0x0;
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

S32 LLFolderView::getItemHeight()
{
	if(!hasVisibleChildren())
	{
		//We need to display status textbox, let's reserve some place for it
		return llmax(0, mStatusTextBox->getTextPixelHeight());
	}
	return 0;
}
