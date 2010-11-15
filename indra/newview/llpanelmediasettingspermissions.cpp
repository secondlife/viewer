/**
 * @file llpanelmediasettingspermissions.cpp
 * @brief LLPanelMediaSettingsPermissions class implementation
 *
 * note that "permissions" tab is really "Controls" tab - refs to 'perms' and
 * 'permissions' not changed to 'controls' since we don't want to change 
 * shared files in server code and keeping everything the same seemed best.
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
    buildFromFile( "panel_media_settings_permissions.xml");
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

	getChild<LLUICtrl>("perms_group_name")->setValue(LLStringUtil::null);
	LLUUID group_id;
	BOOL groups_identical = LLSelectMgr::getInstance()->selectGetGroup(group_id);
	if (groups_identical)
	{
		if(mPermsGroupName)
		{
			mPermsGroupName->setNameID(group_id, true);
		}
	}
	else
	{
		if(mPermsGroupName)
		{
			mPermsGroupName->setNameID(LLUUID::null, TRUE);
			mPermsGroupName->refresh(LLUUID::null, std::string(), true);
		}
	}
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

	self->getChild< LLTextBox >("controls_label")->setEnabled(editable);
	self->getChild< LLTextBox >("owner_label")->setEnabled(editable);
	self->getChild< LLTextBox >("group_label")->setEnabled(editable);
	self->getChild< LLNameBox >("perms_group_name")->setEnabled(editable);
	self->getChild< LLTextBox >("anyone_label")->setEnabled(editable);
}

////////////////////////////////////////////////////////////////////////////////
// static 
void LLPanelMediaSettingsPermissions::initValues( void* userdata, const LLSD& media_settings ,  bool editable)
{
    LLPanelMediaSettingsPermissions *self =(LLPanelMediaSettingsPermissions *)userdata;
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

	// *NOTE: If any of a particular flavor is tentative, we have to disable 
	// them all because of an architectural issue: namely that we represent 
	// these as a bit field, and we can't selectively apply only one bit to all selected
	// faces if they don't match.  Also see the *NOTE below.
	if ( self->mPermsOwnerInteract->getTentative() ||
		 self->mPermsGroupInteract->getTentative() ||
		 self->mPermsWorldInteract->getTentative())
	{
		self->mPermsOwnerInteract->setEnabled(false);
		self->mPermsGroupInteract->setEnabled(false);
		self->mPermsWorldInteract->setEnabled(false);
	}	
	if ( self->mPermsOwnerControl->getTentative() ||
		 self->mPermsGroupControl->getTentative() ||
		 self->mPermsWorldControl->getTentative())
	{
		self->mPermsOwnerControl->setEnabled(false);
		self->mPermsGroupControl->setEnabled(false);
		self->mPermsWorldControl->setEnabled(false);
	}
	
	self->getChild< LLTextBox >("controls_label")->setEnabled(editable);
	self->getChild< LLTextBox >("owner_label")->setEnabled(editable);
	self->getChild< LLTextBox >("group_label")->setEnabled(editable);
	self->getChild< LLNameBox >("perms_group_name")->setEnabled(editable);
	self->getChild< LLTextBox >("anyone_label")->setEnabled(editable);
}

////////////////////////////////////////////////////////////////////////////////
// 
void LLPanelMediaSettingsPermissions::preApply()
{
    // no-op
}

////////////////////////////////////////////////////////////////////////////////
//
void LLPanelMediaSettingsPermissions::getValues( LLSD &fill_me_in, bool include_tentative )
{
	// moved over from the 'General settings' tab
	if (include_tentative || !mControls->getTentative()) fill_me_in[LLMediaEntry::CONTROLS_KEY] = (LLSD::Integer)mControls->getCurrentIndex();
	
	// *NOTE: For some reason, gcc does not like these symbol references in the 
	// expressions below (inside the static_casts).	 I have NO idea why :(.
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
		(mPermsOwnerInteract->getValue() ? owner: none	) |
		(mPermsGroupInteract->getValue() ? group : none ) |
		(mPermsWorldInteract->getValue() ? anyone : none ));
	
	// *TODO: This will fill in the values of all permissions values, even if
	// one or more is tentative.  This is not quite the user expectation...what
	// it should do is only change the bit that was made "untentative", but in
	// a multiple-selection situation, this isn't possible given the architecture
	// for how settings are applied.
	if (include_tentative || 
		!mPermsOwnerControl->getTentative() || 
		!mPermsGroupControl->getTentative() || 
		!mPermsWorldControl->getTentative())
	{
		fill_me_in[LLMediaEntry::PERMS_CONTROL_KEY] = control;
	}
	if (include_tentative || 
		!mPermsOwnerInteract->getTentative() || 
		!mPermsGroupInteract->getTentative() || 
		!mPermsWorldInteract->getTentative())
	{
		fill_me_in[LLMediaEntry::PERMS_INTERACT_KEY] = interact;
	}
}


////////////////////////////////////////////////////////////////////////////////
// 
void LLPanelMediaSettingsPermissions::postApply()
{
    // no-op
}


