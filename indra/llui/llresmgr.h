/** 
 * @file llresmgr.h
 * @brief Localized resource manager
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// NOTE: this is a MINIMAL implementation.  The interface will remain, but the implementation will
// (when the time is right) become dynamic and probably use external files.

#ifndef LL_LLRESMGR_H
#define LL_LLRESMGR_H

#include "locale.h"
#include "stdtypes.h"
#include "llstring.h"

enum LLLOCALE_ID
{
	LLLOCALE_USA,
	LLLOCALE_UK,
	LLLOCALE_COUNT	// Number of values in this enum.  Keep at end.
};

/*
enum LLSTR_ID 
{
	LLSTR_HELLO,
	LLSTR_GOODBYE,
	LLSTR_CHAT_LABEL,
	LLSTR_STATUS_LABEL,
	LLSTR_X,
	LLSTR_Y,
	LLSTR_Z,
	LLSTR_POSITION,
	LLSTR_SCALE,
	LLSTR_ROTATION,
	LLSTR_HAS_PHYSICS,
	LLSTR_SCRIPT,
	LLSTR_HELP,
	LLSTR_REMOVE,
	LLSTR_CLEAR,
	LLSTR_APPLY,
	LLSTR_CANCEL,
	LLSTR_MATERIAL,
	LLSTR_FACE,
	LLSTR_TEXTURE,
	LLSTR_TEXTURE_SIZE,
	LLSTR_TEXTURE_OFFSET,
	LLSTR_TEXTURE_ROTATION,
	LLSTR_U,
	LLSTR_V,
	LLSTR_OWNERSHIP,
	LLSTR_PUBLIC,
	LLSTR_PRIVATE,
	LLSTR_REVERT,
	LLSTR_INSERT_SAMPLE,
	LLSTR_SET_TEXTURE,
	LLSTR_EDIT_SCRIPT,
	LLSTR_MOUSELOOK_INSTRUCTIONS,
	LLSTR_EDIT_FACE_INSTRUCTIONS,
	LLSTR_CLOSE,
	LLSTR_MOVE,
	LLSTR_ROTATE,
	LLSTR_RESIZE,
	LLSTR_PLACE_BOX,
	LLSTR_PLACE_PRISM,
	LLSTR_PLACE_PYRAMID,
	LLSTR_PLACE_TETRAHEDRON,
	LLSTR_PLACE_CYLINDER,
	LLSTR_PLACE_HALF_CYLINDER,
	LLSTR_PLACE_CONE,
	LLSTR_PLACE_HALF_CONE,
	LLSTR_PLACE_SPHERE,
	LLSTR_PLACE_HALF_SPHERE,
	LLSTR_PLACE_BIRD,
	LLSTR_PLACE_SNAKE,
	LLSTR_PLACE_ROCK,
	LLSTR_PLACE_TREE,
	LLSTR_PLACE_GRASS,
	LLSTR_MODIFY_LAND,
	LLSTR_COUNT		// Number of values in this enum.  Keep at end.
};
*/

enum LLFONT_ID
{
	LLFONT_OCRA,
	LLFONT_SANSSERIF,
	LLFONT_SANSSERIF_SMALL,
	LLFONT_SANSSERIF_BIG,
	LLFONT_SMALL,
	LLFONT_COUNT	// Number of values in this enum.  Keep at end.
};

class LLFontGL;

class LLResMgr
{
public:
	LLResMgr();

	void				setLocale( LLLOCALE_ID locale_id );
	LLLOCALE_ID			getLocale() const						{ return mLocale; }

	char				getDecimalPoint() const;
	char				getThousandsSeparator() const;

	char				getMonetaryDecimalPoint() const;	
	char				getMonetaryThousandsSeparator() const;
	void				getMonetaryString( LLString& output, S32 input ) const;
	void				getIntegerString( LLString& output, S32 input ) const;

//	const char*			getRes( LLSTR_ID string_id ) const		{ return mStrings[ string_id ]; }
	const LLFontGL*		getRes( LLFONT_ID font_id ) const		{ return mFonts[ font_id ]; }
	const LLFontGL*		getRes( LLString font_id ) const;

private:
	LLLOCALE_ID			mLocale;
//	const char**		mStrings;
	const LLFontGL**	mFonts;

//	const char*			mUSAStrings[LLSTR_COUNT];
	const LLFontGL*		mUSAFonts[LLFONT_COUNT];

//	const char*			mUKStrings[LLSTR_COUNT];
	const LLFontGL*		mUKFonts[LLFONT_COUNT];
};

class LLLocale
{
public:
	LLLocale(const LLString& locale_string);
	virtual ~LLLocale();

public:
	static const LLString USER_LOCALE;
	static const LLString SYSTEM_LOCALE;

protected:
	LLString	mPrevLocaleString;
};

extern LLResMgr* gResMgr;

#endif  // LL_RESMGR_
