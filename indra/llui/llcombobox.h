/** 
 * @file llcombobox.h
 * @brief LLComboBox base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// A control that displays the name of the chosen item, which when clicked
// shows a scrolling box of choices.

#ifndef LL_LLCOMBOBOX_H
#define LL_LLCOMBOBOX_H

#include "llbutton.h"
#include "lluictrl.h"
#include "llctrlselectioninterface.h"
#include "llimagegl.h"
#include "llrect.h"

// Classes

class LLFontGL;
class LLButton;
class LLSquareButton;
class LLScrollListCtrl;
class LLLineEditor;
class LLViewBorder;

extern S32 LLCOMBOBOX_HEIGHT;
extern S32 LLCOMBOBOX_WIDTH;

class LLComboBox
:	public LLUICtrl, public LLCtrlListInterface
{
public:
	typedef enum e_preferred_position
	{
		ABOVE,
		BELOW
	} EPreferredPosition;

	LLComboBox(
		const LLString& name, 
		const LLRect &rect,
		const LLString& label,
		void (*commit_callback)(LLUICtrl*, void*) = NULL,
		void *callback_userdata = NULL
		);
	virtual ~LLComboBox(); 

	// LLView interface
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_COMBO_BOX; }
	virtual LLString getWidgetTag() const { return LL_COMBO_BOX_TAG; }
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	virtual void	draw();
	virtual void	onFocusLost();

	virtual void	setEnabled(BOOL enabled);

	virtual BOOL	handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect);
	virtual BOOL	handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent);

	// LLUICtrl interface
	virtual void	clear();					// select nothing
	virtual void	onCommit();
	virtual BOOL	acceptsTextInput() const		{ return mAllowTextEntry; }
	virtual BOOL	isDirty() const;			// Returns TRUE if the user has modified this control.
	virtual void	resetDirty();				// Clear dirty state

	virtual void	setFocus(BOOL b);

	// Selects item by underlying LLSD value, using LLSD::asString() matching.  
	// For simple items, this is just the name of the label.
	virtual void	setValue(const LLSD& value );

	// Gets underlying LLSD value for currently selected items.  For simple
	// items, this is just the label.
	virtual LLSD	getValue() const;

	void			setAllowTextEntry(BOOL allow, S32 max_chars = 50, BOOL make_tentative = TRUE);
	void			setTextEntry(const LLString& text);

	void			add(const LLString& name, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE);	// add item "name" to menu
	void			add(const LLString& name, const LLUUID& id, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE);
	void			add(const LLString& name, void* userdata, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE);
	void			add(const LLString& name, LLSD value, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE);
	BOOL			remove( S32 index );	// remove item by index, return TRUE if found and removed
	void			removeall() { clearRows(); }

	void			sortByName(); // Sort the entries in the combobox by name

	// Select current item by name using selectSimpleItem.  Returns FALSE if not found.
	BOOL			setSimple(const LLString& name);
	// Get name of current item. Returns an empty string if not found.
	const LLString&	getSimple() const;
	// Get contents of column x of selected row
	const LLString& getSimpleSelectedItem(S32 column = 0) const;

	// Sets the label, which doesn't have to exist in the label.
	// This is probably a UI abuse.
	void			setLabel(const LLString& name);

	BOOL			remove(const LLString& name);	// remove item "name", return TRUE if found and removed
	
	BOOL			setCurrentByIndex( S32 index );
	S32				getCurrentIndex() const;

	//========================================================================
	LLCtrlSelectionInterface* getSelectionInterface()	{ return (LLCtrlSelectionInterface*)this; };
	LLCtrlListInterface* getListInterface()				{ return (LLCtrlListInterface*)this; };

	// LLCtrlListInterface functions
	// See llscrolllistctrl.h
	virtual S32		getItemCount() const;
	// Overwrites the default column (See LLScrollListCtrl for format)
	virtual void 	addColumn(const LLSD& column, EAddPosition pos = ADD_BOTTOM);
	virtual void 	clearColumns();
	virtual void	setColumnLabel(const LLString& column, const LLString& label);
	virtual LLScrollListItem* addElement(const LLSD& value, EAddPosition pos = ADD_BOTTOM, void* userdata = NULL);
	virtual LLScrollListItem* addSimpleElement(const LLString& value, EAddPosition pos = ADD_BOTTOM, const LLSD& id = LLSD());
	virtual void 	clearRows();
	virtual void 	sortByColumn(LLString name, BOOL ascending);

	// LLCtrlSelectionInterface functions
	virtual BOOL	getCanSelect() const				{ return TRUE; }
	virtual BOOL	selectFirstItem()					{ return setCurrentByIndex(0); }
	virtual BOOL	selectNthItem( S32 index )			{ return setCurrentByIndex(index); }
	virtual S32		getFirstSelectedIndex() const		{ return getCurrentIndex(); }
	virtual BOOL	setCurrentByID( const LLUUID& id );
	virtual LLUUID	getCurrentID();				// LLUUID::null if no items in menu
	virtual BOOL	setSelectedByValue(LLSD value, BOOL selected);
	virtual LLSD	getSimpleSelectedValue();
	virtual BOOL	isSelected(LLSD value);
	virtual BOOL	operateOnSelection(EOperation op);
	virtual BOOL	operateOnAll(EOperation op);

	//========================================================================
	
	void*			getCurrentUserdata();

	void			setPrearrangeCallback( void (*cb)(LLUICtrl*,void*) ) { mPrearrangeCallback = cb; }
	void			setTextEntryCallback( void (*cb)(LLLineEditor*, void*) ) { mTextEntryCallback = cb; }

	void			setButtonVisible(BOOL visible);

	static void		onButtonDown(void *userdata);
	static void		onItemSelected(LLUICtrl* item, void *userdata);
	static void		onListFocusChanged(LLUICtrl* item, void *userdata);
	static void		onTextEntry(LLLineEditor* line_editor, void* user_data);
	static void		onTextCommit(LLUICtrl* caller, void* user_data);

	void			updateSelection();
	virtual void	showList();
	virtual void	hideList();
	
protected:
	LLButton*			mButton;
	LLScrollListCtrl*	mList;
	LLViewBorder*		mBorder;
	BOOL				mKeyboardFocusOnClick;
	BOOL				mDrawArrow;
	LLLineEditor*		mTextEntry;
	LLPointer<LLImageGL>	mArrowImage;
	S32					mArrowImageWidth;
	BOOL				mAllowTextEntry;
	S32					mMaxChars;
	BOOL				mTextEntryTentative;
	EPreferredPosition	mListPosition;
	void				(*mPrearrangeCallback)(LLUICtrl*,void*);
	void				(*mTextEntryCallback)(LLLineEditor*, void*);
};

#endif
