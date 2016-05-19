/** 
 * @file llchatmsgbox.h
 * @author Martin Reddy
 * @brief chat history text box, able to show array of strings with separator
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLCHATMSGBOX_H
#define LL_LLCHATMSGBOX_H

#include "lltextbox.h"
#include "lluictrl.h"
#include "v4color.h"
#include "llstring.h"

///
/// LLChatMsgBox provides a text box with support for multiple blocks
/// of text that can be added incrementally. Each block of text is
/// visual separated from the previous block (e.g., with a horizontal
/// line).
///
class LLChatMsgBox :
	public LLTextBox
{
public:
	struct Params : public LLInitParam::Block<Params, LLTextBox::Params>
	{
		Optional<S32>	block_spacing;

		Params();
	};

protected:
	LLChatMsgBox(const Params&);
	friend class LLUICtrlFactory;

public:
	void				addText(const LLStringExplicit &text, const LLStyle::Params& input_params = LLStyle::Params());
	
private:
	S32					mBlockSpacing;
};

#endif

