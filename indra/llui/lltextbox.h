/** 
 * @file lltextbox.h
 * @brief A single text item display
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

#ifndef LL_LLTEXTBOX_H
#define LL_LLTEXTBOX_H

#include "v4color.h"
#include "llstring.h"
#include "lluistring.h"
#include "lltextbase.h"

class LLTextBox :
	public LLTextBase
{
public:
	
	// *TODO: Add callback to Params
	typedef boost::function<void (void)> callback_t;
	
	struct Params : public LLInitParam::Block<Params, LLTextBase::Params>
	{};

protected:
	LLTextBox(const Params&);
	friend class LLUICtrlFactory;

public:
	virtual ~LLTextBox() {}

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);

	/*virtual*/ void			setText( const LLStringExplicit& text );
	
	void			setRightAlign()							{ mHAlign = LLFontGL::RIGHT; }
	void			setHAlign( LLFontGL::HAlign align )		{ mHAlign = align; }
	void			setClickedCallback( boost::function<void (void*)> cb, void* userdata = NULL ){ mClickedCallback = boost::bind(cb, userdata); }		// mouse down and up within button

	//const LLFontGL* getFont() const							{ return mDefaultFont; }
	//void			setFont(const LLFontGL* font)			{ mDefaultFont = font; }

	void			reshapeToFitText();

	//const std::string&	getText() const							{ return mText.getString(); }
	S32				getTextPixelWidth();
	S32				getTextPixelHeight();

	virtual LLSD	getValue() const						{ return LLSD(getText()); }
	virtual BOOL	setTextArg( const std::string& key, const LLStringExplicit& text );

protected:
	void            onUrlLabelUpdated(const std::string &url, const std::string &label);

	LLUIString			mText;
	callback_t			mClickedCallback;
};

#endif
