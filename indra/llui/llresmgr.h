/** 
 * @file llresmgr.h
 * @brief Localized resource manager
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
    LLLOCALE_COUNT  // Number of values in this enum.  Keep at end.
};

class LLResMgr : public LLSingleton<LLResMgr>
{
    LLSINGLETON(LLResMgr);

public:
    void                setLocale( LLLOCALE_ID locale_id );
    LLLOCALE_ID         getLocale() const                       { return mLocale; }

    char                getDecimalPoint() const;
    char                getThousandsSeparator() const;

    char                getMonetaryDecimalPoint() const;    
    char                getMonetaryThousandsSeparator() const;
    std::string         getMonetaryString( S32 input ) const;
    void                getIntegerString( std::string& output, S32 input ) const;


private:
    LLLOCALE_ID         mLocale;
};

class LLLocale
{
public:
    LLLocale(const std::string& locale_string);
    virtual ~LLLocale();

    static const std::string USER_LOCALE;
    static const std::string SYSTEM_LOCALE;

private:
    std::string mPrevLocaleString;
};

#endif  // LL_RESMGR_
