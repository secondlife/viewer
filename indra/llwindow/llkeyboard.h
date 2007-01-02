/** 
 * @file llkeyboard.h
 * @brief Handler for assignable key bindings
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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


	F32				getCurKeyElapsedTime()	{ return getKeyElapsedTime( mCurScanKey ); }
	F32				getCurKeyElapsedFrameCount()	{ return (F32)getKeyElapsedFrameCount( mCurScanKey ); }
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
protected:
	F32				getKeyElapsedTime( KEY key );  // Returns time in seconds since key was pressed.
	S32				getKeyElapsedFrameCount( KEY key );  // Returns time in frames since key was pressed.
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
