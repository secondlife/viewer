/** 
 * @file llresmgr.h
 * @brief Localized resource manager
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


#ifndef LL_LLRESMGR_H
#define LL_LLRESMGR_H

#include "locale.h"
#include "stdtypes.h"
#include "llstring.h"
#include "llmemory.h"

enum LLLOCALE_ID
{
	LLLOCALE_USA,
	LLLOCALE_UK,
	LLLOCALE_COUNT	// Number of values in this enum.  Keep at end.
};

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

class LLResMgr : public LLSingleton<LLResMgr>
{
public:
	LLResMgr();

	void				setLocale( LLLOCALE_ID locale_id );
	LLLOCALE_ID			getLocale() const						{ return mLocale; }

	char				getDecimalPoint() const;
	char				getThousandsSeparator() const;

	char				getMonetaryDecimalPoint() const;	
	char				getMonetaryThousandsSeparator() const;
	std::string			getMonetaryString( S32 input ) const;
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

	static const LLString USER_LOCALE;
	static const LLString SYSTEM_LOCALE;

private:
	LLString	mPrevLocaleString;
};

#endif  // LL_RESMGR_
