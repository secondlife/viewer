/** 
 * @file llviewergesture.h
 * @brief LLViewerGesture class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLVIEWERGESTURE_H
#define LL_LLVIEWERGESTURE_H

#include "llanimationstates.h"
#include "lluuid.h"
#include "llstring.h"
#include "llgesture.h"

class LLMessageSystem;

class LLViewerGesture : public LLGesture
{
public:
    LLViewerGesture();
    LLViewerGesture(KEY key, MASK mask, const std::string &trigger, 
        const LLUUID &sound_item_id, const std::string &animation, 
        const std::string &output_string);

    LLViewerGesture(U8 **buffer, S32 max_size); // deserializes, advances buffer
    LLViewerGesture(const LLViewerGesture &gesture);

    // Triggers if a key/mask matches it
    virtual BOOL trigger(KEY key, MASK mask);

    // Triggers if case-insensitive substring matches (assumes string is lowercase)
    virtual BOOL trigger(const std::string &string);

    void doTrigger( BOOL send_chat );

protected:
    static const F32    SOUND_VOLUME;
};

class LLViewerGestureList : public LLGestureList
{
public:
    LLViewerGestureList();

    //void requestFromServer();
    BOOL getIsLoaded() { return mIsLoaded; }

    //void requestResetFromServer( BOOL is_male );

    // See if the prefix matches any gesture.  If so, return TRUE
    // and place the full text of the gesture trigger into
    // output_str
    BOOL matchPrefix(const std::string& in_str, std::string* out_str);

    static void xferCallback(void *data, S32 size, void** /*user_data*/, S32 status);

protected:
    LLGesture *create_gesture(U8 **buffer, S32 max_size);

protected:
    BOOL mIsLoaded;
};

extern LLViewerGestureList gGestureList;

#endif
