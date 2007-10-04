/** 
 * @file llkeyboard.h
 * @brief Handler for assignable key bindings
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

typedef void (*LLKeyFunc)(EKeystate keystate);

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
	typedef enum e_numpad_distinct
	{
		ND_NEVER,
		ND_NUMLOCK_OFF,
		ND_NUMLOCK_ON
	} ENumpadDistinct;

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

	static BOOL		maskFromString(const LLString& str, MASK *mask);		// False on failure
	static BOOL		keyFromString(const LLString& str, KEY *key);			// False on failure
	static LLString	stringFromKey(KEY key);

	e_numpad_distinct getNumpadDistinct() { return mNumpadDistinct; }
	void setNumpadDistinct(e_numpad_distinct val) { mNumpadDistinct = val; }

	void setCallbacks(LLWindowCallbacks *cbs) { mCallbacks = cbs; }
	F32				getKeyElapsedTime( KEY key );  // Returns time in seconds since key was pressed.
	S32				getKeyElapsedFrameCount( KEY key );  // Returns time in frames since key was pressed.

protected:
	void 			addKeyName(KEY key, const LLString& name);

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

	e_numpad_distinct mNumpadDistinct;

	EKeyboardInsertMode mInsertMode;

	static std::map<KEY,LLString> sKeysToNames;
	static std::map<LLString,KEY> sNamesToKeys;
};

extern LLKeyboard *gKeyboard;

#endif
