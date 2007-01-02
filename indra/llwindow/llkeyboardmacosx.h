/** 
 * @file llkeyboardmacosx.h
 * @brief Handler for assignable key bindings
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLKEYBOARDMACOSX_H
#define LL_LLKEYBOARDMACOSX_H

#include "llkeyboard.h"

class LLKeyboardMacOSX : public LLKeyboard
{
public:
	LLKeyboardMacOSX();
	/*virtual*/ ~LLKeyboardMacOSX() {};
	
	/*virtual*/ BOOL	handleKeyUp(const U16 key, MASK mask);
	/*virtual*/ BOOL	handleKeyDown(const U16 key, MASK mask);
	/*virtual*/ void	resetMaskKeys();
	/*virtual*/ MASK	currentMask(BOOL for_mouse_event);
	/*virtual*/ void	scanKeyboard();
	
protected:
	MASK	updateModifiers(const U32 mask);
	void	setModifierKeyLevel( KEY key, BOOL new_state );
	BOOL	translateNumpadKey( const U16 os_key, KEY *translated_key );
	U16		inverseTranslateNumpadKey(const KEY translated_key);
private:
	std::map<U16, KEY> mTranslateNumpadMap;  // special map for translating OS keys to numpad keys
	std::map<KEY, U16> mInvTranslateNumpadMap; // inverse of the above
};

#endif
