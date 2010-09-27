/** 
 * @file llviewerkeyboard.h
 * @brief LLViewerKeyboard class header file
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLVIEWERKEYBOARD_H
#define LL_LLVIEWERKEYBOARD_H

#include "llkeyboard.h" // For EKeystate

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
	LLViewerKeyboard();

	BOOL			handleKey(KEY key, MASK mask, BOOL repeated);

	void			bindNamedFunction(const std::string& name, LLKeyFunc func);

	S32				loadBindings(const std::string& filename);										// returns number bound, 0 on error
	EKeyboardMode	getMode();

	BOOL			modeFromString(const std::string& string, S32 *mode);			// False on failure

	void			scanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level);
protected:
	BOOL			bindKey(const S32 mode, const KEY key, const MASK mask, const std::string& function_name);
	S32				mNamedFunctionCount;
	LLNamedFunction	mNamedFunctions[MAX_NAMED_FUNCTIONS];

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
