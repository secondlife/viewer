/**
 * @file llpanelmediasettingsgeneral.cpp
 * @brief LLPanelMediaSettingsGeneral class implementation
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

#include "llpanelmediasettingsgeneral.h"
#include "llcombobox.h"
#include "llcheckboxctrl.h"
#include "llspinctrl.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llsdutil.h"
#include "llselectmgr.h"
#include "llbutton.h"
#include "lltexturectrl.h"
#include "llurl.h"
#include "llwindow.h"
#include "llmediaentry.h"
#include "llmediactrl.h"
#include "llpanelcontents.h"
#include "llpluginclassmedia.h"
#include "llfloatermediasettings.h"

////////////////////////////////////////////////////////////////////////////////
//
LLPanelMediaSettingsGeneral::LLPanelMediaSettingsGeneral() :
	mControls( NULL ),
	mAutoLoop( NULL ),
	mFirstClick( NULL ),
	mAutoZoom( NULL ),
	mAutoPlay( NULL ),
	mAutoScale( NULL ),
	mWidthPixels( NULL ),
	mHeightPixels( NULL ),
	mHomeURL( NULL ),
	mCurrentURL( NULL ),
	mAltImageEnable( NULL ),
	mParent( NULL )
{
	// build dialog from XML
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_media_settings_general.xml");
	mCommitCallbackRegistrar.add("Media.ResetCurrentUrl",		boost::bind(&LLPanelMediaSettingsGeneral::onBtnResetCurrentUrl, this));
//	mCommitCallbackRegistrar.add("Media.CommitHomeURL",			boost::bind(&LLPanelMediaSettingsGeneral::onCommitHomeURL, this));	

}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLPanelMediaSettingsGeneral::postBuild()
{
	// connect member vars with UI widgets
    mAltImageEnable = getChild< LLCheckBoxCtrl >( LLMediaEntry::ALT_IMAGE_ENABLE_KEY );
	mAutoLoop = getChild< LLCheckBoxCtrl >( LLMediaEntry::AUTO_LOOP_KEY );
	mAutoPlay = getChild< LLCheckBoxCtrl >( LLMediaEntry::AUTO_PLAY_KEY );
	mAutoScale = getChild< LLCheckBoxCtrl >( LLMediaEntry::AUTO_SCALE_KEY );
	mAutoZoom = getChild< LLCheckBoxCtrl >( LLMediaEntry::AUTO_ZOOM_KEY );
	mControls = getChild< LLComboBox >( LLMediaEntry::CONTROLS_KEY );
	mCurrentURL = getChild< LLLineEditor >( LLMediaEntry::CURRENT_URL_KEY );
	mFirstClick = getChild< LLCheckBoxCtrl >( LLMediaEntry::FIRST_CLICK_INTERACT_KEY );
	mHeightPixels = getChild< LLSpinCtrl >( LLMediaEntry::HEIGHT_PIXELS_KEY );
	mHomeURL = getChild< LLLineEditor >( LLMediaEntry::HOME_URL_KEY );
	mWidthPixels = getChild< LLSpinCtrl >( LLMediaEntry::WIDTH_PIXELS_KEY );
	mPreviewMedia = getChild<LLMediaCtrl>("preview_media");

	// watch commit action for HOME URL
	childSetCommitCallback( LLMediaEntry::HOME_URL_KEY, onCommitHomeURL, this);

	// interrogates controls and updates widgets as required
	updateMediaPreview();
	updateCurrentURL();

	return true;
}

////////////////////////////////////////////////////////////////////////////////
// virtual
LLPanelMediaSettingsGeneral::~LLPanelMediaSettingsGeneral()
{
}

////////////////////////////////////////////////////////////////////////////////
// static
void LLPanelMediaSettingsGeneral::draw()
{
	// housekeeping
	LLPanel::draw();

	// enable/disable pixel values image entry based on auto scale checkbox 
	if ( mAutoScale->getValue().asBoolean() == false )
	{
		childSetEnabled( LLMediaEntry::WIDTH_PIXELS_KEY, true );
		childSetEnabled( LLMediaEntry::HEIGHT_PIXELS_KEY, true );
	}
	else
	{
		childSetEnabled( LLMediaEntry::WIDTH_PIXELS_KEY, false );
		childSetEnabled( LLMediaEntry::HEIGHT_PIXELS_KEY, false );
	};

	// enable/disable UI based on type of media
	bool reset_button_is_active = true;
	if( mPreviewMedia )
	{
		LLPluginClassMedia* media_plugin = mPreviewMedia->getMediaPlugin();
		if( media_plugin )
		{
			// some controls are only appropriate for time or browser type plugins
			// so we selectively enable/disable them - need to do it in draw
			// because the information from plugins arrives assynchronously
			bool show_time_controls = media_plugin->pluginSupportsMediaTime();
			if ( show_time_controls )
			{
				childSetEnabled( LLMediaEntry::CURRENT_URL_KEY, false );
				reset_button_is_active = false;
				childSetEnabled( "current_url_label", false );
				childSetEnabled( LLMediaEntry::AUTO_LOOP_KEY, true );
			}
			else
			{
				childSetEnabled( LLMediaEntry::CURRENT_URL_KEY, true );
				reset_button_is_active = true;
				childSetEnabled( "current_url_label", true );
				childSetEnabled( LLMediaEntry::AUTO_LOOP_KEY, false );
			};
		};
	};

	// current URL can change over time.
	updateCurrentURL();

	// enable/disable RESRET button depending on permissions
	// since this is the same as a navigate action
	U32 owner_mask_on;
	U32 owner_mask_off;
	U32 valid_owner_perms = LLSelectMgr::getInstance()->selectGetPerm( PERM_OWNER, 
												&owner_mask_on, &owner_mask_off );
	U32 group_mask_on;
	U32 group_mask_off;
	U32 valid_group_perms = LLSelectMgr::getInstance()->selectGetPerm( PERM_GROUP, 
												&group_mask_on, &group_mask_off );
	U32 everyone_mask_on;
	U32 everyone_mask_off;
	S32 valid_everyone_perms = LLSelectMgr::getInstance()->selectGetPerm( PERM_EVERYONE, 
												&everyone_mask_on, &everyone_mask_off );
	
	bool user_can_press_reset = false;

	// if perms we got back are valid
	if ( valid_owner_perms &&
		 valid_group_perms && 
		 valid_everyone_perms )
	{
		// if user is allowed to press the RESET button
		if ( ( owner_mask_on & PERM_MODIFY ) ||
			 ( group_mask_on & PERM_MODIFY ) || 
			 ( group_mask_on & PERM_MODIFY ) )
		{
			user_can_press_reset = true;
		}
		else
		// user is NOT allowed to press the RESET button
		{
			user_can_press_reset = false;
		};
	};

	// several places modify this widget so we must collect states in one place
	if ( reset_button_is_active )
	{
		// user has perms to press reset button and it is active
		if ( user_can_press_reset )
		{
			childSetEnabled( "current_url_reset_btn", true );
		}
		// user does not has perms to press reset button and it is active
		else
		{
			childSetEnabled( "current_url_reset_btn", false );
		};
	}
	else
	// reset button is inactive so we just slam it to off - other states don't matter
	{
		childSetEnabled( "current_url_reset_btn", false );
	};
}

////////////////////////////////////////////////////////////////////////////////
// static 
void LLPanelMediaSettingsGeneral::clearValues( void* userdata )
{	
	LLPanelMediaSettingsGeneral *self =(LLPanelMediaSettingsGeneral *)userdata;
	self->mAltImageEnable ->clear();
	self->mAutoLoop->clear();
	self->mAutoPlay->clear();
	self->mAutoScale->clear();
	self->mAutoZoom ->clear();
	self->mControls->clear();
	self->mCurrentURL->clear();
	self->mFirstClick->clear();
	self->mHeightPixels->clear();
	self->mHomeURL->clear();
	self->mWidthPixels->clear();
	self->mPreviewMedia->unloadMediaSource(); 
}

////////////////////////////////////////////////////////////////////////////////
// static 
void LLPanelMediaSettingsGeneral::initValues( void* userdata, const LLSD& media_settings )
{
	LLPanelMediaSettingsGeneral *self =(LLPanelMediaSettingsGeneral *)userdata;

	//llinfos << "---------------" << llendl;
	//llinfos << ll_pretty_print_sd(media_settings) << llendl;
	//llinfos << "---------------" << llendl;

	std::string base_key( "" );
	std::string tentative_key( "" );

	struct 
	{
		std::string key_name;
		LLUICtrl* ctrl_ptr;
		std::string ctrl_type;

	} data_set [] = 
	{ 
        { LLMediaEntry::AUTO_LOOP_KEY,				self->mAutoLoop,		"LLCheckBoxCtrl" },
		{ LLMediaEntry::AUTO_PLAY_KEY,				self->mAutoPlay,		"LLCheckBoxCtrl" },
		{ LLMediaEntry::AUTO_SCALE_KEY,				self->mAutoScale,		"LLCheckBoxCtrl" },
		{ LLMediaEntry::AUTO_ZOOM_KEY,				self->mAutoZoom,		"LLCheckBoxCtrl" },
		{ LLMediaEntry::CONTROLS_KEY,				self->mControls,		"LLComboBox" },
		{ LLMediaEntry::CURRENT_URL_KEY,			self->mCurrentURL,		"LLLineEditor" },
		{ LLMediaEntry::HEIGHT_PIXELS_KEY,			self->mHeightPixels,	"LLSpinCtrl" },
		{ LLMediaEntry::HOME_URL_KEY,				self->mHomeURL,			"LLLineEditor" },
		{ LLMediaEntry::FIRST_CLICK_INTERACT_KEY,	self->mFirstClick,		"LLCheckBoxCtrl" },
		{ LLMediaEntry::WIDTH_PIXELS_KEY,			self->mWidthPixels,		"LLSpinCtrl" },
		{ LLMediaEntry::ALT_IMAGE_ENABLE_KEY,		self->mAltImageEnable,	"LLCheckBoxCtrl" },
		{ "", NULL , "" }
	};

	for( int i = 0; data_set[ i ].key_name.length() > 0; ++i )
	{
		base_key = std::string( data_set[ i ].key_name );
		tentative_key = base_key + std::string( LLPanelContents::TENTATIVE_SUFFIX );
		// TODO: CP - I bet there is a better way to do this using Boost
		if ( media_settings[ base_key ].isDefined() )
		{
			if ( data_set[ i ].ctrl_type == "LLLineEditor" )
			{
				static_cast< LLLineEditor* >( data_set[ i ].ctrl_ptr )->
					setText( media_settings[ base_key ].asString() );
			}
			else
			if ( data_set[ i ].ctrl_type == "LLCheckBoxCtrl" )
				static_cast< LLCheckBoxCtrl* >( data_set[ i ].ctrl_ptr )->
					setValue( media_settings[ base_key ].asBoolean() );
			else
			if ( data_set[ i ].ctrl_type == "LLComboBox" )
				static_cast< LLComboBox* >( data_set[ i ].ctrl_ptr )->
					setCurrentByIndex( media_settings[ base_key ].asInteger() );
			else
			if ( data_set[ i ].ctrl_type == "LLSpinCtrl" )
				static_cast< LLSpinCtrl* >( data_set[ i ].ctrl_ptr )->
					setValue( media_settings[ base_key ].asInteger() );

			data_set[ i ].ctrl_ptr->setTentative( media_settings[ tentative_key ].asBoolean() );
		};
	};

	// interrogates controls and updates widgets as required
	self->updateMediaPreview();
	self->updateCurrentURL();
}

////////////////////////////////////////////////////////////////////////////////
// Helper to set media control to media URL as required
void LLPanelMediaSettingsGeneral::updateMediaPreview()
{
	if ( mHomeURL->getValue().asString().length() > 0 )
	{
		mPreviewMedia->navigateTo( mHomeURL->getValue().asString() );
	}
	else
	// new home URL will be empty if media is deleted but
	// we still need to clean out the preview.
	{
		mPreviewMedia->unloadMediaSource();
	};
}

////////////////////////////////////////////////////////////////////////////////
// Helper to set current URL
void LLPanelMediaSettingsGeneral::updateCurrentURL()
{
	if( mPreviewMedia )
	{
		LLPluginClassMedia* media_plugin = mPreviewMedia->getMediaPlugin();
		if( media_plugin )
		{
			// get current URL from plugin and display
			std::string current_location = media_plugin->getLocation();
			if ( current_location.length() )
			{
				childSetText( "current_url", current_location );
			}
			else
			// current location may be empty so we need to clear it
			{
				const std::string empty_string( "" );
				childSetText( "current_url", empty_string );
			};
		};
	};
}

////////////////////////////////////////////////////////////////////////////////

void LLPanelMediaSettingsGeneral::onClose()
{
	if(mPreviewMedia)
	{
		mPreviewMedia->unloadMediaSource();
	}
}

////////////////////////////////////////////////////////////////////////////////
// static
void LLPanelMediaSettingsGeneral::onCommitHomeURL( LLUICtrl* ctrl, void *userdata )
{
	LLPanelMediaSettingsGeneral* self =(LLPanelMediaSettingsGeneral *)userdata;
	self->updateMediaPreview();
}


////////////////////////////////////////////////////////////////////////////////
void LLPanelMediaSettingsGeneral::onBtnResetCurrentUrl()
{
	// TODO: reset home URL but need to consider permissions too
	//LLPanelMediaSettingsGeneral* self =(LLPanelMediaSettingsGeneral *)userdata;
}

////////////////////////////////////////////////////////////////////////////////
// static
void LLPanelMediaSettingsGeneral::apply( void* userdata )
{
	LLPanelMediaSettingsGeneral *self =(LLPanelMediaSettingsGeneral *)userdata;

	// build LLSD Fragment
	LLSD media_data_general;
	self->getValues(media_data_general);

	// this merges contents of LLSD passed in with what's there so this is ok
	LLSelectMgr::getInstance()->selectionSetMediaData( media_data_general );
}

////////////////////////////////////////////////////////////////////////////////
//
void LLPanelMediaSettingsGeneral::getValues( LLSD &fill_me_in )
{
    fill_me_in[LLMediaEntry::ALT_IMAGE_ENABLE_KEY] = mAltImageEnable->getValue();
    fill_me_in[LLMediaEntry::AUTO_LOOP_KEY] = mAutoLoop->getValue();
    fill_me_in[LLMediaEntry::AUTO_PLAY_KEY] = mAutoPlay->getValue();
    fill_me_in[LLMediaEntry::AUTO_SCALE_KEY] = mAutoScale->getValue();
    fill_me_in[LLMediaEntry::AUTO_ZOOM_KEY] = mAutoZoom->getValue();
    fill_me_in[LLMediaEntry::CONTROLS_KEY] = mControls->getCurrentIndex();
    // XXX Don't send current URL!
    //fill_me_in[LLMediaEntry::CURRENT_URL_KEY] = mCurrentURL->getValue();
    fill_me_in[LLMediaEntry::HEIGHT_PIXELS_KEY] = mHeightPixels->getValue();
    fill_me_in[LLMediaEntry::HOME_URL_KEY] = mHomeURL->getValue();
    fill_me_in[LLMediaEntry::FIRST_CLICK_INTERACT_KEY] = mFirstClick->getValue();
    fill_me_in[LLMediaEntry::WIDTH_PIXELS_KEY] = mWidthPixels->getValue();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLPanelMediaSettingsGeneral::setParent( LLFloaterMediaSettings* parent )
{
	mParent = parent;
};
