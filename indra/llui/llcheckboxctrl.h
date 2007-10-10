/** 
 * @file llcheckboxctrl.h
 * @brief LLCheckBoxCtrl base class
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

#ifndef LL_LLCHECKBOXCTRL_H
#define LL_LLCHECKBOXCTRL_H


#include "stdtypes.h"
#include "lluictrl.h"
#include "llbutton.h"
#include "v4color.h"
#include "llrect.h"

//
// Constants
//
const S32 LLCHECKBOXCTRL_BTN_SIZE = 13;
const S32 LLCHECKBOXCTRL_VPAD = 2;
const S32 LLCHECKBOXCTRL_HPAD = 2;
const S32 LLCHECKBOXCTRL_SPACING = 5;
const S32 LLCHECKBOXCTRL_HEIGHT = 16;

// Deprecated, don't use.
#define CHECKBOXCTRL_HEIGHT LLCHECKBOXCTRL_HEIGHT

const BOOL	RADIO_STYLE = TRUE;
const BOOL	CHECK_STYLE = FALSE;

//
// Classes
//
class LLFontGL;
class LLTextBox;
class LLViewBorder;

class LLCheckBoxCtrl
: public LLUICtrl
{
public:
	LLCheckBoxCtrl(const LLString& name, const LLRect& rect, const LLString& label,	
		const LLFontGL* font = NULL,
		void (*commit_callback)(LLUICtrl*, void*) = NULL,
		void* callback_userdata = NULL,
		BOOL initial_value = FALSE,
		BOOL use_radio_style = FALSE, // if true, draw radio button style icons
		const LLString& control_which = LLString::null);
	virtual ~LLCheckBoxCtrl();

	// LLView interface
	virtual EWidgetType getWidgetType() const	{ return WIDGET_TYPE_CHECKBOX; }
	virtual LLString getWidgetTag() const { return LL_CHECK_BOX_CTRL_TAG; }
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	virtual void		setEnabled( BOOL b );

	virtual void		draw();
	virtual void		reshape(S32 width, S32 height, BOOL called_from_parent);

	// LLUICtrl interface
	virtual void		setValue(const LLSD& value );
	virtual LLSD		getValue() const;
			BOOL		get() { return (BOOL)getValue().asBoolean(); }
			void		set(BOOL value) { setValue(value); }

	virtual void		setTentative(BOOL b)	{ mButton->setTentative(b); }
	virtual BOOL		getTentative() const	{ return mButton->getTentative(); }

	virtual BOOL		setLabelArg( const LLString& key, const LLStringExplicit& text );

	virtual void		clear();
	virtual void		onCommit();

	// LLCheckBoxCtrl interface
	virtual BOOL		toggle()				{ return mButton->toggleState(); }		// returns new state

	void				setEnabledColor( const LLColor4 &color ) { mTextEnabledColor = color; }
	void				setDisabledColor( const LLColor4 &color ) { mTextDisabledColor = color; }

	void				setLabel( const LLStringExplicit& label );
	LLString			getLabel() const;

	virtual void		setControlName(const LLString& control_name, LLView* context);
	virtual LLString 	getControlName() const;

	static void			onButtonPress(void *userdata);

	virtual BOOL		isDirty()	const;		// Returns TRUE if the user has modified this control.
	virtual void		resetDirty();			// Clear dirty state

protected:
	// note: value is stored in toggle state of button
	LLButton*		mButton;
	LLTextBox*		mLabel;
	const LLFontGL* mFont;
	LLColor4		mTextEnabledColor;
	LLColor4		mTextDisabledColor;
	BOOL			mRadioStyle;
	BOOL			mInitialValue;			// Value set in constructor
	BOOL			mSetValue;				// Value set programmatically
	BOOL			mKeyboardFocusOnClick;
	LLViewBorder*	mBorder;
};


// HACK: fix old capitalization problem
//typedef LLCheckBoxCtrl LLCheckboxCtrl;
#define LLCheckboxCtrl LLCheckBoxCtrl


#endif  // LL_LLCHECKBOXCTRL_H
