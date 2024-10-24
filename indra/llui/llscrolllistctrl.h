/**
 * @file llscrolllistctrl.h
 * @brief A scrolling list of items.  This is the one you want to use
 * in UI code.  LLScrollListCell, LLScrollListItem, etc. are utility
 * classes.
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

#ifndef LL_SCROLLLISTCTRL_H
#define LL_SCROLLLISTCTRL_H

#include <vector>
#include <deque>

#include "lluictrl.h"
#include "llctrlselectioninterface.h"
#include "llfontgl.h"
#include "llui.h"
#include "llstring.h"   // LLWString
#include "lleditmenuhandler.h"
#include "llframetimer.h"

#include "llscrollbar.h"
#include "lldate.h"
#include "llscrolllistitem.h"
#include "llscrolllistcolumn.h"
#include "llviewborder.h"

class LLScrollListCell;
class LLTextBox;
class LLContextMenu;

class LLScrollListCtrl : public LLUICtrl, public LLEditMenuHandler,
    public LLCtrlListInterface, public LLCtrlScrollInterface
{
public:
    typedef enum e_selection_type
    {
        ROW, // default
        CELL, // does not support multi-selection
        HEADER, // when pointing to cells in column 0 will highlight whole row, otherwise cell, no multi-select
    } ESelectionType;

    struct SelectionTypeNames : public LLInitParam::TypeValuesHelper<LLScrollListCtrl::ESelectionType, SelectionTypeNames>
    {
        static void declareValues();
    };

    struct Contents : public LLInitParam::Block<Contents>
    {
        Multiple<LLScrollListColumn::Params>    columns;
        Multiple<LLScrollListItem::Params>      rows;

        //Multiple<Contents>                        groups;

        Contents();
    };

    // *TODO: Add callbacks to Params
    typedef boost::function<void (void)> callback_t;

    template<typename T> struct maximum
    {
        typedef T result_type;

        template<typename InputIterator>
        T operator()(InputIterator first, InputIterator last) const
        {
            // If there are no slots to call, just return the
            // default-constructed value
            if(first == last ) return T();
            T max_value = *first++;
            while (first != last) {
                if (max_value < *first)
                max_value = *first;
                ++first;
            }

            return max_value;
        }
    };


    typedef boost::signals2::signal<S32 (S32,const LLScrollListItem*,const LLScrollListItem*),maximum<S32> > sort_signal_t;
    typedef boost::signals2::signal<bool(const LLUUID& user_id)> is_friend_signal_t;

    struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        // behavioral flags
        Optional<bool>  multi_select,
                        commit_on_keyboard_movement,
                        commit_on_selection_change,
                        mouse_wheel_opaque;

        Optional<ESelectionType, SelectionTypeNames> selection_type;

        // display flags
        Optional<bool>  has_border,
                        draw_heading,
                        draw_stripes,
                        background_visible,
                        scroll_bar_bg_visible;

        // layout
        Optional<S32>   column_padding,
                        row_padding,
                        page_lines,
                        heading_height;

        // sort and search behavior
        Optional<S32>   search_column,
                        sort_column;
        Optional<bool>  sort_ascending,
                        can_sort; // whether user is allowed to sort

        // colors
        Optional<LLUIColor> fg_unselected_color,
                            fg_selected_color,
                            bg_selected_color,
                            fg_disable_color,
                            bg_writeable_color,
                            bg_readonly_color,
                            bg_stripe_color,
                            hovered_color,
                            highlighted_color,
                            scroll_bar_bg_color;

        Optional<Contents> contents;

        Optional<LLViewBorder::Params> border;

        Params();
    };

protected:
    friend class LLUICtrlFactory;

    LLScrollListCtrl(const Params&);

public:
    virtual ~LLScrollListCtrl();

    S32             isEmpty() const;

    void            deleteAllItems() { clearRows(); }

    // Sets an array of column descriptors
    void            sortByColumnIndex(U32 column, bool ascending);

    // LLCtrlListInterface functions
    virtual S32  getItemCount() const;
    // Adds a single column descriptor: ["name" : string, "label" : string, "width" : integer, "relwidth" : integer ]
    virtual void addColumn(const LLScrollListColumn::Params& column, EAddPosition pos = ADD_BOTTOM);
    virtual void addColumn(const LLSD& column, EAddPosition pos = ADD_BOTTOM);
    virtual void clearColumns();
    virtual void setColumnLabel(const std::string& column, const std::string& label);
    virtual bool    preProcessChildNode(LLXMLNodePtr child);
    virtual LLScrollListColumn* getColumn(S32 index);
    virtual LLScrollListColumn* getColumn(const std::string& name);
    virtual S32 getNumColumns() const { return static_cast<S32>(mColumnsIndexed.size()); }

    // Adds a single element, from an array of:
    // "columns" => [ "column" => column name, "value" => value, "type" => type, "font" => font, "font-style" => style ], "id" => uuid
    // Creates missing columns automatically.
    virtual LLScrollListItem* addElement(const LLSD& element, EAddPosition pos = ADD_BOTTOM, void* userdata = NULL);
    virtual LLScrollListItem* addRow(LLScrollListItem *new_item, const LLScrollListItem::Params& value, EAddPosition pos = ADD_BOTTOM);
    virtual LLScrollListItem* addRow(const LLScrollListItem::Params& value, EAddPosition pos = ADD_BOTTOM);
    // Simple add element. Takes a single array of:
    // [ "value" => value, "font" => font, "font-style" => style ]
    virtual void clearRows(); // clears all elements
    virtual void sortByColumn(const std::string& name, bool ascending);

    // These functions take and return an array of arrays of elements, as above
    virtual void    setValue(const LLSD& value );
    virtual LLSD    getValue() const;

    LLCtrlSelectionInterface*   getSelectionInterface() { return (LLCtrlSelectionInterface*)this; }
    LLCtrlListInterface*        getListInterface()      { return (LLCtrlListInterface*)this; }
    LLCtrlScrollInterface*      getScrollInterface()    { return (LLCtrlScrollInterface*)this; }

    // DEPRECATED: Use setSelectedByValue() below.
    bool            setCurrentByID( const LLUUID& id )  { return selectByID(id); }
    virtual LLUUID  getCurrentID() const                { return getStringUUIDSelectedItem(); }

    bool            operateOnSelection(EOperation op);
    bool            operateOnAll(EOperation op);

    // returns false if unable to set the max count so low
    bool            setMaxItemCount(S32 max_count);

    bool            selectByID( const LLUUID& id );     // false if item not found

    // Match item by value.asString(), which should work for string, integer, uuid.
    // Returns false if not found.
    bool            setSelectedByValue(const LLSD& value, bool selected);

    bool            isSorted() const { return mSorted; }

    virtual bool    isSelected(const LLSD& value) const;

    bool            hasSelectedItem() const;

    bool            handleClick(S32 x, S32 y, MASK mask);
    bool            selectFirstItem();
    bool            selectNthItem( S32 index );
    bool            selectItemRange( S32 first, S32 last );
    bool            selectItemAt(S32 x, S32 y, MASK mask);

    void            deleteSingleItem( S32 index );
    void            deleteItems(const LLSD& sd);
    void            deleteSelectedItems();
    void            deselectAllItems(bool no_commit_on_change = false); // by default, go ahead and commit on selection change

    void            clearHighlightedItems();

    virtual void    mouseOverHighlightNthItem( S32 index );

    S32             getHighlightedItemInx() const { return mHighlightedItem; }

    void            setDoubleClickCallback( callback_t cb ) { mOnDoubleClickCallback = cb; }
    void            setMaximumSelectCallback( callback_t cb) { mOnMaximumSelectCallback = cb; }
    void            setSortChangedCallback( callback_t cb)  { mOnSortChangedCallback = cb; }
    // Convenience function; *TODO: replace with setter above + boost::bind() in calling code
    void            setDoubleClickCallback( boost::function<void (void* userdata)> cb, void* userdata) { mOnDoubleClickCallback = boost::bind(cb, userdata); }

    void            swapWithNext(S32 index);
    void            swapWithPrevious(S32 index);

    void            setCanSelect(bool can_select)       { mCanSelect = can_select; }
    virtual bool    getCanSelect() const                { return mCanSelect; }

    S32             getItemIndex( LLScrollListItem* item ) const;
    S32             getItemIndex( const LLUUID& item_id ) const;

    void            setCommentText( const std::string& comment_text);
    LLScrollListItem* addSeparator(EAddPosition pos);

    // "Simple" interface: use this when you're creating a list that contains only unique strings, only
    // one of which can be selected at a time.
    virtual LLScrollListItem* addSimpleElement(const std::string& value, EAddPosition pos = ADD_BOTTOM, const LLSD& id = LLSD());

    bool            selectItemByLabel(const std::string& item, bool case_sensitive = true, S32 column = 0);       // false if item not found
    bool            selectItemByPrefix(const std::string& target, bool case_sensitive = true, S32 column = -1);
    bool            selectItemByPrefix(const LLWString& target, bool case_sensitive = true, S32 column = -1);
    LLScrollListItem* getItemByLabel(const std::string& item, bool case_sensitive = true, S32 column = 0);
    LLScrollListItem* getItemByIndex(S32 index);
    std::string     getSelectedItemLabel(S32 column = 0) const;
    LLSD            getSelectedValue();

    // If multi select is on, select all element that include substring,
    // otherwise select first match only.
    // If focus is true will scroll to selection.
    // Returns number of results.
    // Note: at the moment search happens in one go and is expensive
    U32         searchItems(const std::string& substring, bool case_sensitive = false, bool focus = true);
    U32         searchItems(const LLWString& substring, bool case_sensitive = false, bool focus = true);

    // DEPRECATED: Use LLSD versions of setCommentText() and getSelectedValue().
    // "StringUUID" interface: use this when you're creating a list that contains non-unique strings each of which
    // has an associated, unique UUID, and only one of which can be selected at a time.
    LLScrollListItem*   addStringUUIDItem(const std::string& item_text, const LLUUID& id, EAddPosition pos = ADD_BOTTOM, bool enabled = true);
    LLUUID              getStringUUIDSelectedItem() const;

    LLScrollListItem*   getFirstSelected() const;
    virtual S32         getFirstSelectedIndex() const;
    std::vector<LLScrollListItem*> getAllSelected() const;
    std::vector<LLSD> getAllSelectedValues() const;
    S32                 getNumSelected() const;
    LLScrollListItem*   getLastSelectedItem() const { return mLastSelected; }

    // iterate over all items
    LLScrollListItem*   getFirstData() const;
    LLScrollListItem*   getLastData() const;
    LLScrollListItem*   getNthData(size_t index) const;
    std::vector<LLScrollListItem*>  getAllData() const;

    LLScrollListItem*   getItem(const LLSD& sd) const;

    void setAllowMultipleSelection(bool mult )  { mAllowMultipleSelection = mult; }

    void setBgWriteableColor(const LLColor4 &c) { mBgWriteableColor = c; }
    void setReadOnlyBgColor(const LLColor4 &c)  { mBgReadOnlyColor = c; }
    void setBgSelectedColor(const LLColor4 &c)  { mBgSelectedColor = c; }
    void setBgStripeColor(const LLColor4& c)    { mBgStripeColor = c; }
    void setFgSelectedColor(const LLColor4 &c)  { mFgSelectedColor = c; }
    void setFgUnselectedColor(const LLColor4 &c){ mFgUnselectedColor = c; }
    void setHoveredColor(const LLColor4 &c)     { mHoveredColor = c; }
    void setHighlightedColor(const LLColor4 &c) { mHighlightedColor = c; }
    void setFgDisableColor(const LLColor4 &c)   { mFgDisabledColor = c; }

    void setBackgroundVisible(bool b)           { mBackgroundVisible = b; }
    void setDrawStripes(bool b)                 { mDrawStripes = b; }
    void setColumnPadding(const S32 c)          { mColumnPadding = c; }
    S32  getColumnPadding() const               { return mColumnPadding; }
    void setRowPadding(const S32 c)             { mColumnPadding = c; }
    S32  getRowPadding() const                  { return mColumnPadding; }
    void setCommitOnKeyboardMovement(bool b)    { mCommitOnKeyboardMovement = b; }
    void setCommitOnSelectionChange(bool b)     { mCommitOnSelectionChange = b; }
    void setAllowKeyboardMovement(bool b)       { mAllowKeyboardMovement = b; }

    void            setMaxSelectable(U32 max_selected) { mMaxSelectable = max_selected; }
    S32             getMaxSelectable() const { return mMaxSelectable; }


    virtual S32     getScrollPos() const;
    virtual void    setScrollPos( S32 pos );
    S32             getSearchColumn();
    void            setSearchColumn(S32 column) { mSearchColumn = column; }
    S32             getColumnIndexFromOffset(S32 x);
    S32             getColumnOffsetFromIndex(S32 index);
    S32             getRowOffsetFromIndex(S32 index);

    void            clearSearchString() { mSearchString.clear(); }

    // support right-click context menus for avatar/group lists
    enum ContextMenuType { MENU_NONE, MENU_AVATAR, MENU_GROUP };
    void setContextMenu(const ContextMenuType &menu) { mContextMenuType = menu; }
    ContextMenuType getContextMenuType() const { return mContextMenuType; }

    // Overridden from LLView
    /*virtual*/ void    draw();
    /*virtual*/ bool    handleMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool    handleMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ bool    handleRightMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool    handleDoubleClick(S32 x, S32 y, MASK mask);
    /*virtual*/ bool    handleHover(S32 x, S32 y, MASK mask);
    /*virtual*/ bool    handleKeyHere(KEY key, MASK mask);
    /*virtual*/ bool    handleUnicodeCharHere(llwchar uni_char);
    /*virtual*/ bool    handleScrollWheel(S32 x, S32 y, S32 clicks);
    /*virtual*/ bool    handleScrollHWheel(S32 x, S32 y, S32 clicks);
    /*virtual*/ bool    handleToolTip(S32 x, S32 y, MASK mask);
    /*virtual*/ void    setEnabled(bool enabled);
    /*virtual*/ void    setFocus( bool b );
    /*virtual*/ void    onFocusReceived();
    /*virtual*/ void    onFocusLost();
    /*virtual*/ void    onMouseLeave(S32 x, S32 y, MASK mask);
    /*virtual*/ void    reshape(S32 width, S32 height, bool called_from_parent = true);

    virtual bool    isDirty() const;
    virtual void    resetDirty();       // Clear dirty state

    virtual void    updateLayout();
    virtual void    fitContents(S32 max_width, S32 max_height);

    virtual LLRect  getRequiredRect();

    LLRect          getItemListRect() { return mItemListRect; }

    /// Returns rect, in local coords, of a given row/column
    LLRect          getCellRect(S32 row_index, S32 column_index);

    // Used "internally" by the scroll bar.
    void            onScrollChange( S32 new_pos, LLScrollbar* src );

    static void     onClickColumn(void *userdata);

    virtual void    updateColumns(bool force_update = false);
    S32             calcMaxContentWidth();
    bool            updateColumnWidths();

    void            setHeadingHeight(S32 heading_height);
    /**
     * Sets  max visible  lines without scroolbar, if this value equals to 0,
     * then display all items.
     */
    void setPageLines(S32 page_lines );

    LLScrollListItem*   hitItem(S32 x,S32 y);
    virtual void        scrollToShowSelected();

    // LLEditMenuHandler functions
    virtual void    copy();
    virtual bool    canCopy() const;
    virtual void    cut();
    virtual bool    canCut() const;
    virtual void    selectAll();
    virtual bool    canSelectAll() const;
    virtual void    deselect();
    virtual bool    canDeselect() const;

    void            setNumDynamicColumns(S32 num) { mNumDynamicWidthColumns = num; }
    void            updateStaticColumnWidth(LLScrollListColumn* col, S32 new_width);
    S32             getTotalStaticColumnWidth() const { return mTotalStaticColumnWidth; }

    std::string     getSortColumnName();
    bool            getSortAscending() { return mSortColumns.empty() ? true : mSortColumns.back().second; }
    bool            hasSortOrder() const;
    void            clearSortOrder();

    void            setAlternateSort() { mAlternateSort = true; }

    void            selectPrevItem(bool extend_selection = false);
    void            selectNextItem(bool extend_selection = false);
    S32             selectMultiple(uuid_vec_t ids);
    // conceptually const, but mutates mItemList
    void            updateSort() const;
    // sorts a list without affecting the permanent sort order (so further list insertions can be unsorted, for example)
    void            sortOnce(S32 column, bool ascending);

    // manually call this whenever editing list items in place to flag need for resorting
    void            setNeedsSort(bool val = true) { mSorted = !val; }
    void            dirtyColumns(); // some operation has potentially affected column layout or ordering

    bool highlightMatchingItems(const std::string& filter_str);

    boost::signals2::connection setSortCallback(sort_signal_t::slot_type cb )
    {
        if (!mSortCallback) mSortCallback = new sort_signal_t();
        return mSortCallback->connect(cb);
    }

    boost::signals2::connection setIsFriendCallback(const is_friend_signal_t::slot_type& cb);


protected:
    // "Full" interface: use this when you're creating a list that has one or more of the following:
    // * contains icons
    // * contains multiple columns
    // * allows multiple selection
    // * has items that are not guarenteed to have unique names
    // * has additional per-item data (e.g. a UUID or void* userdata)
    //
    // To add items using this approach, create new LLScrollListItems and LLScrollListCells.  Add the
    // cells (column entries) to each item, and add the item to the LLScrollListCtrl.
    //
    // The LLScrollListCtrl owns its items and is responsible for deleting them
    // (except in the case that the addItem() call fails, in which case it is up
    // to the caller to delete the item)
    //
    // returns false if item faile to be added to list, does NOT delete 'item'
    bool            addItem( LLScrollListItem* item, EAddPosition pos = ADD_BOTTOM, bool requires_column = true );

    typedef std::deque<LLScrollListItem *> item_list;
    item_list&      getItemList() { return mItemList; }

    void            updateLineHeight();

private:
    void            drawItems();

    void            updateLineHeightInsert(LLScrollListItem* item);
    void            reportInvalidInput();
    bool            isRepeatedChars(const LLWString& string) const;
    void            selectItem(LLScrollListItem* itemp, S32 cell, bool single_select = true);
    void            deselectItem(LLScrollListItem* itemp);
    void            commitIfChanged();
    bool            setSort(S32 column, bool ascending);
    S32             getLinesPerPage();

    static void     showProfile(std::string id, bool is_group);
    static void     sendIM(std::string id);
    static void     addFriend(std::string id);
    static void     removeFriend(std::string id);
    static void     reportAbuse(std::string id, bool is_group);
    static void     showNameDetails(std::string id, bool is_group);
    static void     copyNameToClipboard(std::string id, bool is_group);
    static void     copySLURLToClipboard(std::string id, bool is_group);

    S32             mLineHeight;    // the max height of a single line
    S32             mScrollLines;   // how many lines we've scrolled down
    S32             mPageLines;     // max number of lines is it possible to see on the screen given mRect and mLineHeight
    S32             mHeadingHeight; // the height of the column header buttons, if visible
    U32             mMaxSelectable;
    LLScrollbar*    mScrollbar;
    bool            mAllowMultipleSelection;
    bool            mAllowKeyboardMovement;
    bool            mCommitOnKeyboardMovement;
    bool            mCommitOnSelectionChange;
    bool            mSelectionChanged;
    ESelectionType  mSelectionType;
    bool            mNeedsScroll;
    bool            mMouseWheelOpaque;
    bool            mCanSelect;
    bool            mCanSort;       // Whether user is allowed to sort
    bool            mDisplayColumnHeaders;
    bool            mColumnsDirty;
    bool            mColumnWidthsDirty;

    bool            mAlternateSort;

    mutable item_list   mItemList;

    LLScrollListItem *mLastSelected;

    S32             mMaxItemCount;

    LLRect          mItemListRect;
    S32             mColumnPadding;
    S32             mRowPadding;

    bool            mBackgroundVisible;
    bool            mDrawStripes;

    LLUIColor       mBgWriteableColor;
    LLUIColor       mBgReadOnlyColor;
    LLUIColor       mBgSelectedColor;
    LLUIColor       mBgStripeColor;
    LLUIColor       mFgSelectedColor;
    LLUIColor       mFgUnselectedColor;
    LLUIColor       mFgDisabledColor;
    LLUIColor       mHoveredColor;
    LLUIColor       mHighlightedColor;

    S32             mBorderThickness;
    callback_t      mOnDoubleClickCallback;
    callback_t      mOnMaximumSelectCallback;
    callback_t      mOnSortChangedCallback;

    S32             mHighlightedItem;
    class LLViewBorder* mBorder;
    LLHandle<LLContextMenu> mPopupMenuHandle;

    LLTextBox*      mCommentText = nullptr;

    LLWString       mSearchString;
    LLFrameTimer    mSearchTimer;

    S32             mSearchColumn;
    S32             mNumDynamicWidthColumns;
    S32             mTotalStaticColumnWidth;
    S32             mTotalColumnPadding;

    mutable bool    mSorted;

    typedef std::map<std::string, LLScrollListColumn*> column_map_t;
    column_map_t mColumns;

    bool            mDirty;
    S32             mOriginalSelection;

    ContextMenuType mContextMenuType;

    typedef std::vector<LLScrollListColumn*> ordered_columns_t;
    ordered_columns_t   mColumnsIndexed;

    typedef std::pair<S32, bool> sort_column_t;
    std::vector<sort_column_t>  mSortColumns;

    sort_signal_t*  mSortCallback;

    is_friend_signal_t* mIsFriendSignal;

    friend class LLComboBox;
}; // end class LLScrollListCtrl

#endif  // LL_SCROLLLISTCTRL_H
