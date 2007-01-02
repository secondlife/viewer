/** 
 * @file llkeyboardwin32.h
 * @brief Handler for assignable key bindings
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
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

	/*virtual*/ BOOL	handleKeyUp(const U16 key, MASK mask);
	/*virtual*/ BOOL	handleKeyDown(const U16 key, MASK mask);
	/*virtual*/ void	resetMaskKeys();
	/*virtual*/ MASK	currentMask(BOOL for_mouse_event);
	/*virtual*/ void	scanKeyboard();
	BOOL				translateExtendedKey(const U16 os_key, const MASK mask, KEY *translated_key);
	U16					inverseTranslateExtendedKey(const KEY translated_key);

protected:
	MASK	updateModifiers();
	void	setModifierKeyLevel( KEY key, BOOL new_state );
private:
	std::map<U16, KEY> mTranslateNumpadMap;
	std::map<KEY, U16> mInvTranslateNumpadMap;
};

#endif
