/** 
 * @file llchatmsgbox.h
 * @author Martin Reddy
 * @brief chat history text box, able to show array of strings with separator
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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
	void				addText(const LLStringExplicit &text);
	
private:
	S32					mBlockSpacing;
};

#endif

