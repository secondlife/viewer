/**
 * @author This module has many fathers, and it shows.
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

#ifndef LL_LLKEYBOARDSDL2_H
#define LL_LLKEYBOARDSDL2_H

#include "llkeyboard.h"
#include "SDL2/SDL.h"

class LLKeyboardSDL : public LLKeyboard
{
public:
    LLKeyboardSDL();
    /*virtual*/ ~LLKeyboardSDL() {};

    /*virtual*/ BOOL	handleKeyUp(const U32 key, MASK mask);
    /*virtual*/ BOOL	handleKeyDown(const U32 key, MASK mask);
    /*virtual*/ void	resetMaskKeys();
    /*virtual*/ MASK	currentMask(BOOL for_mouse_event);
    /*virtual*/ void	scanKeyboard();

protected:
    MASK	updateModifiers(const U32 mask);
    void	setModifierKeyLevel( KEY key, BOOL new_state );
    BOOL	translateNumpadKey( const U32 os_key, KEY *translated_key );
    U16	inverseTranslateNumpadKey(const KEY translated_key);
private:
    std::map<U32, KEY> mTranslateNumpadMap;  // special map for translating OS keys to numpad keys
    std::map<KEY, U32> mInvTranslateNumpadMap; // inverse of the above

public:
    static U32 mapSDL2toWin( U32 );
};

#endif
