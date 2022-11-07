/** 
 * @file llkeyboardmacosx.h
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

#ifndef LL_LLKEYBOARDMACOSX_H
#define LL_LLKEYBOARDMACOSX_H

#include "llkeyboard.h"

// These more or less mirror their equivalents in NSEvent.h.
enum EMacEventKeys {
    MAC_SHIFT_KEY = 1 << 17,
    MAC_CTRL_KEY = 1 << 18,
    MAC_ALT_KEY = 1 << 19,
    MAC_CMD_KEY = 1 << 20,
    MAC_FN_KEY = 1 << 23
};

class LLKeyboardMacOSX : public LLKeyboard
{
public:
    LLKeyboardMacOSX();
    /*virtual*/ ~LLKeyboardMacOSX() {};
    
    /*virtual*/ BOOL    handleKeyUp(const U16 key, MASK mask);
    /*virtual*/ BOOL    handleKeyDown(const U16 key, MASK mask);
    /*virtual*/ void    resetMaskKeys();
    /*virtual*/ MASK    currentMask(BOOL for_mouse_event);
    /*virtual*/ void    scanKeyboard();
    /*virtual*/ void    handleModifier(MASK mask);
    
protected:
    MASK    updateModifiers(const U32 mask);
    void    setModifierKeyLevel( KEY key, BOOL new_state );
    BOOL    translateNumpadKey( const U16 os_key, KEY *translated_key );
    U16     inverseTranslateNumpadKey(const KEY translated_key);
private:
    std::map<U16, KEY> mTranslateNumpadMap;  // special map for translating OS keys to numpad keys
    std::map<KEY, U16> mInvTranslateNumpadMap; // inverse of the above
};

#endif
