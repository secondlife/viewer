/** 
 * @file llkeyboardwin32.h
 * @brief Handler for assignable key bindings
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLKEYBOARDWIN32_H
#define LL_LLKEYBOARDWIN32_H

#include "llkeyboard.h"

// this mask distinguishes extended keys, which include non-numpad arrow keys 
// (and, curiously, the num lock and numpad '/')
const MASK MASK_EXTENDED =  0x0100;

class LLKeyboardWin32 : public LLKeyboard
{
public:
    LLKeyboardWin32();
    /*virtual*/ ~LLKeyboardWin32() {};

    /*virtual*/ BOOL    handleKeyUp(const U16 key, MASK mask);
    /*virtual*/ BOOL    handleKeyDown(const U16 key, MASK mask);
    /*virtual*/ void    resetMaskKeys();
    /*virtual*/ MASK    currentMask(BOOL for_mouse_event);
    /*virtual*/ void    scanKeyboard();
    BOOL                translateExtendedKey(const U16 os_key, const MASK mask, KEY *translated_key);
    U16                 inverseTranslateExtendedKey(const KEY translated_key);

protected:
    MASK    updateModifiers();
    //void  setModifierKeyLevel( KEY key, BOOL new_state );
private:
    std::map<U16, KEY> mTranslateNumpadMap;
    std::map<KEY, U16> mInvTranslateNumpadMap;
};

#endif
