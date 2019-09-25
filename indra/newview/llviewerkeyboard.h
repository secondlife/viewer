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

class LLKeyboardBinding
{
public:
    KEY				mKey;
    MASK			mMask;
    bool			mIgnoreMask; // whether CTRL/ALT/SHIFT should be checked

    LLKeyFunc		mFunction;
};

class LLMouseBinding
{
public:
    EMouseClickType	mMouse;
    MASK			mMask;
    bool			mIgnoreMask; // whether CTRL/ALT/SHIFT should be checked

    LLKeyFunc		mFunction;
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

class LLWindow;

void bind_keyboard_functions();

class LLViewerKeyboard
{
public:
	struct KeyBinding : public LLInitParam::Block<KeyBinding>
	{
		Mandatory<std::string>	key,
								mask,
								command;
		Optional<std::string>	mouse; // Note, not mandatory for the sake of backward campatibility
		Optional<bool>			ignore;

		KeyBinding();
	};

	struct KeyMode : public LLInitParam::Block<KeyMode>
	{
		Multiple<KeyBinding>		bindings;

		KeyMode();
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
	BOOL			handleKeyUp(KEY key, MASK mask);

	S32				loadBindings(const std::string& filename);										// returns number bound, 0 on error
	S32				loadBindingsXML(const std::string& filename);										// returns number bound, 0 on error
	EKeyboardMode	getMode() const;

	BOOL			modeFromString(const std::string& string, S32 *mode) const;			// False on failure
	BOOL			mouseFromString(const std::string& string, EMouseClickType *mode) const;// False on failure

    void			scanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level) const;

    // handleMouse() records state, scanMouse() goes through states, scanMouse(click) processes individual saved states after UI is done with them
    BOOL			handleMouse(LLWindow *window_impl, LLCoordGL pos, MASK mask, EMouseClickType clicktype, BOOL down);
	void LLViewerKeyboard::scanMouse();

private:
    bool            scanKey(const LLKeyboardBinding* binding,
                            S32 binding_count,
                            KEY key,
                            MASK mask,
                            BOOL key_down,
                            BOOL key_up,
                            BOOL key_level,
                            bool repeat) const;

    enum EMouseState
    {
        MOUSE_STATE_DOWN, // key down this frame
        MOUSE_STATE_CLICK, // key went up and down in scope of same frame
        MOUSE_STATE_LEVEL, // clicked again fast, or never released
        MOUSE_STATE_UP, // went up this frame
        MOUSE_STATE_SILENT // notified about 'up', do not notify again
    };
    bool			scanMouse(EMouseClickType click, EMouseState state) const;
    bool            scanMouse(const LLMouseBinding *binding,
                          S32 binding_count,
                          EMouseClickType mouse,
                          MASK mask,
                          EMouseState state) const;

    S32				loadBindingMode(const LLViewerKeyboard::KeyMode& keymode, S32 mode);
    BOOL			bindKey(const S32 mode, const KEY key, const MASK mask, const bool ignore, const std::string& function_name);
    BOOL			bindMouse(const S32 mode, const EMouseClickType mouse, const MASK mask, const bool ignore, const std::string& function_name);
    void			resetBindings();

	// Hold all the ugly stuff torn out to make LLKeyboard non-viewer-specific here
    // We process specific keys first, then keys that should ignore mask.
    // This way with W+ignore defined to 'forward' and with ctrl+W tied to camera,
    // pressing undefined Shift+W will do 'forward', yet ctrl+W will do 'camera forward'
    // With A defined to 'turn', shift+A defined to strafe, pressing Shift+W+A will do strafe+forward
    S32				mKeyBindingCount[MODE_COUNT];
    S32				mKeyIgnoreMaskCount[MODE_COUNT];
    S32				mMouseBindingCount[MODE_COUNT];
    S32				mMouseIgnoreMaskCount[MODE_COUNT];
    LLKeyboardBinding	mKeyBindings[MODE_COUNT][MAX_KEY_BINDINGS];
    LLKeyboardBinding	mKeyIgnoreMask[MODE_COUNT][MAX_KEY_BINDINGS];
    LLMouseBinding		mMouseBindings[MODE_COUNT][MAX_KEY_BINDINGS];
    LLMouseBinding		mMouseIgnoreMask[MODE_COUNT][MAX_KEY_BINDINGS];

	typedef std::map<U32, U32> key_remap_t;
	key_remap_t		mRemapKeys[MODE_COUNT];
	std::set<KEY>	mKeysSkippedByUI;
	BOOL			mKeyHandledByUI[KEY_COUNT];		// key processed successfully by UI

    // This is indentical to what llkeyboard does (mKeyRepeated, mKeyLevel, mKeyDown e t c),
    // just instead of remembering individually as bools,  we record state as enum
    EMouseState		mMouseLevel[CLICK_COUNT];	// records of key state
};

extern LLViewerKeyboard gViewerKeyboard;

#endif // LL_LLVIEWERKEYBOARD_H
