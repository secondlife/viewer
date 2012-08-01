/** 
 * @file llkeyboard.cpp
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

#include "linden_common.h"
#include "indra_constants.h"
#include "llkeyboard.h"

#include "llwindowcallbacks.h"

//
// Globals
//

LLKeyboard *gKeyboard = NULL;

//static
std::map<KEY,std::string> LLKeyboard::sKeysToNames;
std::map<std::string,KEY> LLKeyboard::sNamesToKeys;
LLKeyStringTranslatorFunc*	LLKeyboard::mStringTranslator = NULL;	// Used for l10n + PC/Mac/Linux accelerator labeling


//
// Class Implementation
//

LLKeyboard::LLKeyboard() : mCallbacks(NULL)
{
	S32 i;

	// Constructor for LLTimer inits each timer. We want them to
	// be constructed without being initialized, so we shut them down here.
	for (i = 0; i < KEY_COUNT; i++)
	{
		mKeyLevelFrameCount[i] = 0;
		mKeyLevel[i] = FALSE;
		mKeyUp[i]    = FALSE;
		mKeyDown[i]  = FALSE;
		mKeyRepeated[i] = FALSE;
	}

	mInsertMode = LL_KIM_INSERT;
	mCurTranslatedKey = KEY_NONE;
	mCurScanKey = KEY_NONE;

	addKeyName(' ', "Space" );
	addKeyName(KEY_RETURN, "Enter" );
	addKeyName(KEY_LEFT, "Left" );
	addKeyName(KEY_RIGHT, "Right" );
	addKeyName(KEY_UP, "Up" );
	addKeyName(KEY_DOWN, "Down" );
	addKeyName(KEY_ESCAPE, "Esc" );
	addKeyName(KEY_HOME, "Home" );
	addKeyName(KEY_END, "End" );
	addKeyName(KEY_PAGE_UP, "PgUp" );
	addKeyName(KEY_PAGE_DOWN, "PgDn" );
	addKeyName(KEY_F1, "F1" );
	addKeyName(KEY_F2, "F2" );
	addKeyName(KEY_F3, "F3" );
	addKeyName(KEY_F4, "F4" );
	addKeyName(KEY_F5, "F5" );
	addKeyName(KEY_F6, "F6" );
	addKeyName(KEY_F7, "F7" );
	addKeyName(KEY_F8, "F8" );
	addKeyName(KEY_F9, "F9" );
	addKeyName(KEY_F10, "F10" );
	addKeyName(KEY_F11, "F11" );
	addKeyName(KEY_F12, "F12" );
	addKeyName(KEY_TAB, "Tab" );
	addKeyName(KEY_ADD, "Add" );
	addKeyName(KEY_SUBTRACT, "Subtract" );
	addKeyName(KEY_MULTIPLY, "Multiply" );
	addKeyName(KEY_DIVIDE, "Divide" );
	addKeyName(KEY_PAD_DIVIDE, "PAD_DIVIDE" );
	addKeyName(KEY_PAD_LEFT, "PAD_LEFT" );
	addKeyName(KEY_PAD_RIGHT, "PAD_RIGHT" );
	addKeyName(KEY_PAD_DOWN, "PAD_DOWN" );
	addKeyName(KEY_PAD_UP, "PAD_UP" );
	addKeyName(KEY_PAD_HOME, "PAD_HOME" );
	addKeyName(KEY_PAD_END, "PAD_END" );
	addKeyName(KEY_PAD_PGUP, "PAD_PGUP" );
	addKeyName(KEY_PAD_PGDN, "PAD_PGDN" );
	addKeyName(KEY_PAD_CENTER, "PAD_CENTER" );
	addKeyName(KEY_PAD_INS, "PAD_INS" );
	addKeyName(KEY_PAD_DEL, "PAD_DEL" );
	addKeyName(KEY_PAD_RETURN, "PAD_Enter" );
	addKeyName(KEY_BUTTON0, "PAD_BUTTON0" );
	addKeyName(KEY_BUTTON1, "PAD_BUTTON1" );
	addKeyName(KEY_BUTTON2, "PAD_BUTTON2" );
	addKeyName(KEY_BUTTON3, "PAD_BUTTON3" );
	addKeyName(KEY_BUTTON4, "PAD_BUTTON4" );
	addKeyName(KEY_BUTTON5, "PAD_BUTTON5" );
	addKeyName(KEY_BUTTON6, "PAD_BUTTON6" );
	addKeyName(KEY_BUTTON7, "PAD_BUTTON7" );
	addKeyName(KEY_BUTTON8, "PAD_BUTTON8" );
	addKeyName(KEY_BUTTON9, "PAD_BUTTON9" );
	addKeyName(KEY_BUTTON10, "PAD_BUTTON10" );
	addKeyName(KEY_BUTTON11, "PAD_BUTTON11" );
	addKeyName(KEY_BUTTON12, "PAD_BUTTON12" );
	addKeyName(KEY_BUTTON13, "PAD_BUTTON13" );
	addKeyName(KEY_BUTTON14, "PAD_BUTTON14" );
	addKeyName(KEY_BUTTON15, "PAD_BUTTON15" );
	
	addKeyName(KEY_BACKSPACE, "Backsp" );
	addKeyName(KEY_DELETE, "Del" );
	addKeyName(KEY_SHIFT, "Shift" );
	addKeyName(KEY_CONTROL, "Ctrl" );
	addKeyName(KEY_ALT, "Alt" );
	addKeyName(KEY_HYPHEN, "-" );
	addKeyName(KEY_EQUALS, "=" );
	addKeyName(KEY_INSERT, "Ins" );
	addKeyName(KEY_CAPSLOCK, "CapsLock" );
}


LLKeyboard::~LLKeyboard()
{
	// nothing
}

void LLKeyboard::addKeyName(KEY key, const std::string& name)
{
	sKeysToNames[key] = name;
	std::string nameuc = name;
	LLStringUtil::toUpper(nameuc);
	sNamesToKeys[nameuc] = key;
}

// BUG this has to be called when an OS dialog is shown, otherwise modifier key state
// is wrong because the keyup event is never received by the main window. JC
void LLKeyboard::resetKeys()
{
	S32 i;

	for (i = 0; i < KEY_COUNT; i++)
	{
		if( mKeyLevel[i] )
		{
			mKeyLevel[i] = FALSE;
		}
	}

	for (i = 0; i < KEY_COUNT; i++)
	{
		mKeyUp[i] = FALSE;
	}

	for (i = 0; i < KEY_COUNT; i++)
	{
		mKeyDown[i] = FALSE;
	}

	for (i = 0; i < KEY_COUNT; i++)
	{
		mKeyRepeated[i] = FALSE;
	}
}


BOOL LLKeyboard::translateKey(const U16 os_key, KEY *out_key)
{
	std::map<U16, KEY>::iterator iter;

	// Only translate keys in the map, ignore all other keys for now
	iter = mTranslateKeyMap.find(os_key);
	if (iter == mTranslateKeyMap.end())
	{
		//llwarns << "Unknown virtual key " << os_key << llendl;
		*out_key = 0;
		return FALSE;
	}
	else
	{
		*out_key = iter->second;
		return TRUE;
	}
}


U16 LLKeyboard::inverseTranslateKey(const KEY translated_key)
{
	std::map<KEY, U16>::iterator iter;
	iter = mInvTranslateKeyMap.find(translated_key);
	if (iter == mInvTranslateKeyMap.end())
	{
		return 0;
	}
	else
	{
		return iter->second;
	}
}


BOOL LLKeyboard::handleTranslatedKeyDown(KEY translated_key, U32 translated_mask)
{
	BOOL handled = FALSE;
	BOOL repeated = FALSE;

	// is this the first time the key went down?
	// if so, generate "character" message
	if( !mKeyLevel[translated_key] )
	{
		mKeyLevel[translated_key] = TRUE;
		mKeyLevelTimer[translated_key].reset();
		mKeyLevelFrameCount[translated_key] = 0;
		mKeyRepeated[translated_key] = FALSE;
	}
	else
	{
		// Level is already down, assume it's repeated.
		repeated = TRUE;
		mKeyRepeated[translated_key] = TRUE;
	}
	
	mKeyDown[translated_key] = TRUE;
	mCurTranslatedKey = (KEY)translated_key;
	handled = mCallbacks->handleTranslatedKeyDown(translated_key, translated_mask, repeated);
	return handled;
}


BOOL LLKeyboard::handleTranslatedKeyUp(KEY translated_key, U32 translated_mask)
{	
	BOOL handled = FALSE;
	if( mKeyLevel[translated_key] )
	{
		mKeyLevel[translated_key] = FALSE;
		
		// Only generate key up events if the key is thought to
		// be down.  This allows you to call resetKeys() in the
		// middle of a frame and ignore subsequent KEY_UP
		// messages in the same frame.  This was causing the
		// sequence W<return> in chat to move agents forward. JC
		mKeyUp[translated_key] = TRUE;
		handled = mCallbacks->handleTranslatedKeyUp(translated_key, translated_mask);
	}
	
	lldebugst(LLERR_USER_INPUT) << "keyup -" << translated_key << "-" << llendl;

	return handled;
}


void LLKeyboard::toggleInsertMode()
{
	if (LL_KIM_INSERT == mInsertMode)
	{
		mInsertMode = LL_KIM_OVERWRITE;
	}
	else
	{
		mInsertMode = LL_KIM_INSERT;
	}
}


// Returns time in seconds since key was pressed.
F32 LLKeyboard::getKeyElapsedTime(KEY key)
{
	return mKeyLevelTimer[key].getElapsedTimeF32();
}

// Returns time in frames since key was pressed.
S32 LLKeyboard::getKeyElapsedFrameCount(KEY key)
{
	return mKeyLevelFrameCount[key];
}

// static
BOOL LLKeyboard::keyFromString(const std::string& str, KEY *key)
{
	std::string instring(str);
	size_t length = instring.size();

	if (length < 1)
	{
		return FALSE;
	}
	if (length == 1)
	{
		char ch = toupper(instring[0]);
		if (('0' <= ch && ch <= '9') ||
			('A' <= ch && ch <= 'Z') ||
			('!' <= ch && ch <= '/') || // !"#$%&'()*+,-./
			(':' <= ch && ch <= '@') || // :;<=>?@
			('[' <= ch && ch <= '`') || // [\]^_`
			('{' <= ch && ch <= '~'))   // {|}~
		{
			*key = ch;
			return TRUE;
		}
	}

	LLStringUtil::toUpper(instring);
	KEY res = get_if_there(sNamesToKeys, instring, (KEY)0);
	if (res != 0)
	{
		*key = res;
		return TRUE;
	}
	llwarns << "keyFromString failed: " << str << llendl;
	return FALSE;
}


// static
std::string LLKeyboard::stringFromKey(KEY key)
{
	std::string res = get_if_there(sKeysToNames, key, std::string());
	if (res.empty())
	{
		char buffer[2];		/* Flawfinder: ignore */
		buffer[0] = key;
		buffer[1] = '\0';
		res = std::string(buffer);
	}

	LLKeyStringTranslatorFunc *trans = gKeyboard->mStringTranslator;
	if (trans != NULL)
	{
		res = trans(res.c_str());
	}

	return res;
}


//static
std::string LLKeyboard::stringFromAccelerator( MASK accel_mask, KEY key )
{
	std::string res;
	
	// break early if this is a silly thing to do.
	if( KEY_NONE == key )
	{
		return res;
	}
	
	LLKeyStringTranslatorFunc *trans = gKeyboard->mStringTranslator;
	
	if( trans == NULL )
	{
		llerrs << "No mKeyStringTranslator" << llendl;
		return res;
	}
	
	// Append any masks
#ifdef LL_DARWIN
	// Standard Mac names for modifier keys in menu equivalents
	// We could use the symbol characters, but they only exist in certain fonts.
	if( accel_mask & MASK_CONTROL )
	{
		if ( accel_mask & MASK_MAC_CONTROL )
		{
			res.append( trans("accel-mac-control") );
		}
		else
		{
			res.append( trans("accel-mac-command") );		// Symbol would be "\xE2\x8C\x98"
		}
	}
	if( accel_mask & MASK_ALT )
		res.append( trans("accel-mac-option") );		// Symbol would be "\xE2\x8C\xA5"
	if( accel_mask & MASK_SHIFT )
		res.append( trans("accel-mac-shift") );		// Symbol would be "\xE2\x8C\xA7"
#else
	if( accel_mask & MASK_CONTROL )
		res.append( trans("accel-win-control") );
	if( accel_mask & MASK_ALT )
		res.append( trans("accel-win-alt") );
	if( accel_mask & MASK_SHIFT )
		res.append( trans("accel-win-shift") );
#endif
	std::string key_string = LLKeyboard::stringFromKey(key);
	if ((accel_mask & MASK_NORMALKEYS) &&
		(key_string[0] == '-' || key_string[0] == '=' || key_string[0] == '+'))
	{
		res.append( " " );
	}

	std::string keystr = stringFromKey( key );
	res.append( keystr );
	
	return res;
}


//static
BOOL LLKeyboard::maskFromString(const std::string& str, MASK *mask)
{
	std::string instring(str);
	if (instring == "NONE")
	{
		*mask = MASK_NONE;
		return TRUE;
	}
	else if (instring == "SHIFT")
	{
		*mask = MASK_SHIFT;
		return TRUE;
	}
	else if (instring == "CTL")
	{
		*mask = MASK_CONTROL;
		return TRUE;
	}
	else if (instring == "ALT")
	{
		*mask = MASK_ALT;
		return TRUE;
	}
	else if (instring == "CTL_SHIFT")
	{
		*mask = MASK_CONTROL | MASK_SHIFT;
		return TRUE;
	}
	else if (instring == "ALT_SHIFT")
	{
		*mask = MASK_ALT | MASK_SHIFT;
		return TRUE;
	}
	else if (instring == "CTL_ALT")
	{
		*mask = MASK_CONTROL | MASK_ALT;
		return TRUE;
	}
	else if (instring == "CTL_ALT_SHIFT")
	{
		*mask = MASK_CONTROL | MASK_ALT | MASK_SHIFT;
		return TRUE;
	}
	else 
	{
		return FALSE;
	}
}


//static
void LLKeyboard::setStringTranslatorFunc( LLKeyStringTranslatorFunc *trans_func )
{
	mStringTranslator = trans_func;
}
