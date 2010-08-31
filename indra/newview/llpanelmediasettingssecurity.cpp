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

#include "llpanelmediasettingssecurity.h"

#include "llfloaterreg.h"
#include "llpanelcontents.h"
#include "llcheckboxctrl.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "lluictrlfactory.h"
#include "llwindow.h"
#include "llviewerwindow.h"
#include "llsdutil.h"
#include "llselectmgr.h"
#include "llmediaentry.h"
#include "lltextbox.h"
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
	mHomeUrlFailsWhiteListText = getChild<LLTextBox>( "home_url_fails_whitelist" );
	
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
}

////////////////////////////////////////////////////////////////////////////////
// static 
void LLPanelMediaSettingsSecurity::initValues( void* userdata, const LLSD& media_settings , bool editable)
{
	LLPanelMediaSettingsSecurity *self =(LLPanelMediaSettingsSecurity *)userdata;
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

		bool enabled_overridden = false;
		
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
				
				// better be the whitelist
				llassert(data_set[ i ].ctrl_ptr == self->mWhiteListList);
				
				// If tentative, don't add entries
				if (media_settings[ tentative_key ].asBoolean())
				{
					self->mWhiteListList->setEnabled(false);
					enabled_overridden = true;
				}
				else {
					// iterate over them and add to scroll list
					LLSD::array_iterator iter = url_list.beginArray();
					while( iter != url_list.endArray() )
					{
						std::string entry = *iter;
						self->addWhiteListEntry( entry );
						++iter;
					}
				}
			};
			if ( ! enabled_overridden) data_set[ i ].ctrl_ptr->setEnabled(editable);
			data_set[ i ].ctrl_ptr->setTentative( media_settings[ tentative_key ].asBoolean() );
		};
	};

	// initial update - hides/shows status messages etc.
	self->updateWhitelistEnableStatus();
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
void LLPanelMediaSettingsSecurity::getValues( LLSD &fill_me_in, bool include_tentative )
{
    if (include_tentative || !mEnableWhiteList->getTentative()) 
		fill_me_in[LLMediaEntry::WHITELIST_ENABLE_KEY] = (LLSD::Boolean)mEnableWhiteList->getValue();
	
	if (include_tentative || !mWhiteListList->getTentative())
	{
		// iterate over white list and extract items
		std::vector< LLScrollListItem* > whitelist_items = mWhiteListList->getAllData();
		std::vector< LLScrollListItem* >::iterator iter = whitelist_items.begin();
		
		// *NOTE: need actually set the key to be an emptyArray(), or the merge
		// we do with this LLSD will think there's nothing to change.
		fill_me_in[LLMediaEntry::WHITELIST_KEY] = LLSD::emptyArray();
		while( iter != whitelist_items.end() )
		{
			LLScrollListCell* cell = (*iter)->getColumn( ENTRY_COLUMN );
			std::string whitelist_url = cell->getValue().asString();
			
			fill_me_in[ LLMediaEntry::WHITELIST_KEY ].append( whitelist_url );
			++iter;
		};
	}
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
// white list list box widget and build a list to test against. 
bool LLPanelMediaSettingsSecurity::urlPassesWhiteList( const std::string& test_url )
{
	// If the whitlelist list is tentative, it means we have multiple settings.
	// In that case, we have no choice but to return true
	if ( mWhiteListList->getTentative() ) return true;
	
	// the checkUrlAgainstWhitelist(..) function works on a vector
	// of strings for the white list entries - in this panel, the white list
	// is stored in the widgets themselves so we need to build something compatible.
	std::vector< std::string > whitelist_strings;
	whitelist_strings.clear();	// may not be required - I forget what the spec says.

	// step through whitelist widget entries and grab them as strings
    std::vector< LLScrollListItem* > whitelist_items = mWhiteListList->getAllData();
    std::vector< LLScrollListItem* >::iterator iter = whitelist_items.begin(); 
	while( iter != whitelist_items.end()  )
    {
		LLScrollListCell* cell = (*iter)->getColumn( ENTRY_COLUMN );
		std::string whitelist_url = cell->getValue().asString();

		whitelist_strings.push_back( whitelist_url );

		++iter;
    };

	// possible the URL is just a fragment so we validize it
	const std::string valid_url = makeValidUrl( test_url );

	// indicate if the URL passes whitelist
	return LLMediaEntry::checkUrlAgainstWhitelist( valid_url, whitelist_strings );
}

///////////////////////////////////////////////////////////////////////////////
//
void LLPanelMediaSettingsSecurity::updateWhitelistEnableStatus()
{
	// get the value for home URL and make it a valid URL
	const std::string valid_url = makeValidUrl( mParent->getHomeUrl() );

	// now check to see if the home url passes the whitelist in its entirity 
	if ( urlPassesWhiteList( valid_url ) )
	{
		mEnableWhiteList->setEnabled( true );
		mHomeUrlFailsWhiteListText->setVisible( false );
	}
	else
	{
		mEnableWhiteList->set( false );
		mEnableWhiteList->setEnabled( false );
		mHomeUrlFailsWhiteListText->setVisible( true );
	};
}

///////////////////////////////////////////////////////////////////////////////
// Add an entry to the whitelist scrollbox and indicate if the current
// home URL passes this entry or not using an icon
void LLPanelMediaSettingsSecurity::addWhiteListEntry( const std::string& entry )
{
	// grab the home url
	std::string home_url( "" );
	if ( mParent )
		home_url = mParent->getHomeUrl();

	// try to make a valid URL based on what the user entered - missing scheme for example
	const std::string valid_url = makeValidUrl( home_url );

	// check the home url against this single whitelist entry
	std::vector< std::string > whitelist_entries;
	whitelist_entries.push_back( entry );
	bool home_url_passes_entry = LLMediaEntry::checkUrlAgainstWhitelist( valid_url, whitelist_entries );

	// build an icon cell based on whether or not the home url pases it or not
	LLSD row;
	if ( home_url_passes_entry || home_url.empty() )
	{
		row[ "columns" ][ ICON_COLUMN ][ "type" ] = "icon";
		row[ "columns" ][ ICON_COLUMN ][ "value" ] = "";
		row[ "columns" ][ ICON_COLUMN ][ "width" ] = 20;
	}
	else
	{
		row[ "columns" ][ ICON_COLUMN ][ "type" ] = "icon";
		row[ "columns" ][ ICON_COLUMN ][ "value" ] = "Parcel_Exp_Color";
		row[ "columns" ][ ICON_COLUMN ][ "width" ] = 20;
	};

	// always add in the entry itself
	row[ "columns" ][ ENTRY_COLUMN ][ "type" ] = "text";
	row[ "columns" ][ ENTRY_COLUMN ][ "value" ] = entry;
	
	// add to the white list scroll box
	mWhiteListList->addElement( row );
};

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

	// contents of whitelist changed so recheck it against home url
	self->updateWhitelistEnableStatus();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLPanelMediaSettingsSecurity::setParent( LLFloaterMediaSettings* parent )
{
	mParent = parent;
};
