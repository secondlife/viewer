/** 
 * @file llviewerkeyboard.h
 * @brief LLViewerKeyboard class header file
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#ifndef LL_LLVIEWERKEYBOARD_H
#define LL_LLVIEWERKEYBOARD_H

#include "llkeyboard.h" // For EKeystate
#include "llinitparam.h"

const S32 MAX_NAMED_FUNCTIONS = 100;
const S32 MAX_KEY_BINDINGS = 128; // was 60

class LLNamedFunction
{
public:
	LLNamedFunction() : mFunction(NULL) { };
	~LLNamedFunction() { };

	std::string	mName;
	LLKeyFunc	mFunction;
};

typedef enum e_keyboard_mode
{
	MODE_FIRST_PERSON,
	MODE_THIRD_PERSON,
	MODE_EDIT,
	MODE_EDIT_AVATAR,
	MODE_SITTING,
	MODE_COUNT
} EKeyboardMode;


void bind_keyboard_functions();

class LLViewerKeyboard
{
public:
	struct KeyBinding : public LLInitParam::Block<KeyBinding>
	{
		Mandatory<std::string>	key,
								mask,
								command;

		KeyBinding();
	};

	struct KeyMode : public LLInitParam::Block<KeyMode>
	{
		Multiple<KeyBinding>		bindings;
		EKeyboardMode				mode;
		KeyMode(EKeyboardMode mode);
	};

	struct Keys : public LLInitParam::Block<Keys>
	{
		Optional<KeyMode>	first_person,
							third_person,
							edit,
							sitting,
							edit_avatar;

		Keys();
	};

	LLViewerKeyboard();

	BOOL			handleKey(KEY key, MASK mask, BOOL repeated);

	S32				loadBindings(const std::string& filename);										// returns number bound, 0 on error
	S32				loadBindingsXML(const std::string& filename);										// returns number bound, 0 on error
	EKeyboardMode	getMode();

	BOOL			modeFromString(const std::string& string, S32 *mode);			// False on failure

	void			scanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level);

private:
	S32				loadBindingMode(const LLViewerKeyboard::KeyMode& keymode);
	BOOL			bindKey(const S32 mode, const KEY key, const MASK mask, const std::string& function_name);

	// Hold all the ugly stuff torn out to make LLKeyboard non-viewer-specific here
	S32				mBindingCount[MODE_COUNT];
	LLKeyBinding	mBindings[MODE_COUNT][MAX_KEY_BINDINGS];

	typedef std::map<U32, U32> key_remap_t;
	key_remap_t		mRemapKeys[MODE_COUNT];
	std::set<KEY>	mKeysSkippedByUI;
	BOOL			mKeyHandledByUI[KEY_COUNT];		// key processed successfully by UI
};

extern LLViewerKeyboard gViewerKeyboard;

#endif // LL_LLVIEWERKEYBOARD_H
