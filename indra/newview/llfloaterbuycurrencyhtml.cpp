/** 
 * @file llfloaterbuycurrencyhtml.cpp
 * @brief buy currency implemented in HTML floater - uses embedded media browser control
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2006-2010, Linden Research, Inc.
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

	// kick off the navigation
	mBrowser->navigateTo( buy_currency_url );
}

////////////////////////////////////////////////////////////////////////////////
//
void LLFloaterBuyCurrencyHTML::handleMediaEvent( LLPluginClassMedia* self, EMediaEvent event )
{
	// placeholder for now - just in case we want to catch media events
	if ( LLPluginClassMediaOwner::MEDIA_EVENT_NAVIGATE_COMPLETE == event )
	{
	};
}

////////////////////////////////////////////////////////////////////////////////
//
void LLFloaterBuyCurrencyHTML::onClose( bool app_quitting )
{
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
