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

class LLFetchAvatarPaymentInfo;

class LLBuyCurrencyHTML
{
    public:
        // choke point for opening a legacy or new currency floater - this overload is when the L$ sum is not required
        static void openCurrencyFloater();

        // choke point for opening a legacy or new currency floater - this overload is when the L$ sum is required
        static void openCurrencyFloater( const std::string& message, S32 sum );

        // show and give focus to actual currency floater - this is used for both cases
        // where the shortfall is required and where it is not
        static void showDialog( S32 shortfall = 0 );

        // close (and destroy) the currency floater
        static void closeDialog();

        // probe BuyCurrencyPacksURL once at login; clears sWebFloaterEnabled if 501.
        // also launches the BuyCurrencyAddPaymentURL probe that sets sAddPaymentEnabled
        static void checkFeatureFlag();

        static bool sWebFloaterEnabled;

        // true only when BuyCurrencyAddPaymentURL answered 200, meaning the web floater
        // can add a payment method; otherwise residents without one are sent to the LindeX
        static bool sAddPaymentEnabled;

    private:
        // route to the legacy floater, the web floater, or the LindeX payment page
        static void routeCurrencyRequest( bool has_target, const std::string& message, S32 sum );

        // open the web floater, with the shortfall and fallback context when there is a target
        static void openWebFloater( bool has_target, const std::string& message, S32 sum );

        // pending payment info check made before opening the web floater
        static LLFetchAvatarPaymentInfo* sPaymentInfoRequest;
};

#endif  // LL_LLBUYCURRENCYHTML_H
