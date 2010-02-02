/**
 * @file llpanelmediasettingspermissions.cpp
 * @brief LLPanelMediaSettingsPermissions class implementation
 *
 * note that "permissions" tab is really "Controls" tab - refs to 'perms' and
 * 'permissions' not changed to 'controls' since we don't want to change 
 * shared files in server code and keeping everything the same seemed best.
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

#include "llpanelmediasettingspermissions.h"
#include "llpanelcontents.h"
#include "llcombobox.h"
#include "llcheckboxctrl.h"
#include "llspinctrl.h"
#include "llurlhistory.h"
#include "lluictrlfactory.h"
#include "llwindow.h"
#include "llviewerwindow.h"
#include "llsdutil.h"
#include "llselectmgr.h"
#include "llmediaentry.h"
#include "llnamebox.h"
#include "lltrans.h"
#include "llfloatermediasettings.h"

////////////////////////////////////////////////////////////////////////////////
//
LLPanelMediaSettingsPermissions::LLPanelMediaSettingsPermissions() :
	mControls( NULL ),
    mPermsOwnerInteract( 0 ),
    mPermsOwnerControl( 0 ),
	mPermsGroupName( 0 ),
    mPermsGroupInteract( 0 ),
    mPermsGroupControl( 0 ),
    mPermsWorldInteract( 0 ),
    mPermsWorldControl( 0 )
{
    // build dialog from XML
    LLUICtrlFactory::getInstance()->buildPanel(this, "panel_media_settings_permissions.xml");
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLPanelMediaSettingsPermissions::postBuild()
{
    // connect member vars with UI widgets
	mControls = getChild< LLComboBox >( LLMediaEntry::CONTROLS_KEY );
    mPermsOwnerInteract = getChild< LLCheckBoxCtrl >( LLPanelContents::PERMS_OWNER_INTERACT_KEY );
    mPermsOwnerControl = getChild< LLCheckBoxCtrl >( LLPanelContents::PERMS_OWNER_CONTROL_KEY );
    mPermsGroupInteract = getChild< LLCheckBoxCtrl >( LLPanelContents::PERMS_GROUP_INTERACT_KEY );
    mPermsGroupControl = getChild< LLCheckBoxCtrl >( LLPanelContents::PERMS_GROUP_CONTROL_KEY );
    mPermsWorldInteract = getChild< LLCheckBoxCtrl >( LLPanelContents::PERMS_ANYONE_INTERACT_KEY );
    mPermsWorldControl = getChild< LLCheckBoxCtrl >( LLPanelContents::PERMS_ANYONE_CONTROL_KEY );

	mPermsGroupName = getChild< LLNameBox >( "perms_group_name" );

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// virtual
LLPanelMediaSettingsPermissions::~LLPanelMediaSettingsPermissions()
{
}

////////////////////////////////////////////////////////////////////////////////
// virtual
void LLPanelMediaSettingsPermissions::draw()
{
	// housekeeping
	LLPanel::draw();

	childSetText("perms_group_name",LLStringUtil::null);
	LLUUID group_id;
	BOOL groups_identical = LLSelectMgr::getInstance()->selectGetGroup(group_id);
	if (groups_identical)
	{
		if(mPermsGroupName)
		{
			mPermsGroupName->setNameID(group_id, true);
			mPermsGroupName->setEnabled(true);
		};
	}
	else
	{
		if(mPermsGroupName)
		{
			mPermsGroupName->setNameID(LLUUID::null, TRUE);
			mPermsGroupName->refresh(LLUUID::null, std::string(), true);
			mPermsGroupName->setEnabled(false);
		};
	};
}

////////////////////////////////////////////////////////////////////////////////
// static 
void LLPanelMediaSettingsPermissions::clearValues( void* userdata, bool editable)
{	
	LLPanelMediaSettingsPermissions *self =(LLPanelMediaSettingsPermissions *)userdata;

	self->mControls->clear();
	self->mPermsOwnerInteract->clear();
	self->mPermsOwnerControl->clear();
	self->mPermsGroupInteract->clear();
	self->mPermsGroupControl->clear();
	self->mPermsWorldInteract->clear();
	self->mPermsWorldControl->clear();
	
	self->mControls->setEnabled(editable);
	self->mPermsOwnerInteract->setEnabled(editable);
	self->mPermsOwnerControl->setEnabled(editable);
	self->mPermsGroupInteract->setEnabled(editable);
	self->mPermsGroupControl->setEnabled(editable);
	self->mPermsWorldInteract->setEnabled(editable);
	self->mPermsWorldControl->setEnabled(editable);
}

////////////////////////////////////////////////////////////////////////////////
// static 
void LLPanelMediaSettingsPermissions::initValues( void* userdata, const LLSD& media_settings ,  bool editable)
{
    LLPanelMediaSettingsPermissions *self =(LLPanelMediaSettingsPermissions *)userdata;

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
		{ LLMediaEntry::CONTROLS_KEY,					self->mControls,			"LLComboBox" },
        { LLPanelContents::PERMS_OWNER_INTERACT_KEY,    self->mPermsOwnerInteract,  "LLCheckBoxCtrl" },
        { LLPanelContents::PERMS_OWNER_CONTROL_KEY,     self->mPermsOwnerControl,   "LLCheckBoxCtrl" },
        { LLPanelContents::PERMS_GROUP_INTERACT_KEY,    self->mPermsGroupInteract,  "LLCheckBoxCtrl" },
        { LLPanelContents::PERMS_GROUP_CONTROL_KEY,     self->mPermsGroupControl,   "LLCheckBoxCtrl" },
        { LLPanelContents::PERMS_ANYONE_INTERACT_KEY,   self->mPermsWorldInteract,  "LLCheckBoxCtrl" },
        { LLPanelContents::PERMS_ANYONE_CONTROL_KEY,    self->mPermsWorldControl,   "LLCheckBoxCtrl" },
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
				// Most recent change to the "sense" of these checkboxes
				// means the value in the checkbox matches that on the server
                static_cast< LLCheckBoxCtrl* >( data_set[ i ].ctrl_ptr )->
                    setValue( media_settings[ base_key ].asBoolean() );
            }
            else
            if ( data_set[ i ].ctrl_type == "LLComboBox" )
                static_cast< LLComboBox* >( data_set[ i ].ctrl_ptr )->
                    setCurrentByIndex( media_settings[ base_key ].asInteger() );

			data_set[ i ].ctrl_ptr->setEnabled(editable);
            data_set[ i ].ctrl_ptr->setTentative( media_settings[ tentative_key ].asBoolean() );
        };
    };

	self->childSetEnabled("media_perms_label_owner", editable );
	self->childSetText("media_perms_label_owner",  LLTrans::getString("Media Perms Owner") );
	self->childSetEnabled("media_perms_label_group", editable );
	self->childSetText("media_perms_label_group",  LLTrans::getString("Media Perms Group") );
	self->childSetEnabled("media_perms_label_anyone", editable );
	self->childSetText("media_perms_label_anyone", LLTrans::getString("Media Perms Anyone") );
}

////////////////////////////////////////////////////////////////////////////////
// 
void LLPanelMediaSettingsPermissions::preApply()
{
    // no-op
}

////////////////////////////////////////////////////////////////////////////////
//
void LLPanelMediaSettingsPermissions::getValues( LLSD &fill_me_in )
{
	// moved over from the 'General settings' tab
	fill_me_in[LLMediaEntry::CONTROLS_KEY] = (LLSD::Integer)mControls->getCurrentIndex();

    // *NOTE: For some reason, gcc does not like these symbol references in the 
    // expressions below (inside the static_casts).  I have NO idea why :(.
    // For some reason, assigning them to const temp vars here fixes the link
    // error.  Bizarre.
    const U8 none = LLMediaEntry::PERM_NONE;
    const U8 owner = LLMediaEntry::PERM_OWNER;
    const U8 group = LLMediaEntry::PERM_GROUP;
    const U8 anyone = LLMediaEntry::PERM_ANYONE;
    const LLSD::Integer control = static_cast<LLSD::Integer>(
		(mPermsOwnerControl->getValue() ? owner : none ) |
		(mPermsGroupControl->getValue() ? group: none  ) |
		(mPermsWorldControl->getValue() ? anyone : none ));
    const LLSD::Integer interact = static_cast<LLSD::Integer>(
		(mPermsOwnerInteract->getValue() ? owner: none  ) |
		(mPermsGroupInteract->getValue() ? group : none ) |
		(mPermsWorldInteract->getValue() ? anyone : none ));
    fill_me_in[LLMediaEntry::PERMS_CONTROL_KEY] = control;
    fill_me_in[LLMediaEntry::PERMS_INTERACT_KEY] = interact;
}


////////////////////////////////////////////////////////////////////////////////
// 
void LLPanelMediaSettingsPermissions::postApply()
{
    // no-op
}


