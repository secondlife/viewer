/**
 * @file llpanelmediasettingsgeneral.cpp
 * @brief LLPanelMediaSettingsGeneral class implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llpanelmediasettingsgeneral.h"

// library includes
#include "llcombobox.h"
#include "llcheckboxctrl.h"
#include "llnotificationsutil.h"
#include "llspinctrl.h"
#include "lluictrlfactory.h"

// project includes
#include "llagent.h"
#include "llviewerwindow.h"
#include "llviewermedia.h"
#include "llsdutil.h"
#include "llselectmgr.h"
#include "llbutton.h"
#include "lltexturectrl.h"
#include "llurl.h"
#include "llwindow.h"
#include "llmediaentry.h"
#include "llmediactrl.h"
#include "llpanelcontents.h"
#include "llpermissions.h"
#include "llpluginclassmedia.h"
#include "llfloatermediasettings.h"
#include "llfloatertools.h"
#include "lltrans.h"
#include "lltextbox.h"
#include "llpanelmediasettingssecurity.h"

const char *CHECKERBOARD_DATA_URL = "data:image/svg+xml,%3Csvg xmlns=%22http://www.w3.org/2000/svg%22 width=%22100%%22 height=%22100%%22 %3E%3Cdefs%3E%3Cpattern id=%22checker%22 patternUnits=%22userSpaceOnUse%22 x=%220%22 y=%220%22 width=%22128%22 height=%22128%22 viewBox=%220 0 128 128%22 %3E%3Crect x=%220%22 y=%220%22 width=%2264%22 height=%2264%22 fill=%22#ddddff%22 /%3E%3Crect x=%2264%22 y=%2264%22 width=%2264%22 height=%2264%22 fill=%22#ddddff%22 /%3E%3C/pattern%3E%3C/defs%3E%3Crect x=%220%22 y=%220%22 width=%22100%%22 height=%22100%%22 fill=%22url(#checker)%22 /%3E%3C/svg%3E";

////////////////////////////////////////////////////////////////////////////////
//
LLPanelMediaSettingsGeneral::LLPanelMediaSettingsGeneral() :
	mAutoLoop( NULL ),
	mFirstClick( NULL ),
	mAutoZoom( NULL ),
	mAutoPlay( NULL ),
	mAutoScale( NULL ),
	mWidthPixels( NULL ),
	mHeightPixels( NULL ),
	mHomeURL( NULL ),
	mCurrentURL( NULL ),
	mParent( NULL ),
	mMediaEditable(false)
{
	// build dialog from XML
	buildFromFile( "panel_media_settings_general.xml");
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLPanelMediaSettingsGeneral::postBuild()
{
	// connect member vars with UI widgets
	mAutoLoop = getChild< LLCheckBoxCtrl >( LLMediaEntry::AUTO_LOOP_KEY );
	mAutoPlay = getChild< LLCheckBoxCtrl >( LLMediaEntry::AUTO_PLAY_KEY );
	mAutoScale = getChild< LLCheckBoxCtrl >( LLMediaEntry::AUTO_SCALE_KEY );
	mAutoZoom = getChild< LLCheckBoxCtrl >( LLMediaEntry::AUTO_ZOOM_KEY );
	mCurrentURL = getChild< LLTextBox >( LLMediaEntry::CURRENT_URL_KEY );
	mFirstClick = getChild< LLCheckBoxCtrl >( LLMediaEntry::FIRST_CLICK_INTERACT_KEY );
	mHeightPixels = getChild< LLSpinCtrl >( LLMediaEntry::HEIGHT_PIXELS_KEY );
	mHomeURL = getChild< LLLineEditor >( LLMediaEntry::HOME_URL_KEY );
	mWidthPixels = getChild< LLSpinCtrl >( LLMediaEntry::WIDTH_PIXELS_KEY );
	mPreviewMedia = getChild<LLMediaCtrl>("preview_media");
	mFailWhiteListText = getChild<LLTextBox>( "home_fails_whitelist_label" );

	// watch commit action for HOME URL
	childSetCommitCallback( LLMediaEntry::HOME_URL_KEY, onCommitHomeURL, this);
	childSetCommitCallback( "current_url_reset_btn",onBtnResetCurrentUrl, this);

	// interrogates controls and updates widgets as required
	updateMediaPreview();

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

	// TODO: we need to call this repeatedly until the floater panels are fully
	// created but once we have a valid answer, we should stop looking here - the
	// commit callback will handle it
	checkHomeUrlPassesWhitelist();

	// enable/disable pixel values image entry based on auto scale checkbox 
	if ( mAutoScale->getValue().asBoolean() == false )
	{
		getChildView( LLMediaEntry::WIDTH_PIXELS_KEY )->setEnabled( true );
		getChildView( LLMediaEntry::HEIGHT_PIXELS_KEY )->setEnabled( true );
	}
	else
	{
		getChildView( LLMediaEntry::WIDTH_PIXELS_KEY )->setEnabled( false );
		getChildView( LLMediaEntry::HEIGHT_PIXELS_KEY )->setEnabled( false );
	};

	// enable/disable UI based on type of media
	bool reset_button_is_active = true;
	if( mPreviewMedia )
	{
		LLPluginClassMedia* media_plugin = mPreviewMedia->getMediaPlugin();
		if( media_plugin )
		{
			// turn off volume (if we can) for preview. Note: this really only
			// works for QuickTime movies right now - no way to control the 
			// volume of a flash app embedded in a page for example
			media_plugin->setVolume( 0 );

			// some controls are only appropriate for time or browser type plugins
			// so we selectively enable/disable them - need to do it in draw
			// because the information from plugins arrives assynchronously
			bool show_time_controls = media_plugin->pluginSupportsMediaTime();
			if ( show_time_controls )
			{
				getChildView( LLMediaEntry::CURRENT_URL_KEY )->setEnabled( false );
				reset_button_is_active = false;
				getChildView("current_url_label")->setEnabled(false );
				getChildView( LLMediaEntry::AUTO_LOOP_KEY )->setEnabled( true );
			}
			else
			{
				getChildView( LLMediaEntry::CURRENT_URL_KEY )->setEnabled( true );
				reset_button_is_active = true;
				getChildView("current_url_label")->setEnabled(true );
				getChildView( LLMediaEntry::AUTO_LOOP_KEY )->setEnabled( false );
			};
		};
	};

	// current URL can change over time, update it here
	updateCurrentUrl();
	
	LLPermissions perm;
	bool user_can_press_reset = mMediaEditable;

	// several places modify this widget so we must collect states in one place
	if ( reset_button_is_active )
	{
		// user has perms to press reset button and it is active
		if ( user_can_press_reset )
		{
			getChildView("current_url_reset_btn")->setEnabled(true );
		}
		// user does not has perms to press reset button and it is active
		else
		{
			getChildView("current_url_reset_btn")->setEnabled(false );
		};
	}
	else
	// reset button is inactive so we just slam it to off - other states don't matter
	{
		getChildView("current_url_reset_btn")->setEnabled(false );
	};
}

////////////////////////////////////////////////////////////////////////////////
// static 
void LLPanelMediaSettingsGeneral::clearValues( void* userdata, bool editable)
{	
	LLPanelMediaSettingsGeneral *self =(LLPanelMediaSettingsGeneral *)userdata;
	self->mAutoLoop->clear();
	self->mAutoPlay->clear();
	self->mAutoScale->clear();
	self->mAutoZoom ->clear();
	self->mCurrentURL->clear();
	self->mFirstClick->clear();
	self->mHeightPixels->clear();
	self->mHomeURL->clear();
	self->mWidthPixels->clear();
	self->mAutoLoop ->setEnabled(editable);
	self->mAutoPlay ->setEnabled(editable);
	self->mAutoScale ->setEnabled(editable);
	self->mAutoZoom  ->setEnabled(editable);
	self->mCurrentURL ->setEnabled(editable);
	self->mFirstClick ->setEnabled(editable);
	self->mHeightPixels ->setEnabled(editable);
	self->mHomeURL ->setEnabled(editable);
	self->mWidthPixels ->setEnabled(editable);
	self->updateMediaPreview();
}

// static
bool LLPanelMediaSettingsGeneral::isMultiple()
{
	// IF all the faces have media (or all dont have media)
	if ( LLFloaterMediaSettings::getInstance()->mIdenticalHasMediaInfo )
	{
		if(LLFloaterMediaSettings::getInstance()->mMultipleMedia) 
		{
			return true;
		}
		
	}
	else
	{
		if(LLFloaterMediaSettings::getInstance()->mMultipleValidMedia) 
		{
			return true;
		}
	}
	return false;
}	

////////////////////////////////////////////////////////////////////////////////
// static 
void LLPanelMediaSettingsGeneral::initValues( void* userdata, const LLSD& _media_settings, bool editable)
{
	LLPanelMediaSettingsGeneral *self =(LLPanelMediaSettingsGeneral *)userdata;
	self->mMediaEditable = editable;

	LLSD media_settings = _media_settings;
	
	if ( LLPanelMediaSettingsGeneral::isMultiple() )
	{
		// *HACK:  "edit" the incoming media_settings
		media_settings[LLMediaEntry::CURRENT_URL_KEY] = LLTrans::getString("Multiple Media");
		media_settings[LLMediaEntry::HOME_URL_KEY] = LLTrans::getString("Multiple Media");
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
        { LLMediaEntry::AUTO_LOOP_KEY,				self->mAutoLoop,		"LLCheckBoxCtrl" },
		{ LLMediaEntry::AUTO_PLAY_KEY,				self->mAutoPlay,		"LLCheckBoxCtrl" },
		{ LLMediaEntry::AUTO_SCALE_KEY,				self->mAutoScale,		"LLCheckBoxCtrl" },
		{ LLMediaEntry::AUTO_ZOOM_KEY,				self->mAutoZoom,		"LLCheckBoxCtrl" },
		{ LLMediaEntry::CURRENT_URL_KEY,			self->mCurrentURL,		"LLTextBox" },
		{ LLMediaEntry::HEIGHT_PIXELS_KEY,			self->mHeightPixels,	"LLSpinCtrl" },
		{ LLMediaEntry::HOME_URL_KEY,				self->mHomeURL,			"LLLineEditor" },
		{ LLMediaEntry::FIRST_CLICK_INTERACT_KEY,	self->mFirstClick,		"LLCheckBoxCtrl" },
		{ LLMediaEntry::WIDTH_PIXELS_KEY,			self->mWidthPixels,		"LLSpinCtrl" },
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

			data_set[ i ].ctrl_ptr->setEnabled(self->mMediaEditable);
			data_set[ i ].ctrl_ptr->setTentative( media_settings[ tentative_key ].asBoolean() );
		};
	};

	// interrogates controls and updates widgets as required
	self->updateMediaPreview();
}

////////////////////////////////////////////////////////////////////////////////
// Helper to set media control to media URL as required
void LLPanelMediaSettingsGeneral::updateMediaPreview()
{
	if ( mHomeURL->getValue().asString().length() > 0 )
	{
		if(mPreviewMedia->getCurrentNavUrl() != mHomeURL->getValue().asString())
		{
			mPreviewMedia->navigateTo( mHomeURL->getValue().asString() );
		}
	}
	else
	// new home URL will be empty if media is deleted so display a 
	// "preview goes here" data url page
	{
		if(mPreviewMedia->getCurrentNavUrl() != CHECKERBOARD_DATA_URL)
		{
			mPreviewMedia->navigateTo( CHECKERBOARD_DATA_URL );
		}
	};
}

////////////////////////////////////////////////////////////////////////////////

// virtual
void LLPanelMediaSettingsGeneral::onClose(bool app_quitting)
{
	if(mPreviewMedia)
	{
		mPreviewMedia->unloadMediaSource();
	}
}

////////////////////////////////////////////////////////////////////////////////
// 
void LLPanelMediaSettingsGeneral::checkHomeUrlPassesWhitelist()
{
	// parent floater has not constructed the security panel yet
	if ( mParent->getPanelSecurity() == 0 ) 
		return;

	std::string home_url = getHomeUrl();
	if ( home_url.empty() || mParent->getPanelSecurity()->urlPassesWhiteList( home_url ) )
	{
		// Home URL is empty or passes the white list so hide the warning message
		mFailWhiteListText->setVisible( false );
	}
	else
	{
		// Home URL does not pass the white list so show the warning message
		mFailWhiteListText->setVisible( true );
	};
}

////////////////////////////////////////////////////////////////////////////////
// static
void LLPanelMediaSettingsGeneral::onCommitHomeURL( LLUICtrl* ctrl, void *userdata )
{
	LLPanelMediaSettingsGeneral* self =(LLPanelMediaSettingsGeneral *)userdata;

	// check home url passes whitelist and display warning if not
	self->checkHomeUrlPassesWhitelist();

	self->updateMediaPreview();
}

////////////////////////////////////////////////////////////////////////////////
// static
void LLPanelMediaSettingsGeneral::onBtnResetCurrentUrl(LLUICtrl* ctrl, void *userdata)
{
	LLPanelMediaSettingsGeneral* self =(LLPanelMediaSettingsGeneral *)userdata;
	self->navigateHomeSelectedFace(false);
}

////////////////////////////////////////////////////////////////////////////////
// 
void LLPanelMediaSettingsGeneral::preApply()
{
	// Make sure the home URL entry is committed
	mHomeURL->onCommit();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLPanelMediaSettingsGeneral::getValues( LLSD &fill_me_in, bool include_tentative )
{
	if (include_tentative || !mAutoLoop->getTentative()) fill_me_in[LLMediaEntry::AUTO_LOOP_KEY] = (LLSD::Boolean)mAutoLoop->getValue();
	if (include_tentative || !mAutoPlay->getTentative()) fill_me_in[LLMediaEntry::AUTO_PLAY_KEY] = (LLSD::Boolean)mAutoPlay->getValue();
	if (include_tentative || !mAutoScale->getTentative()) fill_me_in[LLMediaEntry::AUTO_SCALE_KEY] = (LLSD::Boolean)mAutoScale->getValue();
	if (include_tentative || !mAutoZoom->getTentative()) fill_me_in[LLMediaEntry::AUTO_ZOOM_KEY] = (LLSD::Boolean)mAutoZoom->getValue();
	//Don't fill in current URL: this is only supposed to get changed via navigate
	// if (include_tentative || !mCurrentURL->getTentative()) fill_me_in[LLMediaEntry::CURRENT_URL_KEY] = mCurrentURL->getValue();
	if (include_tentative || !mHeightPixels->getTentative()) fill_me_in[LLMediaEntry::HEIGHT_PIXELS_KEY] = (LLSD::Integer)mHeightPixels->getValue();
	// Don't fill in the home URL if it is the special "Multiple Media" string!
	if ((include_tentative || !mHomeURL->getTentative())
		&& LLTrans::getString("Multiple Media") != mHomeURL->getValue())
			fill_me_in[LLMediaEntry::HOME_URL_KEY] = (LLSD::String)mHomeURL->getValue();
	if (include_tentative || !mFirstClick->getTentative()) fill_me_in[LLMediaEntry::FIRST_CLICK_INTERACT_KEY] = (LLSD::Boolean)mFirstClick->getValue();
	if (include_tentative || !mWidthPixels->getTentative()) fill_me_in[LLMediaEntry::WIDTH_PIXELS_KEY] = (LLSD::Integer)mWidthPixels->getValue();
}

////////////////////////////////////////////////////////////////////////////////
// 
void LLPanelMediaSettingsGeneral::postApply()
{	
	// Make sure to navigate to the home URL if the current URL is empty and 
	// autoplay is on
	navigateHomeSelectedFace(true);
}


////////////////////////////////////////////////////////////////////////////////
//
void LLPanelMediaSettingsGeneral::setParent( LLFloaterMediaSettings* parent )
{
	mParent = parent;
};

////////////////////////////////////////////////////////////////////////////////
//
bool LLPanelMediaSettingsGeneral::navigateHomeSelectedFace(bool only_if_current_is_empty)
{
	struct functor_navigate_media : public LLSelectedTEGetFunctor< bool>
	{
		functor_navigate_media(bool flag) : only_if_current_is_empty(flag) {}
		bool get( LLViewerObject* object, S32 face )
		{
			if ( object && object->getTE(face) && object->permModify() )
			{
				const LLMediaEntry *media_data = object->getTE(face)->getMediaData();
				if ( media_data )
				{	
					if (!only_if_current_is_empty || (media_data->getCurrentURL().empty() && media_data->getAutoPlay()))
					{
						viewer_media_t media_impl =
							LLViewerMedia::getMediaImplFromTextureID(object->getTE(face)->getMediaData()->getMediaID());
						if(media_impl)
						{
							media_impl->navigateHome();
							return true;
						}
					}
				}
			}
			return false;
		};
		bool only_if_current_is_empty;
				
	} functor_navigate_media(only_if_current_is_empty);
	
	bool all_face_media_navigated = false;
	LLObjectSelectionHandle selected_objects =LLSelectMgr::getInstance()->getSelection();
	selected_objects->getSelectedTEValue( &functor_navigate_media, all_face_media_navigated );
	
	// Note: we don't update the 'current URL' field until the media data itself changes

	return all_face_media_navigated;
}

////////////////////////////////////////////////////////////////////////////////
//
const std::string LLPanelMediaSettingsGeneral::getHomeUrl()
{
	return mHomeURL->getValue().asString(); 
}

////////////////////////////////////////////////////////////////////////////////
//
void LLPanelMediaSettingsGeneral::updateCurrentUrl()
{
	// Get the current URL from the selection
	const LLMediaEntry default_media_data;
	std::string value_str = default_media_data.getCurrentURL();
	struct functor_getter_current_url : public LLSelectedTEGetFunctor< std::string >
	{
		functor_getter_current_url(const LLMediaEntry& entry): mMediaEntry(entry) {}
		
		std::string get( LLViewerObject* object, S32 face )
		{
			if ( object )
				if ( object->getTE(face) )
					if ( object->getTE(face)->getMediaData() )
						return object->getTE(face)->getMediaData()->getCurrentURL();
			return mMediaEntry.getCurrentURL();
		};
		
		const LLMediaEntry &  mMediaEntry;
		
	} func_current_url(default_media_data);
	bool identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func_current_url, value_str );
	mCurrentURL->setText(value_str);
	mCurrentURL->setTentative(identical);

	if ( LLPanelMediaSettingsGeneral::isMultiple() )
	{
		mCurrentURL->setText(LLTrans::getString("Multiple Media"));
	}
}	
