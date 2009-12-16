/** 
 * @file llradiogroup.h
 * @brief LLRadioGroup base class
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

#ifndef LL_LLRADIOGROUP_H
#define LL_LLRADIOGROUP_H

#include "lluictrl.h"
#include "llcheckboxctrl.h"
#include "llctrlselectioninterface.h"

/*
 * An invisible view containing multiple mutually exclusive toggling 
 * buttons (usually radio buttons).  Automatically handles the mutex
 * condition by highlighting only one button at a time.
 */
class LLRadioGroup
:	public LLUICtrl, public LLCtrlSelectionInterface
{
public:

	struct ItemParams : public LLInitParam::Block<ItemParams, LLCheckBoxCtrl::Params>
	{
		Optional<LLSD>	value;
		ItemParams();
	};

	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<bool>						has_border;
		Multiple<ItemParams, AtLeast<1> >	items;
		Params();
	};

protected:
	LLRadioGroup(const Params&);
	friend class LLUICtrlFactory;

public:

	/*virtual*/ void initFromParams(const Params&);

	virtual ~LLRadioGroup();
	
	virtual BOOL postBuild();
	
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	
	virtual BOOL handleKeyHere(KEY key, MASK mask);

	void setIndexEnabled(S32 index, BOOL enabled);
	// return the index value of the selected item
	S32 getSelectedIndex() const { return mSelectedIndex; }
	
	// set the index value programatically
	BOOL setSelectedIndex(S32 index, BOOL from_event = FALSE);

	// Accept and retrieve strings of the radio group control names
	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;

	// Update the control as needed.  Userdata must be a pointer to the button.
	void onClickButton(LLUICtrl* clicked_radio);
	
	//========================================================================
	LLCtrlSelectionInterface* getSelectionInterface()	{ return (LLCtrlSelectionInterface*)this; };

	// LLCtrlSelectionInterface functions
	/*virtual*/ S32		getItemCount() const  				{ return mRadioButtons.size(); }
	/*virtual*/ BOOL	getCanSelect() const				{ return TRUE; }
	/*virtual*/ BOOL	selectFirstItem()					{ return setSelectedIndex(0); }
	/*virtual*/ BOOL	selectNthItem( S32 index )			{ return setSelectedIndex(index); }
	/*virtual*/ BOOL	selectItemRange( S32 first, S32 last ) { return setSelectedIndex(first); }
	/*virtual*/ S32		getFirstSelectedIndex() const		{ return getSelectedIndex(); }
	/*virtual*/ BOOL	setCurrentByID( const LLUUID& id );
	/*virtual*/ LLUUID	getCurrentID() const;				// LLUUID::null if no items in menu
	/*virtual*/ BOOL	setSelectedByValue(const LLSD& value, BOOL selected);
	/*virtual*/ LLSD	getSelectedValue();
	/*virtual*/ BOOL	isSelected(const LLSD& value) const;
	/*virtual*/ BOOL	operateOnSelection(EOperation op);
	/*virtual*/ BOOL	operateOnAll(EOperation op);

private:
	const LLFontGL* mFont;
	S32 mSelectedIndex;
	typedef std::vector<class LLRadioCtrl*> button_list_t;
	button_list_t mRadioButtons;

	BOOL mHasBorder;
};

#endif
