/** 
 * @file llkeyboardmacosx.cpp
 * @brief Handler for assignable key bindings
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

#if LL_DARWIN

#include "linden_common.h"
#include "llkeyboardmacosx.h"
#include "llwindowcallbacks.h"

#include <Carbon/Carbon.h>

LLKeyboardMacOSX::LLKeyboardMacOSX()
{
	// Virtual keycode mapping table.  Yes, this was as annoying to generate as it looks.
	mTranslateKeyMap[0x00] = 'A';
	mTranslateKeyMap[0x01] = 'S';
	mTranslateKeyMap[0x02] = 'D';
	mTranslateKeyMap[0x03] = 'F';
	mTranslateKeyMap[0x04] = 'H';
	mTranslateKeyMap[0x05] = 'G';
	mTranslateKeyMap[0x06] = 'Z';
	mTranslateKeyMap[0x07] = 'X';
	mTranslateKeyMap[0x08] = 'C';
	mTranslateKeyMap[0x09] = 'V';
	mTranslateKeyMap[0x0b] = 'B';
	mTranslateKeyMap[0x0c] = 'Q';
	mTranslateKeyMap[0x0d] = 'W';
	mTranslateKeyMap[0x0e] = 'E';
	mTranslateKeyMap[0x0f] = 'R';
	mTranslateKeyMap[0x10] = 'Y';
	mTranslateKeyMap[0x11] = 'T';
	mTranslateKeyMap[0x12] = '1';
	mTranslateKeyMap[0x13] = '2';
	mTranslateKeyMap[0x14] = '3';
	mTranslateKeyMap[0x15] = '4';
	mTranslateKeyMap[0x16] = '6';
	mTranslateKeyMap[0x17] = '5';
	mTranslateKeyMap[0x18] = '=';	// KEY_EQUALS
	mTranslateKeyMap[0x19] = '9';
	mTranslateKeyMap[0x1a] = '7';
	mTranslateKeyMap[0x1b] = '-';	// KEY_HYPHEN
	mTranslateKeyMap[0x1c] = '8';
	mTranslateKeyMap[0x1d] = '0';
	mTranslateKeyMap[0x1e] = ']';
	mTranslateKeyMap[0x1f] = 'O';
	mTranslateKeyMap[0x20] = 'U';
	mTranslateKeyMap[0x21] = '[';
	mTranslateKeyMap[0x22] = 'I';
	mTranslateKeyMap[0x23] = 'P';
	mTranslateKeyMap[0x24] = KEY_RETURN,
	mTranslateKeyMap[0x25] = 'L';
	mTranslateKeyMap[0x26] = 'J';
	mTranslateKeyMap[0x27] = '\'';
	mTranslateKeyMap[0x28] = 'K';
	mTranslateKeyMap[0x29] = ';';
	mTranslateKeyMap[0x2a] = '\\';
	mTranslateKeyMap[0x2b] = ',';
	mTranslateKeyMap[0x2c] = KEY_DIVIDE;
	mTranslateKeyMap[0x2d] = 'N';
	mTranslateKeyMap[0x2e] = 'M';
	mTranslateKeyMap[0x2f] = '.';
	mTranslateKeyMap[0x30] = KEY_TAB;
	mTranslateKeyMap[0x31] = ' ';	// space!
	mTranslateKeyMap[0x32] = '`';
	mTranslateKeyMap[0x33] = KEY_BACKSPACE;
	mTranslateKeyMap[0x35] = KEY_ESCAPE;
	//mTranslateKeyMap[0x37] = 0;	// Command key.  (not used yet)
	mTranslateKeyMap[0x38] = KEY_SHIFT;
	mTranslateKeyMap[0x39] = KEY_CAPSLOCK;
	mTranslateKeyMap[0x3a] = KEY_ALT;
	mTranslateKeyMap[0x3b] = KEY_CONTROL;
	mTranslateKeyMap[0x41] = '.';	// keypad
	mTranslateKeyMap[0x43] = '*';	// keypad
	mTranslateKeyMap[0x45] = '+';	// keypad
	mTranslateKeyMap[0x4b] = KEY_PAD_DIVIDE;	// keypad
	mTranslateKeyMap[0x4c] = KEY_RETURN;	// keypad enter
	mTranslateKeyMap[0x4e] = '-';	// keypad
	mTranslateKeyMap[0x51] = '=';	// keypad
	mTranslateKeyMap[0x52] = '0';	// keypad
	mTranslateKeyMap[0x53] = '1';	// keypad
	mTranslateKeyMap[0x54] = '2';	// keypad
	mTranslateKeyMap[0x55] = '3';	// keypad
	mTranslateKeyMap[0x56] = '4';	// keypad
	mTranslateKeyMap[0x57] = '5';	// keypad
	mTranslateKeyMap[0x58] = '6';	// keypad
	mTranslateKeyMap[0x59] = '7';	// keypad
	mTranslateKeyMap[0x5b] = '8';	// keypad
	mTranslateKeyMap[0x5c] = '9';	// keypad
	mTranslateKeyMap[0x60] = KEY_F5;
	mTranslateKeyMap[0x61] = KEY_F6;
	mTranslateKeyMap[0x62] = KEY_F7;
	mTranslateKeyMap[0x63] = KEY_F3;
	mTranslateKeyMap[0x64] = KEY_F8;
	mTranslateKeyMap[0x65] = KEY_F9;
	mTranslateKeyMap[0x67] = KEY_F11;
	mTranslateKeyMap[0x6d] = KEY_F10;
	mTranslateKeyMap[0x6f] = KEY_F12;
	mTranslateKeyMap[0x72] = KEY_INSERT;
	mTranslateKeyMap[0x73] = KEY_HOME;
	mTranslateKeyMap[0x74] = KEY_PAGE_UP;
	mTranslateKeyMap[0x75] = KEY_DELETE;
	mTranslateKeyMap[0x76] = KEY_F4;
	mTranslateKeyMap[0x77] = KEY_END;
	mTranslateKeyMap[0x78] = KEY_F2;
	mTranslateKeyMap[0x79] = KEY_PAGE_DOWN;
	mTranslateKeyMap[0x7a] = KEY_F1;
	mTranslateKeyMap[0x7b] = KEY_LEFT;
	mTranslateKeyMap[0x7c] = KEY_RIGHT;
	mTranslateKeyMap[0x7d] = KEY_DOWN;
	mTranslateKeyMap[0x7e] = KEY_UP;

	// Build inverse map
	std::map<U16, KEY>::iterator iter;
	for (iter = mTranslateKeyMap.begin(); iter != mTranslateKeyMap.end(); iter++)
	{
		mInvTranslateKeyMap[iter->second] = iter->first;
	}
	
	// build numpad maps
	mTranslateNumpadMap[0x52] = KEY_PAD_INS;    // keypad 0
	mTranslateNumpadMap[0x53] = KEY_PAD_END;   // keypad 1
	mTranslateNumpadMap[0x54] = KEY_PAD_DOWN;	// keypad 2
	mTranslateNumpadMap[0x55] = KEY_PAD_PGDN;	// keypad 3
	mTranslateNumpadMap[0x56] = KEY_PAD_LEFT;	// keypad 4
	mTranslateNumpadMap[0x57] = KEY_PAD_CENTER;	// keypad 5
	mTranslateNumpadMap[0x58] = KEY_PAD_RIGHT;	// keypad 6
	mTranslateNumpadMap[0x59] = KEY_PAD_HOME;	// keypad 7
	mTranslateNumpadMap[0x5b] = KEY_PAD_UP;		// keypad 8
	mTranslateNumpadMap[0x5c] = KEY_PAD_PGUP;	// keypad 9
	mTranslateNumpadMap[0x41] = KEY_PAD_DEL;	// keypad .
	mTranslateNumpadMap[0x4c] = KEY_PAD_RETURN;	// keypad enter
	
	// Build inverse numpad map
	for (iter = mTranslateNumpadMap.begin(); iter != mTranslateNumpadMap.end(); iter++)
	{
		mInvTranslateNumpadMap[iter->second] = iter->first;
	}
}

void LLKeyboardMacOSX::resetMaskKeys()
{
	U32 mask = GetCurrentEventKeyModifiers();

	// MBW -- XXX -- This mirrors the operation of the Windows version of resetMaskKeys().
	//    It looks a bit suspicious, as it won't correct for keys that have been released.
	//    Is this the way it's supposed to work?

	if(mask & shiftKey)
	{
		mKeyLevel[KEY_SHIFT] = TRUE;
	}

	if(mask & (controlKey))
	{
		mKeyLevel[KEY_CONTROL] = TRUE;
	}

	if(mask & optionKey)
	{
		mKeyLevel[KEY_ALT] = TRUE;
	}
}

/*
static BOOL translateKeyMac(const U16 key, const U32 mask, KEY &outKey, U32 &outMask)
{
	// Translate the virtual keycode into the keycodes the keyboard system expects.
	U16 virtualKey = (mask >> 24) & 0x0000007F;
	outKey = macKeyTransArray[virtualKey];


	return(outKey != 0);
}
*/

MASK LLKeyboardMacOSX::updateModifiers(const U32 mask)
{
	// translate the mask
	MASK out_mask = 0;

	if(mask & shiftKey)
	{
		out_mask |= MASK_SHIFT;
	}

	if(mask & (controlKey | cmdKey))
	{
		out_mask |= MASK_CONTROL;
	}

	if(mask & optionKey)
	{
		out_mask |= MASK_ALT;
	}

	return out_mask;
}

BOOL LLKeyboardMacOSX::handleKeyDown(const U16 key, const U32 mask)
{
	KEY		translated_key = 0;
	U32		translated_mask = 0;
	BOOL	handled = FALSE;

	translated_mask = updateModifiers(mask);

	if(translateNumpadKey(key, &translated_key))
	{
		handled = handleTranslatedKeyDown(translated_key, translated_mask);
	}

	return handled;
}


BOOL LLKeyboardMacOSX::handleKeyUp(const U16 key, const U32 mask)
{
	KEY		translated_key = 0;
	U32		translated_mask = 0;
	BOOL	handled = FALSE;

	translated_mask = updateModifiers(mask);

	if(translateNumpadKey(key, &translated_key))
	{
		handled = handleTranslatedKeyUp(translated_key, translated_mask);
	}

	return handled;
}

MASK LLKeyboardMacOSX::currentMask(BOOL for_mouse_event)
{
	MASK result = MASK_NONE;
	U32 mask = GetCurrentEventKeyModifiers();

	if (mask & shiftKey)			result |= MASK_SHIFT;
	if (mask & controlKey)			result |= MASK_CONTROL;
	if (mask & optionKey)			result |= MASK_ALT;

	// For keyboard events, consider Command equivalent to Control
	if (!for_mouse_event)
	{
		if (mask & cmdKey) result |= MASK_CONTROL;
	}

	return result;
}

void LLKeyboardMacOSX::scanKeyboard()
{
	S32 key;
	for (key = 0; key < KEY_COUNT; key++)
	{
		// Generate callback if any event has occurred on this key this frame.
		// Can't just test mKeyLevel, because this could be a slow frame and
		// key might have gone down then up. JC
		if (mKeyLevel[key] || mKeyDown[key] || mKeyUp[key])
		{
			mCurScanKey = key;
			mCallbacks->handleScanKey(key, mKeyDown[key], mKeyUp[key], mKeyLevel[key]);
		}
	}

	// Reset edges for next frame
	for (key = 0; key < KEY_COUNT; key++)
	{
		mKeyUp[key] = FALSE;
		mKeyDown[key] = FALSE;
		if (mKeyLevel[key])
		{
			mKeyLevelFrameCount[key]++;
		}
	}
}

BOOL LLKeyboardMacOSX::translateNumpadKey( const U16 os_key, KEY *translated_key )
{
	return translateKey(os_key, translated_key);
}

U16	LLKeyboardMacOSX::inverseTranslateNumpadKey(const KEY translated_key)
{
	return inverseTranslateKey(translated_key);
}

#endif // LL_DARWIN
