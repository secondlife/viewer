/** 
 * @file llviewchildren.h
 * @brief LLViewChildren class header file
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLVIEWCHILDREN_H
#define LL_LLVIEWCHILDREN_H

class LLPanel;

class LLViewChildren
    // makes it easy to manipulate children of a view by id safely
    // encapsulates common operations into simple, one line calls
{
public:
    LLViewChildren(LLPanel& parent);
    
    // all views
    void show(const std::string& id, bool visible = true);
    void hide(const std::string& id) { show(id, false); }

    void enable(const std::string& id, bool enabled = true);
    void disable(const std::string& id) { enable(id, false); };

    //
    // LLTextBox
    void setText(const std::string& id,
        const std::string& text, bool visible = true);

    // LLIconCtrl
    enum Badge { BADGE_OK, BADGE_NOTE, BADGE_WARN, BADGE_ERROR };
    
    void setBadge(const std::string& id, Badge b, bool visible = true);

    
    // LLButton
    void setAction(const std::string& id, void(*function)(void*), void* value);


private:
    LLPanel& mParent;
};

#endif
