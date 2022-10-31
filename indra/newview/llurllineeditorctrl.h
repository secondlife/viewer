/** 
 * @file llurllineeditorctrl.h
 * @brief Combobox-like location input control
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

#ifndef LLURLLINEEDITOR_H_
#define LLURLLINEEDITOR_H_

#include "linden_common.h"

#include "lllineeditor.h"
#include "lluictrl.h"

// LLURLLineEditor class performing escaping of an URL while copying or cutting the target text
class LLURLLineEditor: public LLLineEditor {
    LOG_CLASS( LLURLLineEditor);

public:
    // LLLineEditor overrides to do necessary escaping
    /*virtual*/     void copy();
    /*virtual*/     void cut();

protected:
    LLURLLineEditor(const Params&);
    friend class LLUICtrlFactory;
    friend class LLFloaterEditUI;

private:
    // util function to escape selected text and copy it to clipboard
    void            copyEscapedURLToClipboard();

    // Helper class to do rollback if needed
    class LLURLLineEditorRollback
    {
    public:
        LLURLLineEditorRollback( LLURLLineEditor* ed )
            :
            mCursorPos( ed->mCursorPos ),
            mScrollHPos( ed->mScrollHPos ),
            mIsSelecting( ed->mIsSelecting ),
            mSelectionStart( ed->mSelectionStart ),
            mSelectionEnd( ed->mSelectionEnd )
        {
            mText = ed->getText();
        }

        void doRollback( LLURLLineEditor* ed )
        {
            ed->mCursorPos = mCursorPos;
            ed->mScrollHPos = mScrollHPos;
            ed->mIsSelecting = mIsSelecting;
            ed->mSelectionStart = mSelectionStart;
            ed->mSelectionEnd = mSelectionEnd;
            ed->mText = mText;
            ed->mPrevText = mText;
        }

        std::string getText()   { return mText; }

    private:
        std::string mText;
        S32     mCursorPos;
        S32     mScrollHPos;
        BOOL    mIsSelecting;
        S32     mSelectionStart;
        S32     mSelectionEnd;
    }; // end class LLURLLineEditorRollback

};

#endif /* LLURLLINEEDITOR_H_ */
