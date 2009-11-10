/**
 * @file llpanelmediasettingssecurity.cpp
 * @brief LLPanelMediaSettingsSecurity class implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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
#include "llfloaterreg.h"
#include "llpanelmediasettingssecurity.h"
#include "llpanelcontents.h"
#include "llcheckboxctrl.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "lluictrlfactory.h"
#include "llwindow.h"
#include "llviewerwindow.h"
#include "llsdutil.h"
#include "llselectmgr.h"
#include "llmediaentry.h"
#include "llfloaterwhitelistentry.h"
#include "llfloatermediasettings.h"
////////////////////////////////////////////////////////////////////////////////
//
LLPanelMediaSettingsSecurity::LLPanelMediaSettingsSecurity() :
	mParent( NULL )
{
	mCommitCallbackRegistrar.add("Media.whitelistAdd",		boost::bind(&LLPanelMediaSettingsSecurity::onBtnAdd, this));
	mCommitCallbackRegistrar.add("Media.whitelistDelete",	boost::bind(&LLPanelMediaSettingsSecurity::onBtnDel, this));	
	// build dialog from XML
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_media_settings_security.xml");

}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLPanelMediaSettingsSecurity::postBuild()
{
	mEnableWhiteList = getChild< LLCheckBoxCtrl >( LLMediaEntry::WHITELIST_ENABLE_KEY );
	mWhiteListList = getChild< LLScrollListCtrl >( LLMediaEntry::WHITELIST_KEY );
	
	setDefaultBtn("whitelist_add");

	return true;
}

////////////////////////////////////////////////////////////////////////////////
// virtual
LLPanelMediaSettingsSecurity::~LLPanelMediaSettingsSecurity()
{
}

////////////////////////////////////////////////////////////////////////////////
// 
void LLPanelMediaSettingsSecurity::draw()
{
	// housekeeping
	LLPanel::draw();

	// if list is empty, disable DEL button and checkbox to enable use of list
	if ( mWhiteListList->isEmpty() )
	{
		childSetEnabled( "whitelist_del", false );
		childSetEnabled( LLMediaEntry::WHITELIST_KEY, false );
		childSetEnabled( LLMediaEntry::WHITELIST_ENABLE_KEY, false );
	}
	else
	{
		childSetEnabled( "whitelist_del", true );
		childSetEnabled( LLMediaEntry::WHITELIST_KEY, true );
		childSetEnabled( LLMediaEntry::WHITELIST_ENABLE_KEY, true );
	};

	// if nothing is selected, disable DEL button
	if ( mWhiteListList->getSelectedValue().asString().empty() )
	{
		childSetEnabled( "whitelist_del", false );
	}
	else
	{
		childSetEnabled( "whitelist_del", true );
	};
}

////////////////////////////////////////////////////////////////////////////////
// static 
void LLPanelMediaSettingsSecurity::initValues( void* userdata, const LLSD& media_settings , bool editable)
{
	LLPanelMediaSettingsSecurity *self =(LLPanelMediaSettingsSecurity *)userdata;

	if ( LLFloaterMediaSettings::getInstance()->mIdenticalHasMediaInfo )
	{
		if(LLFloaterMediaSettings::getInstance()->mMultipleMedia) 
		{
			self->clearValues(self, editable);
			// only show multiple 
			return;
		}
		
	}
	else
	{
		if(LLFloaterMediaSettings::getInstance()->mMultipleValidMedia) 
		{
			self->clearValues(self, editable);
			// only show multiple 
			return;
		}			
		
	}
	std::string base_key( "" );
	std::string tentative_key( "" );

	struct 
	{
		std::string key_name;
		LLUICtrl* ctrl_ptr;
		std::string ctrl_type;

	} data_set [] = 
	{
		{ LLMediaEntry::WHITELIST_ENABLE_KEY,	self->mEnableWhiteList,		"LLCheckBoxCtrl" },
		{ LLMediaEntry::WHITELIST_KEY,			self->mWhiteListList,		"LLScrollListCtrl" },
		{ "", NULL , "" }
	};

	for( int i = 0; data_set[ i ].key_name.length() > 0; ++i )
	{
		base_key = std::string( data_set[ i ].key_name );
        tentative_key = base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX );

		// TODO: CP - I bet there is a better way to do this using Boost
		if ( media_settings[ base_key ].isDefined() )
		{
			if ( data_set[ i ].ctrl_type == "LLCheckBoxCtrl" )
			{
				static_cast< LLCheckBoxCtrl* >( data_set[ i ].ctrl_ptr )->
					setValue( media_settings[ base_key ].asBoolean() );
			}
			else
			if ( data_set[ i ].ctrl_type == "LLScrollListCtrl" )
			{
				// get control 
				LLScrollListCtrl* list = static_cast< LLScrollListCtrl* >( data_set[ i ].ctrl_ptr );
				list->deleteAllItems();

				// points to list of white list URLs
				LLSD url_list = media_settings[ base_key ];

				// iterate over them and add to scroll list
				LLSD::array_iterator iter = url_list.beginArray();
				while( iter != url_list.endArray() )
				{
					// TODO: is iter guaranteed to be valid here?
					std::string url = *iter;
					list->addSimpleElement( url );
					++iter;
				};
			};
			data_set[ i ].ctrl_ptr->setEnabled(editable);
			data_set[ i ].ctrl_ptr->setTentative( media_settings[ tentative_key ].asBoolean() );
		};
	};
}

////////////////////////////////////////////////////////////////////////////////
// static 
void LLPanelMediaSettingsSecurity::clearValues( void* userdata , bool editable)
{
	LLPanelMediaSettingsSecurity *self =(LLPanelMediaSettingsSecurity *)userdata;
	self->mEnableWhiteList->clear();
	self->mWhiteListList->deleteAllItems();
	self->mEnableWhiteList->setEnabled(editable);
	self->mWhiteListList->setEnabled(editable);
}

////////////////////////////////////////////////////////////////////////////////
// 
void LLPanelMediaSettingsSecurity::preApply()
{
	// no-op
}

////////////////////////////////////////////////////////////////////////////////
//
void LLPanelMediaSettingsSecurity::getValues( LLSD &fill_me_in )
{
    fill_me_in[LLMediaEntry::WHITELIST_ENABLE_KEY] = mEnableWhiteList->getValue();

    // iterate over white list and extract items
    std::vector< LLScrollListItem* > white_list_items = mWhiteListList->getAllData();
    std::vector< LLScrollListItem* >::iterator iter = white_list_items.begin();
    fill_me_in.erase(LLMediaEntry::WHITELIST_KEY);
    while( iter != white_list_items.end() )
    {
        std::string white_list_url = (*iter)->getValue().asString();
        fill_me_in[ LLMediaEntry::WHITELIST_KEY ].append( white_list_url );
        ++iter;
    };
}

////////////////////////////////////////////////////////////////////////////////
// 
void LLPanelMediaSettingsSecurity::postApply()
{
	// no-op
}

///////////////////////////////////////////////////////////////////////////////
// Try to make a valid URL if a fragment (
// white list list box widget and build a list to test against. Can also
const std::string LLPanelMediaSettingsSecurity::makeValidUrl( const std::string& src_url )
{
	// use LLURI to determine if we have a valid scheme
	LLURI candidate_url( src_url );
	if ( candidate_url.scheme().empty() )
	{
		// build a URL comprised of default scheme and the original fragment 
		const std::string default_scheme( "http://" );
		return default_scheme + src_url;
	};

	// we *could* test the "default scheme" + "original fragment" URL again
	// using LLURI to see if it's valid but I think the outcome is the same
	// in either case - our only option is to return the original URL

	// we *think* the original url passed in was valid
	return src_url;
}

///////////////////////////////////////////////////////////////////////////////
// wrapper for testing a URL against the whitelist. We grab entries from
// white list list box widget and build a list to test against. Can also
// optionally pass the URL that you are trying to add to the widget since
// it won't be added until this call returns.
bool LLPanelMediaSettingsSecurity::passesWhiteList( const std::string& added_url,
													const std::string& test_url )
{
	// the checkUrlAgainstWhitelist(..) function works on a vector
	// of strings for the white list entries - in this panel, the white list
	// is stored in the widgets themselves so we need to build something compatible.
	std::vector< std::string > whitelist_strings;
	whitelist_strings.clear();	// may not be required - I forget what the spec says.

	// step through whitelist widget entries and grab them as strings
    std::vector< LLScrollListItem* > white_list_items = mWhiteListList->getAllData();
    std::vector< LLScrollListItem* >::iterator iter = white_list_items.begin(); 
	while( iter != white_list_items.end()  )
    {
        const std::string whitelist_url = (*iter)->getValue().asString();
		whitelist_strings.push_back( whitelist_url );

		++iter;
    };

	// add in the URL that might be added to the whitelist so we can test that too
	if ( added_url.length() )
		whitelist_strings.push_back( added_url );

	// possible the URL is just a fragment so we validize it
	const std::string valid_url = makeValidUrl( test_url );

	// indicate if the URL passes whitelist
	return LLMediaEntry::checkUrlAgainstWhitelist( valid_url, whitelist_strings );
}

///////////////////////////////////////////////////////////////////////////////
//
void LLPanelMediaSettingsSecurity::addWhiteListItem(const std::string& url)
{
	// grab home URL from the general panel (via the parent floater)
	std::string home_url( "" );
	if ( mParent )
		home_url = mParent->getHomeUrl();

	// if the home URL is blank (user hasn't entered it yet) then
	// don't bother to check if it passes the white list
	if ( home_url.empty() )
	{
		mWhiteListList->addSimpleElement( url );
		return;
	};

	// if the URL passes the white list, add it
	if ( passesWhiteList( url, home_url ) )
	{
		mWhiteListList->addSimpleElement( url );
	}
	else
	// display a message indicating you can't do that
	{
		LLNotifications::instance().add("WhiteListInvalidatesHomeUrl");
	};
}

///////////////////////////////////////////////////////////////////////////////
// static
void LLPanelMediaSettingsSecurity::onBtnAdd( void* userdata )
{
	LLFloaterReg::showInstance("whitelist_entry");
}

///////////////////////////////////////////////////////////////////////////////
// static
void LLPanelMediaSettingsSecurity::onBtnDel( void* userdata )
{
	LLPanelMediaSettingsSecurity *self =(LLPanelMediaSettingsSecurity *)userdata;

	self->mWhiteListList->deleteSelectedItems();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLPanelMediaSettingsSecurity::setParent( LLFloaterMediaSettings* parent )
{
	mParent = parent;
};

