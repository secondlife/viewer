/** 
 * @file llkeyboard.h
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

#ifndef LL_LLKEYBOARD_H
#define LL_LLKEYBOARD_H

#include <map>
#include <boost/function.hpp>

#include "llstringtable.h"
#include "lltimer.h"
#include "indra_constants.h"

enum EKeystate 
{
	KEYSTATE_DOWN,
	KEYSTATE_LEVEL,
	KEYSTATE_UP
};

typedef boost::function<bool(EKeystate keystate)> LLKeyFunc;
typedef std::string (LLKeyStringTranslatorFunc)(const char *label);
	
enum EKeyboardInsertMode
{
	LL_KIM_INSERT,
	LL_KIM_OVERWRITE
};

class LLWindowCallbacks;

class LLKeyboard
{
public:
	LLKeyboard();
	virtual ~LLKeyboard();

    void			resetKeyDownAndHandle();
    void			resetKeys();


	F32				getCurKeyElapsedTime()	{ return getKeyDown(mCurScanKey) ? getKeyElapsedTime( mCurScanKey ) : 0.f; }
	F32				getCurKeyElapsedFrameCount()	{ return getKeyDown(mCurScanKey) ? (F32)getKeyElapsedFrameCount( mCurScanKey ) : 0.f; }
	bool			getKeyDown(const KEY key) { return mKeyLevel[key]; }
	bool			getKeyRepeated(const KEY key) { return mKeyRepeated[key]; }

	bool			translateKey(const U16 os_key, KEY *translated_key);
	U16				inverseTranslateKey(const KEY translated_key);
	bool			handleTranslatedKeyUp(KEY translated_key, U32 translated_mask);		// Translated into "Linden" keycodes
	bool			handleTranslatedKeyDown(KEY translated_key, U32 translated_mask);	// Translated into "Linden" keycodes


	virtual bool	handleKeyUp(const U16 key, MASK mask) = 0;
	virtual bool	handleKeyDown(const U16 key, MASK mask) = 0;
	
#ifdef LL_DARWIN
	// We only actually use this for macOS.
	virtual void	handleModifier(MASK mask) = 0;
#endif // LL_DARWIN

	// Asynchronously poll the control, alt, and shift keys and set the
	// appropriate internal key masks.
	virtual void	resetMaskKeys() = 0;
	virtual void	scanKeyboard() = 0;															// scans keyboard, calls functions as necessary
	// Mac must differentiate between Command = Control for keyboard events
	// and Command != Control for mouse events.
	virtual MASK	currentMask(bool for_mouse_event) = 0;
	virtual KEY		currentKey() { return mCurTranslatedKey; }

	EKeyboardInsertMode getInsertMode()	{ return mInsertMode; }
	void toggleInsertMode();

	static bool		maskFromString(const std::string& str, MASK *mask);		// False on failure
	static bool		keyFromString(const std::string& str, KEY *key);			// False on failure
	static std::string stringFromKey(KEY key, bool translate = true);
    static std::string stringFromMouse(EMouseClickType click, bool translate = true);
	static std::string stringFromAccelerator( MASK accel_mask ); // separated for convinience, returns with "+": "Shift+" or "Shift+Alt+"...
	static std::string stringFromAccelerator( MASK accel_mask, KEY key );
    static std::string stringFromAccelerator(MASK accel_mask, EMouseClickType click);

	void setCallbacks(LLWindowCallbacks *cbs) { mCallbacks = cbs; }
	F32				getKeyElapsedTime( KEY key );  // Returns time in seconds since key was pressed.
	S32				getKeyElapsedFrameCount( KEY key );  // Returns time in frames since key was pressed.

	static void		setStringTranslatorFunc( LLKeyStringTranslatorFunc *trans_func );
	
protected:
	void 			addKeyName(KEY key, const std::string& name);

protected:
	std::map<U16, KEY>	mTranslateKeyMap;		// Map of translations from OS keys to Linden KEYs
	std::map<KEY, U16>	mInvTranslateKeyMap;	// Map of translations from Linden KEYs to OS keys
	LLWindowCallbacks *mCallbacks;

	LLTimer			mKeyLevelTimer[KEY_COUNT];	// Time since level was set
	S32				mKeyLevelFrameCount[KEY_COUNT];	// Frames since level was set
	bool			mKeyLevel[KEY_COUNT];		// Levels
	bool			mKeyRepeated[KEY_COUNT];	// Key was repeated
	bool			mKeyUp[KEY_COUNT];			// Up edge
	bool			mKeyDown[KEY_COUNT];		// Down edge
	KEY				mCurTranslatedKey;
	KEY				mCurScanKey;		// Used during the scanKeyboard()

	static LLKeyStringTranslatorFunc*	mStringTranslator;	// Used for l10n + PC/Mac/Linux accelerator labeling
	
	EKeyboardInsertMode mInsertMode;

	static std::map<KEY,std::string> sKeysToNames;
	static std::map<std::string,KEY> sNamesToKeys;
};

// Interface to get key from assigned command
class LLKeyBindingToStringHandler
{
public:
    virtual std::string getKeyBindingAsString(const std::string& mode, const std::string& control) const = 0;
};

extern LLKeyboard *gKeyboard;

#endif
