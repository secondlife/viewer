/** 
 * @file llfloaterpostcard.cpp
 * @brief Postcard send floater, allows setting name, e-mail address, etc.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "llfloaterpostcard.h"

#include "llfontgl.h"
#include "llsys.h"
#include "llgl.h"
#include "v3dmath.h"
#include "lldir.h"

#include "llagent.h"
#include "llui.h"
#include "lllineeditor.h"
#include "llbutton.h"
#include "lltexteditor.h"
#include "llfloaterreg.h"
#include "llnotificationsutil.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "lluictrlfactory.h"
#include "lluploaddialog.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llstatusbar.h"
#include "llviewerregion.h"
#include "lleconomy.h"
#include "message.h"

#include "llimagejpeg.h"
#include "llimagej2c.h"
#include "llvfile.h"
#include "llvfs.h"
#include "llviewertexture.h"
#include "llassetuploadresponders.h"
#include "llagentui.h"

#include <boost/regex.hpp>  //boost.regex lib

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

///----------------------------------------------------------------------------
/// Class LLFloaterPostcard
///----------------------------------------------------------------------------

LLFloaterPostcard::LLFloaterPostcard(const LLSD& key)
:	LLFloater(key),
	mJPEGImage(NULL),
	mViewerImage(NULL),
	mHasFirstMsgFocus(false)
{
}

// Destroys the object
LLFloaterPostcard::~LLFloaterPostcard()
{
	mJPEGImage = NULL; // deletes image
}

BOOL LLFloaterPostcard::postBuild()
{
	// pick up the user's up-to-date email address
	gAgent.sendAgentUserInfoRequest();

	childSetAction("cancel_btn", onClickCancel, this);
	childSetAction("send_btn", onClickSend, this);

	getChildView("from_form")->setEnabled(FALSE);

	std::string name_string;
	LLAgentUI::buildFullname(name_string);
	getChild<LLUICtrl>("name_form")->setValue(LLSD(name_string));

	// For the first time a user focusess to .the msg box, all text will be selected.
	getChild<LLUICtrl>("msg_form")->setFocusChangedCallback(boost::bind(onMsgFormFocusRecieved, _1, this));
	
	getChild<LLUICtrl>("to_form")->setFocus(TRUE);

    return TRUE;
}

// static
LLFloaterPostcard* LLFloaterPostcard::showFromSnapshot(LLImageJPEG *jpeg, LLViewerTexture *img, const LLVector2 &image_scale, const LLVector3d& pos_taken_global)
{
	// Take the images from the caller
	// It's now our job to clean them up
	LLFloaterPostcard* instance = LLFloaterReg::showTypedInstance<LLFloaterPostcard>("postcard", LLSD(img->getID()));
	
	instance->mJPEGImage = jpeg;
	instance->mViewerImage = img;
	instance->mImageScale = image_scale;
	instance->mPosTakenGlobal = pos_taken_global;
	
	return instance;
}

void LLFloaterPostcard::draw()
{
	LLGLSUIDefault gls_ui;
	LLFloater::draw();

	if(!isMinimized() && mViewerImage.notNull() && mJPEGImage.notNull()) 
	{
		LLRect rect(getRect());

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
			rect.mRight = LLRect::tCoordType((F32)rect.mLeft + ((F32)rect.getHeight() * ratio));
		}
		else
		{
			rect.mBottom = LLRect::tCoordType((F32)rect.mTop - ((F32)rect.getWidth() / ratio));
		}
		{
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
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
								 mViewerImage.get(), 
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

		self->closeFloater(false);
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

		std::string from(self->getChild<LLUICtrl>("from_form")->getValue().asString());
		std::string to(self->getChild<LLUICtrl>("to_form")->getValue().asString());
		
		boost::regex emailFormat("[A-Za-z0-9.%+-_]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}(,[ \t]*[A-Za-z0-9.%+-_]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,})*");
		
		if (to.empty() || !boost::regex_match(to, emailFormat))
		{
			LLNotificationsUtil::add("PromptRecipientEmail");
			return;
		}

		if (from.empty() || !boost::regex_match(from, emailFormat))
		{
			LLNotificationsUtil::add("PromptSelfEmail");
			return;
		}

		std::string subject(self->getChild<LLUICtrl>("subject_form")->getValue().asString());
		if(subject.empty() || !self->mHasFirstMsgFocus)
		{
			LLNotificationsUtil::add("PromptMissingSubjMsg", LLSD(), LLSD(), boost::bind(&LLFloaterPostcard::missingSubjMsgAlertCallback, self, _1, _2));
			return;
		}

		if (self->mJPEGImage.notNull())
		{
			self->sendPostcard();
		}
		else
		{
			LLNotificationsUtil::add("ErrorProcessingSnapshot");
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
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(result));
		LLNotificationsUtil::add("ErrorUploadingPostcard", args);
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
		msg->addString("To", self->getChild<LLUICtrl>("to_form")->getValue().asString());
		msg->addString("From", self->getChild<LLUICtrl>("from_form")->getValue().asString());
		msg->addString("Name", self->getChild<LLUICtrl>("name_form")->getValue().asString());
		msg->addString("Subject", self->getChild<LLUICtrl>("subject_form")->getValue().asString());
		msg->addString("Msg", self->getChild<LLUICtrl>("msg_form")->getValue().asString());
		msg->addBOOL("AllowPublish", FALSE);
		msg->addBOOL("MaturePublish", FALSE);
		gAgent.sendReliableMessage();
	}

	self->closeFloater();
}

// static
void LLFloaterPostcard::updateUserInfo(const std::string& email)
{
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("postcard");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin();
		 iter != inst_list.end(); ++iter)
	{
		LLFloater* instance = *iter;
		const std::string& text = instance->getChild<LLUICtrl>("from_form")->getValue().asString();
		if (text.empty())
		{
			// there's no text in this field yet, pre-populate
			instance->getChild<LLUICtrl>("from_form")->setValue(LLSD(email));
		}
	}
}

void LLFloaterPostcard::onMsgFormFocusRecieved(LLFocusableElement* receiver, void* data)
{
	LLFloaterPostcard* self = (LLFloaterPostcard *)data;
	if(self) 
	{
		LLTextEditor* msgForm = self->getChild<LLTextEditor>("msg_form");
		if(msgForm && msgForm == receiver && msgForm->hasFocus() && !(self->mHasFirstMsgFocus))
		{
			self->mHasFirstMsgFocus = true;
			msgForm->setText(LLStringUtil::null);
		}
	}
}

bool LLFloaterPostcard::missingSubjMsgAlertCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if(0 == option)
	{
		// User clicked OK
		if((getChild<LLUICtrl>("subject_form")->getValue().asString()).empty())
		{
			// Stuff the subject back into the form.
			getChild<LLUICtrl>("subject_form")->setValue(getString("default_subject"));
		}

		if(!mHasFirstMsgFocus)
		{
			// The user never switched focus to the messagee window. 
			// Using the default string.
			getChild<LLUICtrl>("msg_form")->setValue(getString("default_message"));
		}

		sendPostcard();
	}
	return false;
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
		body["to"] = getChild<LLUICtrl>("to_form")->getValue().asString();
		body["from"] = getChild<LLUICtrl>("from_form")->getValue().asString();
		body["name"] = getChild<LLUICtrl>("name_form")->getValue().asString();
		body["subject"] = getChild<LLUICtrl>("subject_form")->getValue().asString();
		body["msg"] = getChild<LLUICtrl>("msg_form")->getValue().asString();
		LLHTTPClient::post(url, body, new LLSendPostcardResponder(body, mAssetID, LLAssetType::AT_IMAGE_JPEG));
	} 
	else
	{
		gAssetStorage->storeAssetData(mTransactionID, LLAssetType::AT_IMAGE_JPEG, &uploadCallback, (void *)this, FALSE);
	}

	LLUploadDialog::modalUploadDialog(getString("upload_message"));

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
