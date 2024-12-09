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
#include "linden_common.h"

#include "llflashtimer.h"

#include "linden_common.h"
#include "llfolderviewitem.h"
#include "llfolderview.h"
#include "llfolderviewmodel.h"
#include "llpanel.h"
#include "llcallbacklist.h"
#include "llcriticaldamp.h"
#include "llclipboard.h"
#include "llfocusmgr.h"     // gFocusMgr
#include "lltrans.h"
#include "llwindow.h"

///----------------------------------------------------------------------------
/// Class LLFolderViewItem
///----------------------------------------------------------------------------

static LLDefaultChildRegistry::Register<LLFolderViewItem> r("folder_view_item");

// statics
std::map<U8, LLFontGL*> LLFolderViewItem::sFonts; // map of styles to fonts

LLUIColor LLFolderViewItem::sFgColor;
LLUIColor LLFolderViewItem::sHighlightBgColor;
LLUIColor LLFolderViewItem::sFlashBgColor;
LLUIColor LLFolderViewItem::sFocusOutlineColor;
LLUIColor LLFolderViewItem::sMouseOverColor;
LLUIColor LLFolderViewItem::sFilterBGColor;
LLUIColor LLFolderViewItem::sFilterTextColor;
LLUIColor LLFolderViewItem::sSuffixColor;
LLUIColor LLFolderViewItem::sSearchStatusColor;
S32 LLFolderViewItem::sTopPad = 0;
LLUIImagePtr LLFolderViewItem::sFolderArrowImg;
LLUIImagePtr LLFolderViewItem::sSelectionImg;
LLFontGL* LLFolderViewItem::sSuffixFont = nullptr;

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


const LLFontGL* LLFolderViewItem::getLabelFont()
{
    if (!pLabelFont)
    {
        pLabelFont = getLabelFontForStyle(mLabelStyle);
    }
    return pLabelFont;
}
//static
void LLFolderViewItem::initClass()
{
    const Params& default_params = LLUICtrlFactory::getDefaultParams<LLFolderViewItem>();
    sTopPad = default_params.item_top_pad;
    sFolderArrowImg = default_params.folder_arrow_image;
    sSelectionImg = default_params.selection_image;
    sSuffixFont = getLabelFontForStyle(LLFontGL::NORMAL);

    sFgColor = LLUIColorTable::instance().getColor("MenuItemEnabledColor", DEFAULT_WHITE);
    sHighlightBgColor = LLUIColorTable::instance().getColor("MenuItemHighlightBgColor", DEFAULT_WHITE);
    sFlashBgColor = LLUIColorTable::instance().getColor("MenuItemFlashBgColor", DEFAULT_WHITE);
    sFocusOutlineColor = LLUIColorTable::instance().getColor("InventoryFocusOutlineColor", DEFAULT_WHITE);
    sMouseOverColor = LLUIColorTable::instance().getColor("InventoryMouseOverColor", DEFAULT_WHITE);
    sFilterBGColor = LLUIColorTable::instance().getColor("FilterBackgroundColor", DEFAULT_WHITE);
    sFilterTextColor = LLUIColorTable::instance().getColor("FilterTextColor", DEFAULT_WHITE);
    sSuffixColor = LLUIColorTable::instance().getColor("InventoryItemLinkColor", DEFAULT_WHITE);
    sSearchStatusColor = LLUIColorTable::instance().getColor("InventorySearchStatusColor", DEFAULT_WHITE);
}

//static
void LLFolderViewItem::cleanupClass()
{
    sFonts.clear();
    sFolderArrowImg = nullptr;
    sSelectionImg = nullptr;
    sSuffixFont = nullptr;
}


// NOTE: Optimize this, we call it a *lot* when opening a large inventory
LLFolderViewItem::Params::Params()
:   root(),
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
    single_folder_mode("single_folder_mode", false),
    double_click_override("double_click_override", false),
    arrow_size("arrow_size", 0),
    max_folder_item_overlap("max_folder_item_overlap", 0)
{ }

// Default constructor
LLFolderViewItem::LLFolderViewItem(const LLFolderViewItem::Params& p)
:   LLView(p),
    mLabelWidth(0),
    mLabelWidthDirty(false),
    mSuffixNeedsRefresh(false),
    mLabelPaddingRight(DEFAULT_LABEL_PADDING_RIGHT),
    mParentFolder( NULL ),
    mIsSelected( false ),
    mIsCurSelection( false ),
    mSelectPending(false),
    mIsItemCut(false),
    mCutGeneration(0),
    mLabelStyle( LLFontGL::NORMAL ),
    pLabelFont(nullptr),
    mHasVisibleChildren(false),
    mLocalIndentation(p.folder_indentation),
    mIndentation(0),
    mItemHeight(p.item_height),
    mControlLabelRotation(0.f),
    mDragAndDropTarget(false),
    mLabel(utf8str_to_wstring(p.name)),
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
    mSingleFolderMode(p.single_folder_mode),
    mMaxFolderItemOverlap(p.max_folder_item_overlap),
    mDoubleClickOverride(p.double_click_override)
{
    if (mViewModelItem)
    {
        mViewModelItem->setFolderViewItem(this);
    }
}

// Destroys the object
LLFolderViewItem::~LLFolderViewItem()
{
    mViewModelItem = NULL;
    gFocusMgr.removeKeyboardFocusWithoutCallback(this);
}

bool LLFolderViewItem::postBuild()
{
    LLFolderViewModelItem* vmi = getViewModelItem();
    llassert(vmi); // not supposed to happen, if happens, find out why and fix
    if (vmi)
    {
        // getDisplayName() is expensive (due to internal getLabelSuffix() and name building)
        // it also sets search strings so it requires a filter reset
        mLabel = utf8str_to_wstring(vmi->getDisplayName());
        setToolTip(vmi->getName());

        // Dirty the filter flag of the model from the view (CHUI-849)
        vmi->dirtyFilter();
    }

    // Don't do full refresh on constructor if it is possible to avoid
    // it significantly slows down bulk view creation.
    // Todo: Ideally we need to move getDisplayName() out of constructor as well.
    // Like: make a logic that will let filter update search string,
    // while LLFolderViewItem::arrange() updates visual part
    mSuffixNeedsRefresh = true;
    mLabelWidthDirty = true;
    return true;
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
bool LLFolderViewItem::isDescendantOf( const LLFolderViewFolder* potential_ancestor )
{
    LLFolderViewItem* root = this;
    while( root->mParentFolder )
    {
        if( root->mParentFolder == potential_ancestor )
        {
            return true;
        }
        root = root->mParentFolder;
    }
    return false;
}

LLFolderViewItem* LLFolderViewItem::getNextOpenNode(bool include_children)
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

LLFolderViewItem* LLFolderViewItem::getPreviousOpenNode(bool include_children)
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

bool LLFolderViewItem::passedFilter(S32 filter_generation)
{
    return getViewModelItem()->passedFilter(filter_generation);
}

bool LLFolderViewItem::isPotentiallyVisible(S32 filter_generation)
{
    if (filter_generation < 0)
    {
        filter_generation = getFolderViewModel()->getFilter().getFirstSuccessGeneration();
    }
    LLFolderViewModelItem* model = getViewModelItem();
    bool visible = model->passedFilter(filter_generation);
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

    mLabel = utf8str_to_wstring(vmi.getDisplayName());
    mLabelFontBuffer.reset();
    setToolTip(vmi.getName());
    // icons are slightly expensive to get, can be optimized
    // see LLInventoryIcon::getIcon()
    mIcon = vmi.getIcon();
    mIconOpen = vmi.getIconOpen();
    mIconOverlay = vmi.getIconOverlay();

    if (mRoot->useLabelSuffix())
    {
        // Very Expensive!
        // Can do a number of expensive checks, like checking active motions, wearables or friend list
        mLabelStyle = vmi.getLabelStyle();
        pLabelFont = nullptr; // refresh can be called from a coro, don't use getLabelFontForStyle, coro trips font list tread safety
        mLabelSuffix = utf8str_to_wstring(vmi.getLabelSuffix());
        mSuffixFontBuffer.reset();
    }

    // Dirty the filter flag of the model from the view (CHUI-849)
    vmi.dirtyFilter();

    mLabelWidthDirty = true;
    mSuffixNeedsRefresh = false;
}

void LLFolderViewItem::refreshSuffix()
{
    LLFolderViewModelItem const* vmi = getViewModelItem();

    // icons are slightly expensive to get, can be optimized
    // see LLInventoryIcon::getIcon()
    mIcon = vmi->getIcon();
    mIconOpen = vmi->getIconOpen();
    mIconOverlay = vmi->getIconOverlay();

    if (mRoot->useLabelSuffix())
    {
        // Very Expensive!
        // Can do a number of expensive checks, like checking active motions, wearables or friend list
        mLabelStyle = vmi->getLabelStyle();
        pLabelFont = nullptr;
        mLabelSuffix = utf8str_to_wstring(vmi->getLabelSuffix());
    }

    mLabelWidthDirty = true;
    mSuffixNeedsRefresh = false;
}

// Utility function for LLFolderView
void LLFolderViewItem::arrangeAndSet(bool set_selection,
                                     bool take_keyboard_focus)
{
    LLFolderView* root = getRoot();
    if (getParentFolder())
    {
    getParentFolder()->requestArrange();
    }
    if(set_selection)
    {
        getRoot()->setSelection(this, true, take_keyboard_focus);
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

// addToFolder() returns true if it succeeds. false otherwise
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
        if (mSuffixNeedsRefresh)
        {
            // Expensive. But despite refreshing label,
            // it is purely visual, so it is fine to do at our laisure
            refreshSuffix();
        }
        mLabelWidth = getLabelXPos() + getLabelFontForStyle(mLabelStyle)->getWidth(mLabel.c_str()) + getLabelFontForStyle(LLFontGL::NORMAL)->getWidth(mLabelSuffix.c_str()) + mLabelPaddingRight;
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

S32 LLFolderViewItem::getItemHeight() const
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
bool LLFolderViewItem::setSelection(LLFolderViewItem* selection, bool openitem, bool take_keyboard_focus)
{
    if (selection == this && !mIsSelected)
    {
        selectItem();
    }
    else if (mIsSelected)   // Deselect everything else.
    {
        deselectItem();
    }
    return mIsSelected;
}

bool LLFolderViewItem::changeSelection(LLFolderViewItem* selection, bool selected)
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
        return true;
    }
    return false;
}

void LLFolderViewItem::deselectItem(void)
{
    mIsSelected = false;
}

void LLFolderViewItem::selectItem(void)
{
    if (!mIsSelected)
    {
        mIsSelected = true;
        getViewModelItem()->selectItem();
    }
}

bool LLFolderViewItem::isMovable()
{
    return getViewModelItem()->isItemMovable();
}

bool LLFolderViewItem::isRemovable()
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
//bool LLFolderViewItem::removeRecursively(bool single_item)
bool LLFolderViewItem::remove()
{
    if(!isRemovable())
    {
        return false;
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
bool LLFolderViewItem::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
    if(!mIsSelected)
    {
        getRoot()->setSelection(this, false);
    }
    make_ui_sound("UISndClick");
    return true;
}

bool LLFolderViewItem::handleMouseDown( S32 x, S32 y, MASK mask )
{
    if (LLView::childrenHandleMouseDown(x, y, mask))
    {
        return true;
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
            getRoot()->setSelection(this, false);
        }
        make_ui_sound("UISndClick");
    }
    else
    {
        // If selected, we reserve the decision of deselecting/reselecting to the mouse up moment.
        // This is necessary so we maintain selection consistent when starting a drag.
        mSelectPending = true;
    }

    mDragStartX = x;
    mDragStartY = y;
    return true;
}

bool LLFolderViewItem::handleHover( S32 x, S32 y, MASK mask )
{
    mIsMouseOverTitle = (y > (getRect().getHeight() - mItemHeight));

    if( hasMouseCapture() && isMovable() )
    {
            LLFolderView* root = getRoot();

        if( (x - mDragStartX) * (x - mDragStartX) + (y - mDragStartY) * (y - mDragStartY) > DRAG_N_DROP_DISTANCE_THRESHOLD * DRAG_N_DROP_DISTANCE_THRESHOLD
            && root->getAllowDrag()
            && root->getCurSelectedItem()
            && root->startDrag())
        {
                    // RN: when starting drag and drop, clear out last auto-open
                    root->autoOpenTest(NULL);
                    root->setShowSelectionContext(true);

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

        root->clearHoveredItem();
        return true;
    }
    else
    {
        LLFolderView* pRoot = getRoot();
        pRoot->setHoveredItem(this);
        pRoot->setShowSelectionContext(false);
        getWindow()->setCursor(UI_CURSOR_ARROW);
        // let parent handle this then...
        return false;
    }
}


bool LLFolderViewItem::handleDoubleClick( S32 x, S32 y, MASK mask )
{
    openItem();
    return true;
}

bool LLFolderViewItem::handleMouseUp( S32 x, S32 y, MASK mask )
{
    if (LLView::childrenHandleMouseUp(x, y, mask))
    {
        return true;
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
            getRoot()->setSelection(this, false);
        }
    }

    mSelectPending = false;

    if( hasMouseCapture() )
    {
        if (getRoot())
        {
        getRoot()->setShowSelectionContext(false);
        }
        gFocusMgr.setMouseCapture( NULL );
    }
    return true;
}

void LLFolderViewItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
    mIsMouseOverTitle = false;

    // NOTE: LLViewerWindow::updateUI() calls "enter" before "leave"; if the mouse moved to another item, we can't just outright clear it
    LLFolderView* pRoot = getRoot();
    if (this == pRoot->getHoveredItem())
    {
        pRoot->clearHoveredItem();
    }
}

bool LLFolderViewItem::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
                                         EDragAndDropType cargo_type,
                                         void* cargo_data,
                                         EAcceptance* accept,
                                         std::string& tooltip_msg)
{
    bool handled = false;
    bool accepted = getViewModelItem()->dragOrDrop(mask,drop,cargo_type,cargo_data, tooltip_msg);
        handled = accepted;
        if (accepted)
        {
            mDragAndDropTarget = true;
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

void LLFolderViewItem::drawOpenFolderArrow()
{
    //--------------------------------------------------------------------------------//
    // Draw open folder arrow
    //

    if (hasVisibleChildren() || !isFolderComplete())
    {
        gl_draw_scaled_rotated_image(
            mIndentation, getRect().getHeight() - mArrowSize - mTextPad - sTopPad,
            mArrowSize, mArrowSize, mControlLabelRotation, sFolderArrowImg->getImage(), sFgColor);
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
    static const LLClipboard& clipboard = LLClipboard::instance(); // Make it a 'simpleton'?
    if (mCutGeneration != clipboard.getGeneration())
    {
        mCutGeneration = clipboard.getGeneration();
        mIsItemCut = clipboard.isCutMode()
                     && ((getParentFolder() && getParentFolder()->isFadeItem())
                        || getViewModelItem()->isCutToClipboard());
    }
    return mIsItemCut;
}

void LLFolderViewItem::drawHighlight(bool showContent, bool hasKeyboardFocus,
    const LLUIColor& selectColor, const LLUIColor& flashColor,
    const LLUIColor& focusOutlineColor, const LLUIColor& mouseOverColor)
{
    const S32 focus_top = getRect().getHeight();
    const S32 focus_bottom = getRect().getHeight() - mItemHeight;
    const bool folder_open = (getRect().getHeight() > mItemHeight + 4);
    const S32 FOCUS_LEFT = 1;

    // Determine which background color to use for highlighting
    const LLUIColor& bgColor = isFlashing() ? flashColor : selectColor;

    //--------------------------------------------------------------------------------//
    // Draw highlight for selected items
    // Note: Always render "current" item or flashing item, only render other selected
    // items if mShowSingleSelection is false.
    //
    if (isHighlightAllowed())
    {
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

        // Highlight for selected but not current items
        if (!isHighlightActive() && !isFlashing())
        {
            LLColor4 bg_color = bgColor;
            // do time-based fade of extra objects
            F32 fade_time = getRoot() ? getRoot()->getSelectionFadeElapsedTime() : 0.f;
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
                focusOutlineColor, false);
        }

        if (folder_open)
        {
            gl_rect_2d(FOCUS_LEFT,
                focus_bottom + 1, // overlap with bottom edge of above rect
                getRect().getWidth() - 2,
                0,
                focusOutlineColor, false);
            if (showContent && !isFlashing())
            {
                gl_rect_2d(FOCUS_LEFT,
                    focus_bottom + 1,
                    getRect().getWidth() - 2,
                    0,
                    bgColor, true);
            }
        }
    }
    else if (mIsMouseOverTitle)
    {
        gl_rect_2d(FOCUS_LEFT,
            focus_top,
            getRect().getWidth() - 2,
            focus_bottom,
            mouseOverColor, false);
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
            bgColor, false);
        if (folder_open)
        {
            gl_rect_2d(FOCUS_LEFT,
                focus_bottom + 1, // overlap with bottom edge of above rect
                getRect().getWidth() - 2,
                0,
                bgColor, false);
        }
        mDragAndDropTarget = false;
    }
}

void LLFolderViewItem::drawLabel(const LLFontGL * font, const F32 x, const F32 y, const LLColor4& color, F32 &right_x)
{
    //--------------------------------------------------------------------------------//
    // Draw the actual label text
    //
    mLabelFontBuffer.render(font, mLabel, 0, x, y, color,
        LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
        S32_MAX, getRect().getWidth() - (S32) x - mLabelPaddingRight, &right_x, /*use_ellipses*/true);
}

void LLFolderViewItem::draw()
{
    const bool show_context = (getRoot() ? getRoot()->getShowSelectionContext() : false);
    const bool filled = show_context || (getRoot() ? getRoot()->getParentPanel()->hasFocus() : false); // If we have keyboard focus, draw selection filled

    const LLFontGL* font = getLabelFont();
    S32 line_height = font->getLineHeight();

    getViewModelItem()->update();

    if (!mSingleFolderMode)
    {
        drawOpenFolderArrow();
    }

    drawHighlight(show_context, filled, sHighlightBgColor, sFlashBgColor, sFocusOutlineColor, sMouseOverColor);

    //--------------------------------------------------------------------------------//
    // Draw open icon
    //
    const S32 icon_x = mIndentation + mArrowSize + mTextPad;
    const S32 rect_height = getRect().getHeight();
    if (!mIconOpen.isNull() && (llabs(mControlLabelRotation) > 80)) // For open folders
    {
        mIconOpen->draw(icon_x, rect_height - mIconOpen->getHeight() - sTopPad + 1);
    }
    else if (mIcon)
    {
        mIcon->draw(icon_x, rect_height - mIcon->getHeight() - sTopPad + 1);
    }

    if (mIconOverlay && getRoot()->showItemLinkOverlays())
    {
        mIconOverlay->draw(icon_x, rect_height - mIcon->getHeight() - sTopPad + 1);
    }

    //--------------------------------------------------------------------------------//
    // Exit if no label to draw
    //
    if (mLabel.empty())
    {
        return;
    }

    S32 filter_string_length = mViewModelItem->hasFilterStringMatch() ? (S32)mViewModelItem->getFilterStringSize() : 0;
    F32 right_x  = 0;
    F32 y = (F32)rect_height - line_height - (F32)mTextPad - (F32)sTopPad;
    F32 text_left = (F32)getLabelXPos();
    LLWString combined_string = mLabel + mLabelSuffix;

    S32 filter_offset = static_cast<S32>(mViewModelItem->getFilterStringOffset());
    if (filter_string_length > 0)
    {
        S32 bottom = rect_height - line_height - 3 - sTopPad;
        S32 top = rect_height - sTopPad;
        if(mLabelSuffix.empty() || (font == sSuffixFont))
        {
            S32 left = ll_round(text_left) + font->getWidth(combined_string.c_str(), 0, filter_offset) - 2;
            S32 right = left + font->getWidth(combined_string.c_str(), filter_offset, filter_string_length) + 2;

            LLRect box_rect(left, top, right, bottom);
            sSelectionImg->draw(box_rect, sFilterBGColor);
        }
        else
        {
            S32 label_filter_length = llmin((S32)mLabel.size() - filter_offset, (S32)filter_string_length);
            if(label_filter_length > 0)
            {
                S32 left = (S32)(ll_round(text_left) + font->getWidthF32(mLabel.c_str(), 0, llmin(filter_offset, (S32)mLabel.size()))) - 2;
                S32 right = left + (S32)font->getWidthF32(mLabel.c_str(), filter_offset, label_filter_length) + 2;
                LLRect box_rect(left, top, right, bottom);
                sSelectionImg->draw(box_rect, sFilterBGColor);
            }
            S32 suffix_filter_length = label_filter_length > 0 ? filter_string_length - label_filter_length : filter_string_length;
            if(suffix_filter_length > 0)
            {
                S32 suffix_offset = llmax(0, filter_offset - (S32)mLabel.size());
                S32 left = (S32)(ll_round(text_left) + font->getWidthF32(mLabel.c_str(), 0, static_cast<S32>(mLabel.size())) + sSuffixFont->getWidthF32(mLabelSuffix.c_str(), 0, suffix_offset)) - 2;
                S32 right = left + (S32)sSuffixFont->getWidthF32(mLabelSuffix.c_str(), suffix_offset, suffix_filter_length) + 2;
                LLRect box_rect(left, top, right, bottom);
                sSelectionImg->draw(box_rect, sFilterBGColor);
            }
        }
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
        mSuffixFontBuffer.render(sSuffixFont, mLabelSuffix, 0, right_x, y, isFadeItem() ? color : sSuffixColor.get(),
            LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
            S32_MAX, S32_MAX, &right_x);
    }

    //--------------------------------------------------------------------------------//
    // Highlight string match
    //
    if (filter_string_length > 0)
    {
        if(mLabelSuffix.empty() || (font == sSuffixFont))
        {
            F32 match_string_left = text_left + font->getWidthF32(combined_string.c_str(), 0, filter_offset + filter_string_length) - font->getWidthF32(combined_string.c_str(), filter_offset, filter_string_length);
            F32 yy = (F32)rect_height - line_height - (F32)mTextPad - (F32)sTopPad;
            font->render(combined_string, filter_offset, match_string_left, yy,
                sFilterTextColor, LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
                filter_string_length, S32_MAX, &right_x);
        }
        else
        {
            S32 label_filter_length = llmin((S32)mLabel.size() - filter_offset, (S32)filter_string_length);
            if(label_filter_length > 0)
            {
                F32 match_string_left = text_left + font->getWidthF32(mLabel.c_str(), 0, filter_offset + label_filter_length) - font->getWidthF32(mLabel.c_str(), filter_offset, label_filter_length);
                F32 yy = (F32)rect_height - line_height - (F32)mTextPad - (F32)sTopPad;
                font->render(mLabel, filter_offset, match_string_left, yy,
                    sFilterTextColor, LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
                    label_filter_length, S32_MAX, &right_x);
            }

            S32 suffix_filter_length = label_filter_length > 0 ? filter_string_length - label_filter_length : filter_string_length;
            if(suffix_filter_length > 0)
            {
                S32 suffix_offset = llmax(0, filter_offset - (S32)mLabel.size());
                F32 match_string_left = text_left + font->getWidthF32(mLabel.c_str(), 0, static_cast<S32>(mLabel.size())) + sSuffixFont->getWidthF32(mLabelSuffix.c_str(), 0, suffix_offset + suffix_filter_length) - sSuffixFont->getWidthF32(mLabelSuffix.c_str(), suffix_offset, suffix_filter_length);
                F32 yy = (F32)rect_height - sSuffixFont->getLineHeight() - (F32)mTextPad - (F32)sTopPad;
                sSuffixFont->render(mLabelSuffix, suffix_offset, match_string_left, yy, sFilterTextColor,
                    LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
                    suffix_filter_length, S32_MAX, &right_x);
            }
        }

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
    mIsOpen(false),
    mExpanderHighlighted(false),
    mCurHeight(0.f),
    mTargetHeight(0.f),
    mAutoOpenCountdown(0.f),
    mIsFolderComplete(false), // folder might have children that are not loaded yet.
    mAreChildrenInited(false), // folder might have children that are not built yet.
    mLastArrangeGeneration( -1 ),
    mLastCalculatedWidth(0)
{
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

// addToFolder() returns true if it succeeds. false otherwise
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
    if (mAreChildrenInited)
    {
        getRoot()->getFolderViewModel()->sort(this);
    }

    LL_RECORD_BLOCK_TIME(FTM_ARRANGE);

    // evaluate mHasVisibleChildren
    mHasVisibleChildren = false;
    if (mAreChildrenInited && getViewModelItem()->descendantsPassedFilter())
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
    if (!mIsFolderComplete && mAreChildrenInited)
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
                (*fit)->setVisible(false);
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
                (*iit)->setVisible(false);
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

bool LLFolderViewFolder::needsArrange()
{
    return mLastArrangeGeneration < getRoot()->getArrangeGeneration();
}

bool LLFolderViewFolder::descendantsPassedFilter(S32 filter_generation)
{
    return getViewModelItem()->descendantsPassedFilter(filter_generation);
}

// Passes selection information on to children and record selection
// information if necessary.
bool LLFolderViewFolder::setSelection(LLFolderViewItem* selection, bool openitem,
                                      bool take_keyboard_focus)
{
    bool rv = false;
    if (selection == this)
    {
        if (!isSelected())
        {
            selectItem();
        }
        rv = true;
    }
    else
    {
        if (isSelected())
        {
            deselectItem();
        }
        rv = false;
    }
    bool child_selected = false;

    for (folders_t::iterator iter = mFolders.begin();
        iter != mFolders.end();)
    {
        folders_t::iterator fit = iter++;
        if((*fit)->setSelection(selection, openitem, take_keyboard_focus))
        {
            rv = true;
            child_selected = true;
        }
    }
    for (items_t::iterator iter = mItems.begin();
        iter != mItems.end();)
    {
        items_t::iterator iit = iter++;
        if((*iit)->setSelection(selection, openitem, take_keyboard_focus))
        {
            rv = true;
            child_selected = true;
        }
    }
    if(openitem && child_selected && !mSingleFolderMode)
    {
        setOpenArrangeRecursively(true);
    }
    return rv;
}

// This method is used to change the selection of an item.
// Recursively traverse all children; if 'selection' is 'this' then change
// the select status if necessary.
// Returns true if the selection state of this folder, or of a child, was changed.
bool LLFolderViewFolder::changeSelection(LLFolderViewItem* selection, bool selected)
{
    bool rv = false;
    if(selection == this)
    {
        if (isSelected() != selected)
        {
            rv = true;
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
            rv = true;
        }
    }
    for (items_t::iterator iter = mItems.begin();
        iter != mItems.end();)
    {
        items_t::iterator iit = iter++;
        if((*iit)->changeSelection(selection, selected))
        {
            rv = true;
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
    if (!getRoot()->getAllowMultiSelect())
        return;

    LLFolderViewItem* cur_selected_item = getRoot()->getCurSelectedItem();
    if (cur_selected_item == NULL)
    {
        cur_selected_item = new_selection;
    }


    bool reverse = false;
    LLFolderViewFolder* common_ancestor = getCommonAncestor(cur_selected_item, new_selection, reverse);
    if (!common_ancestor)
        return;

    LLFolderViewItem* last_selected_item_from_cur = cur_selected_item;
    LLFolderViewFolder* cur_folder = cur_selected_item->getParentFolder();

    std::vector<LLFolderViewItem*> items_to_select_forward;

    while (cur_folder != common_ancestor)
    {
        cur_folder->gatherChildRangeExclusive(last_selected_item_from_cur, NULL, reverse, items_to_select_forward);

        last_selected_item_from_cur = cur_folder;
        cur_folder = cur_folder->getParentFolder();
    }

    std::vector<LLFolderViewItem*> items_to_select_reverse;

    LLFolderViewItem* last_selected_item_from_new = new_selection;
    cur_folder = new_selection->getParentFolder();
    while (cur_folder != common_ancestor)
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

    bool selection_reverse = new_selection->isSelected(); //indication that some elements are being deselected

    // array always go from 'will be selected' to ' will be unselected', iterate
    // in opposite direction to simplify identification of 'point of origin' in
    // case it is in the list we are working with
    for (std::vector<LLFolderViewItem*>::reverse_iterator it = items_to_select_forward.rbegin(), end_it = items_to_select_forward.rend();
        it != end_it;
        ++it)
    {
        LLFolderViewItem* item = *it;
        bool selected = item->isSelected();
        if (!selection_reverse && selected)
        {
            // it is our 'point of origin' where we shift/expand from
            // don't deselect it
            selection_reverse = true;
        }
        else
        {
            root->changeSelection(item, !selected);
        }
    }

    if (selection_reverse)
    {
        // at some point we reversed selection, first element should be deselected
        root->changeSelection(last_selected_item_from_cur, false);
    }

    // element we expand to should always be selected
    root->changeSelection(new_selection, true);
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
void LLFolderViewFolder::extractItem( LLFolderViewItem* item, bool deparent_model )
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
    if (deparent_model)
    {
        // in some cases model does not belong to parent view, is shared between views
        getViewModelItem()->removeChild(item->getViewModelItem());
    }
    //because an item is going away regardless of filter status, force rearrange
    requestArrange();
    removeChild(item);
}

bool LLFolderViewFolder::isMovable()
{
    if( !(getViewModelItem()->isItemMovable()) )
    {
            return false;
        }

        for (items_t::iterator iter = mItems.begin();
            iter != mItems.end();)
        {
            items_t::iterator iit = iter++;
            if(!(*iit)->isMovable())
            {
                return false;
            }
        }

        for (folders_t::iterator iter = mFolders.begin();
            iter != mFolders.end();)
        {
            folders_t::iterator fit = iter++;
            if(!(*fit)->isMovable())
            {
                return false;
            }
        }
    return true;
}


bool LLFolderViewFolder::isRemovable()
{
    if( !(getViewModelItem()->isItemRemovable()) )
    {
            return false;
        }

        for (items_t::iterator iter = mItems.begin();
            iter != mItems.end();)
        {
            items_t::iterator iit = iter++;
            if(!(*iit)->isRemovable())
            {
                return false;
            }
        }

        for (folders_t::iterator iter = mFolders.begin();
            iter != mFolders.end();)
        {
            folders_t::iterator fit = iter++;
            if(!(*fit)->isRemovable())
            {
                return false;
            }
        }
    return true;
}

void LLFolderViewFolder::destroyRoot()
{
    delete this;
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
    item->setVisible(false);

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
    folder->setVisible(false);
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
void LLFolderViewFolder::setOpen(bool openitem)
{
    if(mSingleFolderMode)
    {
        // navigateToFolder can destroy this view
        // delay it in case setOpen was called from click or key processing
        doOnIdleOneTime([this]()
                        {
                            getViewModelItem()->navigateToFolder();
                        });
    }
    else
    {
        setOpenArrangeRecursively(openitem);
    }
}

void LLFolderViewFolder::setOpenArrangeRecursively(bool openitem, ERecurseType recurse)
{
    bool was_open = isOpen();
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
            (*fit)->setOpenArrangeRecursively(openitem, RECURSE_DOWN);      /* Flawfinder: ignore */
        }
    }
    if (mParentFolder
        &&  (recurse == RECURSE_UP
            || recurse == RECURSE_UP_DOWN))
    {
        mParentFolder->setOpenArrangeRecursively(openitem, RECURSE_UP);
    }

    if (was_open != isOpen())
    {
        requestArrange();
    }
}

bool LLFolderViewFolder::handleDragAndDropFromChild(MASK mask,
                                                    bool drop,
                                                    EDragAndDropType c_type,
                                                    void* cargo_data,
                                                    EAcceptance* accept,
                                                    std::string& tooltip_msg)
{
    bool accepted = mViewModelItem->dragOrDrop(mask,drop,c_type,cargo_data, tooltip_msg);
    if (accepted)
    {
        mDragAndDropTarget = true;
        *accept = ACCEPT_YES_MULTI;
    }
    else
    {
        *accept = ACCEPT_NO;
    }

    // drag and drop to child item, so clear pending auto-opens
    getRoot()->autoOpenTest(NULL);

    return true;
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
bool LLFolderViewFolder::handleDragAndDrop(S32 x, S32 y, MASK mask,
                                           bool drop,
                                           EDragAndDropType cargo_type,
                                           void* cargo_data,
                                           EAcceptance* accept,
                                           std::string& tooltip_msg)
{
    bool handled = false;

    if (isOpen())
    {
        handled = (childrenHandleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg) != NULL);
    }

    if (!handled)
    {
        handleDragAndDropToThisFolder(mask, drop, cargo_type, cargo_data, accept, tooltip_msg);

        LL_DEBUGS("UserInput") << "dragAndDrop handled by LLFolderViewFolder" << LL_ENDL;
    }

    return true;
}

bool LLFolderViewFolder::handleDragAndDropToThisFolder(MASK mask,
                                                       bool drop,
                                                       EDragAndDropType cargo_type,
                                                       void* cargo_data,
                                                       EAcceptance* accept,
                                                       std::string& tooltip_msg)
{
    if (!mAllowDrop)
    {
        *accept = ACCEPT_NO;
        tooltip_msg = LLTrans::getString("TooltipOutboxCannotDropOnRoot");
        return true;
    }

    bool accepted = getViewModelItem()->dragOrDrop(mask,drop,cargo_type,cargo_data, tooltip_msg);

    if (accepted)
    {
        mDragAndDropTarget = true;
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

    return true;
}


bool LLFolderViewFolder::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
    bool handled = false;

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


bool LLFolderViewFolder::handleHover(S32 x, S32 y, MASK mask)
{
    mIsMouseOverTitle = (y > (getRect().getHeight() - mItemHeight));

    bool handled = LLView::handleHover(x, y, mask);

    if (!handled)
    {
        // this doesn't do child processing
        handled = LLFolderViewItem::handleHover(x, y, mask);
    }

    return handled;
}

bool LLFolderViewFolder::handleMouseDown( S32 x, S32 y, MASK mask )
{
    bool handled = false;
    if( isOpen() )
    {
        handled = childrenHandleMouseDown(x,y,mask) != NULL;
    }
    if( !handled )
    {
        if((mIndentation < x && x < mIndentation + (isCollapsed() ? 0 : mArrowSize) + mTextPad)
           && !mSingleFolderMode)
        {
            toggleOpen();
            handled = true;
        }
        else
        {
            // do normal selection logic
            handled = LLFolderViewItem::handleMouseDown(x, y, mask);
        }
    }

    return handled;
}

bool LLFolderViewFolder::handleDoubleClick( S32 x, S32 y, MASK mask )
{
    bool handled = false;
    if(mSingleFolderMode)
    {
        static LLUICachedControl<bool> double_click_new_window("SingleModeDoubleClickOpenWindow", false);
        if (double_click_new_window)
        {
            getViewModelItem()->navigateToFolder(true);
        }
        else
        {
            // navigating is going to destroy views and change children
            // delay it untill handleDoubleClick processing is complete
            doOnIdleOneTime([this]()
                            {
                                getViewModelItem()->navigateToFolder(false);
                            });
        }
        return true;
    }

    if( isOpen() )
    {
        handled = childrenHandleDoubleClick( x, y, mask ) != NULL;
    }
    if( !handled )
    {
        if(mDoubleClickOverride)
        {
            static LLUICachedControl<U32> double_click_action("MultiModeDoubleClickFolder", false);
            if (double_click_action == 1)
            {
                getViewModelItem()->navigateToFolder(true);
                return true;
            }
            if (double_click_action == 2)
            {
                getViewModelItem()->navigateToFolder(false, true);
                return true;
            }
        }
        if(mIndentation < x && x < mIndentation + (isCollapsed() ? 0 : mArrowSize) + mTextPad)
        {
            // don't select when user double-clicks plus sign
            // so as not to contradict single-click behavior
            toggleOpen();
        }
        else
        {
            getRoot()->setSelection(this, false);
            toggleOpen();
        }
        handled = true;
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

    mExpanderHighlighted = false;
}

// this does prefix traversal, as folders are listed above their contents
LLFolderViewItem* LLFolderViewFolder::getNextFromChild( LLFolderViewItem* item, bool include_children )
{
    bool found_item = false;

    LLFolderViewItem* result = NULL;
    // when not starting from a given item, start at beginning
    if(item == NULL)
    {
        found_item = true;
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
                found_item = true;
                // if we are on downwards traversal
                if (include_children && (*fit)->isOpen())
                {
                    // look for first descendant
                    return (*fit)->getNextFromChild(NULL, true);
                }
                // otherwise advance to next folder
                ++fit;
                include_children = true;
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
                    found_item = true;
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
        llassert(false);
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
        result = mParentFolder->getNextFromChild(this, false);
    }

    return result;
}

// this does postfix traversal, as folders are listed above their contents
LLFolderViewItem* LLFolderViewFolder::getPreviousFromChild( LLFolderViewItem* item, bool include_children )
{
    bool found_item = false;

    LLFolderViewItem* result = NULL;
    // when not starting from a given item, start at end
    if(item == NULL)
    {
        found_item = true;
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
                found_item = true;
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
                    found_item = true;
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
        llassert(false);
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

