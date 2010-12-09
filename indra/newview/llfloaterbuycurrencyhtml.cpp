/** 
 * @file llfloaterbuycurrencyhtml.cpp
 * @brief buy currency implemented in HTML floater - uses embedded media browser control
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

#include "llfloaterbuycurrencyhtml.h"
#include "llstatusbar.h"

////////////////////////////////////////////////////////////////////////////////
//
LLFloaterBuyCurrencyHTML::LLFloaterBuyCurrencyHTML( const LLSD& key ):
	LLFloater( key ),
	mSpecificSumRequested( false ),
	mMessage( "" ),
	mSum( 0 )
{
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLFloaterBuyCurrencyHTML::postBuild()
{
	// observer media events
	mBrowser = getChild<LLMediaCtrl>( "browser" );
	mBrowser->addObserver( this );

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLFloaterBuyCurrencyHTML::navigateToFinalURL()
{
	// URL for actual currency buy contents is in XUI file
	std::string buy_currency_url = getString( "buy_currency_url" );

	// replace [LANGUAGE] meta-tag with view language
	LLStringUtil::format_map_t replace;

	// viewer language
	replace[ "[LANGUAGE]" ] = LLUI::getLanguage();

	// flag that specific amount requested 
	replace[ "[SPECIFIC_AMOUNT]" ] = ( mSpecificSumRequested ? "y":"n" );

	// amount requested
	std::ostringstream codec( "" );
	codec << mSum;
	replace[ "[SUM]" ] = codec.str();

	// users' current balance
	codec.clear();
	codec.str( "" );
	codec << gStatusBar->getBalance();
	replace[ "[BAL]" ] = codec.str();

	// message - "This cost L$x,xxx for example
	replace[ "[MSG]" ] = LLURI::escape( mMessage );
	LLStringUtil::format( buy_currency_url, replace );

	// write final URL to debug console
	llinfos << "Buy currency HTML prased URL is " << buy_currency_url << llendl;

	// kick off the navigation
	mBrowser->navigateTo( buy_currency_url, "text/html" );
}

////////////////////////////////////////////////////////////////////////////////
//
void LLFloaterBuyCurrencyHTML::handleMediaEvent( LLPluginClassMedia* self, EMediaEvent event )
{
	// placeholder for now - just in case we want to catch media events
	if ( LLPluginClassMediaOwner::MEDIA_EVENT_NAVIGATE_COMPLETE == event )
	{
		// update currency after we complete a navigation since there are many ways 
		// this can result in a different L$ balance
		LLStatusBar::sendMoneyBalanceRequest();
	};
}

////////////////////////////////////////////////////////////////////////////////
//
void LLFloaterBuyCurrencyHTML::onClose( bool app_quitting )
{
	// Update L$ balance one more time
	LLStatusBar::sendMoneyBalanceRequest();

	destroy();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLFloaterBuyCurrencyHTML::setParams( bool specific_sum_requested, const std::string& message, S32 sum )
{
	// save these away - used to construct URL later
	mSpecificSumRequested = specific_sum_requested;
	mMessage = message;
	mSum = sum;
}
