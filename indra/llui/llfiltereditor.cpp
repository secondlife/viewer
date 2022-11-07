/** 
 * @file llfiltereditor.cpp
 * @brief LLFilterEditor implementation
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

// Text editor widget to let users enter a single line.

#include "linden_common.h"
 
#include "llfiltereditor.h"

LLFilterEditor::LLFilterEditor(const LLFilterEditor::Params& p)
:   LLSearchEditor(p)
{
    setCommitOnFocusLost(FALSE); // we'll commit on every keystroke, don't re-commit when we take focus away (i.e. we go to interact with the actual results!)
}


void LLFilterEditor::handleKeystroke()
{
    this->LLSearchEditor::handleKeystroke();

    // Commit on every keystroke.
    onCommit();
}
