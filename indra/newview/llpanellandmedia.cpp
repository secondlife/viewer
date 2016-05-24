/**
 * @file llpanellandmedia.cpp
 * @brief Allows configuration of "media" for a land parcel,
 *   for example movies, web pages, and audio.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llpanellandmedia.h"

// viewer includes
#include "llmimetypes.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewermedia.h"
#include "llviewerparcelmedia.h"
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

//---------------------------------------------------------------------------
// LLPanelLandMedia
//---------------------------------------------------------------------------

LLPanelLandMedia::LLPanelLandMedia(LLParcelSelectionHandle& parcel)
:	LLPanel(),
	mParcel(parcel),
	mMediaURLEdit(NULL),
	mMediaDescEdit(NULL),
	mMediaTypeCombo(NULL),
	mSetURLButton(NULL),
	mMediaHeightCtrl(NULL),
	mMediaWidthCtrl(NULL),
	mMediaSizeCtrlLabel(NULL),
	mMediaTextureCtrl(NULL),
	mMediaAutoScaleCheck(NULL),
	mMediaLoopCheck(NULL)
{
}


// virtual
LLPanelLandMedia::~LLPanelLandMedia()
{
}

BOOL LLPanelLandMedia::postBuild()
{

	mMediaTextureCtrl = getChild<LLTextureCtrl>("media texture");
	mMediaTextureCtrl->setCommitCallback( onCommitAny, this );
	mMediaTextureCtrl->setAllowNoTexture ( TRUE );
	mMediaTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
	mMediaTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);
	mMediaTextureCtrl->setNonImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);

	mMediaAutoScaleCheck = getChild<LLCheckBoxCtrl>("media_auto_scale");
	childSetCommitCallback("media_auto_scale", onCommitAny, this);

	mMediaLoopCheck = getChild<LLCheckBoxCtrl>("media_loop");
	childSetCommitCallback("media_loop", onCommitAny, this );

	mMediaURLEdit = getChild<LLLineEditor>("media_url");
	childSetCommitCallback("media_url", onCommitAny, this );

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

		mMediaURLEdit->setText(parcel->getMediaURL());
		mMediaURLEdit->setEnabled( FALSE );

		getChild<LLUICtrl>("current_url")->setValue(parcel->getMediaCurrentURL());

		mMediaDescEdit->setText(parcel->getMediaDesc());
		mMediaDescEdit->setEnabled( can_change_media );

		std::string mime_type = parcel->getMediaType();
		if (mime_type.empty() || mime_type == LLMIMETypes::getDefaultMimeType())
		{
			mime_type = LLMIMETypes::getDefaultMimeTypeTranslation();
		}
		setMediaType(mime_type);
		mMediaTypeCombo->setEnabled( can_change_media );
		getChild<LLUICtrl>("mime_type")->setValue(mime_type);

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

	}
}

void LLPanelLandMedia::populateMIMECombo()
{
	std::string default_mime_type = LLMIMETypes::getDefaultMimeType();
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

	mMediaTypeCombo->add( default_label, default_mime_type, ADD_BOTTOM );
}

void LLPanelLandMedia::setMediaType(const std::string& mime_type)
{
	LLParcel *parcel = mParcel->getParcel();
	if(parcel)
		parcel->setMediaType(mime_type);

	std::string media_key = LLMIMETypes::widgetType(mime_type);
	mMediaTypeCombo->setValue(media_key);

	std::string mime_str = mime_type;
	if(LLMIMETypes::getDefaultMimeType() == mime_type)
	{
		// Instead of showing predefined "none/none" we are going to show something 
		// localizable - "none" for example (see EXT-6542)
		mime_str = LLMIMETypes::getDefaultMimeTypeTranslation();
	}
	getChild<LLUICtrl>("mime_type")->setValue(mime_str);
}

void LLPanelLandMedia::setMediaURL(const std::string& media_url)
{
	mMediaURLEdit->setText(media_url);
	LLParcel *parcel = mParcel->getParcel();
	if(parcel)
		parcel->setMediaCurrentURL(media_url);
	// LLViewerMedia::navigateHome();


	mMediaURLEdit->onCommit();
	// LLViewerParcelMedia::sendMediaNavigateMessage(media_url);
	getChild<LLUICtrl>("current_url")->setValue(media_url);
}
std::string LLPanelLandMedia::getMediaURL()
{
	return mMediaURLEdit->getText();	
}

// static
void LLPanelLandMedia::onCommitType(LLUICtrl *ctrl, void *userdata)
{
	LLPanelLandMedia *self = (LLPanelLandMedia *)userdata;
	std::string current_type = LLMIMETypes::widgetType(self->getChild<LLUICtrl>("mime_type")->getValue().asString());
	std::string new_type = self->mMediaTypeCombo->getValue();
	if(current_type != new_type)
	{
		self->getChild<LLUICtrl>("mime_type")->setValue(LLMIMETypes::findDefaultMimeType(new_type));
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
	std::string media_url	= self->mMediaURLEdit->getText();
	std::string media_desc	= self->mMediaDescEdit->getText();
	std::string mime_type	= self->getChild<LLUICtrl>("mime_type")->getValue().asString();
	U8 media_auto_scale		= self->mMediaAutoScaleCheck->get();
	U8 media_loop           = self->mMediaLoopCheck->get();
	S32 media_width			= (S32)self->mMediaWidthCtrl->get();
	S32 media_height		= (S32)self->mMediaHeightCtrl->get();
	LLUUID media_id			= self->mMediaTextureCtrl->getImageAssetID();


	self->getChild<LLUICtrl>("mime_type")->setValue(mime_type);

	// Remove leading/trailing whitespace (common when copying/pasting)
	LLStringUtil::trim(media_url);

	// Push data into current parcel
	parcel->setMediaURL(media_url);
	parcel->setMediaType(mime_type);
	parcel->setMediaDesc(media_desc);
	parcel->setMediaWidth(media_width);
	parcel->setMediaHeight(media_height);
	parcel->setMediaID(media_id);
	parcel->setMediaAutoScale ( media_auto_scale );
	parcel->setMediaLoop ( media_loop );

	// Send current parcel data upstream to server
	LLViewerParcelMgr::getInstance()->sendParcelPropertiesUpdate( parcel );

	// Might have changed properties, so let's redraw!
	self->refresh();
}
// static
void LLPanelLandMedia::onSetBtn(void *userdata)
{
	LLPanelLandMedia *self = (LLPanelLandMedia *)userdata;
	self->mURLEntryFloater = LLFloaterURLEntry::show( self->getHandle(), self->getMediaURL() );
	LLFloater* parent_floater = gFloaterView->getParentFloater(self);
	if (parent_floater)
	{
		parent_floater->addDependentFloater(self->mURLEntryFloater.get());
	}
}

// static
void LLPanelLandMedia::onResetBtn(void *userdata)
{
	LLPanelLandMedia *self = (LLPanelLandMedia *)userdata;
	LLParcel* parcel = self->mParcel->getParcel();
	// LLViewerMedia::navigateHome();
	self->refresh();
	self->getChild<LLUICtrl>("current_url")->setValue(parcel->getMediaURL());
	// LLViewerParcelMedia::sendMediaNavigateMessage(parcel->getMediaURL());

}

