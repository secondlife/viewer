/** 
 * @file llfloaterpostcard.cpp
 * @brief Postcard send floater, allows setting name, e-mail address, etc.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
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

#include "llfloaterpostcard.h"

#include "llfontgl.h"
#include "llsys.h"
#include "llgl.h"
#include "v3dmath.h"
#include "lldir.h"

#include "llagent.h"
#include "llui.h"
#include "lllineeditor.h"
#include "llviewertexteditor.h"
#include "llbutton.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llvieweruictrlfactory.h"
#include "lluploaddialog.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llstatusbar.h"
#include "llviewerregion.h"
#include "lleconomy.h"

#include "llgl.h"
#include "llglheaders.h"
#include "llimagejpeg.h"
#include "llimagej2c.h"
#include "llvfile.h"
#include "llvfs.h"

#include "llassetuploadresponders.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

LLLinkedList<LLFloaterPostcard> LLFloaterPostcard::sInstances;

///----------------------------------------------------------------------------
/// Class LLFloaterPostcard
///----------------------------------------------------------------------------

LLFloaterPostcard::LLFloaterPostcard(LLImageJPEG* jpeg, LLImageGL *img, const LLVector2& img_scale, const LLVector3d& pos_taken_global)
:	LLFloater("Postcard Floater"),
	mJPEGImage(jpeg),
	mViewerImage(img),
	mImageScale(img_scale),
	mPosTakenGlobal(pos_taken_global),
	mHasFirstMsgFocus(false)
{
	init();
}

void LLFloaterPostcard::init()
{
	// pick up the user's up-to-date email address
	if(!gAgent.getID().isNull())
	{
		// we're logged in, so we can get this info.
		gMessageSystem->newMessageFast(_PREHASH_UserInfoRequest);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gAgent.sendReliableMessage();
	}

	sInstances.addData(this);
}

// Destroys the object
LLFloaterPostcard::~LLFloaterPostcard()
{
	sInstances.removeData(this);
	mJPEGImage = NULL; // deletes image
}

BOOL LLFloaterPostcard::postBuild()
{
	childSetAction("cancel_btn", onClickCancel, this);
	childSetAction("send_btn", onClickSend, this);

	childDisable("from_form");

	std::string name_string;
	gAgent.buildFullname(name_string);
	childSetValue("name_form", LLSD(name_string));

	LLTextEditor* MsgField = LLUICtrlFactory::getTextEditorByName(this, "msg_form");
	if (MsgField)
	{
		MsgField->setWordWrap(TRUE);

		// For the first time a user focusess to .the msg box, all text will be selected.
		MsgField->setFocusChangedCallback(onMsgFormFocusRecieved, this);
	}
	
	childSetFocus("to_form", TRUE);

    return TRUE;
}



// static
LLFloaterPostcard* LLFloaterPostcard::showFromSnapshot(LLImageJPEG *jpeg, LLImageGL *img, const LLVector2 &image_scale, const LLVector3d& pos_taken_global)
{
	// Take the images from the caller
	// It's now our job to clean them up
	LLFloaterPostcard *instance = new LLFloaterPostcard(jpeg, img, image_scale, pos_taken_global);

	gUICtrlFactory->buildFloater(instance, "floater_postcard.xml");

	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	instance->setOrigin(left, top - instance->getRect().getHeight());
	
	instance->open();		/*Flawfinder: ignore*/

	return instance;
}

void LLFloaterPostcard::draw()
{
	LLGLSUIDefault gls_ui;
	LLFloater::draw();

	if(getVisible() && !mMinimized && mViewerImage.notNull() && mJPEGImage.notNull()) 
	{
		LLRect rect(mRect);

		// first set the max extents of our preview
		rect.translate(-rect.mLeft, -rect.mBottom);
		rect.mLeft += 280;
		rect.mRight -= 10;
		rect.mTop -= 20;
		rect.mBottom = rect.mTop - 130;

		// then fix the aspect ratio
		F32 ratio = (F32)mJPEGImage->getWidth() / (F32)mJPEGImage->getHeight();
		if ((F32)rect.getWidth() / (F32)rect.getHeight() >= ratio)
		{
			rect.mRight = (S32)((F32)rect.mLeft + ((F32)rect.getHeight() * ratio));
		}
		else
		{
			rect.mBottom = (S32)((F32)rect.mTop - ((F32)rect.getWidth() / ratio));
		}
		{
			LLGLSNoTexture gls_no_texture;
			gl_rect_2d(rect, LLColor4(0.f, 0.f, 0.f, 1.f));
			rect.stretch(-1);
		}
		{

		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		{
			glScalef(mImageScale.mV[VX], mImageScale.mV[VY], 1.f);
			glMatrixMode(GL_MODELVIEW);
			gl_draw_scaled_image(rect.mLeft,
								 rect.mBottom,
								 rect.getWidth(),
								 rect.getHeight(),
								 mViewerImage, 
								 LLColor4::white);
		}
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		}
	}
}

// static
void LLFloaterPostcard::onClickCancel(void* data)
{
	if (data)
	{
		LLFloaterPostcard *self = (LLFloaterPostcard *)data;

		self->close(false);
	}
}

class LLSendPostcardResponder : public LLAssetUploadResponder
{
public:
	LLSendPostcardResponder(const LLSD &post_data,
							const LLUUID& vfile_id,
							LLAssetType::EType asset_type):
	    LLAssetUploadResponder(post_data, vfile_id, asset_type)
	{	
	}
	// *TODO define custom uploadFailed here so it's not such a generic message
	void uploadComplete(const LLSD& content)
	{
		// we don't care about what the server returns from this post, just clean up the UI
		LLUploadDialog::modalUploadFinished();
	}
};

// static
void LLFloaterPostcard::onClickSend(void* data)
{
	if (data)
	{
		LLFloaterPostcard *self = (LLFloaterPostcard *)data;

		std::string from(self->childGetValue("from_form").asString());
		std::string to(self->childGetValue("to_form").asString());

		if (to.empty() || to.find('@') == std::string::npos)
		{
			gViewerWindow->alertXml("PromptRecipientEmail");
			return;
		}

		if (from.empty() || from.find('@') == std::string::npos)
		{
			gViewerWindow->alertXml("PromptSelfEmail");
			return;
		}

		std::string subject(self->childGetValue("subject_form").asString());
		if(subject.empty() || !self->mHasFirstMsgFocus)
		{
			gViewerWindow->alertXml("PromptMissingSubjMsg", missingSubjMsgAlertCallback, self);
			return;
		}

		if (self->mJPEGImage.notNull())
		{
			self->sendPostcard();
		}
		else
		{
			gViewerWindow->alertXml("ErrorProcessingSnapshot");
		}
	}
}

// static
void LLFloaterPostcard::uploadCallback(const LLUUID& asset_id, void *user_data, S32 result, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLFloaterPostcard *self = (LLFloaterPostcard *)user_data;
	
	LLUploadDialog::modalUploadFinished();
	
	if (result)
	{
		LLStringBase<char>::format_map_t args;
		args["[REASON]"] = std::string(LLAssetStorage::getErrorString(result));
		gViewerWindow->alertXml("ErrorUploadingPostcard", args);
	}
	else
	{
		// only create the postcard once the upload succeeds

		// request the postcard
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("SendPostcard");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->addUUID("AssetID", self->mAssetID);
		msg->addVector3d("PosGlobal", self->mPosTakenGlobal);
		msg->addString("To", self->childGetValue("to_form").asString());
		msg->addString("From", self->childGetValue("from_form").asString());
		msg->addString("Name", self->childGetValue("name_form").asString());
		msg->addString("Subject", self->childGetValue("subject_form").asString());
		msg->addString("Msg", self->childGetValue("msg_form").asString());
		msg->addBOOL("AllowPublish", FALSE);
		msg->addBOOL("MaturePublish", FALSE);
		gAgent.sendReliableMessage();
	}

	self->close();
}

// static
void LLFloaterPostcard::updateUserInfo(const char *email)
{
	LLFloaterPostcard *instance;

	sInstances.resetList();
	while ((instance = sInstances.getNextData()))
	{
		const LLString& text = instance->childGetValue("from_form").asString();
		if (text.empty())
		{
			// there's no text in this field yet, pre-populate
			instance->childSetValue("from_form", LLSD(email));
		}
	}
}

void LLFloaterPostcard::onMsgFormFocusRecieved(LLFocusableElement* receiver, void* data)
{
	LLFloaterPostcard* self = (LLFloaterPostcard *)data;
	if(self) 
	{
		LLTextEditor* msgForm = LLUICtrlFactory::getTextEditorByName(self, "msg_form");
		if(msgForm && msgForm == receiver && msgForm->hasFocus() && !(self->mHasFirstMsgFocus))
		{
			self->mHasFirstMsgFocus = true;
			msgForm->setText(LLString::null);
		}
	}
}

void LLFloaterPostcard::missingSubjMsgAlertCallback(S32 option, void* data)
{
	if(data)
	{
		LLFloaterPostcard* self = static_cast<LLFloaterPostcard*>(data);
		if(0 == option)
		{
			// User clicked OK
			if((self->childGetValue("subject_form").asString()).empty())
			{
				// Stuff the subject back into the form.
				self->childSetValue("subject_form", self->childGetText("default_subject"));
			}

			if(!self->mHasFirstMsgFocus)
			{
				// The user never switched focus to the messagee window. 
				// Using the default string.
				self->childSetValue("msg_form", self->childGetText("default_message"));
			}

			self->sendPostcard();
		}
	}
}

void LLFloaterPostcard::sendPostcard()
{
	mTransactionID.generate();
	mAssetID = mTransactionID.makeAssetID(gAgent.getSecureSessionID());
	LLVFile::writeFile(mJPEGImage->getData(), mJPEGImage->getDataSize(), gVFS, mAssetID, LLAssetType::AT_IMAGE_JPEG);

	// upload the image
	std::string url = gAgent.getRegion()->getCapability("SendPostcard");
	if(!url.empty())
	{
		llinfos << "Send Postcard via capability" << llendl;
		LLSD body = LLSD::emptyMap();
		// the capability already encodes: agent ID, region ID
		body["pos-global"] = mPosTakenGlobal.getValue();
		body["to"] = childGetValue("to_form").asString();
		body["from"] = childGetValue("from_form").asString();
		body["name"] = childGetValue("name_form").asString();
		body["subject"] = childGetValue("subject_form").asString();
		body["msg"] = childGetValue("msg_form").asString();
		LLHTTPClient::post(url, body, new LLSendPostcardResponder(body, mAssetID, LLAssetType::AT_IMAGE_JPEG));
	} 
	else
	{
		gAssetStorage->storeAssetData(mTransactionID, LLAssetType::AT_IMAGE_JPEG, &uploadCallback, (void *)this, FALSE);
	}

	LLUploadDialog::modalUploadDialog("Uploading...\n\nPostcard");

	// don't destroy the window until the upload is done
	// this way we keep the information in the form
	setVisible(FALSE);

	// also remove any dependency on another floater
	// so that we can be sure to outlive it while we
	// need to.
	LLFloater* dependee = getDependee();
	if (dependee)
		dependee->removeDependentFloater(this);
}
