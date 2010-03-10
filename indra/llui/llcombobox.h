/** 
 * @file llcombobox.h
 * @brief LLComboBox base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

// A control that displays the name of the chosen item, which when clicked
// shows a scrolling box of choices.

#ifndef LL_LLCOMBOBOX_H
#define LL_LLCOMBOBOX_H

#include "llbutton.h"
#include "lluictrl.h"
#include "llctrlselectioninterface.h"
#include "llrect.h"
#include "llscrolllistctrl.h"
#include "lllineeditor.h"
#include <boost/function.hpp>

// Classes

class LLFontGL;
class LLViewBorder;

class LLComboBox
:	public LLUICtrl, public LLCtrlListInterface
{
public:	
	typedef enum e_preferred_position
	{
		ABOVE,
		BELOW
	} EPreferredPosition;

	struct PreferredPositionValues : public LLInitParam::TypeValuesHelper<EPreferredPosition, PreferredPositionValues>
	{
		static void declareValues();
	};


	struct ItemParams : public LLInitParam::Block<ItemParams, LLScrollListItem::Params>
	{
		Optional<std::string>	label;
		ItemParams();
	};

	struct Params 
	:	public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<bool>						allow_text_entry,
											show_text_as_tentative,
											allow_new_values;
		Optional<S32>						max_chars;
		Optional<commit_callback_t> 		prearrange_callback,
											text_entry_callback;

		Optional<EPreferredPosition, PreferredPositionValues>	list_position;
		
		// components
		Optional<LLButton::Params>			combo_button;
		Optional<LLScrollListCtrl::Params>	combo_list;
		Optional<LLLineEditor::Params>		combo_editor;

		Optional<LLButton::Params>          drop_down_button;

		Multiple<ItemParams>				items;
		
		Params();
	};


	virtual ~LLComboBox(); 
	/*virtual*/ BOOL postBuild();
	
protected:
	friend class LLUICtrlFactory;
	LLComboBox(const Params&);
	void	initFromParams(const Params&);
	void	prearrangeList(std::string filter = "");

public:
	// LLView interface
	virtual void	onFocusLost();

	virtual BOOL	handleToolTip(S32 x, S32 y, MASK mask);
	virtual BOOL	handleKeyHere(KEY key, MASK mask);
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char);

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

	void			setTextEntry(const LLStringExplicit& text);

	LLScrollListItem*	add(const std::string& name, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE);	// add item "name" to menu
	LLScrollListItem*	add(const std::string& name, const LLUUID& id, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE);
	LLScrollListItem*	add(const std::string& name, void* userdata, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE);
	LLScrollListItem*	add(const std::string& name, LLSD value, EAddPosition pos = ADD_BOTTOM, BOOL enabled = TRUE);
	LLScrollListItem*	addSeparator(EAddPosition pos = ADD_BOTTOM);
	BOOL			remove( S32 index );	// remove item by index, return TRUE if found and removed
	void			removeall() { clearRows(); }

	void			sortByName(BOOL ascending = TRUE); // Sort the entries in the combobox by name

	// Select current item by name using selectItemByLabel.  Returns FALSE if not found.
	BOOL			setSimple(const LLStringExplicit& name);
	// Get name of current item. Returns an empty string if not found.
	const std::string	getSimple() const;
	// Get contents of column x of selected row
	const std::string getSelectedItemLabel(S32 column = 0) const;

	// Sets the label, which doesn't have to exist in the label.
	// This is probably a UI abuse.
	void			setLabel(const LLStringExplicit& name);

	BOOL			remove(const std::string& name);	// remove item "name", return TRUE if found and removed
	
	BOOL			setCurrentByIndex( S32 index );
	S32				getCurrentIndex() const;

	void			createLineEditor(const Params&);

	//========================================================================
	LLCtrlSelectionInterface* getSelectionInterface()	{ return (LLCtrlSelectionInterface*)this; };
	LLCtrlListInterface* getListInterface()				{ return (LLCtrlListInterface*)this; };

	// LLCtrlListInterface functions
	// See llscrolllistctrl.h
	virtual S32		getItemCount() const;
	// Overwrites the default column (See LLScrollListCtrl for format)
	virtual void 	addColumn(const LLSD& column, EAddPosition pos = ADD_BOTTOM);
	virtual void 	clearColumns();
	virtual void	setColumnLabel(const std::string& column, const std::string& label);
	virtual LLScrollListItem* addElement(const LLSD& value, EAddPosition pos = ADD_BOTTOM, void* userdata = NULL);
	virtual LLScrollListItem* addSimpleElement(const std::string& value, EAddPosition pos = ADD_BOTTOM, const LLSD& id = LLSD());
	virtual void 	clearRows();
	virtual void 	sortByColumn(const std::string& name, BOOL ascending);

	// LLCtrlSelectionInterface functions
	virtual BOOL	getCanSelect() const				{ return TRUE; }
	virtual BOOL	selectFirstItem()					{ return setCurrentByIndex(0); }
	virtual BOOL	selectNthItem( S32 index )			{ return setCurrentByIndex(index); }
	virtual BOOL	selectItemRange( S32 first, S32 last );
	virtual S32		getFirstSelectedIndex() const		{ return getCurrentIndex(); }
	virtual BOOL	setCurrentByID( const LLUUID& id );
	virtual LLUUID	getCurrentID() const;				// LLUUID::null if no items in menu
	virtual BOOL	setSelectedByValue(const LLSD& value, BOOL selected);
	virtual LLSD	getSelectedValue();
	virtual BOOL	isSelected(const LLSD& value) const;
	virtual BOOL	operateOnSelection(EOperation op);
	virtual BOOL	operateOnAll(EOperation op);

	//========================================================================
	
	void*			getCurrentUserdata();

	void			setPrearrangeCallback( commit_callback_t cb ) { mPrearrangeCallback = cb; }
	void			setTextEntryCallback( commit_callback_t cb ) { mTextEntryCallback = cb; }

	void			setButtonVisible(BOOL visible);

	void			onButtonMouseDown();
	void			onListMouseUp();
	void			onItemSelected(const LLSD& data);
	void			onTextCommit(const LLSD& data);

	void			updateSelection();
	virtual void	showList();
	virtual void	hideList();
	
	virtual void	onTextEntry(LLLineEditor* line_editor);
	
protected:
	LLButton*			mButton;
	LLLineEditor*		mTextEntry;
	LLScrollListCtrl*	mList;
	EPreferredPosition	mListPosition;
	LLPointer<LLUIImage>	mArrowImage;
	LLUIString			mLabel;
	BOOL				mHasAutocompletedText;

private:
	BOOL				mAllowTextEntry;
	BOOL				mAllowNewValues;
	S32					mMaxChars;
	BOOL				mTextEntryTentative;
	commit_callback_t	mPrearrangeCallback;
	commit_callback_t	mTextEntryCallback;
	commit_callback_t	mSelectionCallback;
	boost::signals2::connection mTopLostSignalConnection;
	S32                 mLastSelectedIndex;
};
#endif
