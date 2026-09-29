/**
 * @file llbuycurrencyhtml.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterbuycurrency.h"
#include "llbuycurrencyhtml.h"
#include "llfloaterbuycurrencyhtml.h"

#include "llfloaterreg.h"
#include "llcommandhandler.h"
#include "llviewercontrol.h"
#include "llstatusbar.h"
#include "llcorehttputil.h"
#include "llcoros.h"

// support for secondlife:///app/buycurrencyhtml/{ACTION}/{NEXT_ACTION}/{RETURN_CODE} SLapps
class LLBuyCurrencyHTMLHandler :
    public LLCommandHandler
{
public:
    // requests will be throttled from a non-trusted browser
    LLBuyCurrencyHTMLHandler() : LLCommandHandler( "buycurrencyhtml", UNTRUSTED_THROTTLE) {}

    bool handle(const LLSD& params, const LLSD& query_map, const std::string& grid, LLMediaCtrl* web)
    {
        std::string action( "" );
        if ( params.size() >= 1 )
        {
             action = params[ 0 ].asString();
        };

        std::string next_action( "" );
        if ( params.size() >= 2 )
        {
            next_action = params[ 1 ].asString();
        };

        int result_code = 0;
        if ( params.size() >= 3 )
        {
            result_code = params[ 2 ].asInteger();
            if ( result_code != 0 )
            {
                LL_WARNS("LLBuyCurrency") << "Received nonzero result code: " << result_code << LL_ENDL ;
            }
        };

        // open the legacy XUI based currency floater
        if ( "open_legacy" == next_action )
        {
            LLFloaterBuyCurrency::buyCurrency();
        };

        // ask the Buy Currency floater to close
        // note: this is the last thing we can do so make
        // sure any other actions are processed before this.
        if ( "close" == action )
        {
            LLBuyCurrencyHTML::closeDialog();
        };

        return true;
    };
};
LLBuyCurrencyHTMLHandler gBuyCurrencyHTMLHandler;

bool LLBuyCurrencyHTML::sWebFloaterEnabled = false;
bool LLBuyCurrencyHTML::sAddPaymentEnabled = false;
LLFetchAvatarPaymentInfo* LLBuyCurrencyHTML::sPaymentInfoRequest = NULL;

////////////////////////////////////////////////////////////////////////////////
// static
static void checkFeatureFlag_coro(std::string check_url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("CheckBuyCurrencyURL", httpPolicy);
    LLCore::HttpRequest::ptr_t httpRequest = std::make_shared<LLCore::HttpRequest>();

    LLCore::HttpOptions::ptr_t httpOptions = std::make_shared<LLCore::HttpOptions>();
    httpOptions->setRetries(0);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, check_url, httpOptions);
    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    LLBuyCurrencyHTML::sWebFloaterEnabled = !(status.isHttpStatus() && status.getType() == 501);
}

// static
static void checkAddPaymentFlag_coro(std::string check_url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter = std::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>("CheckBuyCurrencyAddPaymentURL", httpPolicy);
    LLCore::HttpRequest::ptr_t httpRequest = std::make_shared<LLCore::HttpRequest>();

    // a redirect to some other page that answers 200 must not read as enabled
    LLCore::HttpOptions::ptr_t httpOptions = std::make_shared<LLCore::HttpOptions>();
    httpOptions->setRetries(0);
    httpOptions->setFollowRedirects(false);

    LLSD result = httpAdapter->getRawAndSuspend(httpRequest, check_url, httpOptions);
    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    LLBuyCurrencyHTML::sAddPaymentEnabled = status.isHttpStatus() && status.getType() == 200;
    LL_INFOS("LLBuyCurrency") << "Add payment probe returned " << status.toString()
        << ", in-floater add payment " << (LLBuyCurrencyHTML::sAddPaymentEnabled ? "enabled" : "disabled") << LL_ENDL;
}

// static
void LLBuyCurrencyHTML::checkFeatureFlag()
{
    std::string check_url = LLFloaterBuyCurrencyHTML::buildURL();
    LLCoros::instance().launch("checkFeatureFlag_coro",
        [check_url]() { checkFeatureFlag_coro(check_url); });

    std::string add_payment_url = gSavedSettings.getString("BuyCurrencyAddPaymentURL");
    LLCoros::instance().launch("checkAddPaymentFlag_coro",
        [add_payment_url]() { checkAddPaymentFlag_coro(add_payment_url); });
}

////////////////////////////////////////////////////////////////////////////////
// static
// Opens the legacy XUI based floater or new HTML based one based on
// the BuyCurrencyHTML value in settings.xml - this overload is for
// the case where the amount is not requested.
void LLBuyCurrencyHTML::openCurrencyFloater()
{
    routeCurrencyRequest(false, LLStringUtil::null, 0);
}

////////////////////////////////////////////////////////////////////////////////
// static
// Opens the legacy XUI based floater or new HTML based one based on
// the BuyCurrencyHTML value in settings.xml - this overload is for
// the case where the amount and a string to display are requested.
void LLBuyCurrencyHTML::openCurrencyFloater( const std::string& message, S32 sum )
{
    routeCurrencyRequest(true, message, sum);
}

////////////////////////////////////////////////////////////////////////////////
// static
// Without the in-floater add payment flow, the web floater is opened only for
// residents with payment info on file; the rest go to the LindeX payment page.
void LLBuyCurrencyHTML::routeCurrencyRequest( bool has_target, const std::string& message, S32 sum )
{
    if (!gSavedSettings.getBOOL("BuyCurrencyHTML") || !sWebFloaterEnabled)
    {
        // legacy version, which runs its own payment info check
        if (has_target)
        {
            LLFloaterBuyCurrency::buyCurrency(message, sum);
        }
        else
        {
            LLFloaterBuyCurrency::buyCurrency();
        }
    }
    else if (sAddPaymentEnabled)
    {
        openWebFloater(has_target, message, sum);
    }
    else
    {
        delete sPaymentInfoRequest;
        sPaymentInfoRequest = new LLFetchAvatarPaymentInfo(
            [has_target, message, sum](bool has_piof)
            {
                delete sPaymentInfoRequest;
                sPaymentInfoRequest = NULL;

                if (has_piof)
                {
                    openWebFloater(has_target, message, sum);
                }
                else
                {
                    LL_INFOS("LLBuyCurrency") << "No payment info on file, opening payment method page" << LL_ENDL;
                    LLFloaterBuyCurrency::openPaymentMethodPage();
                }
            });
    }
}

////////////////////////////////////////////////////////////////////////////////
// static
void LLBuyCurrencyHTML::openWebFloater( bool has_target, const std::string& message, S32 sum )
{
    if (!has_target)
    {
        LLBuyCurrencyHTML::showDialog();
        return;
    }

    LLBuyCurrencyHTML::showDialog(sum - gStatusBar->getBalance());
    LLFloaterBuyCurrencyHTML* floater = dynamic_cast<LLFloaterBuyCurrencyHTML*>(LLFloaterReg::getInstance("buy_currency_html"));
    if (floater)
    {
        floater->setFallbackContext(message, sum);
    }
}

////////////////////////////////////////////////////////////////////////////////
// static
void LLBuyCurrencyHTML::showDialog(S32 shortfall)
{
    LLFloaterBuyCurrencyHTML* buy_currency_floater = dynamic_cast< LLFloaterBuyCurrencyHTML* >( LLFloaterReg::getInstance( "buy_currency_html" ) );
    if ( buy_currency_floater )
    {
        // pass on flag indicating if we want to buy specific amount and if so, how much
        buy_currency_floater->setShortfall(shortfall);

        // force navigate to new URL
        buy_currency_floater->navigateToFinalURL();

        // make it visible and raise to front
        bool visible = true;
        buy_currency_floater->setVisible( visible );
        bool take_focus = true;
        buy_currency_floater->setFrontmost( take_focus );

        // spec calls for floater to be centered on client window
        //buy_currency_floater->center();
    }
    else
    {
        LL_WARNS() << "Buy Currency (HTML) Floater not found" << LL_ENDL;
    };
}

////////////////////////////////////////////////////////////////////////////////
//
void LLBuyCurrencyHTML::closeDialog()
{
    LLFloaterBuyCurrencyHTML* buy_currency_floater = dynamic_cast< LLFloaterBuyCurrencyHTML* >(LLFloaterReg::getInstance( "buy_currency_html" ) );
    if ( buy_currency_floater )
    {
        buy_currency_floater->closeFloater();
    };
}
