/** 
 * @file llviewerkeyboard.h
 * @brief LLViewerKeyboard class header file
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERKEYBOARD_H
#define LL_LLVIEWERKEYBOARD_H

#include "llkeyboard.h" // For EKeystate

const S32 MAX_NAMED_FUNCTIONS = 100;
const S32 MAX_KEY_BINDINGS = 128; // was 60

class LLNamedFunction
{
public:
	const char *mName;
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


void agent_push_forward( EKeystate s );
void agent_turn_right( EKeystate s );
void bind_keyboard_functions();

class LLViewerKeyboard
{
public:
	LLViewerKeyboard();

	BOOL			handleKey(KEY key, MASK mask, BOOL repeated);

	void			bindNamedFunction(const char *name, LLKeyFunc func);

	S32				loadBindings(const char *filename);										// returns number bound, 0 on error
	EKeyboardMode	getMode();

	BOOL			modeFromString(const char *string, S32 *mode);			// False on failure

	void			scanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level);
protected:
	BOOL			bindKey(const S32 mode, const KEY key, const MASK mask, const char *function_name);
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
