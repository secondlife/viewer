/** 
 * @file llcheckboxctrl.h
 * @brief LLCheckBoxCtrl base class
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
	const LLFontGL*		getFont() { return mFont; }
	
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

// Build time optimization, generate once in .cpp file
#ifndef LLCHECKBOXCTRL_CPP
extern template class LLCheckBoxCtrl* LLView::getChild<class LLCheckBoxCtrl>(
	const std::string& name, BOOL recurse) const;
#endif

#endif  // LL_LLCHECKBOXCTRL_H
