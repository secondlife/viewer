/** 
 * @file llscrolllistctrl.h
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

#ifndef LL_SCROLLLISTCTRL_H
#define LL_SCROLLLISTCTRL_H

#include <vector>
#include <deque>

#include "lluictrl.h"
#include "llctrlselectioninterface.h"
#include "lldarray.h"
#include "llfontgl.h"
#include "llui.h"
#include "llstring.h"
#include "llimagegl.h"
#include "lleditmenuhandler.h"
#include "llframetimer.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llscrollbar.h"
#include "llresizebar.h"

/*
 * Represents a cell in a scrollable table.
 *
 * Sub-classes must return height and other properties 
 * though width accessors are implemented by the base class.
 * It is therefore important for sub-class constructors to call
 * setWidth() with realistic values.
 */
class LLScrollListCell
{
public:
	LLScrollListCell(S32 width = 0) : mWidth(width) {};
	virtual ~LLScrollListCell() {};
	virtual void			draw(const LLColor4& color, const LLColor4& highlight_color) const = 0;		// truncate to given width, if possible
	virtual S32				getWidth() const {return mWidth;}
	virtual S32				getContentWidth() const { return 0; }
	virtual S32				getHeight() const = 0;
	virtual const LLSD		getValue() const { return LLStringUtil::null; }
	virtual void			setValue(const LLSD& value) { }
	virtual BOOL			getVisible() const { return TRUE; }
	virtual void			setWidth(S32 width) { mWidth = width; }
	virtual void			highlightText(S32 offset, S32 num_chars) {}
	virtual BOOL			isText() const = 0;
	virtual void			setColor(const LLColor4&) {}
	virtual void			onCommit() {};

	virtual BOOL			handleClick() { return FALSE; }
	virtual	void			setEnabled(BOOL enable) { }

private:
	S32 mWidth;
};

/*
 * Draws a horizontal line.
 */
class LLScrollListSeparator : public LLScrollListCell
{
public:
	LLScrollListSeparator(S32 width);
	virtual ~LLScrollListSeparator() {};
	virtual void			draw(const LLColor4& color, const LLColor4& highlight_color) const;		// truncate to given width, if possible
	virtual S32				getHeight() const;
	virtual BOOL			isText() const { return FALSE; }
};

/*
 * Cell displaying a text label.
 */
class LLScrollListText : public LLScrollListCell
{
public:
	LLScrollListText( const std::string& text, const LLFontGL* font, S32 width = 0, U8 font_style = LLFontGL::NORMAL, LLFontGL::HAlign font_alignment = LLFontGL::LEFT, LLColor4& color = LLColor4::black, BOOL use_color = FALSE, BOOL visible = TRUE);
	/*virtual*/ ~LLScrollListText();

	virtual void    draw(const LLColor4& color, const LLColor4& highlight_color) const;
	virtual S32		getContentWidth() const;
	virtual S32		getHeight() const;
	virtual void	setValue(const LLSD& value);
	virtual const LLSD getValue() const;
	virtual BOOL	getVisible() const;
	virtual void	highlightText(S32 offset, S32 num_chars);

	virtual void	setColor(const LLColor4&);
	virtual BOOL	isText() const;

	void			setText(const LLStringExplicit& text);
	void			setFontStyle(const U8 font_style) { mFontStyle = font_style; }

private:
	LLUIString		mText;
	const LLFontGL*	mFont;
	LLColor4		mColor;
	U8				mUseColor;
	U8				mFontStyle;
	LLFontGL::HAlign mFontAlignment;
	BOOL			mVisible;
	S32				mHighlightCount;
	S32				mHighlightOffset;

	LLPointer<LLUIImage> mRoundedRectImage;

	static U32 sCount;
};

/*
 * Cell displaying an image.
 */
class LLScrollListIcon : public LLScrollListCell
{
public:
	LLScrollListIcon( LLUIImagePtr icon, S32 width = 0);
	LLScrollListIcon(const LLSD& value, S32 width = 0);
	/*virtual*/ ~LLScrollListIcon();
	virtual void	draw(const LLColor4& color, const LLColor4& highlight_color) const;
	virtual S32		getWidth() const;
	virtual S32		getHeight() const			{ return mIcon ? mIcon->getHeight() : 0; }
	virtual const LLSD		getValue() const { return mIcon.isNull() ? LLStringUtil::null : mIcon->getName(); }
	virtual void	setColor(const LLColor4&);
	virtual BOOL	isText()const { return FALSE; }
	virtual void	setValue(const LLSD& value);

private:
	LLUIImagePtr mIcon;
	LLColor4 mColor;
};

/*
 * An interactive cell containing a check box.
 */
class LLScrollListCheck : public LLScrollListCell
{
public:
	LLScrollListCheck( LLCheckBoxCtrl* check_box, S32 width = 0);
	/*virtual*/ ~LLScrollListCheck();
	virtual void	draw(const LLColor4& color, const LLColor4& highlight_color) const;
	virtual S32		getHeight() const			{ return 0; } 
	virtual const LLSD	getValue() const { return mCheckBox->getValue(); }
	virtual void	setValue(const LLSD& value) { mCheckBox->setValue(value); }
	virtual void	onCommit() { mCheckBox->onCommit(); }

	virtual BOOL	handleClick();
	virtual void	setEnabled(BOOL enable)		{ mCheckBox->setEnabled(enable); }

	LLCheckBoxCtrl*	getCheckBox()				{ return mCheckBox; }
	virtual BOOL	isText() const				{ return FALSE; }

private:
	LLCheckBoxCtrl* mCheckBox;
};

/*
 * A simple data class describing a column within a scroll list.
 */
class LLScrollListColumn
{
public:
	// Default constructor
	LLScrollListColumn() : 
		mName(), 
		mSortingColumn(), 
		mSortAscending(TRUE), 
		mLabel(), 
		mWidth(-1), 
		mRelWidth(-1.0), 
		mDynamicWidth(FALSE), 
		mMaxContentWidth(0),
		mIndex(-1), 
		mParentCtrl(NULL), 
		mHeader(NULL), 
		mFontAlignment(LLFontGL::LEFT)
	{ }

	LLScrollListColumn(std::string name, std::string label, S32 width, F32 relwidth) : 
		mName(name), 
		mSortingColumn(name), 
        	mSortAscending(TRUE), 
		mLabel(label), 
		mWidth(width), 
		mRelWidth(relwidth), 
		mDynamicWidth(FALSE), 
		mMaxContentWidth(0),
		mIndex(-1), 
		mParentCtrl(NULL), 
		mHeader(NULL),
		mFontAlignment(LLFontGL::LEFT)
	{ }

	LLScrollListColumn(const LLSD &sd)
	{
		mMaxContentWidth = 0;

		mName = sd.get("name").asString();
		mSortingColumn = mName;
		if (sd.has("sort"))
		{
			mSortingColumn = sd.get("sort").asString();
		}
		mSortAscending = TRUE;
		if (sd.has("sort_ascending"))
		{
			mSortAscending = sd.get("sort_ascending").asBoolean();
		}
		mLabel = sd.get("label").asString();
		if (sd.has("relwidth") && (F32)sd.get("relwidth").asReal() > 0)
		{
			mRelWidth = (F32)sd.get("relwidth").asReal();
			if (mRelWidth < 0) mRelWidth = 0;
			if (mRelWidth > 1) mRelWidth = 1;
			mDynamicWidth = FALSE;
			mWidth = 0;
		}
		else if(sd.has("dynamicwidth") && (BOOL)sd.get("dynamicwidth").asBoolean() == TRUE)
		{
			mDynamicWidth = TRUE;
			mRelWidth = -1;
			mWidth = 0;
		}
		else
		{
			mWidth = sd.get("width").asInteger();
			mDynamicWidth = FALSE;
			mRelWidth = -1;
		}

		if (sd.has("halign"))
		{
			mFontAlignment = (LLFontGL::HAlign)llclamp(sd.get("halign").asInteger(), (S32)LLFontGL::LEFT, (S32)LLFontGL::HCENTER);
		}

		mIndex = -1;
		mParentCtrl = NULL;
		mHeader = NULL;
		mFontAlignment = LLFontGL::LEFT;
	}

	// Public data is fine so long as this remains a simple struct-like data class.
	// If it ever gets any smarter than that, these should all become private
	// with protected or public accessor methods added as needed. -MG
	std::string			mName;
	std::string			mSortingColumn;
	BOOL				mSortAscending;
	std::string			mLabel;
	S32					mWidth;
	F32					mRelWidth;
	BOOL				mDynamicWidth;
	S32					mMaxContentWidth;
	S32					mIndex;
	LLScrollListCtrl*	mParentCtrl;
	class LLColumnHeader*		mHeader;
	LLFontGL::HAlign	mFontAlignment;
};

class LLColumnHeader : public LLComboBox
{
public:
	LLColumnHeader(const std::string& label, const LLRect &rect, LLScrollListColumn* column, const LLFontGL *font = NULL);
	~LLColumnHeader();

	/*virtual*/ void draw();
	/*virtual*/ BOOL handleDoubleClick(S32 x, S32 y, MASK mask);

	/*virtual*/ void showList();
	/*virtual*/ LLView*	findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding);
	/*virtual*/ void userSetShape(const LLRect& new_rect);
	
	void setImage(const std::string &image_name);
	LLScrollListColumn* getColumn() { return mColumn; }
	void setHasResizableElement(BOOL resizable);
	BOOL canResize();
	void enableResizeBar(BOOL enable);
	std::string getLabel() { return mOrigLabel; }

	static void onSelectSort(LLUICtrl* ctrl, void* user_data);
	static void onClick(void* user_data);
	static void onMouseDown(void* user_data);
	static void onHeldDown(void* user_data);

private:
	LLScrollListColumn* mColumn;
	LLResizeBar*		mResizeBar;
	std::string			mOrigLabel;
	LLUIString			mAscendingText;
	LLUIString			mDescendingText;
	BOOL				mShowSortOptions;
	BOOL				mHasResizableElement;
};

class LLScrollListItem
{
public:
	LLScrollListItem( BOOL enabled = TRUE, void* userdata = NULL, const LLUUID& uuid = LLUUID::null )
		: mSelected(FALSE), mEnabled( enabled ), mUserdata( userdata ), mItemValue( uuid ), mColumns() {}
	LLScrollListItem( LLSD item_value, void* userdata = NULL )
		: mSelected(FALSE), mEnabled( TRUE ), mUserdata( userdata ), mItemValue( item_value ), mColumns() {}

	virtual ~LLScrollListItem();

	void	setSelected( BOOL b )			{ mSelected = b; }
	BOOL	getSelected() const				{ return mSelected; }

	void	setEnabled( BOOL b );
	BOOL	getEnabled() const 				{ return mEnabled; }

	void	setUserdata( void* userdata )	{ mUserdata = userdata; }
	void*	getUserdata() const 			{ return mUserdata; }

	LLUUID	getUUID() const					{ return mItemValue.asUUID(); }
	LLSD	getValue() const				{ return mItemValue; }

	// If width = 0, just use the width of the text.  Otherwise override with
	// specified width in pixels.
	void	addColumn( const std::string& text, const LLFontGL* font, S32 width = 0 , U8 font_style = LLFontGL::NORMAL, LLFontGL::HAlign font_alignment = LLFontGL::LEFT, BOOL visible = TRUE)
				{ mColumns.push_back( new LLScrollListText(text, font, width, font_style, font_alignment, LLColor4::black, FALSE, visible) ); }

	void	addColumn( LLUIImagePtr icon, S32 width = 0 )
				{ mColumns.push_back( new LLScrollListIcon(icon, width) ); }

	void	addColumn( LLCheckBoxCtrl* check, S32 width = 0 )
				{ mColumns.push_back( new LLScrollListCheck(check,width) ); }

	void	setNumColumns(S32 columns);

	void	setColumn( S32 column, LLScrollListCell *cell );
	
	S32		getNumColumns() const			{ return mColumns.size(); }

	LLScrollListCell *getColumn(const S32 i) const	{ if (0 <= i && i < (S32)mColumns.size()) { return mColumns[i]; } return NULL; }

	std::string getContentsCSV() const;

	virtual void draw(const LLRect& rect, const LLColor4& fg_color, const LLColor4& bg_color, const LLColor4& highlight_color, S32 column_padding);

private:
	BOOL	mSelected;
	BOOL	mEnabled;
	void*	mUserdata;
	LLSD	mItemValue;
	std::vector<LLScrollListCell *> mColumns;
};

/*
 * A graphical control representing a scrollable table.
 * Cells in the table can be simple text or more complicated things
 * such as icons or even interactive elements like check boxes.
 */
class LLScrollListItemComment : public LLScrollListItem
{
public:
	LLScrollListItemComment(const std::string& comment_string, const LLColor4& color);

	/*virtual*/ void draw(const LLRect& rect, const LLColor4& fg_color, const LLColor4& bg_color, const LLColor4& highlight_color, S32 column_padding);
private:
	LLColor4 mColor;
};

class LLScrollListItemSeparator : public LLScrollListItem
{
public:
	LLScrollListItemSeparator();

	/*virtual*/ void draw(const LLRect& rect, const LLColor4& fg_color, const LLColor4& bg_color, const LLColor4& highlight_color, S32 column_padding);
};

class LLScrollListCtrl : public LLUICtrl, public LLEditMenuHandler, 
	public LLCtrlListInterface, public LLCtrlScrollInterface
{
public:
	LLScrollListCtrl(
		const std::string& name,
		const LLRect& rect,
		void (*commit_callback)(LLUICtrl*, void*),
		void* callback_userdata,
		BOOL allow_multiple_selection,
		BOOL draw_border = TRUE);

	virtual ~LLScrollListCtrl();

	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	void setScrollListParameters(LLXMLNodePtr node);
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	S32				isEmpty() const;

	void			deleteAllItems() { clearRows(); }
	
	// Sets an array of column descriptors
	void 	   		setColumnHeadings(LLSD headings);
	void   			sortByColumnIndex(U32 column, BOOL ascending);
	
	// LLCtrlListInterface functions
	virtual S32  getItemCount() const;
	// Adds a single column descriptor: ["name" : string, "label" : string, "width" : integer, "relwidth" : integer ]
	virtual void addColumn(const LLSD& column, EAddPosition pos = ADD_BOTTOM);
	virtual void clearColumns();
	virtual void setColumnLabel(const std::string& column, const std::string& label);

	virtual LLScrollListColumn* getColumn(S32 index);
	virtual S32 getNumColumns() const { return mColumnsIndexed.size(); }

	// Adds a single element, from an array of:
	// "columns" => [ "column" => column name, "value" => value, "type" => type, "font" => font, "font-style" => style ], "id" => uuid
	// Creates missing columns automatically.
	virtual LLScrollListItem* addElement(const LLSD& value, EAddPosition pos = ADD_BOTTOM, void* userdata = NULL);
	// Simple add element. Takes a single array of:
	// [ "value" => value, "font" => font, "font-style" => style ]
	virtual void clearRows(); // clears all elements
	virtual void sortByColumn(const std::string& name, BOOL ascending);

	// These functions take and return an array of arrays of elements, as above
	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;

	LLCtrlSelectionInterface*	getSelectionInterface()	{ return (LLCtrlSelectionInterface*)this; }
	LLCtrlListInterface*		getListInterface()		{ return (LLCtrlListInterface*)this; }
	LLCtrlScrollInterface*		getScrollInterface()	{ return (LLCtrlScrollInterface*)this; }

	// DEPRECATED: Use setSelectedByValue() below.
	BOOL			setCurrentByID( const LLUUID& id )	{ return selectByID(id); }
	virtual LLUUID	getCurrentID() const				{ return getStringUUIDSelectedItem(); }

	BOOL			operateOnSelection(EOperation op);
	BOOL			operateOnAll(EOperation op);

	// returns FALSE if unable to set the max count so low
	BOOL 			setMaxItemCount(S32 max_count);

	BOOL			selectByID( const LLUUID& id );		// FALSE if item not found

	// Match item by value.asString(), which should work for string, integer, uuid.
	// Returns FALSE if not found.
	BOOL			setSelectedByValue(const LLSD& value, BOOL selected);

	BOOL			isSorted() const { return mSorted; }

	virtual BOOL	isSelected(const LLSD& value) const;

	BOOL			handleClick(S32 x, S32 y, MASK mask);
	BOOL			selectFirstItem();
	BOOL			selectNthItem( S32 index );
	BOOL			selectItemRange( S32 first, S32 last );
	BOOL			selectItemAt(S32 x, S32 y, MASK mask);
	
	void			deleteSingleItem( S32 index );
	void			deleteItems(const LLSD& sd);
	void 			deleteSelectedItems();
	void			deselectAllItems(BOOL no_commit_on_change = FALSE);	// by default, go ahead and commit on selection change

	void			highlightNthItem( S32 index );
	void			setDoubleClickCallback( void (*cb)(void*) ) { mOnDoubleClickCallback = cb; }
	void			setMaximumSelectCallback( void (*cb)(void*) ) { mOnMaximumSelectCallback = cb; }
	void			setSortChangedCallback( void (*cb)(void*) ) { mOnSortChangedCallback = cb; }

	void			swapWithNext(S32 index);
	void			swapWithPrevious(S32 index);

	void			setCanSelect(BOOL can_select)		{ mCanSelect = can_select; }
	virtual BOOL	getCanSelect() const				{ return mCanSelect; }

	S32				getItemIndex( LLScrollListItem* item ) const;
	S32				getItemIndex( const LLUUID& item_id ) const;

	LLScrollListItem* addCommentText( const std::string& comment_text, EAddPosition pos = ADD_BOTTOM);
	LLScrollListItem* addSeparator(EAddPosition pos);

	// "Simple" interface: use this when you're creating a list that contains only unique strings, only
	// one of which can be selected at a time.
	virtual LLScrollListItem* addSimpleElement(const std::string& value, EAddPosition pos = ADD_BOTTOM, const LLSD& id = LLSD());


	BOOL			selectItemByLabel( const std::string& item, BOOL case_sensitive = TRUE );		// FALSE if item not found
	BOOL			selectItemByPrefix(const std::string& target, BOOL case_sensitive = TRUE);
	BOOL			selectItemByPrefix(const LLWString& target, BOOL case_sensitive = TRUE);
	const std::string	getSelectedItemLabel(S32 column = 0) const;
	LLSD			getSelectedValue();

	// DEPRECATED: Use LLSD versions of addCommentText() and getSelectedValue().
	// "StringUUID" interface: use this when you're creating a list that contains non-unique strings each of which
	// has an associated, unique UUID, and only one of which can be selected at a time.
	LLScrollListItem*	addStringUUIDItem(const std::string& item_text, const LLUUID& id, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE, S32 column_width = 0);
	LLUUID				getStringUUIDSelectedItem() const;

	LLScrollListItem*	getFirstSelected() const;
	virtual S32			getFirstSelectedIndex() const;
	std::vector<LLScrollListItem*> getAllSelected() const;
	LLScrollListItem*	getLastSelectedItem() const { return mLastSelected; }

	// iterate over all items
	LLScrollListItem*	getFirstData() const;
	LLScrollListItem*	getLastData() const;
	std::vector<LLScrollListItem*>	getAllData() const;

	LLScrollListItem*	getItem(const LLSD& sd) const;
	
	void setAllowMultipleSelection(BOOL mult )	{ mAllowMultipleSelection = mult; }

	void setBgWriteableColor(const LLColor4 &c)	{ mBgWriteableColor = c; }
	void setReadOnlyBgColor(const LLColor4 &c)	{ mBgReadOnlyColor = c; }
	void setBgSelectedColor(const LLColor4 &c)	{ mBgSelectedColor = c; }
	void setBgStripeColor(const LLColor4& c)	{ mBgStripeColor = c; }
	void setFgSelectedColor(const LLColor4 &c)	{ mFgSelectedColor = c; }
	void setFgUnselectedColor(const LLColor4 &c){ mFgUnselectedColor = c; }
	void setHighlightedColor(const LLColor4 &c)	{ mHighlightedColor = c; }
	void setFgDisableColor(const LLColor4 &c)	{ mFgDisabledColor = c; }

	void setBackgroundVisible(BOOL b)			{ mBackgroundVisible = b; }
	void setDrawStripes(BOOL b)					{ mDrawStripes = b; }
	void setColumnPadding(const S32 c)          { mColumnPadding = c; }
	S32  getColumnPadding()						{ return mColumnPadding; }
	void setCommitOnKeyboardMovement(BOOL b)	{ mCommitOnKeyboardMovement = b; }
	void setCommitOnSelectionChange(BOOL b)		{ mCommitOnSelectionChange = b; }
	void setAllowKeyboardMovement(BOOL b)		{ mAllowKeyboardMovement = b; }

	void			setMaxSelectable(U32 max_selected) { mMaxSelectable = max_selected; }
	S32				getMaxSelectable() { return mMaxSelectable; }


	virtual S32		getScrollPos() const;
	virtual void	setScrollPos( S32 pos );

	S32				getSearchColumn() { return mSearchColumn; }
	void			setSearchColumn(S32 column) { mSearchColumn = column; }
	S32				getColumnIndexFromOffset(S32 x);
	S32				getColumnOffsetFromIndex(S32 index);
	S32				getRowOffsetFromIndex(S32 index);

	void			clearSearchString() { mSearchString.clear(); }

	// Overridden from LLView
	/*virtual*/ void    draw();
	/*virtual*/ BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleKeyHere(KEY key, MASK mask);
	/*virtual*/ BOOL	handleUnicodeCharHere(llwchar uni_char);
	/*virtual*/ BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	/*virtual*/ BOOL	handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect);
	/*virtual*/ void	setEnabled(BOOL enabled);
	/*virtual*/ void	setFocus( BOOL b );
	/*virtual*/ void	onFocusReceived();
	/*virtual*/ void	onFocusLost();
	/*virtual*/ void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	virtual BOOL	isDirty() const;
	virtual void	resetDirty();		// Clear dirty state

	virtual void	updateLayout();
	virtual void	fitContents(S32 max_width, S32 max_height);

	virtual LLRect	getRequiredRect();
	static  BOOL    rowPreceeds(LLScrollListItem *new_row, LLScrollListItem *test_row);

	LLRect			getItemListRect() { return mItemListRect; }

	// Used "internally" by the scroll bar.
	static void		onScrollChange( S32 new_pos, LLScrollbar* src, void* userdata );

	static void onClickColumn(void *userdata);

	void updateColumns();
	void calcColumnWidths();
	S32 getMaxContentWidth() { return mMaxContentWidth; }

	void setDisplayHeading(BOOL display);
	void setHeadingHeight(S32 heading_height);
	void setCollapseEmptyColumns(BOOL collapse);

	LLScrollListItem*	hitItem(S32 x,S32 y);
	virtual void		scrollToShowSelected();

	// LLEditMenuHandler functions
	virtual void	copy();
	virtual BOOL	canCopy() const;
	virtual void	cut();
	virtual BOOL	canCut() const;
	virtual void	selectAll();
	virtual BOOL	canSelectAll() const;
	virtual void	deselect();
	virtual BOOL	canDeselect() const;

	void setNumDynamicColumns(int num) { mNumDynamicWidthColumns = num; }
	void setTotalStaticColumnWidth(int width) { mTotalStaticColumnWidth = width; }

	std::string     getSortColumnName();
	BOOL			getSortAscending() { return mSortColumns.empty() ? TRUE : mSortColumns.back().second; }
	BOOL			needsSorting();

	S32		selectMultiple( LLDynamicArray<LLUUID> ids );
	void			sortItems();

	// manually call this whenever editing list items in place to flag need for resorting
	void			setSorted(BOOL sorted) { mSorted = sorted; }
	void			dirtyColumns(); // some operation has potentially affected column layout or ordering

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
	// returns FALSE if item faile to be added to list, does NOT delete 'item'
	BOOL			addItem( LLScrollListItem* item, EAddPosition pos = ADD_BOTTOM, BOOL requires_column = TRUE );

	typedef std::deque<LLScrollListItem *> item_list;
	item_list&		getItemList() { return mItemList; }

private:
	void			selectPrevItem(BOOL extend_selection);
	void			selectNextItem(BOOL extend_selection);
	void			drawItems();
	void			updateLineHeight();
	void            updateLineHeightInsert(LLScrollListItem* item);
	void			reportInvalidInput();
	BOOL			isRepeatedChars(const LLWString& string) const;
	void			selectItem(LLScrollListItem* itemp, BOOL single_select = TRUE);
	void			deselectItem(LLScrollListItem* itemp);
	void			commitIfChanged();
	BOOL			setSort(S32 column, BOOL ascending);


	S32				mCurIndex;			// For get[First/Next]Data
	S32				mCurSelectedIndex;  // For get[First/Next]Selected

	S32				mLineHeight;	// the max height of a single line
	S32				mScrollLines;	// how many lines we've scrolled down
	S32				mPageLines;		// max number of lines is it possible to see on the screen given mRect and mLineHeight
	S32				mHeadingHeight;	// the height of the column header buttons, if visible
	U32				mMaxSelectable; 
	LLScrollbar*	mScrollbar;
	BOOL 			mAllowMultipleSelection;
	BOOL			mAllowKeyboardMovement;
	BOOL			mCommitOnKeyboardMovement;
	BOOL			mCommitOnSelectionChange;
	BOOL			mSelectionChanged;
	BOOL			mNeedsScroll;
	BOOL			mCanSelect;
	BOOL			mDisplayColumnHeaders;
	BOOL			mColumnsDirty;

	item_list		mItemList;

	LLScrollListItem *mLastSelected;

	S32				mMaxItemCount; 

	LLRect			mItemListRect;
	S32				mMaxContentWidth;
	S32             mColumnPadding;

	BOOL			mBackgroundVisible;
	BOOL			mDrawStripes;

	LLColor4		mBgWriteableColor;
	LLColor4		mBgReadOnlyColor;
	LLColor4		mBgSelectedColor;
	LLColor4		mBgStripeColor;
	LLColor4		mFgSelectedColor;
	LLColor4		mFgUnselectedColor;
	LLColor4		mFgDisabledColor;
	LLColor4		mHighlightedColor;

	S32				mBorderThickness;
	void			(*mOnDoubleClickCallback)(void* userdata);
	void			(*mOnMaximumSelectCallback)(void* userdata );
	void			(*mOnSortChangedCallback)(void* userdata);

	S32				mHighlightedItem;
	class LLViewBorder*	mBorder;

	LLWString		mSearchString;
	LLFrameTimer	mSearchTimer;
	
	S32				mSearchColumn;
	S32				mNumDynamicWidthColumns;
	S32				mTotalStaticColumnWidth;

	BOOL			mSorted;
	
	typedef std::map<std::string, LLScrollListColumn> column_map_t;
	column_map_t mColumns;

	BOOL			mDirty;
	S32				mOriginalSelection;

	typedef std::vector<LLScrollListColumn*> ordered_columns_t;
	ordered_columns_t	mColumnsIndexed;

	typedef std::pair<S32, BOOL> sort_column_t;
	std::vector<sort_column_t>	mSortColumns;

	// HACK:  Did we draw one selected item this frame?
	BOOL mDrewSelected;
}; // end class LLScrollListCtrl


#endif  // LL_SCROLLLISTCTRL_H
