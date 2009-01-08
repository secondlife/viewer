/** 
 * @file llfloaterauction.cpp
 * @author James Cook, Ian Wilkes
 * @brief Implementation of the auction floater.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
#include "llfloaterauction.h"

#include "lldir.h"
#include "llgl.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "llparcel.h"
#include "llvfile.h"
#include "llvfs.h"

#include "llagent.h"
#include "llcombobox.h"
#include "llnotify.h"
#include "llsavedsettingsglue.h"
#include "llviewerimagelist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llviewerdisplay.h"
#include "llviewercontrol.h"
#include "llui.h"
#include "llrender.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

void auction_j2c_upload_done(const LLUUID& asset_id,
							   void* user_data, S32 status, LLExtStat ext_status);
void auction_tga_upload_done(const LLUUID& asset_id,
							   void* user_data, S32 status, LLExtStat ext_status);

///----------------------------------------------------------------------------
/// Class llfloaterauction
///----------------------------------------------------------------------------

LLFloaterAuction* LLFloaterAuction::sInstance = NULL;

// Default constructor
LLFloaterAuction::LLFloaterAuction() :
	LLFloater(std::string("floater_auction")),
	mParcelID(-1)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_auction.xml");

	childSetValue("fence_check",
		LLSD( gSavedSettings.getBOOL("AuctionShowFence") ) );
	childSetCommitCallback("fence_check",
		LLSavedSettingsGlue::setBOOL, (void*)"AuctionShowFence");

	childSetAction("snapshot_btn", onClickSnapshot, this);
	childSetAction("ok_btn", onClickOK, this);
}

// Destroys the object
LLFloaterAuction::~LLFloaterAuction()
{
	sInstance = NULL;
}

// static
void LLFloaterAuction::show()
{
	if(!sInstance)
	{
		sInstance = new LLFloaterAuction();
		sInstance->center();
		sInstance->setFocus(TRUE);
	}
	sInstance->initialize();
	sInstance->open();	/*Flawfinder: ignore*/
}

void LLFloaterAuction::initialize()
{
	mParcelp = LLViewerParcelMgr::getInstance()->getParcelSelection();
	LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();
	LLParcel* parcelp = mParcelp->getParcel();
	if(parcelp && region && !parcelp->getForSale())
	{
		mParcelHost = region->getHost();
		mParcelID = parcelp->getLocalID();

		childSetText("parcel_text", parcelp->getName());
		childEnable("snapshot_btn");
		childEnable("ok_btn");
	}
	else
	{
		mParcelHost.invalidate();
		if(parcelp && parcelp->getForSale())
		{
			childSetText("parcel_text", getString("already for sale"));
		}
		else
		{
			childSetText("parcel_text", LLStringUtil::null);
		}
		mParcelID = -1;
		childSetEnabled("snapshot_btn", false);
		childSetEnabled("ok_btn", false);
	}
	mImageID.setNull();
	mImage = NULL;
}

void LLFloaterAuction::draw()
{
	LLFloater::draw();

	if(!isMinimized() && mImage.notNull()) 
	{
		LLRect rect;
		if (childGetRect("snapshot_icon", rect))
		{
			{
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				gl_rect_2d(rect, LLColor4(0.f, 0.f, 0.f, 1.f));
				rect.stretch(-1);
			}
			{
				LLGLSUIDefault gls_ui;
				gGL.color3f(1.f, 1.f, 1.f);
				gl_draw_scaled_image(rect.mLeft,
									 rect.mBottom,
									 rect.getWidth(),
									 rect.getHeight(),
									 mImage);
			}
		}
	}
}


// static
void LLFloaterAuction::onClickSnapshot(void* data)
{
	LLFloaterAuction* self = (LLFloaterAuction*)(data);

	LLPointer<LLImageRaw> raw = new LLImageRaw;

	gForceRenderLandFence = self->childGetValue("fence_check").asBoolean();
	BOOL success = gViewerWindow->rawSnapshot(raw,
											  gViewerWindow->getWindowWidth(),
											  gViewerWindow->getWindowHeight(),
											  TRUE, FALSE,
											  FALSE, FALSE);
	gForceRenderLandFence = FALSE;

	if (success)
	{
		self->mTransactionID.generate();
		self->mImageID = self->mTransactionID.makeAssetID(gAgent.getSecureSessionID());

		if(!gSavedSettings.getBOOL("QuietSnapshotsToDisk"))
		{
			gViewerWindow->playSnapshotAnimAndSound();
		}
		llinfos << "Writing TGA..." << llendl;

		LLPointer<LLImageTGA> tga = new LLImageTGA;
		tga->encode(raw);
		LLVFile::writeFile(tga->getData(), tga->getDataSize(), gVFS, self->mImageID, LLAssetType::AT_IMAGE_TGA);
		
		raw->biasedScaleToPowerOfTwo(LLViewerImage::MAX_IMAGE_SIZE_DEFAULT);

		llinfos << "Writing J2C..." << llendl;

		LLPointer<LLImageJ2C> j2c = new LLImageJ2C;
		j2c->encode(raw, 0.0f);
		LLVFile::writeFile(j2c->getData(), j2c->getDataSize(), gVFS, self->mImageID, LLAssetType::AT_TEXTURE);

		self->mImage = new LLImageGL((LLImageRaw*)raw, FALSE);
		gGL.getTexUnit(0)->bind(self->mImage);
		self->mImage->setClamp(TRUE, TRUE);
	}
	else
	{
		llwarns << "Unable to take snapshot" << llendl;
	}
}

// static
void LLFloaterAuction::onClickOK(void* data)
{
	LLFloaterAuction* self = (LLFloaterAuction*)(data);

	if(self->mImageID.notNull())
	{
		LLSD parcel_name = self->childGetValue("parcel_text");

	// create the asset
		std::string* name = new std::string(parcel_name.asString());
		gAssetStorage->storeAssetData(self->mTransactionID, LLAssetType::AT_IMAGE_TGA,
									&auction_tga_upload_done,
									(void*)name,
									FALSE);
		self->getWindow()->incBusyCount();

		std::string* j2c_name = new std::string(parcel_name.asString());
		gAssetStorage->storeAssetData(self->mTransactionID, LLAssetType::AT_TEXTURE,
								   &auction_j2c_upload_done,
								   (void*)j2c_name,
								   FALSE);
		self->getWindow()->incBusyCount();

		LLNotifications::instance().add("UploadingAuctionSnapshot");

	}
	LLMessageSystem* msg = gMessageSystem;

	msg->newMessage("ViewerStartAuction");

	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("ParcelData");
	msg->addS32("LocalID", self->mParcelID);
	msg->addUUID("SnapshotID", self->mImageID);
	msg->sendReliable(self->mParcelHost);

	// clean up floater, and get out
	self->mImageID.setNull();
	self->mImage = NULL;
	self->mParcelID = -1;
	self->mParcelHost.invalidate();
	self->close();
}


///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------

void auction_tga_upload_done(const LLUUID& asset_id, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	std::string* name = (std::string*)(user_data);
	llinfos << "Upload of asset '" << *name << "' " << asset_id
			<< " returned " << status << llendl;
	delete name;

	gViewerWindow->getWindow()->decBusyCount();

	if (0 == status)
	{
		LLNotifications::instance().add("UploadWebSnapshotDone");
	}
	else
	{
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotifications::instance().add("UploadAuctionSnapshotFail", args);
	}
}

void auction_j2c_upload_done(const LLUUID& asset_id, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	std::string* name = (std::string*)(user_data);
	llinfos << "Upload of asset '" << *name << "' " << asset_id
			<< " returned " << status << llendl;
	delete name;

	gViewerWindow->getWindow()->decBusyCount();

	if (0 == status)
	{
		LLNotifications::instance().add("UploadSnapshotDone");
	}
	else
	{
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotifications::instance().add("UploadAuctionSnapshotFail", args);
	}
}
