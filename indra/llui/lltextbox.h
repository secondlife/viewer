/** 
 * @file lltextbox.h
 * @brief A single text item display
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

#ifndef LL_LLTEXTBOX_H
#define LL_LLTEXTBOX_H

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
	virtual ~LLTextBox();

	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);

	/*virtual*/ void setEnabled(BOOL enabled);

	/*virtual*/ void setText( const LLStringExplicit& text, const LLStyle::Params& input_params = LLStyle::Params() );
	
	void			setRightAlign()							{ mHAlign = LLFontGL::RIGHT; }
	void			setHAlign( LLFontGL::HAlign align )		{ mHAlign = align; }
	void			setClickedCallback( boost::function<void (void*)> cb, void* userdata = NULL );

	void			reshapeToFitText();

	S32				getTextPixelWidth();
	S32				getTextPixelHeight();

	/*virtual*/ LLSD	getValue() const;
	/*virtual*/ BOOL	setTextArg( const std::string& key, const LLStringExplicit& text );

	void			setShowCursorHand(bool show_cursor) { mShowCursorHand = show_cursor; }

protected:
	void            onUrlLabelUpdated(const std::string &url, const std::string &label);

	LLUIString			mText;
	callback_t			mClickedCallback;
	bool				mShowCursorHand;
};

// Build time optimization, generate once in .cpp file
#ifndef LLTEXTBOX_CPP
extern template class LLTextBox* LLView::getChild<class LLTextBox>(
	const std::string& name, BOOL recurse) const;
#endif

#endif
