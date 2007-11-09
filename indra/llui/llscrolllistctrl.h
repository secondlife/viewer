/** 
 * @file llscrolllistctrl.h
 * @brief LLScrollListCtrl base class
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
#include "llviewborder.h"
#include "llframetimer.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"

class LLScrollbar;
class LLScrollListCtrl;
class LLColumnHeader;
class LLResizeBar;

class LLScrollListCell
{
public:
	virtual ~LLScrollListCell() {};
	virtual void			drawToWidth(S32 width, const LLColor4& color, const LLColor4& highlight_color) const = 0;		// truncate to given width, if possible
	virtual S32				getWidth() const = 0;
	virtual S32				getHeight() const = 0;
	virtual const LLString&	getText() const { return LLString::null; }
	virtual const LLString&	getTextLower() const { return LLString::null; }
	virtual BOOL			getVisible() const { return TRUE; }
	virtual void			setWidth(S32 width) = 0;
	virtual void			highlightText(S32 offset, S32 num_chars) {}
	virtual BOOL			isText() = 0;
	virtual void			setColor(const LLColor4&) = 0;

	virtual BOOL	handleClick() { return FALSE; }
	virtual	void	setEnabled(BOOL enable) { }
};

class LLScrollListSeparator : public LLScrollListCell
{
public:
	LLScrollListSeparator(S32 width);
	virtual ~LLScrollListSeparator() {};
	virtual void			drawToWidth(S32 width, const LLColor4& color, const LLColor4& highlight_color) const;		// truncate to given width, if possible
	virtual S32				getWidth() const {return mWidth;}
	virtual S32				getHeight() const { return 5; };
	virtual void			setWidth(S32 width) {mWidth = width; }
	virtual void			setColor(const LLColor4&) {};
	virtual BOOL			isText() { return FALSE; }

protected:
	S32 mWidth;
};

class LLScrollListText : public LLScrollListCell
{
public:
	LLScrollListText( const LLString& text, const LLFontGL* font, S32 width = 0, U8 font_style = LLFontGL::NORMAL, LLFontGL::HAlign font_alignment = LLFontGL::LEFT, LLColor4& color = LLColor4::black, BOOL use_color = FALSE, BOOL visible = TRUE);
	/*virtual*/ ~LLScrollListText();

	virtual void    drawToWidth(S32 width, const LLColor4& color, const LLColor4& highlight_color) const;
	virtual S32		getWidth() const			{ return mWidth; }
	virtual void	setWidth(S32 width)			{ mWidth = width; }
	virtual S32		getHeight() const			{ return llround(mFont->getLineHeight()); }
	virtual const LLString&		getText() const		{ return mText.getString(); }
	virtual BOOL	getVisible() const  { return mVisible; }
	virtual void	highlightText(S32 offset, S32 num_chars) {mHighlightOffset = offset; mHighlightCount = num_chars;}
	void			setText(const LLStringExplicit& text);
	virtual void	setColor(const LLColor4&);
	virtual BOOL	isText() { return TRUE; }

private:
	LLUIString		mText;
	const LLFontGL*	mFont;
	LLColor4*		mColor;
	const U8		mFontStyle;
	LLFontGL::HAlign mFontAlignment;
	S32				mWidth;
	BOOL			mVisible;
	S32				mHighlightCount;
	S32				mHighlightOffset;

	LLPointer<LLImageGL> mRoundedRectImage;

	static U32 sCount;
};

class LLScrollListIcon : public LLScrollListCell
{
public:
	LLScrollListIcon( LLImageGL* icon, S32 width = 0, LLUUID image_id = LLUUID::null);
	/*virtual*/ ~LLScrollListIcon();
	virtual void	drawToWidth(S32 width, const LLColor4& color, const LLColor4& highlight_color) const;
	virtual S32		getWidth() const			{ return mWidth; }
	virtual S32		getHeight() const			{ return mIcon->getHeight(); }
	virtual const LLString& getText() const { return mImageUUID; }
	virtual const LLString& getTextLower() const { return mImageUUID; }
	virtual void	setWidth(S32 width)			{ mWidth = width; }
	virtual void	setColor(const LLColor4&);
	virtual BOOL	isText() { return FALSE; }

private:
	LLPointer<LLImageGL> mIcon;
	LLString mImageUUID;
	S32 mWidth;
	LLColor4 mColor;
};

class LLScrollListCheck : public LLScrollListCell
{
public:
	LLScrollListCheck( LLCheckBoxCtrl* check_box, S32 width = 0);
	/*virtual*/ ~LLScrollListCheck();
	virtual void	drawToWidth(S32 width, const LLColor4& color, const LLColor4& highlight_color) const;
	virtual S32		getWidth() const			{ return mWidth; }
	virtual S32		getHeight() const			{ return 0; } 
	virtual void	setWidth(S32 width)			{ mWidth = width; }

	virtual BOOL	handleClick();
	virtual void	setEnabled(BOOL enable)		{ if (mCheckBox) mCheckBox->setEnabled(enable); }
	virtual void	setColor(const LLColor4& color) {};

	LLCheckBoxCtrl*	getCheckBox()				{ return mCheckBox; }
	virtual BOOL	isText() { return FALSE; }

private:
	LLCheckBoxCtrl* mCheckBox;
	S32 mWidth;
};

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

	LLScrollListColumn(LLString name, LLString label, S32 width, F32 relwidth) : 
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
		mHeader(NULL) 
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
	}

	LLString			mName;
	LLString			mSortingColumn;
	BOOL				mSortAscending;
	LLString			mLabel;
	S32					mWidth;
	F32					mRelWidth;
	BOOL				mDynamicWidth;
	S32					mMaxContentWidth;
	S32					mIndex;
	LLScrollListCtrl*	mParentCtrl;
	LLColumnHeader*		mHeader;
	LLFontGL::HAlign	mFontAlignment;
};

class LLColumnHeader : public LLComboBox
{
public:
	LLColumnHeader(const LLString& label, const LLRect &rect, LLScrollListColumn* column, const LLFontGL *font = NULL);
	~LLColumnHeader();

	/*virtual*/ void draw();
	/*virtual*/ BOOL handleDoubleClick(S32 x, S32 y, MASK mask);
	/*virtual*/ void showList();
	/*virtual*/ LLView*	findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding);
	/*virtual*/ void userSetShape(const LLRect& new_rect);
	
	void setImage(const LLString &image_name);
	LLScrollListColumn* getColumn() { return mColumn; }
	void setHasResizableElement(BOOL resizable);
	BOOL canResize();
	void enableResizeBar(BOOL enable);
	LLString getLabel() { return mOrigLabel; }

	static void onSelectSort(LLUICtrl* ctrl, void* user_data);
	static void onClick(void* user_data);
	static void onMouseDown(void* user_data);
	static void onHeldDown(void* user_data);

protected:
	LLScrollListColumn* mColumn;
	LLResizeBar*		mResizeBar;
	LLString			mOrigLabel;
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
	void	addColumn( const LLString& text, const LLFontGL* font, S32 width = 0 , U8 font_style = LLFontGL::NORMAL, LLFontGL::HAlign font_alignment = LLFontGL::LEFT, BOOL visible = TRUE)
				{ mColumns.push_back( new LLScrollListText(text, font, width, font_style, font_alignment, LLColor4::black, FALSE, visible) ); }

	void	addColumn( LLImageGL* icon, S32 width = 0 )
				{ mColumns.push_back( new LLScrollListIcon(icon, width) ); }

	void	addColumn( LLCheckBoxCtrl* check, S32 width = 0 )
				{ mColumns.push_back( new LLScrollListCheck(check,width) ); }

	void	setNumColumns(S32 columns);

	void	setColumn( S32 column, LLScrollListCell *cell );
	
	S32		getNumColumns() const				{ return mColumns.size(); }

	LLScrollListCell *getColumn(const S32 i) const	{ if (0 <= i && i < (S32)mColumns.size()) { return mColumns[i]; } return NULL; }

	virtual BOOL handleClick(S32 x, S32 y, MASK mask);

	LLString getContentsCSV();

private:
	BOOL	mSelected;
	BOOL	mEnabled;
	void*	mUserdata;
	LLSD	mItemValue;
	std::vector<LLScrollListCell *> mColumns;
};


class LLScrollListCtrl : public LLUICtrl, public LLEditMenuHandler, 
	public LLCtrlListInterface, public LLCtrlScrollInterface
{
public:
	LLScrollListCtrl(
		const LLString& name,
		const LLRect& rect,
		void (*commit_callback)(LLUICtrl*, void*),
		void* callback_userdata,
		BOOL allow_multiple_selection,
		BOOL draw_border = TRUE);

	virtual ~LLScrollListCtrl();
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_SCROLL_LIST; }
	virtual LLString getWidgetTag() const { return LL_SCROLL_LIST_CTRL_TAG; }
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	void setScrollListParameters(LLXMLNodePtr node);
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	S32				isEmpty() const;

	void			deleteAllItems() { clearRows(); }
	
	// Sets an array of column descriptors
	void 	   		 setColumnHeadings(LLSD headings);
	// Numerical based sort by column function (used by LLComboBox)
	void   			 sortByColumn(U32 column, BOOL ascending);
	
	// LLCtrlListInterface functions
	virtual S32  getItemCount() const;
	// Adds a single column descriptor: ["name" : string, "label" : string, "width" : integer, "relwidth" : integer ]
	virtual void addColumn(const LLSD& column, EAddPosition pos = ADD_BOTTOM);
	virtual void clearColumns();
	virtual void setColumnLabel(const LLString& column, const LLString& label);

	virtual LLScrollListColumn* getColumn(S32 index);
	virtual S32 getNumColumns() const { return mColumnsIndexed.size(); }

	// Adds a single element, from an array of:
	// "columns" => [ "column" => column name, "value" => value, "type" => type, "font" => font, "font-style" => style ], "id" => uuid
	// Creates missing columns automatically.
	virtual LLScrollListItem* addElement(const LLSD& value, EAddPosition pos = ADD_BOTTOM, void* userdata = NULL);
	// Simple add element. Takes a single array of:
	// [ "value" => value, "font" => font, "font-style" => style ]
	virtual LLScrollListItem* addSimpleElement(const LLString& value, EAddPosition pos = ADD_BOTTOM, const LLSD& id = LLSD());
	virtual void clearRows(); // clears all elements
	virtual void sortByColumn(LLString name, BOOL ascending);

	// These functions take and return an array of arrays of elements, as above
	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;

	LLCtrlSelectionInterface*	getSelectionInterface()	{ return (LLCtrlSelectionInterface*)this; }
	LLCtrlListInterface*		getListInterface()		{ return (LLCtrlListInterface*)this; }
	LLCtrlScrollInterface*		getScrollInterface()	{ return (LLCtrlScrollInterface*)this; }

	// DEPRECATED: Use setSelectedByValue() below.
	BOOL			setCurrentByID( const LLUUID& id )	{ return selectByID(id); }
	virtual LLUUID	getCurrentID()						{ return getStringUUIDSelectedItem(); }

	BOOL			operateOnSelection(EOperation op);
	BOOL			operateOnAll(EOperation op);

	// returns FALSE if unable to set the max count so low
	BOOL 			setMaxItemCount(S32 max_count);

	BOOL			selectByID( const LLUUID& id );		// FALSE if item not found

	// Match item by value.asString(), which should work for string, integer, uuid.
	// Returns FALSE if not found.
	BOOL			setSelectedByValue(LLSD value, BOOL selected);

	BOOL			isSorted();

	virtual BOOL	isSelected(LLSD value);
	
	BOOL			selectFirstItem();
	BOOL			selectNthItem( S32 index );
	BOOL			selectItemAt(S32 x, S32 y, MASK mask);
	
	void			deleteSingleItem( S32 index ) ;
	void 			deleteSelectedItems();
	void			deselectAllItems(BOOL no_commit_on_change = FALSE);	// by default, go ahead and commit on selection change

	void			highlightNthItem( S32 index );
	void			setDoubleClickCallback( void (*cb)(void*) ) { mOnDoubleClickCallback = cb; }
	void			setMaxiumumSelectCallback( void (*cb)(void*) ) { mOnMaximumSelectCallback = cb; }
	void			setSortChangedCallback( void (*cb)(void*) ) { mOnSortChangedCallback = cb; }

	void			swapWithNext(S32 index);
	void			swapWithPrevious(S32 index);

	void			setCanSelect(BOOL can_select)		{ mCanSelect = can_select; }
	virtual BOOL	getCanSelect() const				{ return mCanSelect; }

	S32				getItemIndex( LLScrollListItem* item );
	S32				getItemIndex( LLUUID& item_id );

	// "Simple" interface: use this when you're creating a list that contains only unique strings, only
	// one of which can be selected at a time.
	LLScrollListItem* addSimpleItem( const LLString& item_text, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE );
	// Add an item with an associated LLSD
	LLScrollListItem* addSimpleItem(const LLString& item_text, LLSD sd, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE, S32 column_width = 0 );

	BOOL			selectSimpleItem( const LLString& item, BOOL case_sensitive = TRUE );		// FALSE if item not found
	BOOL			selectSimpleItemByPrefix(const LLString& target, BOOL case_sensitive);
	BOOL			selectSimpleItemByPrefix(const LLWString& target, BOOL case_sensitive);
	const LLString&	getSimpleSelectedItem(S32 column = 0) const;
	LLSD			getSimpleSelectedValue();

	// DEPRECATED: Use LLSD versions of addSimpleItem() and getSimpleSelectedValue().
	// "StringUUID" interface: use this when you're creating a list that contains non-unique strings each of which
	// has an associated, unique UUID, and only one of which can be selected at a time.
	LLScrollListItem*	addStringUUIDItem(const LLString& item_text, const LLUUID& id, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE, S32 column_width = 0);
	LLUUID				getStringUUIDSelectedItem();

	LLScrollListItem*	getFirstSelected() const;
	virtual S32			getFirstSelectedIndex() const;
	std::vector<LLScrollListItem*> getAllSelected() const;

	LLScrollListItem*	getLastSelectedItem() const { return mLastSelected; }

	// iterate over all items
	LLScrollListItem*	getFirstData() const;
	LLScrollListItem*	getLastData() const;
	std::vector<LLScrollListItem*>	getAllData() const;
	
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


	virtual S32		getScrollPos();
	virtual void	setScrollPos( S32 pos );

	S32				getSearchColumn() { return mSearchColumn; }
	void			setSearchColumn(S32 column) { mSearchColumn = column; }

	void			clearSearchString() { mSearchString.clear(); }

	// Overridden from LLView
	virtual void    draw();
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent);
	virtual BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual void	setEnabled(BOOL enabled);
	virtual void	setFocus( BOOL b );
	virtual void	onFocusReceived();
	virtual void	onFocusLost();

	virtual BOOL	isDirty() const;
	virtual void	resetDirty();		// Clear dirty state

	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	virtual void	arrange(S32 max_width, S32 max_height);
	virtual LLRect	getRequiredRect();
	static  BOOL    rowPreceeds(LLScrollListItem *new_row, LLScrollListItem *test_row);

	LLRect			getItemListRect() { return mItemListRect; }

	// Used "internally" by the scroll bar.
	static void		onScrollChange( S32 new_pos, LLScrollbar* src, void* userdata );

	static void onClickColumn(void *userdata);

	void updateColumns();
	void updateMaxContentWidth(LLScrollListItem* changed_item);

	void setDisplayHeading(BOOL display);
	void setHeadingHeight(S32 heading_height);
	void setCollapseEmptyColumns(BOOL collapse);
	void setIsPopup(BOOL is_popup) { mIsPopup = is_popup; }

	LLScrollListItem*	hitItem(S32 x,S32 y);
	virtual void		scrollToShowSelected();

	// LLEditMenuHandler functions
	virtual void	copy();
	virtual BOOL	canCopy();

	virtual void	cut();
	virtual BOOL	canCut();

	virtual void	doDelete();
	virtual BOOL	canDoDelete();

	virtual void	selectAll();
	virtual BOOL	canSelectAll();

	virtual void	deselect();
	virtual BOOL	canDeselect();

	void setNumDynamicColumns(int num) { mNumDynamicWidthColumns = num; }
	void setTotalStaticColumnWidth(int width) { mTotalStaticColumnWidth = width; }

	std::string     getSortColumnName();
	BOOL			getSortAscending() { return mSortAscending; }

	S32		selectMultiple( LLDynamicArray<LLUUID> ids );

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

	// returns FALSE if item faile to be added to list, does NOT delete 'item'
	BOOL			addItem( LLScrollListItem* item, EAddPosition pos = ADD_BOTTOM );
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
	void			setSorted(BOOL sorted);

protected:
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
	BOOL			mCollapseEmptyColumns;
	BOOL			mIsPopup;

	typedef std::deque<LLScrollListItem *> item_list;
	item_list		mItemList;

	LLScrollListItem *mLastSelected;

	S32				mMaxItemCount; 

	LLRect			mItemListRect;

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
	LLViewBorder*	mBorder;

	LLWString		mSearchString;
	LLFrameTimer	mSearchTimer;
	
	LLString		mDefaultColumn;

	S32				mSearchColumn;
	S32				mNumDynamicWidthColumns;
	S32				mTotalStaticColumnWidth;

	S32				mSortColumn;
	BOOL			mSortAscending;
	BOOL			mSorted;

	std::map<LLString, LLScrollListColumn> mColumns;
	std::vector<LLScrollListColumn*> mColumnsIndexed;

	BOOL			mDirty;
	S32				mOriginalSelection;

public:
	// HACK:  Did we draw one selected item this frame?
	BOOL mDrewSelected;
};

const BOOL MULTIPLE_SELECT_YES = TRUE;
const BOOL MULTIPLE_SELECT_NO = FALSE;

const BOOL SHOW_BORDER_YES = TRUE;
const BOOL SHOW_BORDER_NO = FALSE;

#endif  // LL_SCROLLLISTCTRL_H
