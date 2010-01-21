/**
 * @file llpanellandaudio.cpp
 * @brief Allows configuration of "media" for a land parcel,
 *   for example movies, web pages, and audio.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 *
 * Copyright (c) 2007-2008, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llpanellandaudio.h"

// viewer includes
#include "llmimetypes.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "lluictrlfactory.h"

// library includes
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloaterurlentry.h"
#include "llfocusmgr.h"
#include "lllineeditor.h"
#include "llparcel.h"
#include "lltextbox.h"
#include "llradiogroup.h"
#include "llspinctrl.h"
#include "llsdutil.h"
#include "lltexturectrl.h"
#include "roles_constants.h"
#include "llscrolllistctrl.h"

// Values for the parcel voice settings radio group
enum
{
	kRadioVoiceChatEstate = 0,
	kRadioVoiceChatPrivate = 1,
	kRadioVoiceChatDisable = 2
};

//---------------------------------------------------------------------------
// LLPanelLandAudio
//---------------------------------------------------------------------------

LLPanelLandAudio::LLPanelLandAudio(LLParcelSelectionHandle& parcel)
:	LLPanel(/*std::string("land_media_panel")*/), mParcel(parcel)
{
}


// virtual
LLPanelLandAudio::~LLPanelLandAudio()
{
}


BOOL LLPanelLandAudio::postBuild()
{
	mCheckSoundLocal = getChild<LLCheckBoxCtrl>("check sound local");
	childSetCommitCallback("check sound local", onCommitAny, this);

	mCheckParcelEnableVoice = getChild<LLCheckBoxCtrl>("parcel_enable_voice_channel");
	childSetCommitCallback("parcel_enable_voice_channel", onCommitAny, this);

	// This one is always disabled so no need for a commit callback
	mCheckEstateDisabledVoice = getChild<LLCheckBoxCtrl>("parcel_enable_voice_channel_is_estate_disabled");

	mCheckParcelVoiceLocal = getChild<LLCheckBoxCtrl>("parcel_enable_voice_channel_local");
	childSetCommitCallback("parcel_enable_voice_channel_local", onCommitAny, this);

	mMusicURLEdit = getChild<LLLineEditor>("music_url");
	childSetCommitCallback("music_url", onCommitAny, this);

	mMusicUrlCheck = getChild<LLCheckBoxCtrl>("hide_music_url");
	childSetCommitCallback("hide_music_url", onCommitAny, this);

	return TRUE;
}


// public
void LLPanelLandAudio::refresh()
{
	LLParcel *parcel = mParcel->getParcel();

	if (!parcel)
	{
		clearCtrls();
	}
	else
	{
		// something selected, hooray!

		// Display options
		BOOL can_change_media = LLViewerParcelMgr::isParcelModifiableByAgent(parcel, GP_LAND_CHANGE_MEDIA);

		mCheckSoundLocal->set( parcel->getSoundLocal() );
		mCheckSoundLocal->setEnabled( can_change_media );

		mMusicUrlCheck->set( parcel->getObscureMusic() );
		mMusicUrlCheck->setEnabled( can_change_media );

		bool allow_voice = parcel->getParcelFlagAllowVoice();

		LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();
		if (region && region->isVoiceEnabled())
		{
			mCheckEstateDisabledVoice->setVisible(false);

			mCheckParcelEnableVoice->setVisible(true);
			mCheckParcelEnableVoice->setEnabled( can_change_media );
			mCheckParcelEnableVoice->set(allow_voice);

			mCheckParcelVoiceLocal->setEnabled( can_change_media && allow_voice );
		}
		else
		{
			// Voice disabled at estate level, overrides parcel settings
			// Replace the parcel voice checkbox with a disabled one
			// labelled with an explanatory message
			mCheckEstateDisabledVoice->setVisible(true);

			mCheckParcelEnableVoice->setVisible(false);
			mCheckParcelEnableVoice->setEnabled(false);
			mCheckParcelVoiceLocal->setEnabled(false);
		}

		mCheckParcelEnableVoice->set(allow_voice);
		mCheckParcelVoiceLocal->set(!parcel->getParcelFlagUseEstateVoiceChannel());

		mMusicURLEdit->setText(parcel->getMusicURL());
		mMusicURLEdit->setEnabled( can_change_media );
	}
}
// static
void LLPanelLandAudio::onCommitAny(LLUICtrl*, void *userdata)
{
	LLPanelLandAudio *self = (LLPanelLandAudio *)userdata;

	LLParcel* parcel = self->mParcel->getParcel();
	if (!parcel)
	{
		return;
	}

	// Extract data from UI
	BOOL sound_local		= self->mCheckSoundLocal->get();
	std::string music_url	= self->mMusicURLEdit->getText();
	U8 obscure_music		= self->mMusicUrlCheck->get();

	BOOL voice_enabled = self->mCheckParcelEnableVoice->get();
	BOOL voice_estate_chan = !self->mCheckParcelVoiceLocal->get();

	// Remove leading/trailing whitespace (common when copying/pasting)
	LLStringUtil::trim(music_url);

	// Push data into current parcel
	parcel->setParcelFlag(PF_ALLOW_VOICE_CHAT, voice_enabled);
	parcel->setParcelFlag(PF_USE_ESTATE_VOICE_CHAN, voice_estate_chan);
	parcel->setParcelFlag(PF_SOUND_LOCAL, sound_local);
	parcel->setMusicURL(music_url);
	parcel->setObscureMusic(obscure_music);

	// Send current parcel data upstream to server
	LLViewerParcelMgr::getInstance()->sendParcelPropertiesUpdate( parcel );

	// Might have changed properties, so let's redraw!
	self->refresh();
}
