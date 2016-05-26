/** 
* @file llfolderviewitem.cpp
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
#include "../newview/llviewerprecompiledheaders.h"

#include "llflashtimer.h"

#include "linden_common.h"
#include "llfolderviewitem.h"
#include "llfolderview.h"
#include "llfolderviewmodel.h"
#include "llpanel.h"
#include "llcriticaldamp.h"
#include "llclipboard.h"
#include "llfocusmgr.h"		// gFocusMgr
#include "lltrans.h"
#include "llwindow.h"

///----------------------------------------------------------------------------
/// Class LLFolderViewItem
///----------------------------------------------------------------------------

static LLDefaultChildRegistry::Register<LLFolderViewItem> r("folder_view_item");

// statics 
std::map<U8, LLFontGL*> LLFolderViewItem::sFonts; // map of styles to fonts

bool LLFolderViewItem::sColorSetInitialized = false;
LLUIColor LLFolderViewItem::sFgColor;
LLUIColor LLFolderViewItem::sHighlightBgColor;
LLUIColor LLFolderViewItem::sFlashBgColor;
LLUIColor LLFolderViewItem::sFocusOutlineColor;
LLUIColor LLFolderViewItem::sMouseOverColor;
LLUIColor LLFolderViewItem::sFilterBGColor;
LLUIColor LLFolderViewItem::sFilterTextColor;
LLUIColor LLFolderViewItem::sSuffixColor;
LLUIColor LLFolderViewItem::sSearchStatusColor;

// only integers can be initialized in header
const F32 LLFolderViewItem::FOLDER_CLOSE_TIME_CONSTANT = 0.02f;
const F32 LLFolderViewItem::FOLDER_OPEN_TIME_CONSTANT = 0.03f;

const LLColor4U DEFAULT_WHITE(255, 255, 255);


//static
LLFontGL* LLFolderViewItem::getLabelFontForStyle(U8 style)
{
	LLFontGL* rtn = sFonts[style];
	if (!rtn) // grab label font with this style, lazily
	{
		LLFontDescriptor labelfontdesc("SansSerif", "Small", style);
		rtn = LLFontGL::getFont(labelfontdesc);
		if (!rtn)
		{
			rtn = LLFontGL::getFontDefault();
		}
		sFonts[style] = rtn;
	}
	return rtn;
}

//static
void LLFolderViewItem::initClass()
{
}

//static
void LLFolderViewItem::cleanupClass()
{
	sFonts.clear();
}


// NOTE: Optimize this, we call it a *lot* when opening a large inventory
LLFolderViewItem::Params::Params()
:	root(),
	listener(),
	folder_arrow_image("folder_arrow_image"),
	folder_indentation("folder_indentation"),
	selection_image("selection_image"),
	item_height("item_height"),
	item_top_pad("item_top_pad"),
	creation_date(),
    allow_wear("allow_wear", true),
    allow_drop("allow_drop", true),
	font_color("font_color"),
	font_highlight_color("font_highlight_color"),
    left_pad("left_pad", 0),
    icon_pad("icon_pad", 0),
    icon_width("icon_width", 0),
    text_pad("text_pad", 0),
    text_pad_right("text_pad_right", 0),
    arrow_size("arrow_size", 0),
    max_folder_item_overlap("max_folder_item_overlap", 0)
{ }

// Default constructor
LLFolderViewItem::LLFolderViewItem(const LLFolderViewItem::Params& p)
:	LLView(p),
	mLabelWidth(0),
	mLabelWidthDirty(false),
    mLabelPaddingRight(DEFAULT_LABEL_PADDING_RIGHT),
	mParentFolder( NULL ),
	mIsSelected( FALSE ),
	mIsCurSelection( FALSE ),
	mSelectPending(FALSE),
	mIsItemCut(false),
	mCutGeneration(0),
	mLabelStyle( LLFontGL::NORMAL ),
	mHasVisibleChildren(FALSE),
	mIsFolderComplete(true),
    mLocalIndentation(p.folder_indentation),
	mIndentation(0),
	mItemHeight(p.item_height),
	mControlLabelRotation(0.f),
	mDragAndDropTarget(FALSE),
	mLabel(p.name),
	mRoot(p.root),
	mViewModelItem(p.listener),
	mIsMouseOverTitle(false),
	mAllowWear(p.allow_wear),
    mAllowDrop(p.allow_drop),
	mFontColor(p.font_color),
	mFontHighlightColor(p.font_highlight_color),
    mLeftPad(p.left_pad),
    mIconPad(p.icon_pad),
    mIconWidth(p.icon_width),
    mTextPad(p.text_pad),
    mTextPadRight(p.text_pad_right),
    mArrowSize(p.arrow_size),
    mMaxFolderItemOverlap(p.max_folder_item_overlap)
{
	if (!sColorSetInitialized)
	{
		sFgColor = LLUIColorTable::instance().getColor("MenuItemEnabledColor", DEFAULT_WHITE);
		sHighlightBgColor = LLUIColorTable::instance().getColor("MenuItemHighlightBgColor", DEFAULT_WHITE);
		sFlashBgColor = LLUIColorTable::instance().getColor("MenuItemFlashBgColor", DEFAULT_WHITE);
		sFocusOutlineColor = LLUIColorTable::instance().getColor("InventoryFocusOutlineColor", DEFAULT_WHITE);
		sMouseOverColor = LLUIColorTable::instance().getColor("InventoryMouseOverColor", DEFAULT_WHITE);
		sFilterBGColor = LLUIColorTable::instance().getColor("FilterBackgroundColor", DEFAULT_WHITE);
		sFilterTextColor = LLUIColorTable::instance().getColor("FilterTextColor", DEFAULT_WHITE);
		sSuffixColor = LLUIColorTable::instance().getColor("InventoryItemColor", DEFAULT_WHITE);
		sSearchStatusColor = LLUIColorTable::instance().getColor("InventorySearchStatusColor", DEFAULT_WHITE);
		sColorSetInitialized = true;
	}

	if (mViewModelItem)
	{
		mViewModelItem->setFolderViewItem(this);
	}
}

// Destroys the object
LLFolderViewItem::~LLFolderViewItem()
{
	mViewModelItem = NULL;
}

BOOL LLFolderViewItem::postBuild()
{
	refresh();
	return TRUE;
}


LLFolderView* LLFolderViewItem::getRoot()
{
	return mRoot;
}

const LLFolderView* LLFolderViewItem::getRoot() const
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

	// Skip over items that are invisible or are hidden from the UI.
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

BOOL LLFolderViewItem::passedFilter(S32 filter_generation) 
{
	return getViewModelItem()->passedFilter(filter_generation);
}

BOOL LLFolderViewItem::isPotentiallyVisible(S32 filter_generation)
{
	if (filter_generation < 0)
	{
		filter_generation = getFolderViewModel()->getFilter().getFirstSuccessGeneration();
	}
	LLFolderViewModelItem* model = getViewModelItem();
	BOOL visible = model->passedFilter(filter_generation);
	if (model->getMarkedDirtyGeneration() >= filter_generation)
	{
		// unsure visibility state
		// retaining previous visibility until item is updated or filter generation changes
		visible |= getVisible();
	}
	return visible;
}

void LLFolderViewItem::refresh()
{
	LLFolderViewModelItem& vmi = *getViewModelItem();

	mLabel = vmi.getDisplayName();

	setToolTip(vmi.getName());
	mIcon = vmi.getIcon();
	mIconOpen = vmi.getIconOpen();
	mIconOverlay = vmi.getIconOverlay();

	if (mRoot->useLabelSuffix())
	{
		mLabelStyle = vmi.getLabelStyle();
		mLabelSuffix = vmi.getLabelSuffix();
	}

	mLabelWidthDirty = true;
    // Dirty the filter flag of the model from the view (CHUI-849)
	vmi.dirtyFilter();
}

// Utility function for LLFolderView
void LLFolderViewItem::arrangeAndSet(BOOL set_selection,
									 BOOL take_keyboard_focus)
{
	LLFolderView* root = getRoot();
	if (getParentFolder())
	{
	getParentFolder()->requestArrange();
	}
	if(set_selection)
	{
		getRoot()->setSelection(this, TRUE, take_keyboard_focus);
		if(root)
		{
			root->scrollToShowSelection();
		}
	}		
}


std::set<LLFolderViewItem*> LLFolderViewItem::getSelectionList() const
{
	std::set<LLFolderViewItem*> selection;
	return selection;
}

// addToFolder() returns TRUE if it succeeds. FALSE otherwise
void LLFolderViewItem::addToFolder(LLFolderViewFolder* folder)
{
	folder->addItem(this); 

	// Compute indentation since parent folder changed
	mIndentation = (getParentFolder())
		? getParentFolder()->getIndentation() + mLocalIndentation
		: 0; 
}


// Finds width and height of this object and its children.  Also
// makes sure that this view and its children are the right size.
S32 LLFolderViewItem::arrange( S32* width, S32* height )
{
	// Only indent deeper items in hierarchy
	mIndentation = (getParentFolder())
		? getParentFolder()->getIndentation() + mLocalIndentation
		: 0;
	if (mLabelWidthDirty)
	{
		mLabelWidth = getLabelXPos() + getLabelFontForStyle(mLabelStyle)->getWidth(mLabel) + getLabelFontForStyle(mLabelStyle)->getWidth(mLabelSuffix) + mLabelPaddingRight; 
		mLabelWidthDirty = false;
	}

	*width = llmax(*width, mLabelWidth); 

	// determine if we need to use ellipses to avoid horizontal scroll. EXT-719
	bool use_ellipses = getRoot()->getUseEllipses();
	if (use_ellipses)
	{
		// limit to set rect to avoid horizontal scrollbar
		*width = llmin(*width, getRoot()->getRect().getWidth());
	}
	*height = getItemHeight();
	return *height;
}

S32 LLFolderViewItem::getItemHeight()
{
	return mItemHeight;
}

S32 LLFolderViewItem::getLabelXPos()
{
    return getIndentation() + mArrowSize + mTextPad + mIconWidth + mIconPad;
}

S32 LLFolderViewItem::getIconPad()
{
    return mIconPad;
}

S32 LLFolderViewItem::getTextPad()
{
    return mTextPad;
}

// *TODO: This can be optimized a lot by simply recording that it is
// selected in the appropriate places, and assuming that set selection
// means 'deselect' for a leaf item. Do this optimization after
// multiple selection is implemented to make sure it all plays nice
// together.
BOOL LLFolderViewItem::setSelection(LLFolderViewItem* selection, BOOL openitem, BOOL take_keyboard_focus)
{
	if (selection == this && !mIsSelected)
	{
		selectItem();
	}
	else if (mIsSelected)	// Deselect everything else.
	{
		deselectItem();
	}
	return mIsSelected;
}

BOOL LLFolderViewItem::changeSelection(LLFolderViewItem* selection, BOOL selected)
{
	if (selection == this)
	{
		if (mIsSelected)
		{
			deselectItem();
		}
		else
		{
			selectItem();
		}
		return TRUE;
	}
	return FALSE;
}

void LLFolderViewItem::deselectItem(void)
{
	mIsSelected = FALSE;
}

void LLFolderViewItem::selectItem(void)
{
	if (mIsSelected == FALSE)
	{
		mIsSelected = TRUE;
		getViewModelItem()->selectItem();
	}
}

BOOL LLFolderViewItem::isMovable()
{
	return getViewModelItem()->isItemMovable();
}

BOOL LLFolderViewItem::isRemovable()
{
	return getViewModelItem()->isItemRemovable();
}

void LLFolderViewItem::destroyView()
{
	getRoot()->removeFromSelectionList(this);

	if (mParentFolder)
	{
		// removeView deletes me
		mParentFolder->extractItem(this);
	}
	delete this;
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
	return getViewModelItem()->removeItem();
}

// Build an appropriate context menu for the item.
void LLFolderViewItem::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	getViewModelItem()->buildContextMenu(menu, flags);
}

void LLFolderViewItem::openItem( void )
{
	if (mAllowWear || !getViewModelItem()->isItemWearable())
	{
		getViewModelItem()->openItem();
	}
}

void LLFolderViewItem::rename(const std::string& new_name)
{
	if( !new_name.empty() )
	{
		getViewModelItem()->renameItem(new_name);
	}
}

const std::string& LLFolderViewItem::getName( void ) const
{
	static const std::string noName("");
	return getViewModelItem() ? getViewModelItem()->getName() : noName;
}

// LLView functionality
BOOL LLFolderViewItem::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
	if(!mIsSelected)
	{
		getRoot()->setSelection(this, FALSE);
	}
	make_ui_sound("UISndClick");
	return TRUE;
}

BOOL LLFolderViewItem::handleMouseDown( S32 x, S32 y, MASK mask )
{
	if (LLView::childrenHandleMouseDown(x, y, mask))
	{
		return TRUE;
	}
	
	// No handler needed for focus lost since this class has no
	// state that depends on it.
	gFocusMgr.setMouseCapture( this );

	if (!mIsSelected)
	{
		if(mask & MASK_CONTROL)
		{
			getRoot()->changeSelection(this, !mIsSelected);
		}
		else if (mask & MASK_SHIFT)
		{
			getParentFolder()->extendSelectionTo(this);
		}
		else
		{
			getRoot()->setSelection(this, FALSE);
		}
		make_ui_sound("UISndClick");
	}
	else
	{
		// If selected, we reserve the decision of deselecting/reselecting to the mouse up moment.
		// This is necessary so we maintain selection consistent when starting a drag.
		mSelectPending = TRUE;
	}

	mDragStartX = x;
	mDragStartY = y;
	return TRUE;
}

BOOL LLFolderViewItem::handleHover( S32 x, S32 y, MASK mask )
{
	static LLCachedControl<S32> drag_and_drop_threshold(*LLUI::sSettingGroups["config"],"DragAndDropDistanceThreshold", 3);

	mIsMouseOverTitle = (y > (getRect().getHeight() - mItemHeight));

	if( hasMouseCapture() && isMovable() )
	{
			LLFolderView* root = getRoot();

		if( (x - mDragStartX) * (x - mDragStartX) + (y - mDragStartY) * (y - mDragStartY) > drag_and_drop_threshold() * drag_and_drop_threshold() 
			&& root->getCurSelectedItem()
			&& root->startDrag())
		{
					// RN: when starting drag and drop, clear out last auto-open
					root->autoOpenTest(NULL);
					root->setShowSelectionContext(TRUE);

					// Release keyboard focus, so that if stuff is dropped into the
					// world, pressing the delete key won't blow away the inventory
					// item.
					gFocusMgr.setKeyboardFocus(NULL);

			getWindow()->setCursor(UI_CURSOR_ARROW);
		}
		else if (x != mDragStartX || y != mDragStartY)
		{
			getWindow()->setCursor(UI_CURSOR_NOLOCKED);
		}

		return TRUE;
	}
	else
	{
		getRoot()->setShowSelectionContext(FALSE);
		getWindow()->setCursor(UI_CURSOR_ARROW);
		// let parent handle this then...
		return FALSE;
	}
}


BOOL LLFolderViewItem::handleDoubleClick( S32 x, S32 y, MASK mask )
{
	openItem();
	return TRUE;
}

BOOL LLFolderViewItem::handleMouseUp( S32 x, S32 y, MASK mask )
{
	if (LLView::childrenHandleMouseUp(x, y, mask))
	{
		return TRUE;
	}
	
	// if mouse hasn't moved since mouse down...
	if ( pointInView(x, y) && mSelectPending )
	{
		//...then select
		if(mask & MASK_CONTROL)
		{
			getRoot()->changeSelection(this, !mIsSelected);
		}
		else if (mask & MASK_SHIFT)
		{
			getParentFolder()->extendSelectionTo(this);
		}
		else
		{
			getRoot()->setSelection(this, FALSE);
		}
	}

	mSelectPending = FALSE;

	if( hasMouseCapture() )
	{
		if (getRoot())
		{
		getRoot()->setShowSelectionContext(FALSE);
		}
		gFocusMgr.setMouseCapture( NULL );
	}
	return TRUE;
}

void LLFolderViewItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mIsMouseOverTitle = false;
}

BOOL LLFolderViewItem::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										 EDragAndDropType cargo_type,
										 void* cargo_data,
										 EAcceptance* accept,
										 std::string& tooltip_msg)
{
	BOOL handled = FALSE;
	BOOL accepted = getViewModelItem()->dragOrDrop(mask,drop,cargo_type,cargo_data, tooltip_msg);
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
	if(mParentFolder && !handled)
	{
		// store this item to get it in LLFolderBridge::dragItemIntoFolder on drop event.
		mRoot->setDraggingOverItem(this);
		handled = mParentFolder->handleDragAndDropFromChild(mask,drop,cargo_type,cargo_data,accept,tooltip_msg);
		mRoot->setDraggingOverItem(NULL);
	}
	if (handled)
	{
		LL_DEBUGS("UserInput") << "dragAndDrop handled by LLFolderViewItem" << LL_ENDL;
	}

	return handled;
}

void LLFolderViewItem::drawOpenFolderArrow(const Params& default_params, const LLUIColor& fg_color)
{
	//--------------------------------------------------------------------------------//
	// Draw open folder arrow
	//
	const S32 TOP_PAD = default_params.item_top_pad;

	if (hasVisibleChildren() || !isFolderComplete())
	{
		LLUIImage* arrow_image = default_params.folder_arrow_image;
		gl_draw_scaled_rotated_image(
			mIndentation, getRect().getHeight() - mArrowSize - mTextPad - TOP_PAD,
			mArrowSize, mArrowSize, mControlLabelRotation, arrow_image->getImage(), fg_color);
	}
}

/*virtual*/ bool LLFolderViewItem::isHighlightAllowed()
{
	return mIsSelected;
}

/*virtual*/ bool LLFolderViewItem::isHighlightActive()
{
	return mIsCurSelection;
}

/*virtual*/ bool LLFolderViewItem::isFadeItem()
{
    LLClipboard& clipboard = LLClipboard::instance();
    if (mCutGeneration != clipboard.getGeneration())
    {
        mCutGeneration = clipboard.getGeneration();
        mIsItemCut = clipboard.isCutMode()
                     && ((getParentFolder() && getParentFolder()->isFadeItem())
                        || getViewModelItem()->isCutToClipboard());
    }
    return mIsItemCut;
}

void LLFolderViewItem::drawHighlight(const BOOL showContent, const BOOL hasKeyboardFocus, const LLUIColor &selectColor, const LLUIColor &flashColor,  
                                                        const LLUIColor &focusOutlineColor, const LLUIColor &mouseOverColor)
{
    const S32 focus_top = getRect().getHeight();
    const S32 focus_bottom = getRect().getHeight() - mItemHeight;
    const bool folder_open = (getRect().getHeight() > mItemHeight + 4);
    const S32 FOCUS_LEFT = 1;
	
	// Determine which background color to use for highlighting
	LLUIColor bgColor = (isFlashing() ? flashColor : selectColor);

    //--------------------------------------------------------------------------------//
    // Draw highlight for selected items
	// Note: Always render "current" item or flashing item, only render other selected 
	// items if mShowSingleSelection is FALSE.
    //
    if (isHighlightAllowed())	
    							
    {
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		
		// Highlight for selected but not current items
        if (!isHighlightActive() && !isFlashing())
        {
			LLColor4 bg_color = bgColor;
            // do time-based fade of extra objects
            F32 fade_time = (getRoot() ? getRoot()->getSelectionFadeElapsedTime() : 0.0f);
            if (getRoot() && getRoot()->getShowSingleSelection())
            {
                // fading out
                bg_color.mV[VALPHA] = clamp_rescale(fade_time, 0.f, 0.4f, bg_color.mV[VALPHA], 0.f);
            }
            else
            {
                // fading in
                bg_color.mV[VALPHA] = clamp_rescale(fade_time, 0.f, 0.4f, 0.f, bg_color.mV[VALPHA]);
            }
        	gl_rect_2d(FOCUS_LEFT,
					   focus_top, 
					   getRect().getWidth() - 2,
					   focus_bottom,
					   bg_color, hasKeyboardFocus);
        }

		// Highlight for currently selected or flashing item
        if (isHighlightActive())
        {
			// Background
        	gl_rect_2d(FOCUS_LEFT,
                focus_top,
                getRect().getWidth() - 2,
                focus_bottom,
                bgColor, hasKeyboardFocus);
			// Outline
            gl_rect_2d(FOCUS_LEFT, 
                focus_top, 
                getRect().getWidth() - 2,
                focus_bottom,
                focusOutlineColor, FALSE);
        }

        if (folder_open)
        {
            gl_rect_2d(FOCUS_LEFT,
                focus_bottom + 1, // overlap with bottom edge of above rect
                getRect().getWidth() - 2,
                0,
                focusOutlineColor, FALSE);
            if (showContent && !isFlashing())
            {
                gl_rect_2d(FOCUS_LEFT,
                    focus_bottom + 1,
                    getRect().getWidth() - 2,
                    0,
                    bgColor, TRUE);
            }
        }
    }
    else if (mIsMouseOverTitle)
    {
        gl_rect_2d(FOCUS_LEFT,
            focus_top, 
            getRect().getWidth() - 2,
            focus_bottom,
            mouseOverColor, FALSE);
    }

    //--------------------------------------------------------------------------------//
    // Draw DragNDrop highlight
    //
    if (mDragAndDropTarget)
    {
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        gl_rect_2d(FOCUS_LEFT, 
            focus_top, 
            getRect().getWidth() - 2,
            focus_bottom,
            bgColor, FALSE);
        if (folder_open)
        {
            gl_rect_2d(FOCUS_LEFT,
                focus_bottom + 1, // overlap with bottom edge of above rect
                getRect().getWidth() - 2,
                0,
                bgColor, FALSE);
        }
        mDragAndDropTarget = FALSE;
    }
}

void LLFolderViewItem::drawLabel(const LLFontGL * font, const F32 x, const F32 y, const LLColor4& color, F32 &right_x)
{
    //--------------------------------------------------------------------------------//
    // Draw the actual label text
    //
    font->renderUTF8(mLabel, 0, x, y, color,
        LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
        S32_MAX, getRect().getWidth() - (S32) x - mLabelPaddingRight, &right_x, TRUE);
}

void LLFolderViewItem::draw()
{
    const BOOL show_context = (getRoot() ? getRoot()->getShowSelectionContext() : FALSE);
    const BOOL filled = show_context || (getRoot() ? getRoot()->getParentPanel()->hasFocus() : FALSE); // If we have keyboard focus, draw selection filled

	const Params& default_params = LLUICtrlFactory::getDefaultParams<LLFolderViewItem>();
	const S32 TOP_PAD = default_params.item_top_pad;
	
	const LLFontGL* font = getLabelFontForStyle(mLabelStyle);

    getViewModelItem()->update();

    drawOpenFolderArrow(default_params, sFgColor);

    drawHighlight(show_context, filled, sHighlightBgColor, sFlashBgColor, sFocusOutlineColor, sMouseOverColor);

	//--------------------------------------------------------------------------------//
	// Draw open icon
	//
	const S32 icon_x = mIndentation + mArrowSize + mTextPad;
	if (!mIconOpen.isNull() && (llabs(mControlLabelRotation) > 80)) // For open folders
 	{
		mIconOpen->draw(icon_x, getRect().getHeight() - mIconOpen->getHeight() - TOP_PAD + 1);
	}
	else if (mIcon)
	{
 		mIcon->draw(icon_x, getRect().getHeight() - mIcon->getHeight() - TOP_PAD + 1);
 	}

	if (mIconOverlay && getRoot()->showItemLinkOverlays())
	{
		mIconOverlay->draw(icon_x, getRect().getHeight() - mIcon->getHeight() - TOP_PAD + 1);
	}

	//--------------------------------------------------------------------------------//
	// Exit if no label to draw
	//
	if (mLabel.empty())
	{
		return;
	}

	std::string::size_type filter_string_length = mViewModelItem->hasFilterStringMatch() ? mViewModelItem->getFilterStringSize() : 0;
	F32 right_x  = 0;
	F32 y = (F32)getRect().getHeight() - font->getLineHeight() - (F32)mTextPad - (F32)TOP_PAD;
	F32 text_left = (F32)getLabelXPos();
	std::string combined_string = mLabel + mLabelSuffix;

	if (filter_string_length > 0)
	{
		S32 left = ll_round(text_left) + font->getWidth(combined_string, 0, mViewModelItem->getFilterStringOffset()) - 2;
		S32 right = left + font->getWidth(combined_string, mViewModelItem->getFilterStringOffset(), filter_string_length) + 2;
		S32 bottom = llfloor(getRect().getHeight() - font->getLineHeight() - 3 - TOP_PAD);
		S32 top = getRect().getHeight() - TOP_PAD;

		LLUIImage* box_image = default_params.selection_image;
		LLRect box_rect(left, top, right, bottom);
		box_image->draw(box_rect, sFilterBGColor);
    }

    LLColor4 color = (mIsSelected && filled) ? mFontHighlightColor : mFontColor;

    if (isFadeItem())
    {
         // Fade out item color to indicate it's being cut
         color.mV[VALPHA] *= 0.5f;
    }
    drawLabel(font, text_left, y, color, right_x);

	//--------------------------------------------------------------------------------//
	// Draw label suffix
	//
	if (!mLabelSuffix.empty())
	{
		font->renderUTF8( mLabelSuffix, 0, right_x, y, isFadeItem() ? color : (LLColor4)sSuffixColor,
						  LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
						  S32_MAX, S32_MAX, &right_x, FALSE );
	}

	//--------------------------------------------------------------------------------//
	// Highlight string match
	//
    if (filter_string_length > 0)
    {
        F32 match_string_left = text_left + font->getWidthF32(combined_string, 0, mViewModelItem->getFilterStringOffset());
        F32 yy = (F32)getRect().getHeight() - font->getLineHeight() - (F32)mTextPad - (F32)TOP_PAD;
        font->renderUTF8( combined_string, mViewModelItem->getFilterStringOffset(), match_string_left, yy,
            sFilterTextColor, LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
            filter_string_length, S32_MAX, &right_x, FALSE );
    }

    //Gilbert Linden 9-20-2012: Although this should be legal, removing it because it causes the mLabelSuffix rendering to
    //be distorted...oddly. I initially added this in but didn't need it after all. So removing to prevent unnecessary bug.
    //LLView::draw();
}

const LLFolderViewModelInterface* LLFolderViewItem::getFolderViewModel( void ) const
{
	return getRoot()->getFolderViewModel();
}
		
LLFolderViewModelInterface* LLFolderViewItem::getFolderViewModel( void )
{
	return getRoot()->getFolderViewModel();
}

bool LLFolderViewItem::isInSelection() const
{
	return mIsSelected || (mParentFolder && mParentFolder->isInSelection());
}



///----------------------------------------------------------------------------
/// Class LLFolderViewFolder
///----------------------------------------------------------------------------

LLFolderViewFolder::LLFolderViewFolder( const LLFolderViewItem::Params& p ): 
	LLFolderViewItem( p ),
	mIsOpen(FALSE),
	mExpanderHighlighted(FALSE),
	mCurHeight(0.f),
	mTargetHeight(0.f),
	mAutoOpenCountdown(0.f),
	mLastArrangeGeneration( -1 ),
	mLastCalculatedWidth(0)
{
	// folder might have children that are not loaded yet. Mark it as incomplete until chance to check it.
	mIsFolderComplete = false;
}

void LLFolderViewFolder::updateLabelRotation()
{
	if (mAutoOpenCountdown != 0.f)
	{
		mControlLabelRotation = mAutoOpenCountdown * -90.f;
	}
	else if (isOpen())
	{
		mControlLabelRotation = lerp(mControlLabelRotation, -90.f, LLSmoothInterpolation::getInterpolant(0.04f));
	}
	else
	{
		mControlLabelRotation = lerp(mControlLabelRotation, 0.f, LLSmoothInterpolation::getInterpolant(0.025f));
	}
}

// Destroys the object
LLFolderViewFolder::~LLFolderViewFolder( void )
{
	// The LLView base class takes care of object destruction. make sure that we
	// don't have mouse or keyboard focus
	gFocusMgr.releaseFocusIfNeeded( this ); // calls onCommit()
}

// addToFolder() returns TRUE if it succeeds. FALSE otherwise
void LLFolderViewFolder::addToFolder(LLFolderViewFolder* folder)
{
	folder->addFolder(this);

	// Compute indentation since parent folder changed
	mIndentation = (getParentFolder())
		? getParentFolder()->getIndentation() + mLocalIndentation
		: 0; 

	if(isOpen() && folder->isOpen())
	{
		requestArrange();
	}
}

static LLTrace::BlockTimerStatHandle FTM_ARRANGE("Arrange");

// Make everything right and in the right place ready for drawing (CHUI-849)
// * Sort everything correctly if necessary
// * Turn widgets visible/invisible according to their model filtering state
// * Takes animation state into account for opening/closing of folders (this makes widgets visible/invisible)
// * Reposition visible widgets so that they line up correctly with no gap
// * Compute the width and height of the current folder and its children
// * Makes sure that this view and its children are the right size
S32 LLFolderViewFolder::arrange( S32* width, S32* height )
{
	// Sort before laying out contents
    // Note that we sort from the root (CHUI-849)
	getRoot()->getFolderViewModel()->sort(this);

	LL_RECORD_BLOCK_TIME(FTM_ARRANGE);
	
	// evaluate mHasVisibleChildren
	mHasVisibleChildren = false;
	if (getViewModelItem()->descendantsPassedFilter())
	{
		// We have to verify that there's at least one child that's not filtered out
		bool found = false;
		// Try the items first
		for (items_t::iterator iit = mItems.begin(); iit != mItems.end(); ++iit)
		{
			LLFolderViewItem* itemp = (*iit);
			found = itemp->isPotentiallyVisible();
			if (found)
				break;
		}
		if (!found)
		{
			// If no item found, try the folders
			for (folders_t::iterator fit = mFolders.begin(); fit != mFolders.end(); ++fit)
			{
				LLFolderViewFolder* folderp = (*fit);
				found = folderp->isPotentiallyVisible();
				if (found)
					break;
			}
		}

		mHasVisibleChildren = found;
	}
	if (!mIsFolderComplete)
	{
		mIsFolderComplete = getFolderViewModel()->isFolderComplete(this);
	}



	// calculate height as a single item (without any children), and reshapes rectangle to match
	LLFolderViewItem::arrange( width, height );

	// clamp existing animated height so as to never get smaller than a single item
	mCurHeight = llmax((F32)*height, mCurHeight);

	// initialize running height value as height of single item in case we have no children
	F32 running_height = (F32)*height;
	F32 target_height = (F32)*height;

	// are my children visible?
	if (needsArrange())
	{
		// set last arrange generation first, in case children are animating
		// and need to be arranged again
		mLastArrangeGeneration = getRoot()->getArrangeGeneration();
		if (isOpen())
		{
			// Add sizes of children
			S32 parent_item_height = getRect().getHeight();

			for(folders_t::iterator fit = mFolders.begin(); fit != mFolders.end(); ++fit)
			{
				LLFolderViewFolder* folderp = (*fit);
				folderp->setVisible(folderp->isPotentiallyVisible());

				if (folderp->getVisible())
				{
					S32 child_width = *width;
					S32 child_height = 0;
					S32 child_top = parent_item_height - ll_round(running_height);

					target_height += folderp->arrange( &child_width, &child_height );

					running_height += (F32)child_height;
					*width = llmax(*width, child_width);
					folderp->setOrigin( 0, child_top - folderp->getRect().getHeight() );
				}
			}
			for(items_t::iterator iit = mItems.begin();
				iit != mItems.end(); ++iit)
			{
				LLFolderViewItem* itemp = (*iit);
				itemp->setVisible(itemp->isPotentiallyVisible());

				if (itemp->getVisible())
				{
					S32 child_width = *width;
					S32 child_height = 0;
					S32 child_top = parent_item_height - ll_round(running_height);

					target_height += itemp->arrange( &child_width, &child_height );
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
		mCurHeight = lerp(mCurHeight, mTargetHeight, LLSmoothInterpolation::getInterpolant(isOpen() ? FOLDER_OPEN_TIME_CONSTANT : FOLDER_CLOSE_TIME_CONSTANT));

		requestArrange();

		// hide child elements that fall out of current animated height
		for (folders_t::iterator iter = mFolders.begin();
			iter != mFolders.end();)
		{
			folders_t::iterator fit = iter++;
			// number of pixels that bottom of folder label is from top of parent folder
			if (getRect().getHeight() - (*fit)->getRect().mTop + (*fit)->getItemHeight() 
				> ll_round(mCurHeight) + mMaxFolderItemOverlap)
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
			if (getRect().getHeight() - (*iit)->getRect().mBottom
				> ll_round(mCurHeight) + mMaxFolderItemOverlap)
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
	reshape(getRect().getWidth(),ll_round(mCurHeight));

	// pass current height value back to parent
	*height = ll_round(mCurHeight);

	return ll_round(mTargetHeight);
}

BOOL LLFolderViewFolder::needsArrange()
{
	return mLastArrangeGeneration < getRoot()->getArrangeGeneration();
}

// Passes selection information on to children and record selection
// information if necessary.
BOOL LLFolderViewFolder::setSelection(LLFolderViewItem* selection, BOOL openitem,
                                      BOOL take_keyboard_focus)
{
	BOOL rv = FALSE;
	if (selection == this)
	{
		if (!isSelected())
		{
			selectItem();
		}
		rv = TRUE;
	}
	else
	{
		if (isSelected())
		{
			deselectItem();
		}
		rv = FALSE;
	}
	BOOL child_selected = FALSE;

	for (folders_t::iterator iter = mFolders.begin();
		iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		if((*fit)->setSelection(selection, openitem, take_keyboard_focus))
		{
			rv = TRUE;
			child_selected = TRUE;
		}
	}
	for (items_t::iterator iter = mItems.begin();
		iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		if((*iit)->setSelection(selection, openitem, take_keyboard_focus))
		{
			rv = TRUE;
			child_selected = TRUE;
		}
	}
	if(openitem && child_selected)
	{
		setOpenArrangeRecursively(TRUE);
	}
	return rv;
}

// This method is used to change the selection of an item.
// Recursively traverse all children; if 'selection' is 'this' then change
// the select status if necessary.
// Returns TRUE if the selection state of this folder, or of a child, was changed.
BOOL LLFolderViewFolder::changeSelection(LLFolderViewItem* selection, BOOL selected)
{
	BOOL rv = FALSE;
	if(selection == this)
	{
		if (isSelected() != selected)
		{
			rv = TRUE;
			if (selected)
			{
				selectItem();
			}
			else
			{
				deselectItem();
			}
		}
	}

	for (folders_t::iterator iter = mFolders.begin();
		iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		if((*fit)->changeSelection(selection, selected))
		{
			rv = TRUE;
		}
	}
	for (items_t::iterator iter = mItems.begin();
		iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		if((*iit)->changeSelection(selection, selected))
		{
			rv = TRUE;
		}
	}
	return rv;
}

LLFolderViewFolder* LLFolderViewFolder::getCommonAncestor(LLFolderViewItem* item_a, LLFolderViewItem* item_b, bool& reverse)
{
	if (!item_a->getParentFolder() || !item_b->getParentFolder()) return NULL;

	std::deque<LLFolderViewFolder*> item_a_ancestors;

	LLFolderViewFolder* parent = item_a->getParentFolder();
	while(parent)
	{
		item_a_ancestors.push_back(parent);
		parent = parent->getParentFolder();
	}

	std::deque<LLFolderViewFolder*> item_b_ancestors;
	
	parent = item_b->getParentFolder();
	while(parent)
	{
		item_b_ancestors.push_back(parent);
		parent = parent->getParentFolder();
	}

	LLFolderViewFolder* common_ancestor = item_a->getRoot();

	while(item_a_ancestors.size() > item_b_ancestors.size())
	{
		item_a = item_a_ancestors.front();
		item_a_ancestors.pop_front();
	}

	while(item_b_ancestors.size() > item_a_ancestors.size())
	{
		item_b = item_b_ancestors.front();
		item_b_ancestors.pop_front();
	}

	while(item_a_ancestors.size())
	{
		common_ancestor = item_a_ancestors.front();

		if (item_a_ancestors.front() == item_b_ancestors.front())
		{
			// which came first, sibling a or sibling b?
			for (folders_t::iterator it = common_ancestor->mFolders.begin(), end_it = common_ancestor->mFolders.end();
				it != end_it;
				++it)
			{
				LLFolderViewItem* item = *it;

				if (item == item_a)
				{
					reverse = false;
					return common_ancestor;
				}
				if (item == item_b)
				{
					reverse = true;
					return common_ancestor;
				}
			}

			for (items_t::iterator it = common_ancestor->mItems.begin(), end_it = common_ancestor->mItems.end();
				it != end_it;
				++it)
			{
				LLFolderViewItem* item = *it;

				if (item == item_a)
				{
					reverse = false;
					return common_ancestor;
				}
				if (item == item_b)
				{
					reverse = true;
					return common_ancestor;
				}
			}
			break;
		}

		item_a = item_a_ancestors.front();
		item_a_ancestors.pop_front();
		item_b = item_b_ancestors.front();
		item_b_ancestors.pop_front();
	}

	return NULL;
}

void LLFolderViewFolder::gatherChildRangeExclusive(LLFolderViewItem* start, LLFolderViewItem* end, bool reverse, std::vector<LLFolderViewItem*>& items)
{
	bool selecting = start == NULL;
	if (reverse)
	{
		for (items_t::reverse_iterator it = mItems.rbegin(), end_it = mItems.rend();
			it != end_it;
			++it)
		{
			if (*it == end)
			{
				return;
			}
			if (selecting && (*it)->getVisible())
			{
				items.push_back(*it);
			}

			if (*it == start)
			{
				selecting = true;
			}
		}
		for (folders_t::reverse_iterator it = mFolders.rbegin(), end_it = mFolders.rend();
			it != end_it;
			++it)
		{
			if (*it == end)
			{
				return;
			}

			if (selecting && (*it)->getVisible())
			{
				items.push_back(*it);
			}

			if (*it == start)
			{
				selecting = true;
			}
		}
	}
	else
	{
		for (folders_t::iterator it = mFolders.begin(), end_it = mFolders.end();
			it != end_it;
			++it)
		{
			if (*it == end)
			{
				return;
			}

			if (selecting && (*it)->getVisible())
			{
				items.push_back(*it);
			}

			if (*it == start)
			{
				selecting = true;
			}
		}
		for (items_t::iterator it = mItems.begin(), end_it = mItems.end();
			it != end_it;
			++it)
		{
			if (*it == end)
			{
				return;
			}

			if (selecting && (*it)->getVisible())
			{
				items.push_back(*it);
			}

			if (*it == start)
			{
				selecting = true;
			}
		}
	}
}

void LLFolderViewFolder::extendSelectionTo(LLFolderViewItem* new_selection)
{
	if (getRoot()->getAllowMultiSelect() == FALSE) return;

	LLFolderViewItem* cur_selected_item = getRoot()->getCurSelectedItem();
	if (cur_selected_item == NULL)
	{
		cur_selected_item = new_selection;
	}


	bool reverse = false;
	LLFolderViewFolder* common_ancestor = getCommonAncestor(cur_selected_item, new_selection, reverse);
	if (!common_ancestor) return;

	LLFolderViewItem* last_selected_item_from_cur = cur_selected_item;
	LLFolderViewFolder* cur_folder = cur_selected_item->getParentFolder();

	std::vector<LLFolderViewItem*> items_to_select_forward;

	while(cur_folder != common_ancestor)
	{
		cur_folder->gatherChildRangeExclusive(last_selected_item_from_cur, NULL, reverse, items_to_select_forward);
			
		last_selected_item_from_cur = cur_folder;
		cur_folder = cur_folder->getParentFolder();
	}

	std::vector<LLFolderViewItem*> items_to_select_reverse;

	LLFolderViewItem* last_selected_item_from_new = new_selection;
	cur_folder = new_selection->getParentFolder();
	while(cur_folder != common_ancestor)
	{
		cur_folder->gatherChildRangeExclusive(last_selected_item_from_new, NULL, !reverse, items_to_select_reverse);

		last_selected_item_from_new = cur_folder;
		cur_folder = cur_folder->getParentFolder();
	}

	common_ancestor->gatherChildRangeExclusive(last_selected_item_from_cur, last_selected_item_from_new, reverse, items_to_select_forward);

	for (std::vector<LLFolderViewItem*>::reverse_iterator it = items_to_select_reverse.rbegin(), end_it = items_to_select_reverse.rend();
		it != end_it;
		++it)
	{
		items_to_select_forward.push_back(*it);
	}

	LLFolderView* root = getRoot();

	BOOL selection_reverse = new_selection->isSelected(); //indication that some elements are being deselected

	// array always go from 'will be selected' to ' will be unselected', iterate
	// in opposite direction to simplify identification of 'point of origin' in
	// case it is in the list we are working with
	for (std::vector<LLFolderViewItem*>::reverse_iterator it = items_to_select_forward.rbegin(), end_it = items_to_select_forward.rend();
		it != end_it;
		++it)
	{
		LLFolderViewItem* item = *it;
		BOOL selected = item->isSelected();
		if (!selection_reverse && selected)
		{
			// it is our 'point of origin' where we shift/expand from
			// don't deselect it
			selection_reverse = TRUE;
		}
		else
		{
			root->changeSelection(item, !selected);
		}
	}

	if (selection_reverse)
	{
		// at some point we reversed selection, first element should be deselected
		root->changeSelection(last_selected_item_from_cur, FALSE);
	}

	// element we expand to should always be selected
	root->changeSelection(new_selection, TRUE);
}


void LLFolderViewFolder::destroyView()
{
    while (!mItems.empty())
    {
    	LLFolderViewItem *itemp = mItems.back();
        mItems.pop_back();
    	itemp->destroyView(); // LLFolderViewItem::destroyView() removes entry from mItems
    }

	while (!mFolders.empty())
	{
		LLFolderViewFolder *folderp = mFolders.back();
        mFolders.pop_back();
		folderp->destroyView(); // LLFolderVievFolder::destroyView() removes entry from mFolders
	}

	LLFolderViewItem::destroyView();
}

// extractItem() removes the specified item from the folder, but
// doesn't delete it.
void LLFolderViewFolder::extractItem( LLFolderViewItem* item )
{
	if (item->isSelected())
		getRoot()->clearSelection();
	items_t::iterator it = std::find(mItems.begin(), mItems.end(), item);
	if(it == mItems.end())
	{
		// This is an evil downcast. However, it's only doing
		// pointer comparison to find if (which it should be ) the
		// item is in the container, so it's pretty safe.
		LLFolderViewFolder* f = static_cast<LLFolderViewFolder*>(item);
		folders_t::iterator ft;
		ft = std::find(mFolders.begin(), mFolders.end(), f);
		if (ft != mFolders.end())
		{
			mFolders.erase(ft);
		}
	}
	else
	{
		mItems.erase(it);
	}
	//item has been removed, need to update filter
	getViewModelItem()->removeChild(item->getViewModelItem());
	//because an item is going away regardless of filter status, force rearrange
	requestArrange();
	removeChild(item);
}

BOOL LLFolderViewFolder::isMovable()
{
	if( !(getViewModelItem()->isItemMovable()) )
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
	return TRUE;
}


BOOL LLFolderViewFolder::isRemovable()
{
	if( !(getViewModelItem()->isItemRemovable()) )
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
	return TRUE;
}

// this is an internal method used for adding items to folders. 
void LLFolderViewFolder::addItem(LLFolderViewItem* item)
{
	if (item->getParentFolder())
	{
		item->getParentFolder()->extractItem(item);
	}
	item->setParentFolder(this);

	mItems.push_back(item);
	
	item->setRect(LLRect(0, 0, getRect().getWidth(), 0));
	item->setVisible(FALSE);
	
	addChild(item);
	
	// When the model is already hooked into a hierarchy (i.e. has a parent), do not reparent it
	// Note: this happens when models are created before views or shared between views
	if (!item->getViewModelItem()->hasParent())
	{
		getViewModelItem()->addChild(item->getViewModelItem());
	}
}

// this is an internal method used for adding items to folders. 
void LLFolderViewFolder::addFolder(LLFolderViewFolder* folder)
{
	if (folder->mParentFolder)
	{
		folder->mParentFolder->extractItem(folder);
	}
	folder->mParentFolder = this;
	mFolders.push_back(folder);
	folder->setOrigin(0, 0);
	folder->reshape(getRect().getWidth(), 0);
	folder->setVisible(FALSE);
	// rearrange all descendants too, as our indentation level might have changed
	//folder->requestArrange();
	//requestSort();

	addChild(folder);

	// When the model is already hooked into a hierarchy (i.e. has a parent), do not reparent it
	// Note: this happens when models are created before views or shared between views
	if (!folder->getViewModelItem()->hasParent())
	{
		getViewModelItem()->addChild(folder->getViewModelItem());
	}
}

void LLFolderViewFolder::requestArrange()
{
    mLastArrangeGeneration = -1;
    // flag all items up to root
    if (mParentFolder)
    {
        mParentFolder->requestArrange();
    }
}

void LLFolderViewFolder::toggleOpen()
{
	setOpen(!isOpen());
}

// Force a folder open or closed
void LLFolderViewFolder::setOpen(BOOL openitem)
{
	setOpenArrangeRecursively(openitem);
}

void LLFolderViewFolder::setOpenArrangeRecursively(BOOL openitem, ERecurseType recurse)
{
	BOOL was_open = isOpen();
	mIsOpen = openitem;
		if(!was_open && openitem)
		{
			getViewModelItem()->openItem();
			// openItem() will request content, it won't be incomplete
			mIsFolderComplete = true;
		}
		else if(was_open && !openitem)
		{
		getViewModelItem()->closeItem();
	}

	if (recurse == RECURSE_DOWN || recurse == RECURSE_UP_DOWN)
	{
		for (folders_t::iterator iter = mFolders.begin();
			iter != mFolders.end();)
		{
			folders_t::iterator fit = iter++;
			(*fit)->setOpenArrangeRecursively(openitem, RECURSE_DOWN);		/* Flawfinder: ignore */
		}
	}
	if (mParentFolder
		&&	(recurse == RECURSE_UP
			|| recurse == RECURSE_UP_DOWN))
	{
		mParentFolder->setOpenArrangeRecursively(openitem, RECURSE_UP);
	}

	if (was_open != isOpen())
	{
		requestArrange();
	}
}

BOOL LLFolderViewFolder::handleDragAndDropFromChild(MASK mask,
													BOOL drop,
													EDragAndDropType c_type,
													void* cargo_data,
													EAcceptance* accept,
													std::string& tooltip_msg)
{
	BOOL accepted = mViewModelItem->dragOrDrop(mask,drop,c_type,cargo_data, tooltip_msg);
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

void LLFolderViewFolder::openItem( void )
{
	toggleOpen();
}

void LLFolderViewFolder::applyFunctorToChildren(LLFolderViewFunctor& functor)
{
	for (folders_t::iterator iter = mFolders.begin();
		iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		functor.doItem((*fit));
	}
	for (items_t::iterator iter = mItems.begin();
		iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		functor.doItem((*iit));
	}
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

// LLView functionality
BOOL LLFolderViewFolder::handleDragAndDrop(S32 x, S32 y, MASK mask,
										   BOOL drop,
										   EDragAndDropType cargo_type,
										   void* cargo_data,
										   EAcceptance* accept,
										   std::string& tooltip_msg)
{
	BOOL handled = FALSE;

	if (isOpen())
	{
		handled = (childrenHandleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg) != NULL);
	}

	if (!handled)
	{
		handleDragAndDropToThisFolder(mask, drop, cargo_type, cargo_data, accept, tooltip_msg);

		LL_DEBUGS("UserInput") << "dragAndDrop handled by LLFolderViewFolder" << LL_ENDL;
	}

	return TRUE;
}

BOOL LLFolderViewFolder::handleDragAndDropToThisFolder(MASK mask,
													   BOOL drop,
													   EDragAndDropType cargo_type,
													   void* cargo_data,
													   EAcceptance* accept,
													   std::string& tooltip_msg)
{
    if (!mAllowDrop)
    {
		*accept = ACCEPT_NO;
        tooltip_msg = LLTrans::getString("TooltipOutboxCannotDropOnRoot");
        return TRUE;
    }
    
	BOOL accepted = getViewModelItem()->dragOrDrop(mask,drop,cargo_type,cargo_data, tooltip_msg);

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
		getRoot()->autoOpenTest(this);
	}
	
	return TRUE;
}


BOOL LLFolderViewFolder::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;

	if( isOpen() )
	{
		handled = childrenHandleRightMouseDown( x, y, mask ) != NULL;
	}
	if (!handled)
	{
		handled = LLFolderViewItem::handleRightMouseDown( x, y, mask );
	}
	return handled;
}


BOOL LLFolderViewFolder::handleHover(S32 x, S32 y, MASK mask)
{
	mIsMouseOverTitle = (y > (getRect().getHeight() - mItemHeight));

	BOOL handled = LLView::handleHover(x, y, mask);

	if (!handled)
	{
		// this doesn't do child processing
		handled = LLFolderViewItem::handleHover(x, y, mask);
	}

	return handled;
}

BOOL LLFolderViewFolder::handleMouseDown( S32 x, S32 y, MASK mask )
{
	BOOL handled = FALSE;
	if( isOpen() )
	{
		handled = childrenHandleMouseDown(x,y,mask) != NULL;
	}
	if( !handled )
	{
		if(mIndentation < x && x < mIndentation + (isCollapsed() ? 0 : mArrowSize) + mTextPad)
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
	BOOL handled = FALSE;
	if( isOpen() )
	{
		handled = childrenHandleDoubleClick( x, y, mask ) != NULL;
	}
	if( !handled )
	{
		if(mIndentation < x && x < mIndentation + (isCollapsed() ? 0 : mArrowSize) + mTextPad)
		{
			// don't select when user double-clicks plus sign
			// so as not to contradict single-click behavior
			toggleOpen();
		}
		else
		{
			getRoot()->setSelection(this, FALSE);
			toggleOpen();
		}
		handled = TRUE;
	}
	return handled;
}

void LLFolderViewFolder::draw()
{
	updateLabelRotation();

	LLFolderViewItem::draw();

	// draw children if root folder, or any other folder that is open or animating to closed state
	if( getRoot() == this || (isOpen() || mCurHeight != mTargetHeight ))
	{
		LLView::draw();
	}

	mExpanderHighlighted = FALSE;
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
			if ((*fit)->isOpen() && include_children)
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

