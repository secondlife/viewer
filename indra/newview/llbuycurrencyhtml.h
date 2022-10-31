/** 
 * @file llbuycurrencyhtml.h
 * @brief Manages Buy Currency HTML floater
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_LLBUYCURRENCYHTML_H
#define LL_LLBUYCURRENCYHTML_H

#include "llsingleton.h"

class LLFloaterBuyCurrencyHTML;

class LLBuyCurrencyHTML
{
    public:
        // choke point for opening a legacy or new currency floater - this overload is when the L$ sum is not required
        static void openCurrencyFloater();

        // choke point for opening a legacy or new currency floater - this overload is when the L$ sum is required
        static void openCurrencyFloater( const std::string& message, S32 sum );

        // show and give focus to actual currency floater - this is used for both cases
        // where the sum is required and where it is not
        static void showDialog( bool specific_sum_requested, const std::string& message, S32 sum );

        // close (and destroy) the currency floater
        static void closeDialog();
};

#endif  // LL_LLBUYCURRENCYHTML_H
