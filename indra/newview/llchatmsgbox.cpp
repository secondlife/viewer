/** 
 * @file llchatmsgbox.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llchatmsgbox.h"
#include "llwindow.h"


static LLDefaultChildRegistry::Register<LLChatMsgBox> r("text_chat");

class ChatSeparator : public LLTextSegment
{
public:
    ChatSeparator(S32 start, S32 end)
    :   LLTextSegment(start, end),
        mEditor(NULL)
    {}

    /*virtual*/ void linkToDocument(class LLTextBase* editor)
    {
        mEditor = editor;
    }

    /*virtual*/ void unlinkFromDocument(class LLTextBase* editor)
    {
        mEditor = NULL;
    }

    /*virtual*/ S32 getWidth(S32 first_char, S32 num_chars) const
    {
        return mEditor->getDocumentView()->getRect().getWidth();
    }

    /*virtual*/ F32 draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRectf& draw_rect)
    {
        gl_line_2d((S32)(draw_rect.mLeft + 5), (S32)draw_rect.getCenterY(), (S32)(draw_rect.mRight - 5), (S32)draw_rect.getCenterY(), LLColor4::grey);
        return draw_rect.getWidth();
    }

private:
    LLTextBase* mEditor;
};


LLChatMsgBox::Params::Params() :
    block_spacing("block_spacing", 10)
{
    changeDefault(line_spacing.pixels, 4);
}

LLChatMsgBox::LLChatMsgBox(const Params& p) :
    LLTextBox(p),
    mBlockSpacing(p.block_spacing)
{}

void LLChatMsgBox::addText( const LLStringExplicit& text , const LLStyle::Params& input_params )
{
    S32 length = getLength();
    // if there is existing text, add a separator
    if (length > 0)
    {
        // chat separator exists right before the null terminator
        insertSegment(new ChatSeparator(length - 1, length - 1));
    }
    // prepend newline only if there is some existing text
    appendText(text, length > 0, input_params);
}
