/** 
 * @file llcheckboxctrl.h
 * @brief LLCheckBoxCtrl base class
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

#ifndef LL_LLCHECKBOXCTRL_H
#define LL_LLCHECKBOXCTRL_H

#include "lluictrl.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "v4color.h"

//
// Constants
//

const BOOL	RADIO_STYLE = TRUE;
const BOOL	CHECK_STYLE = FALSE;

//
// Classes
//
class LLFontGL;
class LLViewBorder;

class LLCheckBoxCtrl
: public LLUICtrl
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLUIColor>		text_enabled_color;
		Optional<LLUIColor>		text_disabled_color;
		Optional<bool>			initial_value;	// override LLUICtrl initial_value

		Optional<LLTextBox::Params> label_text;
		Optional<LLButton::Params> check_button;

		Ignored					radio_style;

		Params();
	};

	virtual ~LLCheckBoxCtrl();

protected:
	LLCheckBoxCtrl(const Params&);
	friend class LLUICtrlFactory;

public:
	// LLView interface

	virtual void		setEnabled( BOOL b );

	virtual void		reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	// LLUICtrl interface
	virtual void		setValue(const LLSD& value );
	virtual LLSD		getValue() const;
			BOOL		get() { return (BOOL)getValue().asBoolean(); }
			void		set(BOOL value) { setValue(value); }

	virtual void		setTentative(BOOL b);
	virtual BOOL		getTentative() const;

	virtual BOOL		setLabelArg( const std::string& key, const LLStringExplicit& text );

	virtual void		clear();
	virtual void		onCommit();

	// LLCheckBoxCtrl interface
	virtual BOOL		toggle()				{ return mButton->toggleState(); }		// returns new state

	void				setEnabledColor( const LLColor4 &color ) { mTextEnabledColor = color; }
	void				setDisabledColor( const LLColor4 &color ) { mTextDisabledColor = color; }

	void				setLabel( const LLStringExplicit& label );
	std::string			getLabel() const;

	void				setFont( const LLFontGL* font ) { mFont = font; }
	
	virtual void		setControlName(const std::string& control_name, LLView* context);

	void				onButtonPress(const LLSD& data);

	virtual BOOL		isDirty()	const;		// Returns TRUE if the user has modified this control.
	virtual void		resetDirty();			// Clear dirty state

protected:
	// note: value is stored in toggle state of button
	LLButton*		mButton;
	LLTextBox*		mLabel;
	const LLFontGL* mFont;

	LLUIColor		mTextEnabledColor;
	LLUIColor		mTextDisabledColor;
};


#ifdef LL_WINDOWS
#ifndef INSTANTIATE_GETCHILD_CHECKBOX
#pragma warning (disable : 4231)
extern template LLCheckBoxCtrl* LLView::getChild<LLCheckBoxCtrl>( const std::string& name, BOOL recurse, BOOL create_if_missing ) const;
#endif
#endif

#endif  // LL_LLCHECKBOXCTRL_H
