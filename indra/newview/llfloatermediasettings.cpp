/** 
 * @file llfloatermediasettings.cpp
 * @brief Tabbed dialog for media settings - class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llfloaterreg.h"
#include "llfloatermediasettings.h"
#include "llpanelmediasettingsgeneral.h"
#include "llpanelmediasettingssecurity.h"
#include "llpanelmediasettingspermissions.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llbutton.h"
#include "llselectmgr.h"
#include "llsdutil.h"

LLFloaterMediaSettings* LLFloaterMediaSettings::sInstance = NULL;

////////////////////////////////////////////////////////////////////////////////
// 
LLFloaterMediaSettings::LLFloaterMediaSettings(const LLSD& key)
	: LLFloater(key),
	mTabContainer(NULL),
	mPanelMediaSettingsGeneral(NULL),
	mPanelMediaSettingsSecurity(NULL),
	mPanelMediaSettingsPermissions(NULL),
	mWaitingToClose( false ),
	mIdenticalHasMediaInfo( true ),
	mMultipleMedia(false),
	mMultipleValidMedia(false)
{
}

////////////////////////////////////////////////////////////////////////////////
//
LLFloaterMediaSettings::~LLFloaterMediaSettings()
{
	if ( mPanelMediaSettingsGeneral )
	{
		delete mPanelMediaSettingsGeneral;
		mPanelMediaSettingsGeneral = NULL;
	}

	if ( mPanelMediaSettingsSecurity )
	{
		delete mPanelMediaSettingsSecurity;
		mPanelMediaSettingsSecurity = NULL;
	}

	if ( mPanelMediaSettingsPermissions )
	{
		delete mPanelMediaSettingsPermissions;
		mPanelMediaSettingsPermissions = NULL;
	}

	sInstance = NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLFloaterMediaSettings::postBuild()
{
	mApplyBtn = getChild<LLButton>("Apply");
	mApplyBtn->setClickedCallback(onBtnApply, this);
		
	mCancelBtn = getChild<LLButton>("Cancel");
	mCancelBtn->setClickedCallback(onBtnCancel, this);

	mOKBtn = getChild<LLButton>("OK");
	mOKBtn->setClickedCallback(onBtnOK, this);
			
	mTabContainer = getChild<LLTabContainer>( "tab_container" );
	
	mPanelMediaSettingsGeneral = new LLPanelMediaSettingsGeneral();
	mTabContainer->addTabPanel( 
			LLTabContainer::TabPanelParams().
			panel(mPanelMediaSettingsGeneral));
	mPanelMediaSettingsGeneral->setParent( this );

	// note that "permissions" tab is really "Controls" tab - refs to 'perms' and
	// 'permissions' not changed to 'controls' since we don't want to change 
	// shared files in server code and keeping everything the same seemed best.
	mPanelMediaSettingsPermissions = new LLPanelMediaSettingsPermissions();
	mTabContainer->addTabPanel( 
			LLTabContainer::TabPanelParams().
			panel(mPanelMediaSettingsPermissions));

	mPanelMediaSettingsSecurity = new LLPanelMediaSettingsSecurity();
	mTabContainer->addTabPanel( 
			LLTabContainer::TabPanelParams().
			panel(mPanelMediaSettingsSecurity));
	mPanelMediaSettingsSecurity->setParent( this );
		
	// restore the last tab viewed from persistance variable storage
	if (!mTabContainer->selectTab(gSavedSettings.getS32("LastMediaSettingsTab")))
	{
		mTabContainer->selectFirstTab();
	};

	sInstance = this;

	return TRUE;
}

//static 
LLFloaterMediaSettings* LLFloaterMediaSettings::getInstance()
{
	if ( !sInstance )
	{
		sInstance = (LLFloaterReg::getTypedInstance<LLFloaterMediaSettings>("media_settings"));
	}
	
	return sInstance;
}

//static 
void LLFloaterMediaSettings::apply()
{
	if (sInstance->haveValuesChanged())
	{
		LLSD settings;
		sInstance->mPanelMediaSettingsGeneral->preApply();
		sInstance->mPanelMediaSettingsGeneral->getValues( settings, false );
		sInstance->mPanelMediaSettingsSecurity->preApply();
		sInstance->mPanelMediaSettingsSecurity->getValues( settings, false );
		sInstance->mPanelMediaSettingsPermissions->preApply();
		sInstance->mPanelMediaSettingsPermissions->getValues( settings, false );
			
		LLSelectMgr::getInstance()->selectionSetMedia( LLTextureEntry::MF_HAS_MEDIA, settings );

		sInstance->mPanelMediaSettingsGeneral->postApply();
		sInstance->mPanelMediaSettingsSecurity->postApply();
		sInstance->mPanelMediaSettingsPermissions->postApply();
	}
}

////////////////////////////////////////////////////////////////////////////////
void LLFloaterMediaSettings::onClose(bool app_quitting)
{
	if(mPanelMediaSettingsGeneral)
	{
		mPanelMediaSettingsGeneral->onClose(app_quitting);
	}
	LLFloaterReg::hideInstance("whitelist_entry");
}

////////////////////////////////////////////////////////////////////////////////
//static 
void LLFloaterMediaSettings::initValues( const LLSD& media_settings, bool editable )
{
	if (sInstance->hasFocus()) return;
	
	sInstance->clearValues(editable);
	// update all panels with values from simulator
	sInstance->mPanelMediaSettingsGeneral->
		initValues( sInstance->mPanelMediaSettingsGeneral, media_settings, editable );

	sInstance->mPanelMediaSettingsSecurity->
		initValues( sInstance->mPanelMediaSettingsSecurity, media_settings, editable );

	sInstance->mPanelMediaSettingsPermissions->
		initValues( sInstance->mPanelMediaSettingsPermissions, media_settings, editable );
	
	// Squirrel away initial values 
	sInstance->mInitialValues.clear();
	sInstance->mPanelMediaSettingsGeneral->getValues( sInstance->mInitialValues );
	sInstance->mPanelMediaSettingsSecurity->getValues( sInstance->mInitialValues );
	sInstance->mPanelMediaSettingsPermissions->getValues( sInstance->mInitialValues );
	
	sInstance->mApplyBtn->setEnabled(editable);
	sInstance->mOKBtn->setEnabled(editable);
}

////////////////////////////////////////////////////////////////////////////////
// 
void LLFloaterMediaSettings::commitFields()
{
	if (hasFocus())
	{
		LLUICtrl* cur_focus = dynamic_cast<LLUICtrl*>(gFocusMgr.getKeyboardFocus());
		if (cur_focus && cur_focus->acceptsTextInput())
		{
			cur_focus->onCommit();
		};
	};
}

////////////////////////////////////////////////////////////////////////////////
//static 
void LLFloaterMediaSettings::clearValues( bool editable)
{
	if (sInstance)
	{
		// clean up all panels before updating
		sInstance->mPanelMediaSettingsGeneral	 ->clearValues(sInstance->mPanelMediaSettingsGeneral,  editable);
		sInstance->mPanelMediaSettingsSecurity	 ->clearValues(sInstance->mPanelMediaSettingsSecurity,	editable);
		sInstance->mPanelMediaSettingsPermissions->clearValues(sInstance->mPanelMediaSettingsPermissions,  editable);
	}
}

////////////////////////////////////////////////////////////////////////////////
// static
void LLFloaterMediaSettings::onBtnOK( void* userdata )
{
	sInstance->commitFields();

	sInstance->apply();

	sInstance->closeFloater();
}

////////////////////////////////////////////////////////////////////////////////
// static
void LLFloaterMediaSettings::onBtnApply( void* userdata )
{
	sInstance->commitFields();

	sInstance->apply();

	sInstance->mInitialValues.clear();
	sInstance->mPanelMediaSettingsGeneral->getValues( sInstance->mInitialValues );
	sInstance->mPanelMediaSettingsSecurity->getValues( sInstance->mInitialValues );
	sInstance->mPanelMediaSettingsPermissions->getValues( sInstance->mInitialValues );

}

////////////////////////////////////////////////////////////////////////////////
// static
void LLFloaterMediaSettings::onBtnCancel( void* userdata )
{
	sInstance->closeFloater(); 
}

////////////////////////////////////////////////////////////////////////////////
// static
void LLFloaterMediaSettings::onTabChanged(void* user_data, bool from_click)
{
	LLTabContainer* self = (LLTabContainer*)user_data;
	gSavedSettings.setS32("LastMediaSettingsTab", self->getCurrentPanelIndex());
}
////////////////////////////////////////////////////////////////////////////////
//
const std::string LLFloaterMediaSettings::getHomeUrl()
{
	if ( mPanelMediaSettingsGeneral )
		return mPanelMediaSettingsGeneral->getHomeUrl();
	else
		return std::string( "" );
}

////////////////////////////////////////////////////////////////////////////////
// virtual 
void LLFloaterMediaSettings::draw()
{
	if (NULL != mApplyBtn)
	{
		// Set the enabled state of the "Apply" button if values changed
		mApplyBtn->setEnabled( haveValuesChanged() );
	}
	
	LLFloater::draw();
}


//private
bool LLFloaterMediaSettings::haveValuesChanged() const
{
	bool values_changed = false;
	// *NOTE: The code below is very inefficient.  Better to do this
	// only when data change.
	// Every frame, check to see what the values are.  If they are not
	// the same as the initial media data, enable the OK/Apply buttons
	LLSD settings;
	sInstance->mPanelMediaSettingsGeneral->getValues( settings );
	sInstance->mPanelMediaSettingsSecurity->getValues( settings );
	sInstance->mPanelMediaSettingsPermissions->getValues( settings );	
	LLSD::map_const_iterator iter = settings.beginMap();
	LLSD::map_const_iterator end = settings.endMap();
	for ( ; iter != end; ++iter )
	{
		const std::string &current_key = iter->first;
		const LLSD &current_value = iter->second;
		if ( ! llsd_equals(current_value, mInitialValues[current_key]))
		{
			values_changed = true;
			break;
		}
	}
	return values_changed;
}

bool LLFloaterMediaSettings::instanceExists()
{
	return LLFloaterReg::findTypedInstance<LLFloaterMediaSettings>("media_settings");
}


