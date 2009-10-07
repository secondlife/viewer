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
LLPanelMediaSettingsSecurity::LLPanelMediaSettingsSecurity()
{
	// build dialog from XML
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_media_settings_security.xml");
	mCommitCallbackRegistrar.add("Media.whitelistAdd",		boost::bind(&LLPanelMediaSettingsSecurity::onBtnAdd, this));
	mCommitCallbackRegistrar.add("Media.whitelistDelete",	boost::bind(&LLPanelMediaSettingsSecurity::onBtnDel, this));	
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLPanelMediaSettingsSecurity::postBuild()
{
	mEnableWhiteList = getChild< LLCheckBoxCtrl >( LLMediaEntry::WHITELIST_ENABLE_KEY );
	mWhiteListList = getChild< LLScrollListCtrl >( LLMediaEntry::WHITELIST_KEY );

	childSetAction("whitelist_add", onBtnAdd, this);
	childSetAction("whitelist_del", onBtnDel, this);

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
// static
void LLPanelMediaSettingsSecurity::apply( void* userdata )
{
	LLPanelMediaSettingsSecurity *self =(LLPanelMediaSettingsSecurity *)userdata;

	// build LLSD Fragment
	LLSD media_data_security;
	self->getValues(media_data_security);
	// this merges contents of LLSD passed in with what's there so this is ok
	LLSelectMgr::getInstance()->selectionSetMediaData( media_data_security );
}

////////////////////////////////////////////////////////////////////////////////
//
void LLPanelMediaSettingsSecurity::getValues( LLSD &fill_me_in )
{
    fill_me_in[LLMediaEntry::WHITELIST_ENABLE_KEY] = mEnableWhiteList->getValue();

    // iterate over white list and extract items
    std::vector< LLScrollListItem* > white_list_items = mWhiteListList->getAllData();
    std::vector< LLScrollListItem* >::iterator iter = white_list_items.begin();
    fill_me_in[LLMediaEntry::WHITELIST_KEY].clear();
    while( iter != white_list_items.end() )
    {
        std::string white_list_url = (*iter)->getValue().asString();
        fill_me_in[ LLMediaEntry::WHITELIST_KEY ].append( white_list_url );
        ++iter;
    };
}


///////////////////////////////////////////////////////////////////////////////
// static
void LLPanelMediaSettingsSecurity::addWhiteListItem(const std::string& url)
{
	mWhiteListList->addSimpleElement( url );
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
