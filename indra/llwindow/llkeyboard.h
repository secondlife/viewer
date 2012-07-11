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

#include "string_table.h"
#include "lltimer.h"
#include "indra_constants.h"

enum EKeystate 
{
	KEYSTATE_DOWN,
	KEYSTATE_LEVEL,
	KEYSTATE_UP 
};

typedef boost::function<void(EKeystate keystate)> LLKeyFunc;
typedef std::string (LLKeyStringTranslatorFunc)(const char *label);
	
enum EKeyboardInsertMode
{
	LL_KIM_INSERT,
	LL_KIM_OVERWRITE
};

class LLKeyBinding
{
public:
	KEY				mKey;
	MASK			mMask;
// 	const char		*mName; // unused
	LLKeyFunc		mFunction;
};

class LLWindowCallbacks;

class LLKeyboard
{
public:
	LLKeyboard();
	virtual ~LLKeyboard();

	void			resetKeys();


	F32				getCurKeyElapsedTime()	{ return getKeyDown(mCurScanKey) ? getKeyElapsedTime( mCurScanKey ) : 0.f; }
	F32				getCurKeyElapsedFrameCount()	{ return getKeyDown(mCurScanKey) ? (F32)getKeyElapsedFrameCount( mCurScanKey ) : 0.f; }
	BOOL			getKeyDown(const KEY key) { return mKeyLevel[key]; }
	BOOL			getKeyRepeated(const KEY key) { return mKeyRepeated[key]; }

	BOOL			translateKey(const U16 os_key, KEY *translated_key);
	U16				inverseTranslateKey(const KEY translated_key);
	BOOL			handleTranslatedKeyUp(KEY translated_key, U32 translated_mask);		// Translated into "Linden" keycodes
	BOOL			handleTranslatedKeyDown(KEY translated_key, U32 translated_mask);	// Translated into "Linden" keycodes


	virtual BOOL	handleKeyUp(const U16 key, MASK mask) = 0;
	virtual BOOL	handleKeyDown(const U16 key, MASK mask) = 0;

	// Asynchronously poll the control, alt, and shift keys and set the
	// appropriate internal key masks.
	virtual void	resetMaskKeys() = 0;
	virtual void	scanKeyboard() = 0;															// scans keyboard, calls functions as necessary
	// Mac must differentiate between Command = Control for keyboard events
	// and Command != Control for mouse events.
	virtual MASK	currentMask(BOOL for_mouse_event) = 0;
	virtual KEY		currentKey() { return mCurTranslatedKey; }

	EKeyboardInsertMode getInsertMode()	{ return mInsertMode; }
	void toggleInsertMode();

	static BOOL		maskFromString(const std::string& str, MASK *mask);		// False on failure
	static BOOL		keyFromString(const std::string& str, KEY *key);			// False on failure
	static std::string stringFromKey(KEY key);
	static std::string stringFromAccelerator( MASK accel_mask, KEY key );

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
	BOOL			mKeyLevel[KEY_COUNT];		// Levels
	BOOL			mKeyRepeated[KEY_COUNT];	// Key was repeated
	BOOL			mKeyUp[KEY_COUNT];			// Up edge
	BOOL			mKeyDown[KEY_COUNT];		// Down edge
	KEY				mCurTranslatedKey;
	KEY				mCurScanKey;		// Used during the scanKeyboard()

	static LLKeyStringTranslatorFunc*	mStringTranslator;	// Used for l10n + PC/Mac/Linux accelerator labeling
	
	EKeyboardInsertMode mInsertMode;

	static std::map<KEY,std::string> sKeysToNames;
	static std::map<std::string,KEY> sNamesToKeys;
};

extern LLKeyboard *gKeyboard;

#endif
