/** 
 * @file llradiogroup.h
 * @brief LLRadioGroup base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// An invisible view containing multiple mutually exclusive toggling 
// buttons (usually radio buttons).  Automatically handles the mutex
// condition by highlighting only one button at a time.

#ifndef LL_LLRADIOGROUP_H
#define LL_LLRADIOGROUP_H

#include "lluictrl.h"
#include "llcheckboxctrl.h"
#include "llctrlselectioninterface.h"

class LLFontGL;

// Radio controls are checkbox controls with use_radio_style true
class LLRadioCtrl : public LLCheckBoxCtrl 
{
public:
	LLRadioCtrl(const LLString& name, const LLRect& rect, const LLString& label,	
		const LLFontGL* font = NULL,
		void (*commit_callback)(LLUICtrl*, void*) = NULL,
		void* callback_userdata = NULL);
	/*virtual*/ ~LLRadioCtrl();

	/*virtual*/ void setValue(const LLSD& value);
};

class LLRadioGroup
:	public LLUICtrl, public LLCtrlSelectionInterface
{
public:
	// Build a radio group.  The number (0...n-1) of the currently selected
	// element will be stored in the named control.  After the control is
	// changed the callback will be called.
	LLRadioGroup(const LLString& name, const LLRect& rect, 
		const LLString& control_name, 
		LLUICtrlCallback callback = NULL,
		void* userdata = NULL,
		BOOL border = TRUE);

	// Another radio group constructor, but this one doesn't rely on
	// needing a control
	LLRadioGroup(const LLString& name, const LLRect& rect,
				 S32 initial_index,
				 LLUICtrlCallback callback = NULL,
				 void* userdata = NULL,
				 BOOL border = TRUE);

	virtual ~LLRadioGroup();
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_RADIO_GROUP; }
	virtual LLString getWidgetTag() const { return LL_RADIO_GROUP_TAG; }

	virtual BOOL handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);

	virtual void setEnabled(BOOL enabled);
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	void setIndexEnabled(S32 index, BOOL enabled);
	
	// return the index value of the selected item
	S32 getSelectedIndex() const;
	
	// set the index value programatically
	BOOL setSelectedIndex(S32 index, BOOL from_event = FALSE);

	// Accept and retrieve strings of the radio group control names
	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;

	// Draw the group, but also fix the highlighting based on the
	// control.
	void draw();

	// You must use this method to add buttons to a radio group.
	// Don't use addChild -- it won't set the callback function
	// correctly.
	LLRadioCtrl* addRadioButton(const LLString& name, const LLString& label, const LLRect& rect, const LLFontGL* font);
	LLRadioCtrl* getRadioButton(const S32& index) { return mRadioButtons[index]; }
	// Update the control as needed.  Userdata must be a pointer to the
	// button.
	static void onClickButton(LLUICtrl* radio, void* userdata);
	
	//========================================================================
	LLCtrlSelectionInterface* getSelectionInterface()	{ return (LLCtrlSelectionInterface*)this; };

	// LLCtrlSelectionInterface functions
	/*virtual*/ S32		getItemCount() const  				{ return mRadioButtons.size(); }
	/*virtual*/ BOOL	getCanSelect() const				{ return TRUE; }
	/*virtual*/ BOOL	selectFirstItem()					{ return setSelectedIndex(0); }
	/*virtual*/ BOOL	selectNthItem( S32 index )			{ return setSelectedIndex(index); }
	/*virtual*/ S32		getFirstSelectedIndex()				{ return getSelectedIndex(); }
	/*virtual*/ BOOL	setCurrentByID( const LLUUID& id );
	/*virtual*/ LLUUID	getCurrentID();				// LLUUID::null if no items in menu
	/*virtual*/ BOOL	setSelectedByValue(LLSD value, BOOL selected);
	/*virtual*/ LLSD	getSimpleSelectedValue();
	/*virtual*/ BOOL	isSelected(LLSD value);
	/*virtual*/ BOOL	operateOnSelection(EOperation op);
	/*virtual*/ BOOL	operateOnAll(EOperation op);

protected:
	// protected function shared by the two constructors.
	void init(BOOL border);

	S32 mSelectedIndex;
	typedef std::vector<LLRadioCtrl*> button_list_t;
	button_list_t mRadioButtons;

	BOOL mHasBorder;
};


#endif
