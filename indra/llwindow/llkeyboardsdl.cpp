/** 
 * @file llkeyboardsdl.cpp
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

#if LL_SDL

#include "linden_common.h"
#include "llkeyboardsdl.h"
#include "llwindowcallbacks.h"
#include "SDL/SDL.h"

LLKeyboardSDL::LLKeyboardSDL()
{
	// Set up key mapping for SDL - eventually can read this from a file?
	// Anything not in the key map gets dropped
	// Add default A-Z

	// Virtual key mappings from SDL_keysym.h ...

	// SDL maps the letter keys to the ASCII you'd expect, but it's lowercase...
	U16 cur_char;
	for (cur_char = 'A'; cur_char <= 'Z'; cur_char++)
	{
		mTranslateKeyMap[cur_char] = cur_char;
	}
	for (cur_char = 'a'; cur_char <= 'z'; cur_char++)
	{
		mTranslateKeyMap[cur_char] = (cur_char - 'a') + 'A';
	}

	for (cur_char = '0'; cur_char <= '9'; cur_char++)
	{
		mTranslateKeyMap[cur_char] = cur_char;
	}

	// These ones are translated manually upon keydown/keyup because
	// SDL doesn't handle their numlock transition.
	//mTranslateKeyMap[SDLK_KP4] = KEY_PAD_LEFT;
	//mTranslateKeyMap[SDLK_KP6] = KEY_PAD_RIGHT;
	//mTranslateKeyMap[SDLK_KP8] = KEY_PAD_UP;
	//mTranslateKeyMap[SDLK_KP2] = KEY_PAD_DOWN;
	//mTranslateKeyMap[SDLK_KP_PERIOD] = KEY_DELETE;
	//mTranslateKeyMap[SDLK_KP7] = KEY_HOME;
	//mTranslateKeyMap[SDLK_KP1] = KEY_END;
	//mTranslateKeyMap[SDLK_KP9] = KEY_PAGE_UP;
	//mTranslateKeyMap[SDLK_KP3] = KEY_PAGE_DOWN;
	//mTranslateKeyMap[SDLK_KP0] = KEY_INSERT;

	mTranslateKeyMap[SDLK_SPACE] = ' ';
	mTranslateKeyMap[SDLK_RETURN] = KEY_RETURN;
	mTranslateKeyMap[SDLK_LEFT] = KEY_LEFT;
	mTranslateKeyMap[SDLK_RIGHT] = KEY_RIGHT;
	mTranslateKeyMap[SDLK_UP] = KEY_UP;
	mTranslateKeyMap[SDLK_DOWN] = KEY_DOWN;
	mTranslateKeyMap[SDLK_ESCAPE] = KEY_ESCAPE;
	mTranslateKeyMap[SDLK_KP_ENTER] = KEY_RETURN;
	mTranslateKeyMap[SDLK_ESCAPE] = KEY_ESCAPE;
	mTranslateKeyMap[SDLK_BACKSPACE] = KEY_BACKSPACE;
	mTranslateKeyMap[SDLK_DELETE] = KEY_DELETE;
	mTranslateKeyMap[SDLK_LSHIFT] = KEY_SHIFT;
	mTranslateKeyMap[SDLK_RSHIFT] = KEY_SHIFT;
	mTranslateKeyMap[SDLK_LCTRL] = KEY_CONTROL;
	mTranslateKeyMap[SDLK_RCTRL] = KEY_CONTROL;
	mTranslateKeyMap[SDLK_LALT] = KEY_ALT;
	mTranslateKeyMap[SDLK_RALT] = KEY_ALT;
	mTranslateKeyMap[SDLK_HOME] = KEY_HOME;
	mTranslateKeyMap[SDLK_END] = KEY_END;
	mTranslateKeyMap[SDLK_PAGEUP] = KEY_PAGE_UP;
	mTranslateKeyMap[SDLK_PAGEDOWN] = KEY_PAGE_DOWN;
	mTranslateKeyMap[SDLK_MINUS] = KEY_HYPHEN;
	mTranslateKeyMap[SDLK_EQUALS] = KEY_EQUALS;
	mTranslateKeyMap[SDLK_KP_EQUALS] = KEY_EQUALS;
	mTranslateKeyMap[SDLK_INSERT] = KEY_INSERT;
	mTranslateKeyMap[SDLK_CAPSLOCK] = KEY_CAPSLOCK;
	mTranslateKeyMap[SDLK_TAB] = KEY_TAB;
	mTranslateKeyMap[SDLK_KP_PLUS] = KEY_ADD;
	mTranslateKeyMap[SDLK_KP_MINUS] = KEY_SUBTRACT;
	mTranslateKeyMap[SDLK_KP_MULTIPLY] = KEY_MULTIPLY;
	mTranslateKeyMap[SDLK_KP_DIVIDE] = KEY_PAD_DIVIDE;
	mTranslateKeyMap[SDLK_F1] = KEY_F1;
	mTranslateKeyMap[SDLK_F2] = KEY_F2;
	mTranslateKeyMap[SDLK_F3] = KEY_F3;
	mTranslateKeyMap[SDLK_F4] = KEY_F4;
	mTranslateKeyMap[SDLK_F5] = KEY_F5;
	mTranslateKeyMap[SDLK_F6] = KEY_F6;
	mTranslateKeyMap[SDLK_F7] = KEY_F7;
	mTranslateKeyMap[SDLK_F8] = KEY_F8;
	mTranslateKeyMap[SDLK_F9] = KEY_F9;
	mTranslateKeyMap[SDLK_F10] = KEY_F10;
	mTranslateKeyMap[SDLK_F11] = KEY_F11;
	mTranslateKeyMap[SDLK_F12] = KEY_F12;
	mTranslateKeyMap[SDLK_PLUS]   = '=';
	mTranslateKeyMap[SDLK_COMMA]  = ',';
	mTranslateKeyMap[SDLK_MINUS]  = '-';
	mTranslateKeyMap[SDLK_PERIOD] = '.';
	mTranslateKeyMap[SDLK_BACKQUOTE] = '`';
	mTranslateKeyMap[SDLK_SLASH] = KEY_DIVIDE;
	mTranslateKeyMap[SDLK_SEMICOLON] = ';';
	mTranslateKeyMap[SDLK_LEFTBRACKET] = '[';
	mTranslateKeyMap[SDLK_BACKSLASH] = '\\';
	mTranslateKeyMap[SDLK_RIGHTBRACKET] = ']';
	mTranslateKeyMap[SDLK_QUOTE] = '\'';

	// Build inverse map
	std::map<U16, KEY>::iterator iter;
	for (iter = mTranslateKeyMap.begin(); iter != mTranslateKeyMap.end(); iter++)
	{
		mInvTranslateKeyMap[iter->second] = iter->first;
	}

	// numpad map
	mTranslateNumpadMap[SDLK_KP0] = KEY_PAD_INS;
	mTranslateNumpadMap[SDLK_KP1] = KEY_PAD_END;
	mTranslateNumpadMap[SDLK_KP2] = KEY_PAD_DOWN;
	mTranslateNumpadMap[SDLK_KP3] = KEY_PAD_PGDN;
	mTranslateNumpadMap[SDLK_KP4] = KEY_PAD_LEFT;
	mTranslateNumpadMap[SDLK_KP5] = KEY_PAD_CENTER;
	mTranslateNumpadMap[SDLK_KP6] = KEY_PAD_RIGHT;
	mTranslateNumpadMap[SDLK_KP7] = KEY_PAD_HOME;
	mTranslateNumpadMap[SDLK_KP8] = KEY_PAD_UP;
	mTranslateNumpadMap[SDLK_KP9] = KEY_PAD_PGUP;
	mTranslateNumpadMap[SDLK_KP_PERIOD] = KEY_PAD_DEL;

	// build inverse numpad map
	for (iter = mTranslateNumpadMap.begin();
	     iter != mTranslateNumpadMap.end();
	     iter++)
	{
		mInvTranslateNumpadMap[iter->second] = iter->first;
	}
}

void LLKeyboardSDL::resetMaskKeys()
{
	SDLMod mask = SDL_GetModState();

	// MBW -- XXX -- This mirrors the operation of the Windows version of resetMaskKeys().
	//    It looks a bit suspicious, as it won't correct for keys that have been released.
	//    Is this the way it's supposed to work?

	if(mask & KMOD_SHIFT)
	{
		mKeyLevel[KEY_SHIFT] = TRUE;
	}

	if(mask & KMOD_CTRL)
	{
		mKeyLevel[KEY_CONTROL] = TRUE;
	}

	if(mask & KMOD_ALT)
	{
		mKeyLevel[KEY_ALT] = TRUE;
	}
}


MASK LLKeyboardSDL::updateModifiers(const U32 mask)
{
	// translate the mask
	MASK out_mask = MASK_NONE;

	if(mask & KMOD_SHIFT)
	{
		out_mask |= MASK_SHIFT;
	}

	if(mask & KMOD_CTRL)
	{
		out_mask |= MASK_CONTROL;
	}

	if(mask & KMOD_ALT)
	{
		out_mask |= MASK_ALT;
	}

	return out_mask;
}


static U16 adjustNativekeyFromUnhandledMask(const U16 key, const U32 mask)
{
	// SDL doesn't automatically adjust the keysym according to
	// whether NUMLOCK is engaged, so we massage the keysym manually.
	U16 rtn = key;
	if (!(mask & KMOD_NUM))
	{
		switch (key)
		{
		case SDLK_KP_PERIOD: rtn = SDLK_DELETE; break;
		case SDLK_KP0: rtn = SDLK_INSERT; break;
		case SDLK_KP1: rtn = SDLK_END; break;
		case SDLK_KP2: rtn = SDLK_DOWN; break;
		case SDLK_KP3: rtn = SDLK_PAGEDOWN; break;
		case SDLK_KP4: rtn = SDLK_LEFT; break;
		case SDLK_KP6: rtn = SDLK_RIGHT; break;
		case SDLK_KP7: rtn = SDLK_HOME; break;
		case SDLK_KP8: rtn = SDLK_UP; break;
		case SDLK_KP9: rtn = SDLK_PAGEUP; break;
		}
	}
	return rtn;
}


BOOL LLKeyboardSDL::handleKeyDown(const U16 key, const U32 mask)
{
	U16     adjusted_nativekey;
	KEY	translated_key = 0;
	U32	translated_mask = MASK_NONE;
	BOOL	handled = FALSE;

	adjusted_nativekey = adjustNativekeyFromUnhandledMask(key, mask);

	translated_mask = updateModifiers(mask);
	
	if(translateNumpadKey(adjusted_nativekey, &translated_key))
	{
		handled = handleTranslatedKeyDown(translated_key, translated_mask);
	}

	return handled;
}


BOOL LLKeyboardSDL::handleKeyUp(const U16 key, const U32 mask)
{
	U16     adjusted_nativekey;
	KEY	translated_key = 0;
	U32	translated_mask = MASK_NONE;
	BOOL	handled = FALSE;

	adjusted_nativekey = adjustNativekeyFromUnhandledMask(key, mask);

	translated_mask = updateModifiers(mask);

	if(translateNumpadKey(adjusted_nativekey, &translated_key))
	{
		handled = handleTranslatedKeyUp(translated_key, translated_mask);
	}

	return handled;
}

MASK LLKeyboardSDL::currentMask(BOOL for_mouse_event)
{
	MASK result = MASK_NONE;
	SDLMod mask = SDL_GetModState();

	if (mask & KMOD_SHIFT)			result |= MASK_SHIFT;
	if (mask & KMOD_CTRL)			result |= MASK_CONTROL;
	if (mask & KMOD_ALT)			result |= MASK_ALT;

	// For keyboard events, consider Meta keys equivalent to Control
	if (!for_mouse_event)
	{
		if (mask & KMOD_META) result |= MASK_CONTROL;
	}

	return result;
}

void LLKeyboardSDL::scanKeyboard()
{
	for (S32 key = 0; key < KEY_COUNT; key++)
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
	for (S32 key = 0; key < KEY_COUNT; key++)
	{
		mKeyUp[key] = FALSE;
		mKeyDown[key] = FALSE;
		if (mKeyLevel[key])
		{
			mKeyLevelFrameCount[key]++;
		}
	}
}

 
BOOL LLKeyboardSDL::translateNumpadKey( const U16 os_key, KEY *translated_key)
{
	return translateKey(os_key, translated_key);	
}

U16 LLKeyboardSDL::inverseTranslateNumpadKey(const KEY translated_key)
{
	return inverseTranslateKey(translated_key);
}

#endif

