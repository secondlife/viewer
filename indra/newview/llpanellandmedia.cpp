/**
 * @file llpanellandmedia.cpp
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

#include "llpanellandmedia.h"

// viewer includes
#include "llmimetypes.h"
#include "llviewerparcelmgr.h"
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

// Values for the parcel voice settings radio group
enum
{
	kRadioVoiceChatEstate = 0,
	kRadioVoiceChatPrivate = 1,
	kRadioVoiceChatDisable = 2
};

//---------------------------------------------------------------------------
// LLPanelLandMedia
//---------------------------------------------------------------------------

LLPanelLandMedia::LLPanelLandMedia(LLParcelSelectionHandle& parcel)
:	LLPanel(std::string("land_media_panel")), mParcel(parcel)
{
}


// virtual
LLPanelLandMedia::~LLPanelLandMedia()
{
	// close LLFloaterURLEntry?
}


BOOL LLPanelLandMedia::postBuild()
{
	mCheckSoundLocal = getChild<LLCheckBoxCtrl>("check sound local");
	childSetCommitCallback("check sound local", onCommitAny, this);

	mRadioVoiceChat = getChild<LLRadioGroup>("parcel_voice_channel");
	childSetCommitCallback("parcel_voice_channel", onCommitAny, this);

	mMusicURLEdit = getChild<LLLineEditor>("music_url");
	childSetCommitCallback("music_url", onCommitAny, this);

	mMediaTextureCtrl = getChild<LLTextureCtrl>("media texture");
	mMediaTextureCtrl->setCommitCallback( onCommitAny );
	mMediaTextureCtrl->setCallbackUserData( this );
	mMediaTextureCtrl->setAllowNoTexture ( TRUE );
	mMediaTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
	mMediaTextureCtrl->setNonImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);

	mMediaAutoScaleCheck = getChild<LLCheckBoxCtrl>("media_auto_scale");
	childSetCommitCallback("media_auto_scale", onCommitAny, this);

	mMediaLoopCheck = getChild<LLCheckBoxCtrl>("media_loop");
	childSetCommitCallback("media_loop", onCommitAny, this);

	mMediaUrlCheck = getChild<LLCheckBoxCtrl>("hide_media_url");
	childSetCommitCallback("hide_media_url", onCommitAny, this);

	mMusicUrlCheck = getChild<LLCheckBoxCtrl>("hide_music_url");
	childSetCommitCallback("hide_music_url", onCommitAny, this);

	mMediaURLEdit = getChild<LLLineEditor>("media_url");
	childSetCommitCallback("media_url", onCommitAny, this);

	mMediaDescEdit = getChild<LLLineEditor>("url_description");
	childSetCommitCallback("url_description", onCommitAny, this);

	mMediaTypeCombo = getChild<LLComboBox>("media type");
	childSetCommitCallback("media type", onCommitType, this);
	populateMIMECombo();

	mMediaWidthCtrl = getChild<LLSpinCtrl>("media_size_width");
	childSetCommitCallback("media_size_width", onCommitAny, this);
	mMediaHeightCtrl = getChild<LLSpinCtrl>("media_size_height");
	childSetCommitCallback("media_size_height", onCommitAny, this);
	mMediaSizeCtrlLabel = getChild<LLTextBox>("media_size");

	mSetURLButton = getChild<LLButton>("set_media_url");
	childSetAction("set_media_url", onSetBtn, this);

	return TRUE;
}


// public
void LLPanelLandMedia::refresh()
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

		if(parcel->getParcelFlagAllowVoice())
		{
			if(parcel->getParcelFlagUseEstateVoiceChannel())
				mRadioVoiceChat->setSelectedIndex(kRadioVoiceChatEstate);
			else
				mRadioVoiceChat->setSelectedIndex(kRadioVoiceChatPrivate);
		}
		else
		{
			mRadioVoiceChat->setSelectedIndex(kRadioVoiceChatDisable);
		}

		mRadioVoiceChat->setEnabled( can_change_media );

		mMusicURLEdit->setText(parcel->getMusicURL());
		mMusicURLEdit->setEnabled( can_change_media );

		mMediaURLEdit->setText(parcel->getMediaURL());
		mMediaURLEdit->setEnabled( FALSE );

		mMediaDescEdit->setText(parcel->getMediaDesc());
		mMediaDescEdit->setEnabled( can_change_media );

		std::string mime_type = parcel->getMediaType();
		if (mime_type.empty())
		{
			mime_type = "none/none";
		}
		setMediaType(mime_type);
		mMediaTypeCombo->setEnabled( can_change_media );
		childSetText("mime_type", mime_type);

		mMediaUrlCheck->set( parcel->getObscureMedia() );
		mMediaUrlCheck->setEnabled( can_change_media );

		mMusicUrlCheck->set( parcel->getObscureMusic() );
		mMusicUrlCheck->setEnabled( can_change_media );

		// don't display urls if you're not able to change it
		// much requested change in forums so people can't 'steal' urls
		// NOTE: bug#2009 means this is still vunerable - however, bug
		// should be closed since this bug opens up major security issues elsewhere.
		bool obscure_media = ! can_change_media && parcel->getObscureMedia();
		bool obscure_music = ! can_change_media && parcel->getObscureMusic();

		// Special code to disable asterixes for html type
		if(mime_type == "text/html")
		{
			obscure_media = false;
			mMediaUrlCheck->set( 0 );
			mMediaUrlCheck->setEnabled( false );
		}

		mMusicURLEdit->setDrawAsterixes( obscure_music );
		mMediaURLEdit->setDrawAsterixes( obscure_media );

		mMediaAutoScaleCheck->set( parcel->getMediaAutoScale () );
		mMediaAutoScaleCheck->setEnabled ( can_change_media );

		// Special code to disable looping checkbox for HTML MIME type
		// (DEV-10042 -- Parcel Media: "Loop Media" should be disabled for static media types)
		bool allow_looping = LLMIMETypes::findAllowLooping( mime_type );
		if ( allow_looping )
			mMediaLoopCheck->set( parcel->getMediaLoop () );
		else
			mMediaLoopCheck->set( false );
		mMediaLoopCheck->setEnabled ( can_change_media && allow_looping );

		// disallow media size change for mime types that don't allow it
		bool allow_resize = LLMIMETypes::findAllowResize( mime_type );
		if ( allow_resize )
			mMediaWidthCtrl->setValue( parcel->getMediaWidth() );
		else
			mMediaWidthCtrl->setValue( 0 );
		mMediaWidthCtrl->setEnabled ( can_change_media && allow_resize );

		if ( allow_resize )
			mMediaHeightCtrl->setValue( parcel->getMediaHeight() );
		else
			mMediaHeightCtrl->setValue( 0 );
		mMediaHeightCtrl->setEnabled ( can_change_media && allow_resize );

		// enable/disable for text label for completeness
		mMediaSizeCtrlLabel->setEnabled( can_change_media && allow_resize );

		LLUUID tmp = parcel->getMediaID();
		mMediaTextureCtrl->setImageAssetID ( parcel->getMediaID() );
		mMediaTextureCtrl->setEnabled( can_change_media );

		mSetURLButton->setEnabled( can_change_media );

		#if 0
		// there is a media url and a media texture selected
		if ( ( ! ( std::string ( parcel->getMediaURL() ).empty () ) ) && ( ! ( parcel->getMediaID ().isNull () ) ) )
		{
			// turn on transport controls if allowed for this parcel
			mMediaStopButton->setEnabled ( editable );
			mMediaStartButton->setEnabled ( editable );
		}
		else
		{
			// no media url or no media texture
			mMediaStopButton->setEnabled ( FALSE );
			mMediaStartButton->setEnabled ( FALSE );
		};
		#endif

		LLFloaterURLEntry* floater_url_entry = (LLFloaterURLEntry*)mURLEntryFloater.get();
		if (floater_url_entry)
		{
			floater_url_entry->updateFromLandMediaPanel();
		}
	}
}

void LLPanelLandMedia::populateMIMECombo()
{
	std::string default_mime_type = "none/none";
	std::string default_label;
	LLMIMETypes::mime_widget_set_map_t::const_iterator it;
	for (it = LLMIMETypes::sWidgetMap.begin(); it != LLMIMETypes::sWidgetMap.end(); ++it)
	{
		const std::string& mime_type = it->first;
		const LLMIMETypes::LLMIMEWidgetSet& info = it->second;
		if (info.mDefaultMimeType == default_mime_type)
		{
			// Add this label at the end to make UI look cleaner
			default_label = info.mLabel;
		}
		else
		{
			mMediaTypeCombo->add(info.mLabel, mime_type);
		}
	}
	// *TODO: The sort order is based on std::map key, which is
	// ASCII-sorted and is wrong in other languages.  TRANSLATE
	mMediaTypeCombo->add( default_label, default_mime_type, ADD_BOTTOM );
}

void LLPanelLandMedia::setMediaType(const std::string& mime_type)
{
	LLParcel *parcel = mParcel->getParcel();
	if(parcel)
		parcel->setMediaType(mime_type);

	std::string media_key = LLMIMETypes::widgetType(mime_type);
	mMediaTypeCombo->setValue(media_key);
	childSetText("mime_type", mime_type);
}

void LLPanelLandMedia::setMediaURL(const std::string& media_url)
{
	mMediaURLEdit->setText(media_url);
	mMediaURLEdit->onCommit();
}

std::string LLPanelLandMedia::getMediaURL()
{
	return mMediaURLEdit->getText();
}

// static
void LLPanelLandMedia::onCommitType(LLUICtrl *ctrl, void *userdata)
{
	LLPanelLandMedia *self = (LLPanelLandMedia *)userdata;
	std::string current_type = LLMIMETypes::widgetType(self->childGetText("mime_type"));
	std::string new_type = self->mMediaTypeCombo->getValue();
	if(current_type != new_type)
	{
		self->childSetText("mime_type", LLMIMETypes::findDefaultMimeType(new_type));
	}
	onCommitAny(ctrl, userdata);

}

// static
void LLPanelLandMedia::onCommitAny(LLUICtrl*, void *userdata)
{
	LLPanelLandMedia *self = (LLPanelLandMedia *)userdata;

	LLParcel* parcel = self->mParcel->getParcel();
	if (!parcel)
	{
		return;
	}

	// Extract data from UI
	BOOL sound_local		= self->mCheckSoundLocal->get();
	int voice_setting		= self->mRadioVoiceChat->getSelectedIndex();
	std::string music_url	= self->mMusicURLEdit->getText();
	std::string media_url	= self->mMediaURLEdit->getText();
	std::string media_desc	= self->mMediaDescEdit->getText();
	std::string mime_type	= self->childGetText("mime_type");
	U8 media_auto_scale		= self->mMediaAutoScaleCheck->get();
	U8 media_loop           = self->mMediaLoopCheck->get();
	U8 obscure_media		= self->mMediaUrlCheck->get();
	U8 obscure_music		= self->mMusicUrlCheck->get();
	S32 media_width			= (S32)self->mMediaWidthCtrl->get();
	S32 media_height		= (S32)self->mMediaHeightCtrl->get();
	LLUUID media_id			= self->mMediaTextureCtrl->getImageAssetID();

	self->childSetText("mime_type", mime_type);

	BOOL voice_enabled;
	BOOL voice_estate_chan;

	switch(voice_setting)
	{
		default:
		case kRadioVoiceChatEstate:
			voice_enabled = TRUE;
			voice_estate_chan = TRUE;
		break;
		case kRadioVoiceChatPrivate:
			voice_enabled = TRUE;
			voice_estate_chan = FALSE;
		break;
		case kRadioVoiceChatDisable:
			voice_enabled = FALSE;
			voice_estate_chan = FALSE;
		break;
	}

	// Remove leading/trailing whitespace (common when copying/pasting)
	LLStringUtil::trim(music_url);
	LLStringUtil::trim(media_url);

	// Push data into current parcel
	parcel->setParcelFlag(PF_ALLOW_VOICE_CHAT, voice_enabled);
	parcel->setParcelFlag(PF_USE_ESTATE_VOICE_CHAN, voice_estate_chan);
	parcel->setParcelFlag(PF_SOUND_LOCAL, sound_local);
	parcel->setMusicURL(music_url);
	parcel->setMediaURL(media_url);
	parcel->setMediaType(mime_type);
	parcel->setMediaDesc(media_desc);
	parcel->setMediaWidth(media_width);
	parcel->setMediaHeight(media_height);
	parcel->setMediaID(media_id);
	parcel->setMediaAutoScale ( media_auto_scale );
	parcel->setMediaLoop ( media_loop );
	parcel->setObscureMedia( obscure_media );
	parcel->setObscureMusic( obscure_music );

	// Send current parcel data upstream to server
	LLViewerParcelMgr::getInstance()->sendParcelPropertiesUpdate( parcel );

	// Might have changed properties, so let's redraw!
	self->refresh();
}
// static
void LLPanelLandMedia::onSetBtn(void *userdata)
{
	LLPanelLandMedia *self = (LLPanelLandMedia *)userdata;
	self->mURLEntryFloater = LLFloaterURLEntry::show( self->getHandle() );
	LLFloater* parent_floater = gFloaterView->getParentFloater(self);
	if (parent_floater)
	{
		parent_floater->addDependentFloater(self->mURLEntryFloater.get());
	}
}
