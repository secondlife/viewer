/** 
 * @file llfloaterpostcard.cpp
 * @brief Postcard send floater, allows setting name, e-mail address, etc.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
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
#include "viewer.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

LLLinkedList<LLFloaterPostcard> LLFloaterPostcard::sInstances;

///----------------------------------------------------------------------------
/// Class LLFloaterPostcard
///----------------------------------------------------------------------------

LLFloaterPostcard::LLFloaterPostcard(LLImageJPEG* jpeg, LLImageGL *img, const LLVector2& img_scale, const LLVector3d& pos_taken_global)
:	LLFloater("Postcard Floater"),
	mViewerImage(img),
	mJPEGImage(jpeg),
	mImageScale(img_scale),
	mPosTakenGlobal(pos_taken_global)
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
	childSetAction("publish_help_btn", onClickPublishHelp, this);

	if (gAgent.mAccess < SIM_ACCESS_MATURE)
	{
		// Disable these buttons if they are PG (Teen) users
		childDisable("allow_publish_check");
		childHide("allow_publish_check");
		childDisable("publish_help_btn");
		childHide("publish_help_btn");
		childDisable("mature_check");
		childHide("mature_check");
	}
	
	LLString name_string;
	gAgent.buildFullname(name_string);
	
	childSetValue("name_form", LLSD(name_string));
	
	// XUI:translate
	LLString msg("Postcard from ");
	msg += gSecondLife;
	childSetValue("subject_form", LLSD(msg));

	LLTextEditor *MsgField = LLUICtrlFactory::getTextEditorByName(this, "msg_form");
	if (MsgField)
	{
		MsgField->setText("Check this out!");
		MsgField->setWordWrap(TRUE);
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
	
	instance->open();

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

		self->onClose(false);
	}
}

// static
void LLFloaterPostcard::onClickSend(void* data)
{
	if (data)
	{
		LLFloaterPostcard *self = (LLFloaterPostcard *)data;

		LLString from(self->childGetValue("from_form").asString().c_str());
		LLString to(self->childGetValue("to_form").asString().c_str());

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

		if (self->mJPEGImage.notNull())
		{
			// upload the image
			self->mTransactionID.generate();
			self->mAssetID = self->mTransactionID.makeAssetID(gAgent.getSecureSessionID());
			LLVFile::writeFile(self->mJPEGImage->getData(), self->mJPEGImage->getDataSize(), gVFS, self->mAssetID, LLAssetType::AT_IMAGE_JPEG);
			
			gAssetStorage->storeAssetData(self->mTransactionID, LLAssetType::AT_IMAGE_JPEG, &uploadCallback, (void *)self, FALSE);

			LLUploadDialog::modalUploadDialog("Uploading...\n\nPostcard");

			// don't destroy the window until the upload is done
			// this way we keep the information in the form
			self->setVisible(FALSE);

			// also remove any dependency on another floater
			// so that we can be sure to outlive it while we
			// need to.
			LLFloater* dependee = self->getDependee();
			if (dependee)
				dependee->removeDependentFloater(self);
		}
		else
		{
			gViewerWindow->alertXml("ErrorProcessingSnapshot");
		}
	}
}

// static
void LLFloaterPostcard::onClickPublishHelp(void* data)
{
	gViewerWindow->alertXml("ClickPublishHelpPostcard");
}

// static
void LLFloaterPostcard::uploadCallback(const LLUUID& asset_id, void *user_data, S32 result) // StoreAssetData callback (fixed)
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
		msg->addBOOL("AllowPublish", self->childGetValue("allow_publish_check").asBoolean());
		msg->addBOOL("MaturePublish", self->childGetValue("mature_check").asBoolean());
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
