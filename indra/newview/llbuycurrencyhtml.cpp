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

// support for secondlife:///app/buycurrencyhtml/{ACTION}/{NEXT_ACTION}/{RETURN_CODE} SLapps
class LLBuyCurrencyHTMLHandler : 
	public LLCommandHandler
{
public:
	// requests will be throttled from a non-trusted browser
	LLBuyCurrencyHTMLHandler() : LLCommandHandler( "buycurrencyhtml", UNTRUSTED_ALLOW ) {}

	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
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

////////////////////////////////////////////////////////////////////////////////
// static
// Opens the legacy XUI based floater or new HTML based one based on 
// the QuickBuyCurrency value in settings.xml - this overload is for
// the case where the amount is not requested.
void LLBuyCurrencyHTML::openCurrencyFloater()
{
	if ( gSavedSettings.getBOOL( "QuickBuyCurrency" ) )
	{
		// HTML version
		LLBuyCurrencyHTML::showDialog( false, "", 0 );
	}
	else
	{
		// legacy version
		LLFloaterBuyCurrency::buyCurrency();
	};
}

////////////////////////////////////////////////////////////////////////////////
// static
// Opens the legacy XUI based floater or new HTML based one based on 
// the QuickBuyCurrency value in settings.xml - this overload is for
// the case where the amount and a string to display are requested.
void LLBuyCurrencyHTML::openCurrencyFloater( const std::string& message, S32 sum )
{
	if ( gSavedSettings.getBOOL( "QuickBuyCurrency" ) )
	{
		// HTML version
		LLBuyCurrencyHTML::showDialog( true, message, sum );
	}
	else
	{
		// legacy version
		LLFloaterBuyCurrency::buyCurrency( message, sum );
	};
}

////////////////////////////////////////////////////////////////////////////////
// static
void LLBuyCurrencyHTML::showDialog( bool specific_sum_requested, const std::string& message, S32 sum )
{
	LLFloaterBuyCurrencyHTML* buy_currency_floater = dynamic_cast< LLFloaterBuyCurrencyHTML* >( LLFloaterReg::getInstance( "buy_currency_html" ) );
	if ( buy_currency_floater )
	{
		// pass on flag indicating if we want to buy specific amount and if so, how much
		buy_currency_floater->setParams( specific_sum_requested, message, sum );

		// force navigate to new URL
		buy_currency_floater->navigateToFinalURL();

		// make it visible and raise to front
		BOOL visible = TRUE;
		buy_currency_floater->setVisible( visible );
		BOOL take_focus = TRUE;
		buy_currency_floater->setFrontmost( take_focus );

		// spec calls for floater to be centered on client window
		buy_currency_floater->center();
	}
	else
	{
		llwarns << "Buy Currency (HTML) Floater not found" << llendl;
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
	
	// Update L$ balance in the status bar in case L$ were purchased
	LLStatusBar::sendMoneyBalanceRequest();
}
