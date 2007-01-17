/** 
 * @file llscrolllistctrl.h
 * @brief LLScrollListCtrl base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

class LLScrollbar;
class LLScrollListCtrl;

class LLScrollListCell
{
public:
	virtual ~LLScrollListCell() {};
	virtual void			drawToWidth(S32 width, const LLColor4& color, const LLColor4& highlight_color) const = 0;		// truncate to given width, if possible
	virtual S32				getWidth() const = 0;
	virtual S32				getHeight() const = 0;
	virtual const LLString&	getText() const { return LLString::null; }
	virtual const LLString&	getTextLower() const { return LLString::null; }
	virtual const BOOL		getVisible() const { return TRUE; }
	virtual void			setWidth(S32 width) = 0;
	virtual void			highlightText(S32 num_chars) {}

	virtual BOOL	handleClick() { return FALSE; }
	virtual	void	setEnabled(BOOL enable) { }
};

class LLScrollListText : public LLScrollListCell
{
	static U32 sCount;
public:
	LLScrollListText( const LLString& text, const LLFontGL* font, S32 width = 0, U8 font_style = LLFontGL::NORMAL, LLColor4& color = LLColor4::black, BOOL use_color = FALSE, BOOL visible = TRUE);
	/*virtual*/ ~LLScrollListText();

	virtual void    drawToWidth(S32 width, const LLColor4& color, const LLColor4& highlight_color) const;
	virtual S32		getWidth() const			{ return mWidth; }
	virtual void	setWidth(S32 width)			{ mWidth = width; }
	virtual S32		getHeight() const			{ return llround(mFont->getLineHeight()); }
	virtual const LLString&		getText() const		{ return mText.getString(); }
	virtual const BOOL			getVisible() const  { return mVisible; }
	virtual void	highlightText(S32 num_chars) {mHighlightChars = num_chars;}
	void			setText(const LLString& text);

private:
	LLUIString		mText;
	const LLFontGL*	mFont;
	LLColor4*		mColor;
	const U8		mFontStyle;
	S32				mWidth;
	S32				mEllipsisWidth;	// in pixels, of "..."
	BOOL			mVisible;
	S32				mHighlightChars;

	LLPointer<LLImageGL> mRoundedRectImage;
};

class LLScrollListIcon : public LLScrollListCell
{
public:
	LLScrollListIcon( LLImageGL* icon, S32 width = 0, LLUUID image_id = LLUUID::null);
	/*virtual*/ ~LLScrollListIcon();
	virtual void	drawToWidth(S32 width, const LLColor4& color, const LLColor4& highlight_color) const	{ gl_draw_image(0, 0, mIcon); }
	virtual S32		getWidth() const			{ return mWidth; }
	virtual S32		getHeight() const			{ return mIcon->getHeight(); }
	virtual const LLString& getText() const { return mImageUUID; }
	virtual const LLString& getTextLower() const { return mImageUUID; }
	virtual void	setWidth(S32 width)			{ mWidth = width; }

private:
	LLPointer<LLImageGL> mIcon;
	LLString mImageUUID;
	S32 mWidth;
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

	LLCheckBoxCtrl*	getCheckBox()				{ return mCheckBox; }

private:
	LLCheckBoxCtrl* mCheckBox;
	S32 mWidth;
};

class LLScrollListColumn
{
public:
	// Default constructor
	LLScrollListColumn() : mName(""), mSortingColumn(""), mLabel(""), mWidth(-1), mRelWidth(-1.0), mDynamicWidth(FALSE), mIndex(-1), mParentCtrl(NULL), mButton(NULL) { }

	LLScrollListColumn(LLString name, LLString label, S32 width, F32 relwidth)
		: mName(name), mSortingColumn(name), mLabel(label), mWidth(width), mRelWidth(relwidth), mDynamicWidth(FALSE), mIndex(-1), mParentCtrl(NULL), mButton(NULL) { }

	LLScrollListColumn(const LLSD &sd)
	{
		mName = sd.get("name").asString();
		mSortingColumn = mName;
		if (sd.has("sort"))
		{
			mSortingColumn = sd.get("sort").asString();
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
		mIndex = -1;
		mParentCtrl = NULL;
		mButton = NULL;
	}

	LLString mName;
	LLString mSortingColumn;
	LLString mLabel;
	S32 mWidth;
	F32 mRelWidth;
	BOOL mDynamicWidth;
	S32 mIndex;
	LLScrollListCtrl *mParentCtrl;
	LLButton *mButton;
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
	void	addColumn( const LLString& text, const LLFontGL* font, S32 width = 0 , U8 font_style = LLFontGL::NORMAL, BOOL visible = TRUE)
				{ mColumns.push_back( new LLScrollListText(text, font, width, font_style, LLColor4::black, FALSE, visible) ); }

	void	addColumn( LLImageGL* icon, S32 width = 0 )
				{ mColumns.push_back( new LLScrollListIcon(icon, width) ); }

	void	addColumn( LLCheckBoxCtrl* check, S32 width = 0 )
				{ mColumns.push_back( new LLScrollListCheck(check,width) ); }

	void	setNumColumns(S32 columns);

	void	setColumn( S32 column, LLScrollListCell *cell );
	
	S32		getNumColumns() const				{ return mColumns.size(); }

	LLScrollListCell *getColumn(const S32 i) const	{ if (i < (S32)mColumns.size()) { return mColumns[i]; } return NULL; }

	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);

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

	virtual BOOL	isSelected(LLSD value);

	BOOL			selectFirstItem();
	BOOL			selectNthItem( S32 index );

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
	// TomY TODO - Deprecate this API and remove it
	BOOL				addItem( LLScrollListItem* item, EAddPosition pos = ADD_BOTTOM );
	LLScrollListItem*	getFirstSelected() const;
	virtual S32			getFirstSelectedIndex();
	std::vector<LLScrollListItem*> getAllSelected() const;

	LLScrollListItem*	getLastSelectedItem() const { return mLastSelected; }

	// iterate over all items
	LLScrollListItem*	getFirstData() const;
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
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent);
	virtual BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual void	setEnabled(BOOL enabled);
	virtual void	setFocus( BOOL b );
	virtual void	onFocusLost();

	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	virtual void	arrange(S32 max_width, S32 max_height);
	virtual LLRect	getRequiredRect();
	static  BOOL    rowPreceeds(LLScrollListItem *new_row, LLScrollListItem *test_row);

	// Used "internally" by the scroll bar.
	static void		onScrollChange( S32 new_pos, LLScrollbar* src, void* userdata );

	static void onClickColumn(void *userdata);

	void updateColumns();
	void updateColumnButtons();

	void setDisplayHeading(BOOL display);
	void setHeadingHeight(S32 heading_height);
	void setHeadingFont(const LLFontGL* heading_font);
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
	void			selectPrevItem(BOOL extend_selection);
	void			selectNextItem(BOOL extend_selection);
	void			drawItems();
	void			updateLineHeight();
	void			reportInvalidInput();
	BOOL			isRepeatedChars(const LLWString& string) const;
	void			selectItem(LLScrollListItem* itemp, BOOL single_select = TRUE);
	void			deselectItem(LLScrollListItem* itemp);
	void			commitIfChanged();

protected:
	S32				mCurIndex;			// For get[First/Next]Data
	S32				mCurSelectedIndex;  // For get[First/Next]Selected

	S32				mLineHeight;	// the max height of a single line
	S32				mScrollLines;	// how many lines we've scrolled down
	S32				mPageLines;		// max number of lines is it possible to see on the screen given mRect and mLineHeight
	S32				mHeadingHeight;	// the height of the column header buttons, if visible
	U32				mMaxSelectable; 
	const LLFontGL*	mHeadingFont;	// the font to use for column head buttons, if visible
	LLScrollbar*	mScrollbar;
	BOOL 			mAllowMultipleSelection;
	BOOL			mAllowKeyboardMovement;
	BOOL			mCommitOnKeyboardMovement;
	BOOL			mCommitOnSelectionChange;
	BOOL			mSelectionChanged;
	BOOL			mCanSelect;
	BOOL			mDisplayColumnButtons;
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

	U32      mSortColumn;
	BOOL     mSortAscending;

	std::map<LLString, LLScrollListColumn> mColumns;
	std::vector<LLScrollListColumn*> mColumnsIndexed;

public:
	// HACK:  Did we draw one selected item this frame?
	BOOL mDrewSelected;
};

const BOOL MULTIPLE_SELECT_YES = TRUE;
const BOOL MULTIPLE_SELECT_NO = FALSE;

const BOOL SHOW_BORDER_YES = TRUE;
const BOOL SHOW_BORDER_NO = FALSE;

#endif  // LL_SCROLLLISTCTRL_H
