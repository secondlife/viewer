/** 
 * @file llresmgr.h
 * @brief Localized resource manager
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
#include "llsingleton.h"

enum LLLOCALE_ID
{
	LLLOCALE_USA,
	LLLOCALE_UK,
	LLLOCALE_COUNT	// Number of values in this enum.  Keep at end.
};

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
	void				getIntegerString( std::string& output, S32 input ) const;


private:
	LLLOCALE_ID			mLocale;
};

class LLLocale
{
public:
	LLLocale(const std::string& locale_string);
	virtual ~LLLocale();

	static const std::string USER_LOCALE;
	static const std::string SYSTEM_LOCALE;

private:
	std::string	mPrevLocaleString;
};

#endif  // LL_RESMGR_
