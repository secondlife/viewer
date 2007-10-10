/** 
 * @file llfolderview.cpp
 * @brief Implementation of the folder view collection of classes.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llviewerprecompiledheaders.h"

#include "llfolderview.h"

#include <algorithm>

#include "llviewercontrol.h"
#include "lldbstrings.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llgl.h" 
#include "llinventory.h"

#include "llcallbacklist.h"
#include "llinventoryclipboard.h" // *TODO: remove this once hack below gone.
#include "llinventoryview.h"// hacked in for the bonus context menu items.
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llresmgr.h"
#include "llpreview.h"
#include "llscrollcontainer.h" // hack to allow scrolling
#include "lltooldraganddrop.h"
#include "llui.h"
#include "llviewerimage.h"
#include "llviewerimagelist.h"
#include "llviewerjointattachment.h"
#include "llviewermenu.h"
#include "llvieweruictrlfactory.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llfloaterproperties.h"

// RN: HACK
// We need these because some of the code below relies on things like
// gAgent root folder. Remove them once the abstraction leak is fixed.
#include "llagent.h"
#include "viewer.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

const S32 LEFT_PAD = 5;
const S32 LEFT_INDENTATION = 13;
const S32 ICON_PAD = 2;
const S32 ICON_WIDTH = 16;
const S32 TEXT_PAD = 1;
const S32 ARROW_SIZE = 12;
const S32 RENAME_WIDTH_PAD = 4;
const S32 RENAME_HEIGHT_PAD = 6;
const S32 AUTO_OPEN_STACK_DEPTH = 16;
const S32 MIN_ITEM_WIDTH_VISIBLE = ICON_WIDTH + ICON_PAD + ARROW_SIZE + TEXT_PAD + /*first few characters*/ 40;
const S32 MINIMUM_RENAMER_WIDTH = 80;
const F32 FOLDER_CLOSE_TIME_CONSTANT = 0.02f;
const F32 FOLDER_OPEN_TIME_CONSTANT = 0.03f;
const S32 MAX_FOLDER_ITEM_OVERLAP = 2;

F32 LLFolderView::sAutoOpenTime = 1.f;

void delete_selected_item(void* user_data);
void copy_selected_item(void* user_data);
void open_selected_items(void* user_data);
void properties_selected_items(void* user_data);
void paste_items(void* user_data);
void renamer_focus_lost( LLUICtrl* handler, void* user_data );

///----------------------------------------------------------------------------
/// Class LLFolderViewItem
///----------------------------------------------------------------------------

// statics 
const LLFontGL* LLFolderViewItem::sFont = NULL;
const LLFontGL* LLFolderViewItem::sSmallFont = NULL;
LLColor4 LLFolderViewItem::sFgColor;
LLColor4 LLFolderViewItem::sHighlightBgColor;
LLColor4 LLFolderViewItem::sHighlightFgColor;
LLColor4 LLFolderViewItem::sFilterBGColor;
LLColor4 LLFolderViewItem::sFilterTextColor;

// Default constructor
LLFolderViewItem::LLFolderViewItem( const LLString& name, LLViewerImage* icon,
								   S32 creation_date,
								   LLFolderView* root,
									LLFolderViewEventListener* listener ) :
	LLUICtrl( name, LLRect(0, 0, 0, 0), TRUE, NULL, NULL, FOLLOWS_LEFT|FOLLOWS_TOP|FOLLOWS_RIGHT),
	mLabel( name ),
	mLabelWidth(0),
	mCreationDate(creation_date),
	mParentFolder( NULL ),
	mListener( listener ),
	mIsSelected( FALSE ),
	mIsCurSelection( FALSE ),
	mSelectPending(FALSE),
	mLabelStyle( LLFontGL::NORMAL ),
	mHasVisibleChildren(FALSE),
	mIndentation(0),
	mNumDescendantsSelected(0),
	mFiltered(FALSE),
	mLastFilterGeneration(-1),
	mStringMatchOffset(LLString::npos),
	mControlLabelRotation(0.f),
	mRoot( root ),
	mDragAndDropTarget(FALSE)
{
	setIcon(icon);
	if( !LLFolderViewItem::sFont )
	{
		LLFolderViewItem::sFont = gResMgr->getRes( LLFONT_SANSSERIF_SMALL );
	}

	if (!LLFolderViewItem::sSmallFont)
	{
		LLFolderViewItem::sSmallFont = gResMgr->getRes( LLFONT_SMALL );
	}

	// HACK: Can't be set above because gSavedSettings might not be constructed.
	LLFolderViewItem::sFgColor = gColors.getColor( "MenuItemEnabledColor" );
	LLFolderViewItem::sHighlightBgColor = gColors.getColor( "MenuItemHighlightBgColor" );
	LLFolderViewItem::sHighlightFgColor = gColors.getColor( "MenuItemHighlightFgColor" );
	LLFolderViewItem::sFilterBGColor = gColors.getColor( "FilterBackgroundColor" );
	LLFolderViewItem::sFilterTextColor = gColors.getColor( "FilterTextColor" );

	mArrowImage = gImageList.getImage(LLUUID(gViewerArt.getString("folder_arrow.tga")), MIPMAP_FALSE, TRUE); 
	mBoxImage = gImageList.getImage(LLUUID(gViewerArt.getString("rounded_square.tga")), MIPMAP_FALSE, TRUE);

	refresh();
	setTabStop(FALSE);
}

// Destroys the object
LLFolderViewItem::~LLFolderViewItem( void )
{
	delete mListener;
	mListener = NULL;
	mArrowImage = NULL;
	mBoxImage = NULL;
}

LLFolderView* LLFolderViewItem::getRoot()
{
	return mRoot;
}

// Returns true if this object is a child (or grandchild, etc.) of potential_ancestor.
BOOL LLFolderViewItem::isDescendantOf( const LLFolderViewFolder* potential_ancestor )
{
	LLFolderViewItem* root = this;
	while( root->mParentFolder )
	{
		if( root->mParentFolder == potential_ancestor )
		{
			return TRUE;
		}
		root = root->mParentFolder;
	}
	return FALSE;
}

LLFolderViewItem* LLFolderViewItem::getNextOpenNode(BOOL include_children)
{
	if (!mParentFolder)
	{
		return NULL;
	}
	
	LLFolderViewItem* itemp = mParentFolder->getNextFromChild( this, include_children );
	while(itemp && !itemp->getVisible())
	{
		LLFolderViewItem* next_itemp = itemp->mParentFolder->getNextFromChild( itemp, include_children );
		if (itemp == next_itemp) 
		{
			// hit last item
			return itemp->getVisible() ? itemp : this;
		}
		itemp = next_itemp;
	}

	return itemp;
}

LLFolderViewItem* LLFolderViewItem::getPreviousOpenNode(BOOL include_children)
{
	if (!mParentFolder)
	{
		return NULL;
	}
	
	LLFolderViewItem* itemp = mParentFolder->getPreviousFromChild( this, include_children );
	while(itemp && !itemp->getVisible())
	{
		LLFolderViewItem* next_itemp = itemp->mParentFolder->getPreviousFromChild( itemp, include_children );
		if (itemp == next_itemp) 
		{
			// hit first item
			return itemp->getVisible() ? itemp : this;
		}
		itemp = next_itemp;
	}

	return itemp;
}

// is this item something we think we should be showing?
// for example, if we haven't gotten around to filtering it yet, then the answer is yes
// until we find out otherwise
BOOL LLFolderViewItem::potentiallyVisible()
{
	// we haven't been checked against min required filter
	// or we have and we passed
	return getLastFilterGeneration() < getRoot()->getFilter()->getMinRequiredGeneration() || getFiltered();
}

BOOL LLFolderViewItem::getFiltered() 
{ 
	return mFiltered && mLastFilterGeneration >= mRoot->getFilter()->getMinRequiredGeneration(); 
}

BOOL LLFolderViewItem::getFiltered(S32 filter_generation) 
{
	return mFiltered && mLastFilterGeneration >= filter_generation;
}

void LLFolderViewItem::setFiltered(BOOL filtered, S32 filter_generation)
{
	mFiltered = filtered;
	mLastFilterGeneration = filter_generation;
}

void LLFolderViewItem::setIcon(LLViewerImage* icon)
{
	mIcon = icon;
	if (mIcon)
	{
		mIcon->setBoostLevel(LLViewerImage::BOOST_UI);
	}
}

// refresh information from the listener
void LLFolderViewItem::refresh()
{
	if(mListener)
	{
		const char* label = mListener->getDisplayName().c_str();
		mLabel = label ? label : "";
		setIcon(mListener->getIcon());
		U32 creation_date = mListener->getCreationDate();
		if (mCreationDate != creation_date)
		{
			mCreationDate = mListener->getCreationDate();
			dirtyFilter();
		}
		mLabelStyle = mListener->getLabelStyle();
		mLabelSuffix = mListener->getLabelSuffix();

		LLString searchable_label(mLabel);
		searchable_label.append(mLabelSuffix);
		LLString::toUpper(searchable_label);

		if (mSearchableLabel.compare(searchable_label))
		{
			mSearchableLabel.assign(searchable_label);
			dirtyFilter();
			// some part of label has changed, so overall width has potentially changed
			if (mParentFolder)
			{
				mParentFolder->requestArrange();
			}
		}

		S32 label_width = sFont->getWidth(mLabel);
		if( mLabelSuffix.size() )   
		{   
			label_width += sFont->getWidth( mLabelSuffix );   
		}   

		mLabelWidth = ARROW_SIZE + TEXT_PAD + ICON_WIDTH + ICON_PAD + label_width; 
	}
}

void LLFolderViewItem::applyListenerFunctorRecursively(LLFolderViewListenerFunctor& functor)
{
	functor(mListener);
}

// This function is called when items are added or view filters change. It's
// implemented here but called by derived classes when folding the
// views.
void LLFolderViewItem::filterFromRoot( void )
{
	LLFolderViewItem* root = getRoot();

	root->filter(*((LLFolderView*)root)->getFilter());
}

// This function is called when the folder view is dirty. It's
// implemented here but called by derived classes when folding the
// views.
void LLFolderViewItem::arrangeFromRoot()
{
	LLFolderViewItem* root = getRoot();

	S32 height = 0;
	S32 width = 0;
	root->arrange( &width, &height, 0 );
}

// This function clears the currently selected item, and records the
// specified selected item appropriately for display and use in the
// UI. If open is TRUE, then folders are opened up along the way to
// the selection.
void LLFolderViewItem::setSelectionFromRoot(LLFolderViewItem* selection,
											BOOL open,					/* Flawfinder: ignore */
											BOOL take_keyboard_focus)
{
	getRoot()->setSelection(selection, open, take_keyboard_focus);		/* Flawfinder: ignore */
}

// helper function to change the selection from the root.
void LLFolderViewItem::changeSelectionFromRoot(LLFolderViewItem* selection, BOOL selected)
{
	getRoot()->changeSelection(selection, selected);
}

void LLFolderViewItem::extendSelectionFromRoot(LLFolderViewItem* selection)
{
	LLDynamicArray<LLFolderViewItem*> selected_items;

	getRoot()->extendSelection(selection, NULL, selected_items);
}

EWidgetType LLFolderViewItem::getWidgetType() const
{
	return WIDGET_TYPE_FOLDER_ITEM;
}

LLString LLFolderViewItem::getWidgetTag() const
{
	return LL_FOLDER_VIEW_ITEM_TAG;
}

EInventorySortGroup LLFolderViewItem::getSortGroup() 
{ 
	return SG_ITEM; 
}

// addToFolder() returns TRUE if it succeeds. FALSE otherwise
BOOL LLFolderViewItem::addToFolder(LLFolderViewFolder* folder, LLFolderView* root)
{
	if (!folder)
	{
		return FALSE;
	}
	mParentFolder = folder;
	root->addItemID(getListener()->getUUID(), this);
	return folder->addItem(this);
}


// Finds width and height of this object and it's children.  Also
// makes sure that this view and it's children are the right size.
S32 LLFolderViewItem::arrange( S32* width, S32* height, S32 filter_generation)
{
	mIndentation = mParentFolder ? mParentFolder->getIndentation() + LEFT_INDENTATION : 0;
	*width = llmax(*width, mLabelWidth + mIndentation); 
	*height = getItemHeight();
	return *height;
}

S32 LLFolderViewItem::getItemHeight()
{
	S32 icon_height = mIcon->getHeight();
	S32 label_height = llround(sFont->getLineHeight());
	return llmax( icon_height, label_height ) + ICON_PAD;
}

void LLFolderViewItem::filter( LLInventoryFilter& filter)
{
	BOOL filtered = mListener && filter.check(this);
	
	// if our visibility will change as a result of this filter, then
	// we need to be rearranged in our parent folder
	if (getVisible() != filtered)
	{
		if (mParentFolder)
		{
			mParentFolder->requestArrange();
		}
	}

	setFiltered(filtered, filter.getCurrentGeneration());
	mStringMatchOffset = filter.getStringMatchOffset();
	filter.decrementFilterCount();

	if (getRoot()->getDebugFilters())
	{
		mStatusText = llformat("%d", mLastFilterGeneration);
	}
}

void LLFolderViewItem::dirtyFilter()
{
	mLastFilterGeneration = -1;
	// bubble up dirty flag all the way to root
	if (getParentFolder())
	{
		getParentFolder()->setCompletedFilterGeneration(-1, TRUE);
	}
}

// *TODO: This can be optimized a lot by simply recording that it is
// selected in the appropriate places, and assuming that set selection
// means 'deselect' for a leaf item. Do this optimization after
// multiple selection is implemented to make sure it all plays nice
// together.
BOOL LLFolderViewItem::setSelection(LLFolderViewItem* selection, BOOL open, BOOL take_keyboard_focus)
{
	if( selection == this )
	{
		mIsSelected = TRUE;
		if(mListener)
		{
			mListener->selectItem();
		}
	}
	else
	{
		mIsSelected = FALSE;
	}
	return mIsSelected;
}

BOOL LLFolderViewItem::changeSelection(LLFolderViewItem* selection, BOOL selected)
{
	if(selection == this && mIsSelected != selected)
	{
		mIsSelected = selected;
		if(mListener)
		{
			mListener->selectItem();
		}
		return TRUE;
	}
	return FALSE;
}

void LLFolderViewItem::recursiveDeselect(BOOL deselect_self)
{
	if (mIsSelected && deselect_self)
	{
		mIsSelected = FALSE;

		// update ancestors' count of selected descendents
		LLFolderViewFolder* parent_folder = getParentFolder();
		while(parent_folder)
		{
			parent_folder->mNumDescendantsSelected--;
			parent_folder = parent_folder->getParentFolder();
		}
	}
}


BOOL LLFolderViewItem::isMovable()
{
	if( mListener )
	{
		return mListener->isItemMovable();
	}
	else
	{
		return TRUE;
	}
}

BOOL LLFolderViewItem::isRemovable()
{
	if( mListener )
	{
		return mListener->isItemRemovable();
	}
	else
	{
		return TRUE;
	}
}

void LLFolderViewItem::destroyView()
{
	if (mParentFolder)
	{
		// removeView deletes me
		mParentFolder->removeView(this);
	}
}

// Call through to the viewed object and return true if it can be
// removed.
//BOOL LLFolderViewItem::removeRecursively(BOOL single_item)
BOOL LLFolderViewItem::remove()
{
	if(!isRemovable())
	{
		return FALSE;
	}
	if(mListener)
	{
		return mListener->removeItem();
	}
	return TRUE;
}

// Build an appropriate context menu for the item.
void LLFolderViewItem::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	if(mListener)
	{
		mListener->buildContextMenu(menu, flags);
	}
}

void LLFolderViewItem::open( void )		/* Flawfinder: ignore */
{
	if( mListener )
	{
		mListener->openItem();
	}
}

void LLFolderViewItem::preview( void )
{
	if (mListener)
	{
		mListener->previewItem();
	}
}

void LLFolderViewItem::rename(const LLString& new_name)
{
	if( !new_name.empty() )
	{
		mLabel = new_name.c_str();
		if( mListener )
		{
			mListener->renameItem(new_name);

			if(mParentFolder)
			{
				mParentFolder->resort(this);
			}
		}
	}
}

const LLString& LLFolderViewItem::getSearchableLabel() const
{
	return mSearchableLabel;
}

const LLString& LLFolderViewItem::getName( void ) const
{
	if(mListener)
	{
		return mListener->getName();
	}
	return mLabel;
}

LLFolderViewFolder* LLFolderViewItem::getParentFolder( void )
{
	return mParentFolder;
}

LLFolderViewEventListener* LLFolderViewItem::getListener( void )
{
	return mListener;
}

// LLView functionality
BOOL LLFolderViewItem::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
	if(!mIsSelected)
	{
		setSelectionFromRoot(this, FALSE);
	}
	make_ui_sound("UISndClick");
	return TRUE;
}

BOOL LLFolderViewItem::handleMouseDown( S32 x, S32 y, MASK mask )
{
	// No handler needed for focus lost since this class has no
	// state that depends on it.
	gViewerWindow->setMouseCapture( this );

	if (!mIsSelected)
	{
		if(mask & MASK_CONTROL)
		{
			changeSelectionFromRoot(this, !mIsSelected);
		}
		else if (mask & MASK_SHIFT)
		{
			extendSelectionFromRoot(this);
		}
		else
		{
			setSelectionFromRoot(this, FALSE);
		}
		make_ui_sound("UISndClick");
	}
	else
	{
		mSelectPending = TRUE;
	}

	if( isMovable() )
	{
		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y );
		gToolDragAndDrop->setDragStart( screen_x, screen_y );
	}
	return TRUE;
}

BOOL LLFolderViewItem::handleHover( S32 x, S32 y, MASK mask )
{
	if( hasMouseCapture() && isMovable() )
	{
		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y );
		BOOL can_drag = TRUE;
		if( gToolDragAndDrop->isOverThreshold( screen_x, screen_y ) )
		{
			LLFolderView* root = getRoot();
			
			if(root->getCurSelectedItem())
			{
				LLToolDragAndDrop::ESource src = LLToolDragAndDrop::SOURCE_WORLD;

				// *TODO: push this into listener and remove
				// dependency on llagent
				if(mListener && gInventory.isObjectDescendentOf(mListener->getUUID(), gAgent.getInventoryRootID()))
				{
					src = LLToolDragAndDrop::SOURCE_AGENT;
				}
				else if (mListener && gInventory.isObjectDescendentOf(mListener->getUUID(), gInventoryLibraryRoot))
				{
					src = LLToolDragAndDrop::SOURCE_LIBRARY;
				}

				can_drag = root->startDrag(src);
				if (can_drag)
				{
					// if (mListener) mListener->startDrag();
					// RN: when starting drag and drop, clear out last auto-open
					root->autoOpenTest(NULL);
					root->setShowSelectionContext(TRUE);

					// Release keyboard focus, so that if stuff is dropped into the
					// world, pressing the delete key won't blow away the inventory
					// item.
					gViewerWindow->setKeyboardFocus(NULL, NULL);

					return gToolDragAndDrop->handleHover( x, y, mask );
				}
			}
		}

		if (can_drag)
		{
			gViewerWindow->setCursor(UI_CURSOR_ARROW);
		}
		else
		{
			gViewerWindow->setCursor(UI_CURSOR_NOLOCKED);
		}
		return TRUE;
	}
	else
	{
		getRoot()->setShowSelectionContext(FALSE);
		gViewerWindow->setCursor(UI_CURSOR_ARROW);
		// let parent handle this then...
		return FALSE;
	}
}


BOOL LLFolderViewItem::handleDoubleClick( S32 x, S32 y, MASK mask )
{
	preview();
	return TRUE;
}

BOOL LLFolderViewItem::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if (getParent())
	{
		return getParent()->handleScrollWheel(x, y, clicks);
	}
	return FALSE;
}

BOOL LLFolderViewItem::handleMouseUp( S32 x, S32 y, MASK mask )
{
	// if mouse hasn't moved since mouse down...
	if ( pointInView(x, y) && mSelectPending )
	{
		//...then select
		if(mask & MASK_CONTROL)
		{
			changeSelectionFromRoot(this, !mIsSelected);
		}
		else if (mask & MASK_SHIFT)
		{
			extendSelectionFromRoot(this);
		}
		else
		{
			setSelectionFromRoot(this, FALSE);
		}
	}
	
	mSelectPending = FALSE;

	if( hasMouseCapture() )
	{
		getRoot()->setShowSelectionContext(FALSE);
		gViewerWindow->setMouseCapture( NULL );
	}
	return TRUE;
}

BOOL LLFolderViewItem::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										 EDragAndDropType cargo_type,
										 void* cargo_data,
										 EAcceptance* accept,
										 LLString& tooltip_msg)
{
	BOOL accepted = FALSE;
	BOOL handled = FALSE;
	if(mListener)
	{
		accepted = mListener->dragOrDrop(mask,drop,cargo_type,cargo_data);
		handled = accepted;
		if (accepted)
		{
			mDragAndDropTarget = TRUE;
			*accept = ACCEPT_YES_MULTI;
		}
		else
		{
			*accept = ACCEPT_NO;
		}
	}
	if(mParentFolder && !handled)
	{
		handled = mParentFolder->handleDragAndDropFromChild(mask,drop,cargo_type,cargo_data,accept,tooltip_msg);
	}
	if (handled)
	{
		lldebugst(LLERR_USER_INPUT) << "dragAndDrop handled by LLFolderViewItem" << llendl;
	}

	return handled;
}


void LLFolderViewItem::draw()
{
	bool possibly_has_children = false;
	bool up_to_date = mListener && mListener->isUpToDate();
	if((up_to_date && hasVisibleChildren() ) || // we fetched our children and some of them have passed the filter...
		(!up_to_date && mListener && mListener->hasChildren())) // ...or we know we have children but haven't fetched them (doesn't obey filter)
	{
		possibly_has_children = true;
	}
	if(/*mControlLabel[0] != '\0' && */possibly_has_children)
	{
		LLGLSTexture gls_texture;
		if (mArrowImage)
		{
			gl_draw_scaled_rotated_image(mIndentation, mRect.getHeight() - ARROW_SIZE - TEXT_PAD,
				ARROW_SIZE, ARROW_SIZE, mControlLabelRotation, mArrowImage, sFgColor);
		}
	}

	F32 text_left = (F32)(ARROW_SIZE + TEXT_PAD + ICON_WIDTH + ICON_PAD + mIndentation);

	// If we have keyboard focus, draw selection filled
	BOOL show_context = getRoot()->getShowSelectionContext();
	BOOL filled = show_context || (gFocusMgr.getKeyboardFocus() == getRoot());

	// always render "current" item, only render other selected items if
	// mShowSingleSelection is FALSE
	if( mIsSelected )
	{
		LLGLSNoTexture gls_no_texture;
		LLColor4 bg_color = sHighlightBgColor;
		//const S32 TRAILING_PAD = 5;  // It just looks better with this.
		if (!mIsCurSelection)
		{
			// do time-based fade of extra objects
			F32 fade_time = getRoot()->getSelectionFadeElapsedTime();
			if (getRoot()->getShowSingleSelection())
			{
				// fading out
				bg_color.mV[VALPHA] = clamp_rescale(fade_time, 0.f, 0.4f, bg_color.mV[VALPHA], 0.f);
			}
			else
			{
				// fading in
				bg_color.mV[VALPHA] = clamp_rescale(fade_time, 0.f, 0.4f, 0.f, bg_color.mV[VALPHA]);
			}
		}

		gl_rect_2d(
			0, 
			mRect.getHeight(), 
			mRect.getWidth() - 2,
			llfloor(mRect.getHeight() - sFont->getLineHeight() - ICON_PAD),
			bg_color, filled);
		if (mIsCurSelection)
		{
			gl_rect_2d(
				0, 
				mRect.getHeight(), 
				mRect.getWidth() - 2,
				llfloor(mRect.getHeight() - sFont->getLineHeight() - ICON_PAD),
				sHighlightFgColor, FALSE);
		}
		if (mRect.getHeight() > llround(sFont->getLineHeight()) + ICON_PAD + 2)
		{
			gl_rect_2d(
				0, 
				llfloor(mRect.getHeight() - sFont->getLineHeight() - ICON_PAD) - 2, 
				mRect.getWidth() - 2,
				2,
				sHighlightFgColor, FALSE);
			if (show_context)
			{
				gl_rect_2d(
					0, 
					llfloor(mRect.getHeight() - sFont->getLineHeight() - ICON_PAD) - 2, 
					mRect.getWidth() - 2,
					2,
					sHighlightBgColor, TRUE);
			}
		}
	}
	if (mDragAndDropTarget)
	{
		LLGLSNoTexture gls_no_texture;
		gl_rect_2d(
			0, 
			mRect.getHeight(), 
			mRect.getWidth() - 2,
			llfloor(mRect.getHeight() - sFont->getLineHeight() - ICON_PAD),
			sHighlightBgColor, FALSE);

		if (mRect.getHeight() > llround(sFont->getLineHeight()) + ICON_PAD + 2)
		{
			gl_rect_2d(
				0, 
				llfloor(mRect.getHeight() - sFont->getLineHeight() - ICON_PAD) - 2, 
				mRect.getWidth() - 2,
				2,
				sHighlightBgColor, FALSE);
		}
		mDragAndDropTarget = FALSE;
	}


	if(mIcon)
	{
		gl_draw_image(mIndentation + ARROW_SIZE + TEXT_PAD, mRect.getHeight() - mIcon->getHeight(), mIcon);
		mIcon->addTextureStats( (F32)(mIcon->getWidth() * mIcon->getHeight()));
	}

	if (!mLabel.empty())
	{
		// highlight filtered text
		BOOL debug_filters = getRoot()->getDebugFilters();
		LLColor4 color = ( (mIsSelected && filled) ? sHighlightFgColor : sFgColor );
		F32 right_x;
		F32 y = (F32)mRect.getHeight() - sFont->getLineHeight() - (F32)TEXT_PAD;

		if (debug_filters)
		{
			if (!getFiltered() && !possibly_has_children)
			{
				color.mV[VALPHA] *= 0.5f;
			}
			
			LLColor4 filter_color = mLastFilterGeneration >= getRoot()->getFilter()->getCurrentGeneration() ? LLColor4(0.5f, 0.8f, 0.5f, 1.f) : LLColor4(0.8f, 0.5f, 0.5f, 1.f);
			sSmallFont->renderUTF8(mStatusText, 0, text_left, y, filter_color,
							LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL,
							S32_MAX, S32_MAX, &right_x, FALSE );
			text_left = right_x;
		}

		sFont->renderUTF8( mLabel, 0, text_left, y, color,
							LLFontGL::LEFT, LLFontGL::BOTTOM, mLabelStyle,
							S32_MAX, S32_MAX, &right_x, FALSE );
		if (!mLabelSuffix.empty())
		{
			sFont->renderUTF8( mLabelSuffix, 0, right_x, y, LLColor4(0.75f, 0.85f, 0.85f, 1.f),
								LLFontGL::LEFT, LLFontGL::BOTTOM, mLabelStyle,
								S32_MAX, S32_MAX, &right_x, FALSE );
		}

		if (mBoxImage.notNull() && mStringMatchOffset != LLString::npos)
		{
			// don't draw backgrounds for zero-length strings
			S32 filter_string_length = mRoot->getFilterSubString().size();
			if (filter_string_length > 0)
			{
				LLString combined_string = mLabel + mLabelSuffix;
				S32 left = llround(text_left) + sFont->getWidth(combined_string, 0, mStringMatchOffset) - 1;
				S32 right = left + sFont->getWidth(combined_string, mStringMatchOffset, filter_string_length) + 2;
				S32 bottom = llfloor(mRect.getHeight() - sFont->getLineHeight() - 3);
				S32 top = mRect.getHeight();

				LLViewerImage::bindTexture(mBoxImage);
				glColor4fv(sFilterBGColor.mV);
				gl_segmented_rect_2d_tex(left, top, right, bottom, mBoxImage->getWidth(), mBoxImage->getHeight(), 16);
				F32 match_string_left = text_left + sFont->getWidthF32(combined_string, 0, mStringMatchOffset);
				F32 y = (F32)mRect.getHeight() - sFont->getLineHeight() - (F32)TEXT_PAD;
				sFont->renderUTF8( combined_string, mStringMatchOffset, match_string_left, y,
								sFilterTextColor, LLFontGL::LEFT, LLFontGL::BOTTOM, mLabelStyle,
								filter_string_length, S32_MAX, &right_x, FALSE );
			}
		}
	}

	if( sDebugRects )
	{
		drawDebugRect();
	}
}


///----------------------------------------------------------------------------
/// Class LLFolderViewFolder
///----------------------------------------------------------------------------

// Default constructor
LLFolderViewFolder::LLFolderViewFolder( const LLString& name, LLViewerImage* icon,
										LLFolderView* root,
										LLFolderViewEventListener* listener ): 
	LLFolderViewItem( name, icon, 0, root, listener ),	// 0 = no create time
	mIsOpen(FALSE),
	mExpanderHighlighted(FALSE),
	mCurHeight(0.f),
	mTargetHeight(0.f),
	mAutoOpenCountdown(0.f),
	mSubtreeCreationDate(0),
	mAmTrash(LLFolderViewFolder::UNKNOWN),
	mLastArrangeGeneration( -1 ),
	mLastCalculatedWidth(0),
	mCompletedFilterGeneration(-1),
	mMostFilteredDescendantGeneration(-1)
{
	mType = "(folder)";

	//mItems.setInsertBefore( &sort_item_name );
	//mFolders.setInsertBefore( &folder_insert_before );
}

// Destroys the object
LLFolderViewFolder::~LLFolderViewFolder( void )
{
	// The LLView base class takes care of object destruction. make sure that we
	// don't have mouse or keyboard focus
	gFocusMgr.releaseFocusIfNeeded( this ); // calls onCommit()

	//mItems.reset();
	//mItems.removeAllNodes();
	//mFolders.removeAllNodes();
}

EWidgetType LLFolderViewFolder::getWidgetType() const
{
	return WIDGET_TYPE_FOLDER;
}

LLString LLFolderViewFolder::getWidgetTag() const
{
	return LL_FOLDER_VIEW_FOLDER_TAG;
}

// addToFolder() returns TRUE if it succeeds. FALSE otherwise
BOOL LLFolderViewFolder::addToFolder(LLFolderViewFolder* folder, LLFolderView* root)
{
	if (!folder)
	{
		return FALSE;
	}
	mParentFolder = folder;
	root->addItemID(getListener()->getUUID(), this);
	return folder->addFolder(this);
}

// Finds width and height of this object and it's children.  Also
// makes sure that this view and it's children are the right size.
S32 LLFolderViewFolder::arrange( S32* width, S32* height, S32 filter_generation)
{
	mHasVisibleChildren = hasFilteredDescendants(filter_generation);
	
	LLInventoryFilter::EFolderShow show_folder_state = getRoot()->getShowFolderState();

	// calculate height as a single item (without any children), and reshapes rectangle to match
	LLFolderViewItem::arrange( width, height, filter_generation );

	// clamp existing animated height so as to never get smaller than a single item
	mCurHeight = llmax((F32)*height, mCurHeight);

	// initialize running height value as height of single item in case we have no children
	*height = getItemHeight();
	F32 running_height = (F32)*height;
	F32 target_height = (F32)*height;

	// are my children visible?
	if (needsArrange())
	{
		// set last arrange generation first, in case children are animating
		// and need to be arranged again
		mLastArrangeGeneration = mRoot->getArrangeGeneration();
		if (mIsOpen)
		{
			// Add sizes of children
			S32 parent_item_height = mRect.getHeight();

			folders_t::iterator fit = mFolders.begin();
			folders_t::iterator fend = mFolders.end();
			for(; fit < fend; ++fit)
			{
				LLFolderViewFolder* folderp = (*fit);
				if (getRoot()->getDebugFilters())
				{
					folderp->setVisible(TRUE);
				}
				else
				{
					folderp->setVisible(show_folder_state == LLInventoryFilter::SHOW_ALL_FOLDERS || // always show folders?
										(folderp->getFiltered(filter_generation) || folderp->hasFilteredDescendants(filter_generation))); // passed filter or has descendants that passed filter
				}

				if (folderp->getVisible())
				{
					S32 child_width = *width;
					S32 child_height = 0;
					S32 child_top = parent_item_height - llround(running_height);

					target_height += folderp->arrange( &child_width, &child_height, filter_generation );

					running_height += (F32)child_height;
					*width = llmax(*width, child_width);
					folderp->setOrigin( 0, child_top - folderp->getRect().getHeight() );
				}
			}
			items_t::iterator iit = mItems.begin();
			items_t::iterator iend = mItems.end();
			for(;iit < iend; ++iit)
			{
				LLFolderViewItem* itemp = (*iit);
				if (getRoot()->getDebugFilters())
				{
					itemp->setVisible(TRUE);
				}
				else
				{
					itemp->setVisible(itemp->getFiltered(filter_generation));
				}

				if (itemp->getVisible())
				{
					S32 child_width = *width;
					S32 child_height = 0;
					S32 child_top = parent_item_height - llround(running_height);

					target_height += itemp->arrange( &child_width, &child_height, filter_generation );
					// don't change width, as this item is as wide as its parent folder by construction
					itemp->reshape( itemp->getRect().getWidth(), child_height);

					running_height += (F32)child_height;
					*width = llmax(*width, child_width);
					itemp->setOrigin( 0, child_top - itemp->getRect().getHeight() );
				}
			}
		}

		mTargetHeight = target_height;
		// cache this width so next time we can just return it
		mLastCalculatedWidth = *width;
	}
	else
	{
		// just use existing width
		*width = mLastCalculatedWidth;
	}

	// animate current height towards target height
	if (llabs(mCurHeight - mTargetHeight) > 1.f)
	{
		mCurHeight = lerp(mCurHeight, mTargetHeight, LLCriticalDamp::getInterpolant(mIsOpen ? FOLDER_OPEN_TIME_CONSTANT : FOLDER_CLOSE_TIME_CONSTANT));
		
		requestArrange();

		// hide child elements that fall out of current animated height
		for (folders_t::iterator iter = mFolders.begin();
			 iter != mFolders.end();)
		{
			folders_t::iterator fit = iter++;
			// number of pixels that bottom of folder label is from top of parent folder
			if (mRect.getHeight() - (*fit)->getRect().mTop + (*fit)->getItemHeight() 
				> llround(mCurHeight) + MAX_FOLDER_ITEM_OVERLAP)
			{
				// hide if beyond current folder height
				(*fit)->setVisible(FALSE);
			}
		}

		for (items_t::iterator iter = mItems.begin();
			 iter != mItems.end();)
		{
			items_t::iterator iit = iter++;
			// number of pixels that bottom of item label is from top of parent folder
			if (mRect.getHeight() - (*iit)->getRect().mBottom
				> llround(mCurHeight) + MAX_FOLDER_ITEM_OVERLAP)
			{
				(*iit)->setVisible(FALSE);
			}
		}
	}
	else
	{
		mCurHeight = mTargetHeight;
	}

	// don't change width as this item is already as wide as its parent folder
	reshape(mRect.getWidth(),llround(mCurHeight));

	// pass current height value back to parent
	*height = llround(mCurHeight);

	return llround(mTargetHeight);
}

BOOL LLFolderViewFolder::needsArrange()
{
	return mLastArrangeGeneration < mRoot->getArrangeGeneration(); 
}

void LLFolderViewFolder::setCompletedFilterGeneration(S32 generation, BOOL recurse_up)
{
	mMostFilteredDescendantGeneration = llmin(mMostFilteredDescendantGeneration, generation);
	mCompletedFilterGeneration = generation;
	// only aggregate up if we are a lower (older) value
	if (recurse_up && mParentFolder && generation < mParentFolder->getCompletedFilterGeneration())
	{
		mParentFolder->setCompletedFilterGeneration(generation, TRUE);
	}
}

void LLFolderViewFolder::filter( LLInventoryFilter& filter)
{
	S32 filter_generation = filter.getCurrentGeneration();
	// if failed to pass filter newer than must_pass_generation
	// you will automatically fail this time, so we only
	// check against items that have passed the filter
	S32 must_pass_generation = filter.getMustPassGeneration();

	// if we have already been filtered against this generation, skip out
	if (getCompletedFilterGeneration() >= filter_generation)
	{
		return;
	}

	// filter folder itself
	if (getLastFilterGeneration() < filter_generation)
	{
		if (getLastFilterGeneration() >= must_pass_generation &&		// folder has been compared to a valid precursor filter
			!mFiltered)													// and did not pass the filter
		{
			// go ahead and flag this folder as done
			mLastFilterGeneration = filter_generation;			
		}
		else
		{
			// filter self only on first pass through
			LLFolderViewItem::filter( filter );
		}
	}

	if (getRoot()->getDebugFilters())
	{
		mStatusText = llformat("%d", mLastFilterGeneration);
		mStatusText += llformat("(%d)", mCompletedFilterGeneration);
		mStatusText += llformat("+%d", mMostFilteredDescendantGeneration);
	}

	// all descendants have been filtered later than must pass generation
	// but none passed
	if(getCompletedFilterGeneration() >= must_pass_generation && !hasFilteredDescendants(must_pass_generation))
	{
		// don't traverse children if we've already filtered them since must_pass_generation
		// and came back with nothing
		return;
	}

	// we entered here with at least one filter iteration left
	// check to see if we have any more before continuing on to children
	if (filter.getFilterCount() < 0)
	{
		return;
	}

	// when applying a filter, matching folders get their contents downloaded first
	if (filter.isNotDefault() && getFiltered(filter.getMinRequiredGeneration()) && (mListener && !gInventory.isCategoryComplete(mListener->getUUID())))
	{
		gInventory.startBackgroundFetch(mListener->getUUID());
	}

	// now query children
	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		// have we run out of iterations this frame?
		if (filter.getFilterCount() < 0)
		{
			break;
		}

		// mMostFilteredDescendantGeneration might have been reset
		// in which case we need to update it even for folders that
		// don't need to be filtered anymore
		if ((*fit)->getCompletedFilterGeneration() >= filter_generation)
		{
			// track latest generation to pass any child items
			if ((*fit)->getFiltered() || (*fit)->hasFilteredDescendants(filter.getMinRequiredGeneration()))
			{
				mMostFilteredDescendantGeneration = filter_generation;
				if (mRoot->needsAutoSelect())
				{
					(*fit)->setOpenArrangeRecursively(TRUE);
				}
			}
			// just skip it, it has already been filtered
			continue;
		}

		// update this folders filter status (and children)
		(*fit)->filter( filter );
		
		// track latest generation to pass any child items
		if ((*fit)->getFiltered() || (*fit)->hasFilteredDescendants(filter_generation))
		{
			mMostFilteredDescendantGeneration = filter_generation;
			if (mRoot->needsAutoSelect())
			{
				(*fit)->setOpenArrangeRecursively(TRUE);
			}
		}
	}

	for (items_t::iterator iter = mItems.begin();
		 iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		if (filter.getFilterCount() < 0)
		{
			break;
		}
		if ((*iit)->getLastFilterGeneration() >= filter_generation)
		{
			if ((*iit)->getFiltered())
			{
				mMostFilteredDescendantGeneration = filter_generation;
			}
			continue;
		}

		if ((*iit)->getLastFilterGeneration() >= must_pass_generation && 
			!(*iit)->getFiltered(must_pass_generation))
		{
			// failed to pass an earlier filter that was a subset of the current one
			// go ahead and flag this item as done
			(*iit)->setFiltered(FALSE, filter_generation);
			continue;
		}

		(*iit)->filter( filter );
		
		if ((*iit)->getFiltered(filter.getMinRequiredGeneration()))
		{
			mMostFilteredDescendantGeneration = filter_generation;
		}
	}
	
	// if we didn't use all filter iterations
	// that means we filtered all of our descendants
	// instead of exhausting the filter count for this frame
	if (filter.getFilterCount() > 0)
	{
		// flag this folder as having completed filter pass for all descendants
		setCompletedFilterGeneration(filter_generation, FALSE/*dont recurse up to root*/);
	}
}

void LLFolderViewFolder::setFiltered(BOOL filtered, S32 filter_generation)
{
	// if this folder is now filtered, but wasn't before
	// (it just passed)
	if (filtered && !mFiltered)
	{
		// reset current height, because last time we drew it
		// it might have had more visible items than now
		mCurHeight = 0.f;
	}

	LLFolderViewItem::setFiltered(filtered, filter_generation);
}

void LLFolderViewFolder::dirtyFilter()
{
	// we're a folder, so invalidate our completed generation
	setCompletedFilterGeneration(-1, FALSE);
	LLFolderViewItem::dirtyFilter();
}

BOOL LLFolderViewFolder::hasFilteredDescendants()
{
	return mMostFilteredDescendantGeneration >= mRoot->getFilter()->getCurrentGeneration();
}

// Passes selection information on to children and record selection
// information if necessary.
BOOL LLFolderViewFolder::setSelection(LLFolderViewItem* selection, BOOL open,		/* Flawfinder: ignore */
									  BOOL take_keyboard_focus)
{
	BOOL rv = FALSE;
	if( selection == this )
	{
		mIsSelected = TRUE;
		if(mListener)
		{
			mListener->selectItem();
		}
		rv = TRUE;
	}
	else
	{
		mIsSelected = FALSE;
		rv = FALSE;
	}
	BOOL child_selected = FALSE;
	
	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		if((*fit)->setSelection(selection, open, take_keyboard_focus))		/* Flawfinder: ignore */
		{
			rv = TRUE;
			child_selected = TRUE;
			mNumDescendantsSelected++;
		}
	}
	for (items_t::iterator iter = mItems.begin();
		 iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		if((*iit)->setSelection(selection, open, take_keyboard_focus))		/* Flawfinder: ignore */
		{
			rv = TRUE;
			child_selected = TRUE;
			mNumDescendantsSelected++;
		}
	}
	if(open && child_selected)		/* Flawfinder: ignore */
	{
		setOpenArrangeRecursively(TRUE);
	}
	return rv;
}

// This method is used to change the selection of an item. If
// selection is 'this', then note selection as true. Returns TRUE
// if this or a child is now selected.
BOOL LLFolderViewFolder::changeSelection(LLFolderViewItem* selection,
										 BOOL selected)
{
	BOOL rv = FALSE;
	if(selection == this)
	{
		mIsSelected = selected;
		if(mListener && selected)
		{
			mListener->selectItem();
		}
		rv = TRUE;
	}

	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		if((*fit)->changeSelection(selection, selected))
		{
			if (selected)
			{
				mNumDescendantsSelected++;
			}
			else
			{
				mNumDescendantsSelected--;
			}
			rv = TRUE;
		}
	}
	for (items_t::iterator iter = mItems.begin();
		 iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		if((*iit)->changeSelection(selection, selected))
		{
			if (selected)
			{
				mNumDescendantsSelected++;
			}
			else
			{
				mNumDescendantsSelected--;
			}
			rv = TRUE;
		}
	}
	return rv;
}

S32 LLFolderViewFolder::extendSelection(LLFolderViewItem* selection, LLFolderViewItem* last_selected, LLDynamicArray<LLFolderViewItem*>& selected_items)
{
	S32 num_selected = 0;

	// pass on to child folders first
	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		num_selected += (*fit)->extendSelection(selection, last_selected, selected_items);
		mNumDescendantsSelected += num_selected;
	}

	// handle selection of our immediate children...
	BOOL reverse_select = FALSE;
	BOOL found_last_selected = FALSE;
	BOOL found_selection = FALSE;
	LLDynamicArray<LLFolderViewItem*> items_to_select;
	LLFolderViewItem* item;

	//...folders first...
	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		item = (*fit);
		if(item == selection)
		{
			found_selection = TRUE;
		}
		else if (item == last_selected)
		{
			found_last_selected = TRUE;
			if (found_selection)
			{
				reverse_select = TRUE;
			}
		}

		if (found_selection || found_last_selected)
		{
			// deselect currently selected items so they can be pushed back on queue
			if (item->isSelected())
			{
				item->changeSelection(item, FALSE);
			}
			items_to_select.put(item);
		}

		if (found_selection && found_last_selected)
		{
			break;
		}		
	}

	if (!(found_selection && found_last_selected))
	{
		//,,,then items
		for (items_t::iterator iter = mItems.begin();
			 iter != mItems.end();)
		{
			items_t::iterator iit = iter++;
			item = (*iit);
			if(item == selection)
			{
				found_selection = TRUE;
			}
			else if (item == last_selected)
			{
				found_last_selected = TRUE;
				if (found_selection)
				{
					reverse_select = TRUE;
				}
			}

			if (found_selection || found_last_selected)
			{
				// deselect currently selected items so they can be pushed back on queue
				if (item->isSelected())
				{
					item->changeSelection(item, FALSE);
				}
				items_to_select.put(item);
			}

			if (found_selection && found_last_selected)
			{
				break;
			}
		}
	}

	if (found_last_selected && found_selection)
	{
		// we have a complete selection inside this folder
		for (S32 index = reverse_select ? items_to_select.getLength() - 1 : 0; 
			reverse_select ? index >= 0 : index < items_to_select.getLength(); reverse_select ? index-- : index++)
		{
			LLFolderViewItem* item = items_to_select[index];
			if (item->changeSelection(item, TRUE))
			{
				selected_items.put(item);
				mNumDescendantsSelected++;
				num_selected++;
			}
		}
	}
	else if (found_selection)
	{
		// last selection was not in this folder....go ahead and select just the new item
		if (selection->changeSelection(selection, TRUE))
		{
			selected_items.put(selection);
			mNumDescendantsSelected++;
			num_selected++;
		}
	}

	return num_selected;
}

void LLFolderViewFolder::recursiveDeselect(BOOL deselect_self)
{
	// make sure we don't have negative values
	llassert(mNumDescendantsSelected >= 0);

	if (mIsSelected && deselect_self)
	{
		mIsSelected = FALSE;

		// update ancestors' count of selected descendents
		LLFolderViewFolder* parent_folder = getParentFolder();
		while(parent_folder)
		{
			parent_folder->mNumDescendantsSelected--;
			parent_folder = parent_folder->getParentFolder();
		}
	}

	if (0 == mNumDescendantsSelected)
	{
		return;
	}

	for (items_t::iterator iter = mItems.begin();
		 iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		LLFolderViewItem* item = (*iit);
		item->recursiveDeselect(TRUE);
	}

	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		LLFolderViewFolder* folder = (*fit);
		folder->recursiveDeselect(TRUE);
	}

}

void LLFolderViewFolder::destroyView()
{
	for (items_t::iterator iter = mItems.begin();
		 iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		LLFolderViewItem* item = (*iit);
		getRoot()->removeItemID(item->getListener()->getUUID());
	}

	std::for_each(mItems.begin(), mItems.end(), DeletePointer());
	mItems.clear();

	while (!mFolders.empty())
	{
		LLFolderViewFolder *folderp = mFolders.back();
		folderp->destroyView();
	}

	mFolders.clear();

	deleteAllChildren();
	
	if (mParentFolder)
	{
		mParentFolder->removeView(this);
	}
}

// remove the specified item (and any children) if possible. Return
// TRUE if the item was deleted.
BOOL LLFolderViewFolder::removeItem(LLFolderViewItem* item)
{
	if(item->remove())
	{
		//RN: this seem unneccessary as remove() moves to trash
		//removeView(item);
		return TRUE;
	}
	return FALSE;
}

// simply remove the view (and any children) Don't bother telling the
// listeners.
void LLFolderViewFolder::removeView(LLFolderViewItem* item)
{
	if (!item || item->getParentFolder() != this)
	{
		return;
	}
	// deselect without traversing hierarchy
	item->recursiveDeselect(TRUE);
	getRoot()->removeFromSelectionList(item);
	extractItem(item);
	delete item;
}

// extractItem() removes the specified item from the folder, but
// doesn't delete it.
void LLFolderViewFolder::extractItem( LLFolderViewItem* item )
{
	items_t::iterator it = std::find(mItems.begin(), mItems.end(), item);
	if(it == mItems.end())
	{
		// This is an evil downcast. However, it's only doing
		// pointer comparison to find if (which it should be ) the
		// item is in the container, so it's pretty safe.
		LLFolderViewFolder* f = reinterpret_cast<LLFolderViewFolder*>(item);
		folders_t::iterator ft;
		ft = std::find(mFolders.begin(), mFolders.end(), f);
		if(ft != mFolders.end())
		{
			mFolders.erase(ft);
		}
	}
	else
	{
		mItems.erase(it);
	}
	//item has been removed, need to update filter
	dirtyFilter();
	//because an item is going away regardless of filter status, force rearrange
	requestArrange();
	getRoot()->removeItemID(item->getListener()->getUUID());
	removeChild(item);
}

// This function is called by a child that needs to be resorted.
// This is only called for renaming an object because it won't work for date
void LLFolderViewFolder::resort(LLFolderViewItem* item)
{
	std::sort(mItems.begin(), mItems.end(), mSortFunction);
	std::sort(mFolders.begin(), mFolders.end(), mSortFunction);
}

bool LLFolderViewFolder::isTrash()
{
	if (mAmTrash == LLFolderViewFolder::UNKNOWN)
	{
		mAmTrash = mListener->getUUID() == gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH) ? LLFolderViewFolder::TRASH : LLFolderViewFolder::NOT_TRASH;
	}
	return mAmTrash == LLFolderViewFolder::TRASH;
}

void LLFolderViewFolder::sortBy(U32 order)
{
	if (!mSortFunction.updateSort(order))
	{
		// No changes.
		return;
	}

	// Propegate this change to sub folders
	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		(*fit)->sortBy(order);
	}

	std::sort(mFolders.begin(), mFolders.end(), mSortFunction);
	std::sort(mItems.begin(), mItems.end(), mSortFunction);

	if (order & LLInventoryFilter::SO_DATE)
	{
		U32 latest = 0;
		
		if (!mItems.empty())
		{
			LLFolderViewItem* item = *(mItems.begin());
			latest = item->getCreationDate();
		}

		if (!mFolders.empty())
		{
			LLFolderViewFolder* folder = *(mFolders.begin());
			if (folder->getCreationDate() > latest)
			{
				latest = folder->getCreationDate();
			}
		}
		mSubtreeCreationDate = latest;
	}
}

void LLFolderViewFolder::setItemSortOrder(U32 ordering)
{
	if (mSortFunction.updateSort(ordering))
	{
		for (folders_t::iterator iter = mFolders.begin();
			iter != mFolders.end();)
		{
			folders_t::iterator fit = iter++;
			(*fit)->setItemSortOrder(ordering);
		}

		std::sort(mFolders.begin(), mFolders.end(), mSortFunction);
		std::sort(mItems.begin(), mItems.end(), mSortFunction);
	}
}

EInventorySortGroup LLFolderViewFolder::getSortGroup()
{
	if (isTrash())
	{
		return SG_TRASH_FOLDER;
	}

	// Folders that can't be moved are 'system' folders. 
	if( mListener )
	{
		if( !(mListener->isItemMovable()) )
		{
			return SG_SYSTEM_FOLDER;
		}
	}
	
	return SG_NORMAL_FOLDER;
}

BOOL LLFolderViewFolder::isMovable()
{
	if( mListener )
	{
		if( !(mListener->isItemMovable()) )
		{
			return FALSE;
		}

		for (items_t::iterator iter = mItems.begin();
			 iter != mItems.end();)
		{
			items_t::iterator iit = iter++;
			if(!(*iit)->isMovable())
			{
				return FALSE;
			}
		}

		for (folders_t::iterator iter = mFolders.begin();
			 iter != mFolders.end();)
		{
			folders_t::iterator fit = iter++;
			if(!(*fit)->isMovable())
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}


BOOL LLFolderViewFolder::isRemovable()
{
	if( mListener )
	{
		if( !(mListener->isItemRemovable()) )
		{
			return FALSE;
		}

		for (items_t::iterator iter = mItems.begin();
			 iter != mItems.end();)
		{
			items_t::iterator iit = iter++;
			if(!(*iit)->isRemovable())
			{
				return FALSE;
			}
		}

		for (folders_t::iterator iter = mFolders.begin();
			 iter != mFolders.end();)
		{
			folders_t::iterator fit = iter++;
			if(!(*fit)->isRemovable())
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

// this is an internal method used for adding items to folders. 
BOOL LLFolderViewFolder::addItem(LLFolderViewItem* item)
{

	items_t::iterator it = std::lower_bound(
		mItems.begin(),
		mItems.end(),
		item,
		mSortFunction);
	mItems.insert(it,item);
	item->setRect(LLRect(0, 0, mRect.getWidth(), 0));
	item->setVisible(FALSE);
	addChild( item );
	item->dirtyFilter();
	requestArrange();
	return TRUE;
}

// this is an internal method used for adding items to folders. 
BOOL LLFolderViewFolder::addFolder(LLFolderViewFolder* folder)
{
	folders_t::iterator it = std::lower_bound(
		mFolders.begin(),
		mFolders.end(),
		folder,
		mSortFunction);
	mFolders.insert(it,folder);
	folder->setOrigin(0, 0);
	folder->reshape(mRect.getWidth(), 0);
	folder->setVisible(FALSE);
	addChild( folder );
	folder->dirtyFilter();
	// rearrange all descendants too, as our indentation level might have changed
	folder->requestArrange(TRUE);
	return TRUE;
}

void LLFolderViewFolder::requestArrange(BOOL include_descendants)	
{ 
	mLastArrangeGeneration = -1; 
	// flag all items up to root
	if (mParentFolder)
	{
		mParentFolder->requestArrange();
	}

	if (include_descendants)
	{
		for (folders_t::iterator iter = mFolders.begin();
			iter != mFolders.end();
			++iter)
		{
			(*iter)->requestArrange(TRUE);
		}
	}
}

void LLFolderViewFolder::toggleOpen()
{
	setOpen(!mIsOpen);
}

// Force a folder open or closed
void LLFolderViewFolder::setOpen(BOOL open)		/* Flawfinder: ignore */
{
	setOpenArrangeRecursively(open);		/* Flawfinder: ignore */
}

void LLFolderViewFolder::setOpenArrangeRecursively(BOOL open, ERecurseType recurse)		/* Flawfinder: ignore */
{
	BOOL was_open = mIsOpen;
	mIsOpen = open;		/* Flawfinder: ignore */
	if(!was_open && open)		/* Flawfinder: ignore */
	{
		if(mListener)
		{
			mListener->openItem();
		}
	}
	if (recurse == RECURSE_DOWN || recurse == RECURSE_UP_DOWN)
	{
		for (folders_t::iterator iter = mFolders.begin();
			 iter != mFolders.end();)
		{
			folders_t::iterator fit = iter++;
			(*fit)->setOpenArrangeRecursively(open, RECURSE_DOWN);		/* Flawfinder: ignore */
		}
	}
	if (mParentFolder && (recurse == RECURSE_UP || recurse == RECURSE_UP_DOWN))
	{
		mParentFolder->setOpenArrangeRecursively(open, RECURSE_UP);		/* Flawfinder: ignore */
	}
	
	if (was_open != mIsOpen)
	{
		requestArrange();
	}
}

BOOL LLFolderViewFolder::handleDragAndDropFromChild(MASK mask,
						BOOL drop,
						EDragAndDropType c_type,
						void* cargo_data,
						EAcceptance* accept,
						LLString& tooltip_msg)
{
	BOOL accepted = mListener && mListener->dragOrDrop(mask,drop,c_type,cargo_data);
	if (accepted) 
	{
		mDragAndDropTarget = TRUE;
		*accept = ACCEPT_YES_MULTI;
	}
	else 
	{
		*accept = ACCEPT_NO;
	}

	// drag and drop to child item, so clear pending auto-opens
	getRoot()->autoOpenTest(NULL);

	return TRUE;
}

void LLFolderViewFolder::open( void )		/* Flawfinder: ignore */
{
	toggleOpen();
}

void LLFolderViewFolder::applyFunctorRecursively(LLFolderViewFunctor& functor)
{
	functor.doFolder(this);

	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		(*fit)->applyFunctorRecursively(functor);
	}
	for (items_t::iterator iter = mItems.begin();
		 iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		functor.doItem((*iit));
	}
}

void LLFolderViewFolder::applyListenerFunctorRecursively(LLFolderViewListenerFunctor& functor)
{
	functor(mListener);
	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		(*fit)->applyListenerFunctorRecursively(functor);
	}
	for (items_t::iterator iter = mItems.begin();
		 iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		(*iit)->applyListenerFunctorRecursively(functor);
	}
}

// LLView functionality
BOOL LLFolderViewFolder::handleDragAndDrop(S32 x, S32 y, MASK mask,
										   BOOL drop,
										   EDragAndDropType cargo_type,
										   void* cargo_data,
										   EAcceptance* accept,
										   LLString& tooltip_msg)
{
	LLFolderView* root_view = getRoot();

	BOOL handled = FALSE;
	if(mIsOpen)
	{
		handled = childrenHandleDragAndDrop(x, y, mask, drop, cargo_type,
											cargo_data, accept, tooltip_msg) != NULL;
	}

	if (!handled)
	{
		BOOL accepted = mListener && mListener->dragOrDrop(mask, drop,cargo_type,cargo_data);

		if (accepted) 
		{
			mDragAndDropTarget = TRUE;
			*accept = ACCEPT_YES_MULTI;
		}
		else 
		{
			*accept = ACCEPT_NO;
		}

		if (!drop && accepted)
		{
			root_view->autoOpenTest(this);
		}

		lldebugst(LLERR_USER_INPUT) << "dragAndDrop handled by LLFolderViewFolder" << llendl;
	}

	return TRUE;
}


BOOL LLFolderViewFolder::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	if( getVisible() )
	{
		// fetch contents of this folder, as context menu can depend on contents
		// still, user would have to open context menu again to see the changes
		gInventory.fetchDescendentsOf(mListener->getUUID());
		
		if( mIsOpen )
		{
			handled = childrenHandleRightMouseDown( x, y, mask ) != NULL;
		}
		if (!handled)
		{
			handled = LLFolderViewItem::handleRightMouseDown( x, y, mask );
		}
	}
	return handled;
}


BOOL LLFolderViewFolder::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLView::handleHover(x, y, mask);

	if (!handled)
	{
		// this doesn't do child processing
		handled = LLFolderViewItem::handleHover(x, y, mask);
	}

	//if(x < LEFT_INDENTATION + mIndentation && x > mIndentation - LEFT_PAD && y > mRect.getHeight() - )
	//{
	//	gViewerWindow->setCursor(UI_CURSOR_ARROW);
	//	mExpanderHighlighted = TRUE;
	//	handled = TRUE;
	//}
	return handled;
}

BOOL LLFolderViewFolder::handleMouseDown( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	if( mIsOpen )
	{
		handled = childrenHandleMouseDown(x,y,mask) != NULL;
	}
	if( !handled )
	{
		if(x < LEFT_INDENTATION + mIndentation && x > mIndentation - LEFT_PAD)
		{
			toggleOpen();
			handled = TRUE;
		}
		else
		{
			// do normal selection logic
			handled = LLFolderViewItem::handleMouseDown(x, y, mask);
		}
	}

	return handled;
}

BOOL LLFolderViewFolder::handleDoubleClick( S32 x, S32 y, MASK mask )
{
	if (!getVisible())
	{
		return FALSE;
	}
	BOOL rv = false;
	if( mIsOpen )
	{
		rv = childrenHandleDoubleClick( x, y, mask ) != NULL;
	}
	if( !rv )
	{
		if(x < LEFT_INDENTATION + mIndentation && x > mIndentation - LEFT_PAD)
		{
			// don't select when user double-clicks plus sign
			// so as not to contradict single-click behavior
			toggleOpen();
		}
		else
		{
			setSelectionFromRoot(this, FALSE);
			toggleOpen();
		}
		return TRUE;
	}
	return rv;
}

void LLFolderViewFolder::draw()
{
	if (mAutoOpenCountdown != 0.f)
	{
		mControlLabelRotation = mAutoOpenCountdown * -90.f;
	}
	else if (mIsOpen)
	{
		mControlLabelRotation = lerp(mControlLabelRotation, -90.f, LLCriticalDamp::getInterpolant(0.04f));
	}
	else
	{
		mControlLabelRotation = lerp(mControlLabelRotation, 0.f, LLCriticalDamp::getInterpolant(0.025f));
	}

	LLFolderViewItem::draw();

	// draw children if root folder, or any other folder that is open or animating to closed state
	if( getRoot() == this || (mIsOpen || mCurHeight != mTargetHeight ))
	{
		LLView::draw();
	}

	mExpanderHighlighted = FALSE;
}

U32 LLFolderViewFolder::getCreationDate() const
{
	return llmax<U32>(mCreationDate, mSubtreeCreationDate);
}


BOOL	LLFolderViewFolder::potentiallyVisible()
{
	// folder should be visible by it's own filter status
	return LLFolderViewItem::potentiallyVisible() 	
		 // or one or more of its descendants have passed the minimum filter requirement
		|| hasFilteredDescendants(mRoot->getFilter()->getMinRequiredGeneration())
		// or not all of its descendants have been checked against minimum filter requirement
		|| getCompletedFilterGeneration() < getRoot()->getFilter()->getMinRequiredGeneration(); 
}

// this does prefix traversal, as folders are listed above their contents
LLFolderViewItem* LLFolderViewFolder::getNextFromChild( LLFolderViewItem* item, BOOL include_children )
{
	BOOL found_item = FALSE;

	LLFolderViewItem* result = NULL;
	// when not starting from a given item, start at beginning
	if(item == NULL)
	{
		found_item = TRUE;
	}

	// find current item among children
	folders_t::iterator fit = mFolders.begin();
	folders_t::iterator fend = mFolders.end();

	items_t::iterator iit = mItems.begin();
	items_t::iterator iend = mItems.end();

	// if not trivially starting at the beginning, we have to find the current item
	if (!found_item)
	{
		// first, look among folders, since they are always above items
		for(; fit != fend; ++fit)
		{
			if(item == (*fit))
			{
				found_item = TRUE;
				// if we are on downwards traversal
				if (include_children && (*fit)->isOpen())
				{
					// look for first descendant
					return (*fit)->getNextFromChild(NULL, TRUE);
				}
				// otherwise advance to next folder
				++fit;
				include_children = TRUE;
				break;
			}
		}

		// didn't find in folders?  Check items...
		if (!found_item)
		{
			for(; iit != iend; ++iit)
			{
				if(item == (*iit))
				{
					found_item = TRUE;
					// point to next item
					++iit;
					break;
				}
			}
		}
	}

	if (!found_item)
	{
		// you should never call this method with an item that isn't a child
		// so we should always find something
		llassert(FALSE);
		return NULL;
	}

	// at this point, either iit or fit point to a candidate "next" item
	// if both are out of range, we need to punt up to our parent

	// now, starting from found folder, continue through folders
	// searching for next visible folder
	while(fit != fend && !(*fit)->getVisible())
	{
		// turn on downwards traversal for next folder
		++fit;
	} 
	
	if (fit != fend)
	{
		result = (*fit);
	}
	else
	{
		// otherwise, scan for next visible item
		while(iit != iend && !(*iit)->getVisible())
		{
			++iit;
		} 

		// check to see if we have a valid item
		if (iit != iend)
		{
			result = (*iit);
		}
	}

	if( !result && mParentFolder )
	{
		// If there are no siblings or children to go to, recurse up one level in the tree
		// and skip children for this folder, as we've already discounted them
		result = mParentFolder->getNextFromChild(this, FALSE);
	}

	return result;
}

// this does postfix traversal, as folders are listed above their contents
LLFolderViewItem* LLFolderViewFolder::getPreviousFromChild( LLFolderViewItem* item, BOOL include_children )
{
	BOOL found_item = FALSE;

	LLFolderViewItem* result = NULL;
	// when not starting from a given item, start at end
	if(item == NULL)
	{
		found_item = TRUE;
	}

	// find current item among children
	folders_t::reverse_iterator fit = mFolders.rbegin();
	folders_t::reverse_iterator fend = mFolders.rend();

	items_t::reverse_iterator iit = mItems.rbegin();
	items_t::reverse_iterator iend = mItems.rend();

	// if not trivially starting at the end, we have to find the current item
	if (!found_item)
	{
		// first, look among items, since they are always below the folders
		for(; iit != iend; ++iit)
		{
			if(item == (*iit))
			{
				found_item = TRUE;
				// point to next item
				++iit;
				break;
			}
		}

		// didn't find in items?  Check folders...
		if (!found_item)
		{
			for(; fit != fend; ++fit)
			{
				if(item == (*fit))
				{
					found_item = TRUE;
					// point to next folder
					++fit;
					break;
				}
			}
		}
	}

	if (!found_item)
	{
		// you should never call this method with an item that isn't a child
		// so we should always find something
		llassert(FALSE);
		return NULL;
	}

	// at this point, either iit or fit point to a candidate "next" item
	// if both are out of range, we need to punt up to our parent

	// now, starting from found item, continue through items
	// searching for next visible item
	while(iit != iend && !(*iit)->getVisible())
	{
		++iit;
	} 
	
	if (iit != iend)
	{
		// we found an appropriate item
		result = (*iit);
	}
	else
	{
		// otherwise, scan for next visible folder
		while(fit != fend && !(*fit)->getVisible())
		{
			++fit;
		} 

		// check to see if we have a valid folder
		if (fit != fend)
		{
			// try selecting child element of this folder
			if ((*fit)->isOpen())
			{
				result = (*fit)->getPreviousFromChild(NULL);
			}
			else
			{
				result = (*fit);
			}
		}
	}

	if( !result )
	{
		// If there are no siblings or children to go to, recurse up one level in the tree
		// which gets back to this folder, which will only be visited if it is a valid, visible item
		result = this;
	}

	return result;
}


//---------------------------------------------------------------------------

// Tells all folders in a folderview to sort their items
// (and only their items, not folders) by a certain function.
class LLSetItemSortFunction : public LLFolderViewFunctor
{
public:
	LLSetItemSortFunction(U32 ordering)
		: mSortOrder(ordering) {}
	virtual ~LLSetItemSortFunction() {}
	virtual void doFolder(LLFolderViewFolder* folder);
	virtual void doItem(LLFolderViewItem* item);

	U32 mSortOrder;
};


// Set the sort order.
void LLSetItemSortFunction::doFolder(LLFolderViewFolder* folder)
{
	folder->setItemSortOrder(mSortOrder);
}

// Do nothing.
void LLSetItemSortFunction::doItem(LLFolderViewItem* item)
{
	return;
}

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


// Set the sort order.
void LLCloseAllFoldersFunctor::doFolder(LLFolderViewFolder* folder)
{
	folder->setOpenArrangeRecursively(mOpen);
}

// Do nothing.
void LLCloseAllFoldersFunctor::doItem(LLFolderViewItem* item)
{ }

///----------------------------------------------------------------------------
/// Class LLFolderView
///----------------------------------------------------------------------------

// Default constructor
LLFolderView::LLFolderView( const LLString& name, LLViewerImage* root_folder_icon, 
						   const LLRect& rect, const LLUUID& source_id, LLView *parent_view ) :
#if LL_WINDOWS
#pragma warning( push )
#pragma warning( disable : 4355 ) // warning C4355: 'this' : used in base member initializer list
#endif
	LLFolderViewFolder( name, root_folder_icon, this, NULL ),
#if LL_WINDOWS
#pragma warning( pop )
#endif
	mScrollContainer( NULL ),
	mPopupMenuHandle( LLViewHandle::sDeadHandle ),
	mAllowMultiSelect(TRUE),
	mShowFolderHierarchy(FALSE),
	mSourceID(source_id),
	mRenameItem( NULL ),
	mNeedsScroll( FALSE ),
	mLastScrollItem( NULL ),
	mNeedsAutoSelect( FALSE ),
	mAutoSelectOverride(FALSE),
	mNeedsAutoRename(FALSE),
	mDebugFilters(FALSE),
	mSortOrder(LLInventoryFilter::SO_FOLDERS_BY_NAME),	// This gets overridden by a pref immediately
	mFilter(name),
	mShowSelectionContext(FALSE),
	mShowSingleSelection(FALSE),
	mArrangeGeneration(0),
	mUserData(NULL),
	mSelectCallback(NULL),
	mSelectionChanged(FALSE),
	mMinWidth(0),
	mDragAndDropThisFrame(FALSE)
{
	LLRect new_rect(rect.mLeft, rect.mBottom + mRect.getHeight(), rect.mLeft + mRect.getWidth(), rect.mBottom);
	setRect( rect );
	reshape(rect.getWidth(), rect.getHeight());
	mIsOpen = TRUE; // this view is always open.
	mAutoOpenItems.setDepth(AUTO_OPEN_STACK_DEPTH);
	mAutoOpenCandidate = NULL;
	mAutoOpenTimer.stop();
	mKeyboardSelection = FALSE;
	mIndentation = -LEFT_INDENTATION; // children start at indentation 0
	gIdleCallbacks.addFunction(idle, this);

	//clear label
	// go ahead and render root folder as usual
	// just make sure the label ("Inventory Folder") never shows up
	mLabel = LLString::null;

	mRenamer = new LLLineEditor("ren", mRect, "", sFont,
								DB_INV_ITEM_NAME_STR_LEN,
								&LLFolderView::commitRename,
								NULL,
								NULL,
								this,
								&LLLineEditor::prevalidatePrintableNotPipe,
								LLViewBorder::BEVEL_NONE,
								LLViewBorder::STYLE_LINE,
								2);
	mRenamer->setWriteableBgColor(LLColor4::white);
	// Escape is handled by reverting the rename, not commiting it (default behavior)
	mRenamer->setCommitOnFocusLost(TRUE);
	mRenamer->setVisible(FALSE);
	addChild(mRenamer);

	// make the popup menu available
	LLMenuGL* menu = gUICtrlFactory->buildMenu("menu_inventory.xml", parent_view);
	if (!menu)
	{
		menu = new LLMenuGL("");
	}
	menu->setBackgroundColor(gColors.getColor("MenuPopupBgColor"));
	menu->setVisible(FALSE);
	mPopupMenuHandle = menu->mViewHandle;

	setTabStop(TRUE);
}

// Destroys the object
LLFolderView::~LLFolderView( void )
{
	// The release focus call can potentially call the
	// scrollcontainer, which can potentially be called with a partly
	// destroyed scollcontainer. Just null it out here, and no worries
	// about calling into the invalid scroll container.
	// Same with the renamer.
	mScrollContainer = NULL;
	mRenameItem = NULL;
	mRenamer = NULL;
	gFocusMgr.releaseFocusIfNeeded( this );

	if( gEditMenuHandler == this )
	{
		gEditMenuHandler = NULL;
	}

	mAutoOpenItems.removeAllNodes();
	gIdleCallbacks.deleteFunction(idle, this);

	LLView::deleteViewByHandle(mPopupMenuHandle);

	if(gViewerWindow->hasTopCtrl(mRenamer))
	{
		gViewerWindow->setTopCtrl(NULL);
	}

	mAutoOpenItems.removeAllNodes();
	clearSelection();
	mItems.clear();
	mFolders.clear();

	mItemMap.clear();
}

EWidgetType LLFolderView::getWidgetType() const
{
	return WIDGET_TYPE_FOLDER_VIEW;
}

LLString LLFolderView::getWidgetTag() const
{
	return LL_FOLDER_VIEW_TAG;
}

BOOL LLFolderView::canFocusChildren() const
{
	return FALSE;
}

void LLFolderView::checkTreeResortForModelChanged()
{
	if (mSortOrder & LLInventoryFilter::SO_DATE && !(mSortOrder & LLInventoryFilter::SO_FOLDERS_BY_NAME))
	{
		// This is the case where something got added or removed.  If we are date sorting
		// everything including folders, then we need to rebuild the whole tree.
		// Just set to something not SO_DATE to force the folder most resent date resort.
		mSortOrder = mSortOrder & ~LLInventoryFilter::SO_DATE;
		setSortOrder(mSortOrder | LLInventoryFilter::SO_DATE);
	}
}

void LLFolderView::setSortOrder(U32 order)
{
	if (order != mSortOrder)
	{
		LLFastTimer t(LLFastTimer::FTM_SORT);
		mSortOrder = order;

		for (folders_t::iterator iter = mFolders.begin();
			 iter != mFolders.end();)
		{
			folders_t::iterator fit = iter++;
			(*fit)->sortBy(order);
		}

		arrangeAll();
	}
}


U32 LLFolderView::getSortOrder() const
{
	return mSortOrder;
}

BOOL LLFolderView::addFolder( LLFolderViewFolder* folder)
{
	// enforce sort order of My Inventory followed by Library
	if (folder->getListener()->getUUID() == gInventoryLibraryRoot)
	{
		mFolders.push_back(folder);
	}
	else
	{
		mFolders.insert(mFolders.begin(), folder);
	}
	folder->setOrigin(0, 0);
	folder->reshape(mRect.getWidth(), 0);
	folder->setVisible(FALSE);
	addChild( folder );
	folder->dirtyFilter();
	folder->requestArrange();
	return TRUE;
}

void LLFolderView::closeAllFolders()
{
	// Close all the folders
	setOpenArrangeRecursively(FALSE, LLFolderViewFolder::RECURSE_DOWN);
}

void LLFolderView::openFolder(const LLString& foldername)
{
	LLFolderViewFolder* inv = (LLFolderViewFolder*)getChildByName(foldername);
	if (inv)
	{
		setSelection(inv, FALSE, FALSE);
		inv->setOpen(TRUE);
	}
}

void LLFolderView::setOpenArrangeRecursively(BOOL open, ERecurseType recurse)		/* Flawfinder: ignore */
{
	// call base class to do proper recursion
	LLFolderViewFolder::setOpenArrangeRecursively(open, recurse);		/* Flawfinder: ignore */
	// make sure root folder is always open
	mIsOpen = TRUE;
}

// This view grows and shinks to enclose all of its children items and folders.
S32 LLFolderView::arrange( S32* unused_width, S32* unused_height, S32 filter_generation )
{
	LLFastTimer t2(LLFastTimer::FTM_ARRANGE);

	filter_generation = mFilter.getMinRequiredGeneration();
	mMinWidth = 0;

	mHasVisibleChildren = hasFilteredDescendants(filter_generation);
	// arrange always finishes, so optimistically set the arrange generation to the most current
	mLastArrangeGeneration = mRoot->getArrangeGeneration();

	LLInventoryFilter::EFolderShow show_folder_state = getRoot()->getShowFolderState();

	S32 total_width = LEFT_PAD;
	S32 running_height = mDebugFilters ? llceil(sSmallFont->getLineHeight()) : 0;
	S32 target_height = running_height;
	S32 parent_item_height = mRect.getHeight();

	for (folders_t::iterator iter = mFolders.begin();
		 iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		LLFolderViewFolder* folderp = (*fit);
		if (getDebugFilters())
		{
			folderp->setVisible(TRUE);
		}
		else
		{
			folderp->setVisible(show_folder_state == LLInventoryFilter::SHOW_ALL_FOLDERS || // always show folders?
									(folderp->getFiltered(filter_generation) || folderp->hasFilteredDescendants(filter_generation))); // passed filter or has descendants that passed filter
		}
		if (folderp->getVisible())
		{
			S32 child_height = 0;
			S32 child_width = 0;
			S32 child_top = parent_item_height - running_height;
			
			target_height += folderp->arrange( &child_width, &child_height, filter_generation );

			mMinWidth = llmax(mMinWidth, child_width);
			total_width = llmax( total_width, child_width );
			running_height += child_height;
			folderp->setOrigin( ICON_PAD, child_top - (*fit)->getRect().getHeight() );
		}
	}

	for (items_t::iterator iter = mItems.begin();
		 iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		LLFolderViewItem* itemp = (*iit);
		itemp->setVisible(itemp->getFiltered(filter_generation));

		if (itemp->getVisible())
		{
			S32 child_width = 0;
			S32 child_height = 0;
			S32 child_top = parent_item_height - running_height;
			
			target_height += itemp->arrange( &child_width, &child_height, filter_generation );
			itemp->reshape(itemp->getRect().getWidth(), child_height);

			mMinWidth = llmax(mMinWidth, child_width);
			total_width = llmax( total_width, child_width );
			running_height += child_height;
			itemp->setOrigin( ICON_PAD, child_top - itemp->getRect().getHeight() );
		}
	}

	S32 dummy_s32;
	BOOL dummy_bool;
	S32 min_width;
	mScrollContainer->calcVisibleSize( &min_width, &dummy_s32, &dummy_bool, &dummy_bool);
	reshape( llmax(min_width, total_width), running_height );

	S32 new_min_width;
	mScrollContainer->calcVisibleSize( &new_min_width, &dummy_s32, &dummy_bool, &dummy_bool);
	if (new_min_width != min_width)
	{
		reshape( llmax(min_width, total_width), running_height );
	}

	mTargetHeight = (F32)target_height;
	return llround(mTargetHeight);
}

const LLString LLFolderView::getFilterSubString(BOOL trim)
{
	return mFilter.getFilterSubString(trim);
}

void LLFolderView::filter( LLInventoryFilter& filter )
{
	LLFastTimer t2(LLFastTimer::FTM_FILTER);
	filter.setFilterCount(llclamp(gSavedSettings.getS32("FilterItemsPerFrame"), 1, 5000));

	if (getCompletedFilterGeneration() < filter.getCurrentGeneration())
	{
		mFiltered = FALSE;
		mMinWidth = 0;
		LLFolderViewFolder::filter(filter);
	}
}

void LLFolderView::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	S32 min_width = 0;
	S32 dummy_height;
	BOOL dummy_bool;
	if (mScrollContainer)
	{
		mScrollContainer->calcVisibleSize( &min_width, &dummy_height, &dummy_bool, &dummy_bool);
	}
	width = llmax(mMinWidth, min_width);
	LLView::reshape(width, height, called_from_parent);
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
BOOL LLFolderView::setSelection(LLFolderViewItem* selection, BOOL open,		/* Flawfinder: ignore */
								BOOL take_keyboard_focus)
{
	if( selection == this )
	{
		return FALSE;
	}

	if( selection && take_keyboard_focus)
	{
		setFocus(TRUE);
	}

	// clear selection down here because change of keyboard focus can potentially
	// affect selection
	clearSelection();

	if(selection)
	{
		addToSelectionList(selection);
	}

	BOOL rv = LLFolderViewFolder::setSelection(selection, open, take_keyboard_focus);
	if(open && selection)
	{
		selection->getParentFolder()->requestArrange();
	}

	llassert(mSelectedItems.size() <= 1);

	mSelectionChanged = TRUE;

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

	mSelectionChanged = TRUE;
	
	return rv;
}

S32 LLFolderView::extendSelection(LLFolderViewItem* selection, LLFolderViewItem* last_selected, LLDynamicArray<LLFolderViewItem*>& items)
{
	S32 rv = 0;

	// now store resulting selection
	if (mAllowMultiSelect)
	{
		LLFolderViewItem *cur_selection = getCurSelectedItem();
		rv = LLFolderViewFolder::extendSelection(selection, cur_selection, items);
		for (S32 i = 0; i < items.count(); i++)
		{
			addToSelectionList(items[i]);
			rv++;
		}
	}
	else
	{
		setSelection(selection, FALSE, FALSE);
		rv++;
	}

	mSelectionChanged = TRUE;
	return rv;
}

void LLFolderView::sanitizeSelection()
{
	// store off current item in case it is automatically deselected
	// and we want to preserve context
	LLFolderViewItem* original_selected_item = getCurSelectedItem();

	// Cache "Show all folders" filter setting
	BOOL show_all_folders = (getRoot()->getShowFolderState() == LLInventoryFilter::SHOW_ALL_FOLDERS);

	std::vector<LLFolderViewItem*> items_to_remove;
	selected_items_t::iterator item_iter;
	for (item_iter = mSelectedItems.begin(); item_iter != mSelectedItems.end(); ++item_iter)
	{
		LLFolderViewItem* item = *item_iter;

		// ensure that each ancestor is open and potentially passes filtering
		BOOL visible = item->potentiallyVisible(); // initialize from filter state for this item
		// modify with parent open and filters states
		LLFolderViewFolder* parent_folder = item->getParentFolder();
		if ( parent_folder )
		{
			if ( show_all_folders )
			{	// "Show all folders" is on, so this folder is visible
				visible = TRUE;
			}
			else
			{	// Move up through parent folders and see what's visible
				while(parent_folder)
				{
					visible = visible && parent_folder->isOpen() && parent_folder->potentiallyVisible();
					parent_folder = parent_folder->getParentFolder();
				}
			}
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
	}

	std::vector<LLFolderViewItem*>::iterator item_it;
	for (item_it = items_to_remove.begin(); item_it != items_to_remove.end(); ++item_it )
	{
		changeSelection(*item_it, FALSE); // toggle selection (also removes from list)
	}

	// if nothing selected after prior constraints...
	if (mSelectedItems.empty())
	{
		// ...select first available parent of original selection, or "My Inventory" otherwise
		LLFolderViewItem* new_selection = NULL;
		if (original_selected_item)
		{
			for(LLFolderViewFolder* parent_folder = original_selected_item->getParentFolder();
				parent_folder;
				parent_folder = parent_folder->getParentFolder())
			{
				if (parent_folder->potentiallyVisible())
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
			// nothing selected to start with, so pick "My Inventory" as best guess
			new_selection = getItemByID(gAgent.getInventoryRootID());
		}

		if (new_selection)
		{
			setSelection(new_selection, FALSE, FALSE);
		}
	}
}

void LLFolderView::clearSelection()
{
	if (mSelectedItems.size() > 0)
	{
		recursiveDeselect(FALSE);
		mSelectedItems.clear();
	}
}

BOOL LLFolderView::getSelectionList(std::set<LLUUID> &selection)
{
	selected_items_t::iterator item_it;
	for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
	{
		selection.insert((*item_it)->getListener()->getUUID());
	}

	return (selection.size() != 0);
}

BOOL LLFolderView::startDrag(LLToolDragAndDrop::ESource source)
{
	std::vector<EDragAndDropType> types;
	std::vector<LLUUID> cargo_ids;
	selected_items_t::iterator item_it;
	BOOL can_drag = TRUE;
	if (!mSelectedItems.empty())
	{
		for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
		{
			EDragAndDropType type = DAD_NONE;
			LLUUID id = LLUUID::null;
			can_drag = can_drag && (*item_it)->getListener()->startDrag(&type, &id);

			types.push_back(type);
			cargo_ids.push_back(id);
		}

		gToolDragAndDrop->beginMultiDrag(types, cargo_ids, source, mSourceID); 
	}
	return can_drag;
}

void LLFolderView::commitRename( LLUICtrl* renamer, void* user_data )
{
	LLFolderView* root = reinterpret_cast<LLFolderView*>(user_data);
	if( root )
	{
		root->finishRenamingItem();
	}
}

void LLFolderView::draw()
{
	if (mDebugFilters)
	{
		LLString current_filter_string = llformat("Current Filter: %d, Least Filter: %d, Auto-accept Filter: %d",
										mFilter.getCurrentGeneration(), mFilter.getMinRequiredGeneration(), mFilter.getMustPassGeneration());
		sSmallFont->renderUTF8(current_filter_string, 0, 2, 
			mRect.getHeight() - sSmallFont->getLineHeight(), LLColor4(0.5f, 0.5f, 0.8f, 1.f), 
			LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, S32_MAX, S32_MAX, NULL, FALSE );
	}

	// if cursor has moved off of me during drag and drop
	// close all auto opened folders
	if (!mDragAndDropThisFrame)
	{
		closeAutoOpenedFolders();
	}
	if(gViewerWindow->hasKeyboardFocus(this) && !getVisible())
	{
		gViewerWindow->setKeyboardFocus( NULL, NULL );
	}

	// while dragging, update selection rendering to reflect single/multi drag status
	if (gToolDragAndDrop->hasMouseCapture())
	{
		EAcceptance last_accept = gToolDragAndDrop->getLastAccept();
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

	if (hasVisibleChildren() || getShowFolderState() == LLInventoryFilter::SHOW_ALL_FOLDERS)
	{
		mStatusText.clear();
	}
	else
	{
		if (gInventory.backgroundFetchActive() || mCompletedFilterGeneration < mFilter.getMinRequiredGeneration())
		{
			mStatusText = "Searching..."; // *TODO:translate
			sFont->renderUTF8(mStatusText, 0, 2, 1, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP, LLFontGL::NORMAL, S32_MAX, S32_MAX, NULL, FALSE );
		}
		else
		{
			mStatusText = "No matching items found in inventory."; // *TODO:translate
			sFont->renderUTF8(mStatusText, 0, 2, 1, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP, LLFontGL::NORMAL, S32_MAX, S32_MAX, NULL, FALSE );
		}
	}

	LLFolderViewFolder::draw();

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
		mRenameItem->rename( mRenamer->getText().c_str() );
	}

	mRenamer->setCommitOnFocusLost( FALSE );
	mRenamer->setFocus( FALSE );
	mRenamer->setVisible( FALSE );
	mRenamer->setCommitOnFocusLost( TRUE );
	gViewerWindow->setTopCtrl( NULL );

	if( mRenameItem )
	{
		setSelectionFromRoot( mRenameItem, TRUE );
		mRenameItem = NULL;
	}

	// List is re-sorted alphabeticly, so scroll to make sure the selected item is visible.
	scrollToShowSelection();
}

void LLFolderView::revertRenamingItem( void )
{
	mRenamer->setCommitOnFocusLost( FALSE );
	mRenamer->setFocus( FALSE );
	mRenamer->setVisible( FALSE );
	mRenamer->setCommitOnFocusLost( TRUE );
	gViewerWindow->setTopCtrl( NULL );

	if( mRenameItem )
	{
		setSelectionFromRoot( mRenameItem, TRUE );
		mRenameItem = NULL;
	}
}

void LLFolderView::removeSelectedItems( void )
{
	if(getVisible() && mEnabled)
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
			if(item->isRemovable())
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
				if (parent->removeItem(item_to_delete))
				{
					// change selection on successful delete
					if (new_selection)
					{
						setSelectionFromRoot(new_selection, new_selection->isOpen(), gViewerWindow->childHasKeyboardFocus(this));
					}
					else
					{
						setSelectionFromRoot(NULL, gViewerWindow->childHasKeyboardFocus(this));
					}
				}
			}
			arrangeAll();
		}
		else if (count > 1)
		{
			LLDynamicArray<LLFolderViewEventListener*> listeners;
			LLFolderViewEventListener* listener;
			LLFolderViewItem* last_item = items[count - 1];
			LLFolderViewItem* new_selection = last_item->getNextOpenNode(FALSE);
			while(new_selection && new_selection->isSelected())
			{
				new_selection = new_selection->getNextOpenNode(FALSE);
			}
			if (!new_selection)
			{
				new_selection = last_item->getPreviousOpenNode(FALSE);
				while (new_selection && new_selection->isSelected())
				{
					new_selection = new_selection->getPreviousOpenNode(FALSE);
				}
			}
			if (new_selection)
			{
				setSelectionFromRoot(new_selection, new_selection->isOpen(), gViewerWindow->childHasKeyboardFocus(this));
			}
			else
			{
				setSelectionFromRoot(NULL, gViewerWindow->childHasKeyboardFocus(this));
			}

			for(S32 i = 0; i < count; ++i)
			{
				listener = items[i]->getListener();
				if(listener && (listeners.find(listener) == LLDynamicArray<LLFolderViewEventListener*>::FAIL))
				{
					listeners.put(listener);
				}
			}
			listener = listeners.get(0);
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
	if(getVisible() && mEnabled)
	{
		if (mSelectedItems.size() == 1)
		{
			mSelectedItems.front()->open();		/* Flawfinder: ignore */
		}
		else
		{
			S32 left, top;
			gFloaterView->getNewFloaterPosition(&left, &top);
			LLMultiPreview* multi_previewp = new LLMultiPreview(LLRect(left, top, left + 300, top - 100));
			gFloaterView->getNewFloaterPosition(&left, &top);
			LLMultiProperties* multi_propertiesp = new LLMultiProperties(LLRect(left, top, left + 300, top - 100));

			selected_items_t::iterator item_it;
			for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
			{
				// IT_{OBJECT,ATTACHMENT} creates LLProperties
				// floaters; others create LLPreviews.  Put
				// each one in the right type of container.
				LLFolderViewEventListener* listener = (*item_it)->getListener();
				bool is_prop = listener && (listener->getInventoryType() == LLInventoryType::IT_OBJECT || listener->getInventoryType() == LLInventoryType::IT_ATTACHMENT);
				if (is_prop)
					LLFloater::setFloaterHost(multi_propertiesp);
				else
					LLFloater::setFloaterHost(multi_previewp);
				(*item_it)->open();
			}

			LLFloater::setFloaterHost(NULL);
			// *NOTE: LLMulti* will safely auto-delete when open'd
			// without any children.
			multi_previewp->open();
			multi_propertiesp->open();
		}
	}
}

void LLFolderView::propertiesSelectedItems( void )
{
	if(getVisible() && mEnabled)
	{
		if (mSelectedItems.size() == 1)
		{
			LLFolderViewItem* folder_item = mSelectedItems.front();
			if(!folder_item) return;
			folder_item->getListener()->showProperties();
		}
		else
		{
			S32 left, top;
			gFloaterView->getNewFloaterPosition(&left, &top);

			LLMultiProperties* multi_propertiesp = new LLMultiProperties(LLRect(left, top, left + 100, top - 100));

			LLFloater::setFloaterHost(multi_propertiesp);

			selected_items_t::iterator item_it;
			for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
			{
				(*item_it)->getListener()->showProperties();
			}

			LLFloater::setFloaterHost(NULL);
			multi_propertiesp->open();		/* Flawfinder: ignore */
		}
	}
}

void LLFolderView::autoOpenItem( LLFolderViewFolder* item )
{
	if (mAutoOpenItems.check() == item || mAutoOpenItems.getDepth() >= (U32)AUTO_OPEN_STACK_DEPTH)
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
	scrollToShowItem(item);
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

BOOL LLFolderView::canCopy()
{
	if (!(getVisible() && mEnabled && (mSelectedItems.size() > 0)))
	{
		return FALSE;
	}

	selected_items_t::iterator selected_it;
	for (selected_it = mSelectedItems.begin(); selected_it != mSelectedItems.end(); ++selected_it)
	{
		LLFolderViewItem* item = *selected_it;
		if (!item->getListener()->isItemCopyable())
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
	LLInventoryClipboard::instance().reset();
	S32 count = mSelectedItems.size();
	if(getVisible() && mEnabled && (count > 0))
	{
		LLFolderViewEventListener* listener = NULL;
		selected_items_t::iterator item_it;
		for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
		{
			listener = (*item_it)->getListener();
			if(listener)
			{
				listener->copyToClipboard();
			}
		}
	}
	mSearchString.clear();
}

BOOL LLFolderView::canCut()
{
	return FALSE;
}

void LLFolderView::cut()
{
	// implement Windows-style cut-and-leave
}

BOOL LLFolderView::canPaste()
{
	if (mSelectedItems.empty())
	{
		return FALSE;
	}

	if(getVisible() && mEnabled)
	{
		selected_items_t::iterator item_it;
		for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
		{
			// *TODO: only check folders and parent folders of items
			LLFolderViewItem* item = (*item_it);
			LLFolderViewEventListener* listener = item->getListener();
			if(!listener || !listener->isClipboardPasteable())
			{
				LLFolderViewFolder* folderp = item->getParentFolder();
				listener = folderp->getListener();
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
	if(getVisible() && mEnabled)
	{
		// find set of unique folders to paste into
		std::set<LLFolderViewItem*> folder_set;

		selected_items_t::iterator selected_it;
		for (selected_it = mSelectedItems.begin(); selected_it != mSelectedItems.end(); ++selected_it)
		{
			LLFolderViewItem* item = *selected_it;
			LLFolderViewEventListener* listener = item->getListener();
			if (listener->getInventoryType() != LLInventoryType::IT_CATEGORY)
			{
				item = item->getParentFolder();
			}
			folder_set.insert(item);
		}

		std::set<LLFolderViewItem*>::iterator set_iter;
		for(set_iter = folder_set.begin(); set_iter != folder_set.end(); ++set_iter)
		{
			LLFolderViewEventListener* listener = (*set_iter)->getListener();
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
	if(getVisible() && mEnabled && (count == 1) && item && item->getListener() &&
	   item->getListener()->isItemRenameable())
	{
		mRenameItem = item;

		S32 x = ARROW_SIZE + TEXT_PAD + ICON_WIDTH + ICON_PAD - 1 + item->getIndentation();
		S32 y = llfloor(item->getRect().getHeight()-sFont->getLineHeight()-2);
		item->localPointToScreen( x, y, &x, &y );
		screenPointToLocal( x, y, &x, &y );
		mRenamer->setOrigin( x, y );

		S32 scroller_height = 0;
		S32 scroller_width = gViewerWindow->getWindowWidth();
		BOOL dummy_bool;
		if (mScrollContainer)
		{
			mScrollContainer->calcVisibleSize( &scroller_width, &scroller_height, &dummy_bool, &dummy_bool);
		}

		S32 width = llmax(llmin(item->getRect().getWidth() - x, scroller_width - x - mRect.mLeft), MINIMUM_RENAMER_WIDTH);
		S32 height = llfloor(sFont->getLineHeight() + RENAME_HEIGHT_PAD);
		mRenamer->reshape( width, height, TRUE );

		mRenamer->setText(item->getName());
		mRenamer->selectAll();
		mRenamer->setVisible( TRUE );
		// set focus will fail unless item is visible
		mRenamer->setFocus( TRUE );
		mRenamer->setFocusLostCallback(renamer_focus_lost);
		gViewerWindow->setTopCtrl( mRenamer );
	}
}

void LLFolderView::setFocus(BOOL focus)
{
	if (focus)
	{
		if(!hasFocus())
		{
			gEditMenuHandler = this;
		}
	}

	LLFolderViewFolder::setFocus(focus);
}

BOOL LLFolderView::handleKeyHere( KEY key, MASK mask, BOOL called_from_parent )
{
	BOOL handled = FALSE;

	LLView *item = NULL;
	if (getChildCount() > 0)
	{
		item = *(getChildList()->begin());
	}

	if( getVisible() && mEnabled && !called_from_parent )
	{
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
			// mark flag don't commit
			if( mRenameItem && mRenamer->getVisible() )
			{
				revertRenamingItem();
				handled = TRUE;
			}
			else
			{
				if( gViewerWindow->childHasKeyboardFocus( this ) )
				{
					gViewerWindow->setKeyboardFocus( NULL, NULL );
				}
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
							changeSelectionFromRoot(last_selected, FALSE);
						}
						else if (last_selected->getParentFolder() == next->getParentFolder())
						{
							// grow selection
							changeSelectionFromRoot(next, TRUE);
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
							return FALSE;
						}
						setSelection( next, FALSE, TRUE );
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
							changeSelectionFromRoot(last_selected, FALSE);
						}
						else if (last_selected->getParentFolder() == prev->getParentFolder())
						{
							// grow selection
							changeSelectionFromRoot(prev, TRUE);
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
	}

	if (!handled && gFocusMgr.childHasKeyboardFocus(getRoot()))
	{
		if (key == KEY_BACKSPACE)
		{
			mSearchTimer.reset();
			if (mSearchString.size())
			{
				mSearchString.erase(mSearchString.size() - 1, 1);
			}
			search(getCurSelectedItem(), mSearchString.c_str(), FALSE);
			handled = TRUE;
		}
	}

	return handled;
}


BOOL LLFolderView::handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent)
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
	if (gFocusMgr.childHasKeyboardFocus(getRoot()))
	{
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
		search(getCurSelectedItem(), mSearchString.c_str(), FALSE);

		handled = TRUE;
	}

	return handled;
}


BOOL LLFolderView::canDoDelete()
{
	if (mSelectedItems.size() == 0) return FALSE;
	selected_items_t::iterator item_it;
	for (item_it = mSelectedItems.begin(); item_it != mSelectedItems.end(); ++item_it)
	{
		if (!(*item_it)->getListener()->isItemRemovable())
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

	setFocus(TRUE);

	return LLView::handleMouseDown( x, y, mask );
}

void LLFolderView::onFocusLost( )
{
	if( gEditMenuHandler == this )
	{
		gEditMenuHandler = NULL;
	}
	LLUICtrl::onFocusLost();
}

BOOL LLFolderView::search(LLFolderViewItem* first_item, const LLString &search_string, BOOL backward)
{
	// get first selected item
	LLFolderViewItem* search_item = first_item;

	// make sure search string is upper case
	LLString upper_case_string = search_string;
	LLString::toUpper(upper_case_string);

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

		const LLString current_item_label(search_item->getSearchableLabel());
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
	if (!getVisible())
	{
		return FALSE;
	}

	return LLView::handleDoubleClick( x, y, mask );
}

BOOL LLFolderView::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
	// all user operations move keyboard focus to inventory
	// this way, we know when to stop auto-updating a search
	setFocus(TRUE);

	BOOL handled = childrenHandleRightMouseDown(x, y, mask) != NULL;
	S32 count = mSelectedItems.size();
	LLMenuGL* menu = (LLMenuGL*)LLView::getViewByHandle(mPopupMenuHandle);
	if(handled && (count > 0) && menu)
	{
		//menu->empty();
		const LLView::child_list_t *list = menu->getChildList();

		LLView::child_list_t::const_iterator menu_itor;
		for (menu_itor = list->begin(); menu_itor != list->end(); ++menu_itor)
		{
			(*menu_itor)->setVisible(TRUE);
			(*menu_itor)->setEnabled(TRUE);
		}
		
		// Successively filter out invalid options
		selected_items_t::iterator item_itor;
		U32 flags = FIRST_SELECTED_ITEM;
		for (item_itor = mSelectedItems.begin(); item_itor != mSelectedItems.end(); ++item_itor)
		{
			(*item_itor)->buildContextMenu(*menu, flags);
			flags = 0x0;
		}

		menu->arrange();
		menu->updateParent(LLMenuGL::sMenuContainer);
		LLMenuGL::showPopup(this, menu, x, y);
	}
	else
	{
		if(menu && menu->getVisible())
		{
			menu->setVisible(FALSE);
		}
		setSelection(NULL, FALSE, TRUE);
	}
	return handled;
}

BOOL LLFolderView::handleHover( S32 x, S32 y, MASK mask )
{
	return LLView::handleHover( x, y, mask );
}

BOOL LLFolderView::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data, 
									 EAcceptance* accept,
									 LLString& tooltip_msg)
{
	mDragAndDropThisFrame = TRUE;
	BOOL handled = LLView::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data,
											 accept, tooltip_msg);

	if (handled)
	{
		lldebugst(LLERR_USER_INPUT) << "dragAndDrop handled by LLFolderView" << llendl;
	}

	return handled;
}

BOOL LLFolderView::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if (mScrollContainer)
	{
		return mScrollContainer->handleScrollWheel(x, y, clicks);
	}
	return FALSE;
}

void LLFolderView::deleteAllChildren()
{
	if(gViewerWindow->hasTopCtrl(mRenamer))
	{
		gViewerWindow->setTopCtrl(NULL);
	}
	LLView::deleteViewByHandle(mPopupMenuHandle);
	mPopupMenuHandle = LLViewHandle::sDeadHandle;
	mRenamer = NULL;
	mRenameItem = NULL;
	clearSelection();
	LLView::deleteAllChildren();
}

void LLFolderView::scrollToShowSelection()
{
	if (mSelectedItems.size())
	{
		mNeedsScroll = TRUE;
	}
}

// If the parent is scroll containter, scroll it to make the selection
// is maximally visible.
void LLFolderView::scrollToShowItem(LLFolderViewItem* item)
{
	// don't scroll to items when mouse is being used to scroll/drag and drop
	if (gFocusMgr.childHasMouseCapture(mScrollContainer))
	{
		mNeedsScroll = FALSE;
		return;
	}
	if(item && mScrollContainer)
	{
		LLRect local_rect = item->getRect();
		LLRect item_scrolled_rect; // item position relative to display area of scroller
		
		S32 icon_height = mIcon.isNull() ? 0 : mIcon->getHeight(); 
		S32 label_height = llround(sFont->getLineHeight()); 
		// when navigating with keyboard, only move top of folders on screen, otherwise show whole folder
		S32 max_height_to_show = gFocusMgr.childHasKeyboardFocus(this) ? (llmax( icon_height, label_height ) + ICON_PAD) : local_rect.getHeight(); 
		item->localPointToOtherView(item->getIndentation(), llmax(0, local_rect.getHeight() - max_height_to_show), &item_scrolled_rect.mLeft, &item_scrolled_rect.mBottom, mScrollContainer);
		item->localPointToOtherView(local_rect.getWidth(), local_rect.getHeight(), &item_scrolled_rect.mRight, &item_scrolled_rect.mTop, mScrollContainer);

		item_scrolled_rect.mRight = llmin(item_scrolled_rect.mLeft + MIN_ITEM_WIDTH_VISIBLE, item_scrolled_rect.mRight);
		LLCoordGL scroll_offset(-mScrollContainer->getBorderWidth() - item_scrolled_rect.mLeft, 
				mScrollContainer->getRect().getHeight() - item_scrolled_rect.mTop - 1);

		S32 max_scroll_offset = getVisibleRect().getHeight() - item_scrolled_rect.getHeight();
		if (item != mLastScrollItem || // if we're scrolling to focus on a new item
		// or the item has just appeared on screen and it wasn't onscreen before
			(scroll_offset.mY > 0 && scroll_offset.mY < max_scroll_offset && 
			(mLastScrollOffset.mY < 0 || mLastScrollOffset.mY > max_scroll_offset)))
		{
			// we now have a position on screen that we want to keep stable
			// offset of selection relative to top of visible area
			mLastScrollOffset = scroll_offset;
			mLastScrollItem = item;
		}

		mScrollContainer->scrollToShowRect( item_scrolled_rect, mLastScrollOffset );

		// after scrolling, store new offset
		// in case we don't have room to maintain the original position
		LLCoordGL new_item_left_top;
		item->localPointToOtherView(item->getIndentation(), item->getRect().getHeight(), &new_item_left_top.mX, &new_item_left_top.mY, mScrollContainer);
		mLastScrollOffset.set(-mScrollContainer->getBorderWidth() - new_item_left_top.mX, mScrollContainer->getRect().getHeight() - new_item_left_top.mY - 1);
	}
}

LLRect LLFolderView::getVisibleRect()
{
	S32 visible_height = mScrollContainer->getRect().getHeight();
	S32 visible_width = mScrollContainer->getRect().getWidth();
	LLRect visible_rect;
	visible_rect.setLeftTopAndSize(-mRect.mLeft, visible_height - mRect.mBottom, visible_width, visible_height);
	return visible_rect;
}

BOOL LLFolderView::getShowSelectionContext()
{
	if (mShowSelectionContext)
	{
		return TRUE;
	}
	LLMenuGL* menu = (LLMenuGL*)LLView::getViewByHandle(mPopupMenuHandle);
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

void LLFolderView::addItemID(const LLUUID& id, LLFolderViewItem* itemp)
{
	mItemMap[id] = itemp;
}

void LLFolderView::removeItemID(const LLUUID& id)
{
	mItemMap.erase(id);
}

LLFolderViewItem* LLFolderView::getItemByID(const LLUUID& id)
{
	if (id.isNull())
	{
		return this;
	}

	std::map<LLUUID, LLFolderViewItem*>::iterator map_it;
	map_it = mItemMap.find(id);
	if (map_it != mItemMap.end())
	{
		return map_it->second;
	}

	return NULL;
}


// Main idle routine
void LLFolderView::doIdle()
{
	LLFastTimer t2(LLFastTimer::FTM_INVENTORY);

	BOOL debug_filters = gSavedSettings.getBOOL("DebugInventoryFilters");
	if (debug_filters != getDebugFilters())
	{
		mDebugFilters = debug_filters;
		arrangeAll();
	}

	mFilter.clearModified();
	BOOL filter_modified_and_active = mCompletedFilterGeneration < mFilter.getCurrentGeneration() && 
										mFilter.isNotDefault();
	mNeedsAutoSelect = filter_modified_and_active &&
							!(gFocusMgr.childHasKeyboardFocus(this) || gFocusMgr.getMouseCapture());
	
	// filter to determine visiblity before arranging
	filterFromRoot();

	// automatically show matching items, and select first one
	// do this every frame until user puts keyboard focus into the inventory window
	// signaling the end of the automatic update
	// only do this when mNeedsFilter is set, meaning filtered items have
	// potentially changed
	if (mNeedsAutoSelect)
	{
		LLFastTimer t3(LLFastTimer::FTM_AUTO_SELECT);
		// select new item only if a filtered item not currently selected
		LLFolderViewItem* selected_itemp = mSelectedItems.empty() ? NULL : mSelectedItems.back();
		if ((!selected_itemp || !selected_itemp->getFiltered()) && !mAutoSelectOverride)
		{
			// select first filtered item
			LLSelectFirstFilteredItem filter;
			applyFunctorRecursively(filter);
		}
		scrollToShowSelection();
	}

	BOOL is_visible = isInVisibleChain();

	if ( is_visible )
	{
		sanitizeSelection();
		if( needsArrange() )
		{
			arrangeFromRoot();
		}
	}

	if (mSelectedItems.size() && mNeedsScroll)
	{
		scrollToShowItem(mSelectedItems.back());
		// continue scrolling until animated layout change is done
		if (getCompletedFilterGeneration() >= mFilter.getMinRequiredGeneration() &&
			(!needsArrange() || !is_visible))
		{
			mNeedsScroll = FALSE;
		}
	}

	if (mSelectionChanged && mSelectCallback)
	{
		//RN: we use keyboard focus as a proxy for user-explicit actions
		mSelectCallback(mSelectedItems, gFocusMgr.childHasKeyboardFocus(this), mUserData);
	}
	mSelectionChanged = FALSE;
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

///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------
bool LLInventorySort::updateSort(U32 order)
{
	if (order != mSortOrder)
	{
		mSortOrder = order;
		mByDate = (order & LLInventoryFilter::SO_DATE);
		mSystemToTop = (order & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP);
		mFoldersByName = (order & LLInventoryFilter::SO_FOLDERS_BY_NAME);
		return true;
	}
	return false;
}

bool LLInventorySort::operator()(LLFolderViewItem* a, LLFolderViewItem* b)
{
	// We sort by name if we aren't sorting by date
	// OR if these are folders and we are sorting folders by name.
	bool by_name = (!mByDate 
					|| (mFoldersByName 
						&& (a->getSortGroup() != SG_ITEM)));
	
	if (a->getSortGroup() != b->getSortGroup())
	{
		if (mSystemToTop)
		{
			// Group order is System Folders, Trash, Normal Folders, Items
			return (a->getSortGroup() < b->getSortGroup());
		}
		else if (mByDate)
		{
			// Trash needs to go to the bottom if we are sorting by date
			if ( (a->getSortGroup() == SG_TRASH_FOLDER)
				|| (b->getSortGroup() == SG_TRASH_FOLDER))
			{
				return (b->getSortGroup() == SG_TRASH_FOLDER);
			}
		}
	}

	if (by_name)
	{
		S32 compare = LLString::compareDict(a->getLabel(), b->getLabel());
		if (0 == compare)
		{
			return (a->getCreationDate() > b->getCreationDate());
		}
		else
		{
			return (compare < 0);
		}
	}
	else
	{
		// BUG: This is very very slow.  The getCreationDate() is log n in number
		// of inventory items.
		U32 first_create = a->getCreationDate();
		U32 second_create = b->getCreationDate();
		if (first_create == second_create)
		{
			return (LLString::compareDict(a->getLabel(), b->getLabel()) < 0);
		}
		else
		{
			return (first_create > second_create);
		}
	}
}

void renamer_focus_lost( LLUICtrl* ctrl, void* userdata)
{
	if( ctrl ) 
	{
		ctrl->setVisible( FALSE );
	}
}

void delete_selected_item(void* user_data)
{
	if(user_data)
	{
		LLFolderView* fv = reinterpret_cast<LLFolderView*>(user_data);
		fv->removeSelectedItems();
	}
}

void copy_selected_item(void* user_data)
{
	if(user_data)
	{
		LLFolderView* fv = reinterpret_cast<LLFolderView*>(user_data);
		fv->copy();
	}
}

void paste_items(void* user_data)
{
	if(user_data)
	{
		LLFolderView* fv = reinterpret_cast<LLFolderView*>(user_data);
		fv->paste();
	}
}

void open_selected_items(void* user_data)
{
	if(user_data)
	{
		LLFolderView* fv = reinterpret_cast<LLFolderView*>(user_data);
		fv->openSelectedItems();
	}
}

void properties_selected_items(void* user_data)
{
	if(user_data)
	{
		LLFolderView* fv = reinterpret_cast<LLFolderView*>(user_data);
		fv->propertiesSelectedItems();
	}
}

///----------------------------------------------------------------------------
/// Class LLFolderViewEventListener
///----------------------------------------------------------------------------

void LLFolderViewEventListener::arrangeAndSet(LLFolderViewItem* focus,
											  BOOL set_selection,
											  BOOL take_keyboard_focus)
{
	if(!focus) return;
	LLFolderView* root = focus->getRoot();
	focus->getParentFolder()->requestArrange();
	if(set_selection)
	{
		focus->setSelectionFromRoot(focus, TRUE, take_keyboard_focus);
		if(root)
		{
			root->scrollToShowSelection();
		}
	}
}


///----------------------------------------------------------------------------
/// Class LLInventoryFilter
///----------------------------------------------------------------------------
LLInventoryFilter::LLInventoryFilter(const LLString& name) :
	mName(name),
	mModified(FALSE),
	mNeedTextRebuild(TRUE)
{
	mFilterOps.mFilterTypes = 0xffffffff;
	mFilterOps.mMinDate = 0;
	mFilterOps.mMaxDate = U32_MAX;
	mFilterOps.mHoursAgo = 0;
	mFilterOps.mShowFolderState = SHOW_NON_EMPTY_FOLDERS;
	mFilterOps.mPermissions = PERM_NONE;
	
	mOrder = SO_FOLDERS_BY_NAME; // This gets overridden by a pref immediately

	mSubStringMatchOffset = 0;
	mFilterSubString = "";
	mFilterGeneration = 0;
	mMustPassGeneration = S32_MAX;
	mMinRequiredGeneration = 0;
	mFilterCount = 0;
	mNextFilterGeneration = mFilterGeneration + 1;

	mLastLogoff = gSavedPerAccountSettings.getU32("LastLogoff");
	mFilterBehavior = FILTER_NONE;
}

LLInventoryFilter::~LLInventoryFilter()
{
}

BOOL LLInventoryFilter::check(LLFolderViewItem* item) 
{
	U32 earliest;

	earliest = time_corrected() - mFilterOps.mHoursAgo * 3600;
	if (mFilterOps.mMinDate && mFilterOps.mMinDate < earliest)
	{
		earliest = mFilterOps.mMinDate;
	}
	else if (!mFilterOps.mHoursAgo)
	{
		earliest = 0;
	}
	LLFolderViewEventListener* listener = item->getListener();
	mSubStringMatchOffset = mFilterSubString.size() ? item->getSearchableLabel().find(mFilterSubString) : LLString::npos;
	BOOL passed = (0x1 << listener->getInventoryType() & mFilterOps.mFilterTypes || listener->getInventoryType() == LLInventoryType::IT_NONE)
					&& (mFilterSubString.size() == 0 || mSubStringMatchOffset != LLString::npos)
					&& ((listener->getPermissionMask() & mFilterOps.mPermissions) == mFilterOps.mPermissions)
					&& (listener->getCreationDate() >= earliest && listener->getCreationDate() <= mFilterOps.mMaxDate);
	return passed;
}

const LLString LLInventoryFilter::getFilterSubString(BOOL trim)
{
	return mFilterSubString;
}

std::string::size_type LLInventoryFilter::getStringMatchOffset() const
{
	return mSubStringMatchOffset;
}

// has user modified default filter params?
BOOL LLInventoryFilter::isNotDefault()
{
	return mFilterOps.mFilterTypes != mDefaultFilterOps.mFilterTypes 
		|| mFilterSubString.size() 
		|| mFilterOps.mPermissions != mDefaultFilterOps.mPermissions
		|| mFilterOps.mMinDate != mDefaultFilterOps.mMinDate 
		|| mFilterOps.mMaxDate != mDefaultFilterOps.mMaxDate
		|| mFilterOps.mHoursAgo != mDefaultFilterOps.mHoursAgo;
}

BOOL LLInventoryFilter::isActive()
{
	return mFilterOps.mFilterTypes != 0xffffffff 
		|| mFilterSubString.size() 
		|| mFilterOps.mPermissions != PERM_NONE 
		|| mFilterOps.mMinDate != 0 
		|| mFilterOps.mMaxDate != U32_MAX
		|| mFilterOps.mHoursAgo != 0;
}

BOOL LLInventoryFilter::isModified()
{
	return mModified;
}

BOOL LLInventoryFilter::isModifiedAndClear()
{
	BOOL ret = mModified;
	mModified = FALSE;
	return ret;
}

void LLInventoryFilter::setFilterTypes(U32 types)
{
	if (mFilterOps.mFilterTypes != types)
	{
		// keep current items only if no type bits getting turned off
		BOOL fewer_bits_set = (mFilterOps.mFilterTypes & ~types);
		BOOL more_bits_set = (~mFilterOps.mFilterTypes & types);
	
		mFilterOps.mFilterTypes = types;
		if (more_bits_set && fewer_bits_set)
		{
			// neither less or more restrive, both simultaneously
			// so we need to filter from scratch
			setModified(FILTER_RESTART);
		}
		else if (more_bits_set)
		{
			// target is only one of all requested types so more type bits == less restrictive
			setModified(FILTER_LESS_RESTRICTIVE);
		}
		else if (fewer_bits_set)
		{
			setModified(FILTER_MORE_RESTRICTIVE);
		}

	}
}

void LLInventoryFilter::setFilterSubString(const LLString& string)
{
	if (mFilterSubString != string)
	{
		// hitting BACKSPACE, for example
		BOOL less_restrictive = mFilterSubString.size() >= string.size() && !mFilterSubString.substr(0, string.size()).compare(string);
		// appending new characters
		BOOL more_restrictive = mFilterSubString.size() < string.size() && !string.substr(0, mFilterSubString.size()).compare(mFilterSubString);
		mFilterSubString = string;
		LLString::toUpper(mFilterSubString);
		LLString::trimHead(mFilterSubString);

		if (less_restrictive)
		{
			setModified(FILTER_LESS_RESTRICTIVE);
		}
		else if (more_restrictive)
		{
			setModified(FILTER_MORE_RESTRICTIVE);
		}
		else
		{
			setModified(FILTER_RESTART);
		}
	}
}

void LLInventoryFilter::setFilterPermissions(PermissionMask perms)
{
	if (mFilterOps.mPermissions != perms)
	{
		// keep current items only if no perm bits getting turned off
		BOOL fewer_bits_set = (mFilterOps.mPermissions & ~perms);
		BOOL more_bits_set = (~mFilterOps.mPermissions & perms);
		mFilterOps.mPermissions = perms;

		if (more_bits_set && fewer_bits_set)
		{
			setModified(FILTER_RESTART);
		}
		else if (more_bits_set)
		{
			// target must have all requested permission bits, so more bits == more restrictive
			setModified(FILTER_MORE_RESTRICTIVE);
		}
		else if (fewer_bits_set)
		{
			setModified(FILTER_LESS_RESTRICTIVE);
		}
	}
}

void LLInventoryFilter::setDateRange(U32 min_date, U32 max_date)
{
	mFilterOps.mHoursAgo = 0;
	if (mFilterOps.mMinDate != min_date)
	{
		mFilterOps.mMinDate = min_date;
		setModified();
	}
	if (mFilterOps.mMaxDate != llmax(mFilterOps.mMinDate, max_date))
	{
		mFilterOps.mMaxDate = llmax(mFilterOps.mMinDate, max_date);
		setModified();
	}
}

void LLInventoryFilter::setDateRangeLastLogoff(BOOL sl)
{
	if (sl && !isSinceLogoff())
	{
		setDateRange(mLastLogoff, U32_MAX);
		setModified();
	}
	if (!sl && isSinceLogoff())
	{
		setDateRange(0, U32_MAX);
		setModified();
	}
}

BOOL LLInventoryFilter::isSinceLogoff()
{
	return (mFilterOps.mMinDate == mLastLogoff) && (mFilterOps.mMaxDate == U32_MAX);
}

void LLInventoryFilter::setHoursAgo(U32 hours)
{
	if (mFilterOps.mHoursAgo != hours)
	{
		// *NOTE: need to cache last filter time, in case filter goes stale
		BOOL less_restrictive = (mFilterOps.mMinDate == 0 && mFilterOps.mMaxDate == U32_MAX && hours > mFilterOps.mHoursAgo);
		BOOL more_restrictive = (mFilterOps.mMinDate == 0 && mFilterOps.mMaxDate == U32_MAX && hours <= mFilterOps.mHoursAgo);
		mFilterOps.mHoursAgo = hours;
		mFilterOps.mMinDate = 0;
		mFilterOps.mMaxDate = U32_MAX;
		if (less_restrictive)
		{
			setModified(FILTER_LESS_RESTRICTIVE);
		}
		else if (more_restrictive)
		{
			setModified(FILTER_MORE_RESTRICTIVE);
		}
		else
		{
			setModified(FILTER_RESTART);
		}
	}
}
void LLInventoryFilter::setShowFolderState(EFolderShow state)
{
	if (mFilterOps.mShowFolderState != state)
	{
		mFilterOps.mShowFolderState = state;
		if (state == SHOW_NON_EMPTY_FOLDERS)
		{
			// showing fewer folders than before
			setModified(FILTER_MORE_RESTRICTIVE);
		}
		else if (state == SHOW_ALL_FOLDERS)
		{
			// showing same folders as before and then some
			setModified(FILTER_LESS_RESTRICTIVE);
		}
		else
		{
			setModified();
		}
	}
}

void LLInventoryFilter::setSortOrder(U32 order)
{
	if (mOrder != order)
	{
		mOrder = order;
		setModified();
	}
}

void LLInventoryFilter::markDefault()
{
	mDefaultFilterOps = mFilterOps;
}

void LLInventoryFilter::resetDefault()
{
	mFilterOps = mDefaultFilterOps;
	setModified();
}

void LLInventoryFilter::setModified(EFilterBehavior behavior)
{
	mModified = TRUE;
	mNeedTextRebuild = TRUE;
	mFilterGeneration = mNextFilterGeneration++;
	
	if (mFilterBehavior == FILTER_NONE)
	{
		mFilterBehavior = behavior;
	}
	else if (mFilterBehavior != behavior)
	{
		// trying to do both less restrictive and more restrictive filter
		// basically means restart from scratch
		mFilterBehavior = FILTER_RESTART;
	}

	if (isNotDefault())
	{
		// if not keeping current filter results, update last valid as well
		switch(mFilterBehavior)
		{
		case FILTER_RESTART:
			mMustPassGeneration = mFilterGeneration;
			mMinRequiredGeneration = mFilterGeneration;
			break;
		case FILTER_LESS_RESTRICTIVE:
			mMustPassGeneration = mFilterGeneration;
			break;
		case FILTER_MORE_RESTRICTIVE:
			mMinRequiredGeneration = mFilterGeneration;
			// must have passed either current filter generation (meaningless, as it hasn't been run yet)
			// or some older generation, so keep the value
			mMustPassGeneration = llmin(mMustPassGeneration, mFilterGeneration);
			break;
		default:
			llerrs << "Bad filter behavior specified" << llendl;
		}
	}
	else
	{
		// shortcut disabled filters to show everything immediately
		mMinRequiredGeneration = 0;
		mMustPassGeneration = S32_MAX;
	}
}

BOOL LLInventoryFilter::isFilterWith(LLInventoryType::EType t)
{
	return mFilterOps.mFilterTypes & (0x01 << t);
}

LLString LLInventoryFilter::getFilterText()
{
	if (!mNeedTextRebuild)
	{
		return mFilterText;
	}

	mNeedTextRebuild = FALSE;
	LLString filtered_types;
	LLString not_filtered_types;
	BOOL filtered_by_type = FALSE;
	BOOL filtered_by_all_types = TRUE;
	S32 num_filter_types = 0;
	mFilterText = "";

	if (isFilterWith(LLInventoryType::IT_ANIMATION))
	{
		filtered_types += " Animations,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Animations,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_CALLINGCARD))
	{
		filtered_types += " Calling Cards,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Calling Cards,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_WEARABLE))
	{
		filtered_types += " Clothing,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Clothing,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_GESTURE))
	{
		filtered_types += " Gestures,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Gestures,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_LANDMARK))
	{
		filtered_types += " Landmarks,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Landmarks,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_NOTECARD))
	{
		filtered_types += " Notecards,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Notecards,";
		filtered_by_all_types = FALSE;
	}
	
	if (isFilterWith(LLInventoryType::IT_OBJECT) && isFilterWith(LLInventoryType::IT_ATTACHMENT))
	{
		filtered_types += " Objects,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Objects,";
		filtered_by_all_types = FALSE;
	}
	
	if (isFilterWith(LLInventoryType::IT_LSL))
	{
		filtered_types += " Scripts,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Scripts,";
		filtered_by_all_types = FALSE;
	}
	
	if (isFilterWith(LLInventoryType::IT_SOUND))
	{
		filtered_types += " Sounds,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Sounds,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_TEXTURE))
	{
		filtered_types += " Textures,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Textures,";
		filtered_by_all_types = FALSE;
	}

	if (isFilterWith(LLInventoryType::IT_SNAPSHOT))
	{
		filtered_types += " Snapshots,";
		filtered_by_type = TRUE;
		num_filter_types++;
	}
	else
	{
		not_filtered_types += " Snapshots,";
		filtered_by_all_types = FALSE;
	}

	if (!gInventory.backgroundFetchActive() && filtered_by_type && !filtered_by_all_types)
	{
		mFilterText += " - ";
		if (num_filter_types < 5)
		{
			mFilterText += filtered_types;
		}
		else
		{
			mFilterText += "No ";
			mFilterText += not_filtered_types;
		}
		// remove the ',' at the end
		mFilterText.erase(mFilterText.size() - 1, 1);
	}

	if (isSinceLogoff())
	{
		mFilterText += " - Since Logoff";
	}
	return mFilterText;
}

void LLInventoryFilter::toLLSD(LLSD& data)
{
	data["filter_types"] = (LLSD::Integer)getFilterTypes();
	data["min_date"] = (LLSD::Integer)getMinDate();
	data["max_date"] = (LLSD::Integer)getMaxDate();
	data["hours_ago"] = (LLSD::Integer)getHoursAgo();
	data["show_folder_state"] = (LLSD::Integer)getShowFolderState();
	data["permissions"] = (LLSD::Integer)getFilterPermissions();
	data["substring"] = (LLSD::String)getFilterSubString();
	data["sort_order"] = (LLSD::Integer)getSortOrder();
	data["since_logoff"] = (LLSD::Boolean)isSinceLogoff();
}

void LLInventoryFilter::fromLLSD(LLSD& data)
{
	if(data.has("filter_types"))
	{
		setFilterTypes((U32)data["filter_types"].asInteger());
	}

	if(data.has("min_date") && data.has("max_date"))
	{
		setDateRange((U32)data["min_date"].asInteger(), (U32)data["max_date"].asInteger());
	}

	if(data.has("hours_ago"))
	{
		setHoursAgo((U32)data["hours_ago"].asInteger());
	}

	if(data.has("show_folder_state"))
	{
		setShowFolderState((EFolderShow)data["show_folder_state"].asInteger());
	}

	if(data.has("permissions"))
	{
		setFilterPermissions((PermissionMask)data["permissions"].asInteger());
	}

	if(data.has("substring"))
	{
		setFilterSubString(LLString(data["substring"].asString()));
	}

	if(data.has("sort_order"))
	{
		setSortOrder((U32)data["sort_order"].asInteger());
	}

	if(data.has("since_logoff"))
	{
		setDateRangeLastLogoff((bool)data["since_logoff"].asBoolean());
	}
}
