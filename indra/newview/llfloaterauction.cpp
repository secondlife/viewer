/** 
 * @file llfloaterauction.cpp
 * @author James Cook, Ian Wilkes
 * @brief Implementation of the auction floater.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
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
#include "llvieweruictrlfactory.h"
#include "llviewerwindow.h"
#include "viewer.h"
#include "llui.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

void auction_j2c_upload_done(const LLUUID& asset_id,
							   void* user_data, S32 status);
void auction_tga_upload_done(const LLUUID& asset_id,
							   void* user_data, S32 status);

///----------------------------------------------------------------------------
/// Class llfloaterauction
///----------------------------------------------------------------------------

LLFloaterAuction* LLFloaterAuction::sInstance = NULL;

// Default constructor
LLFloaterAuction::LLFloaterAuction() :
	LLFloater("floater_auction"),
	mParcelID(-1)
{
	gUICtrlFactory->buildFloater(this, "floater_auction.xml");

	childSetValue("fence_check",
		LLSD( gSavedSettings.getBOOL("AuctionShowFence") ) );
	childSetCommitCallback("fence_check",
		LLSavedSettingsGlue::setBOOL, (void*)"AuctionShowFence");

	LLComboBox* combo = LLUICtrlFactory::getComboBoxByName(this, "saletype_combo");
	if (combo)
	{
		combo->selectFirstItem();
	}

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
	mParcelp = gParcelMgr->getParcelSelection();
	LLViewerRegion* region = gParcelMgr->getSelectionRegion();
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
			childSetText("parcel_text", childGetText("already for sale"));
		}
		else
		{
			childSetText("parcel_text", "");
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

	if(getVisible() && !isMinimized() && mImage.notNull()) 
	{
		LLRect rect;
		if (childGetRect("snapshot_icon", rect))
		{
			{
				LLGLSNoTexture gls_no_texture;
				gl_rect_2d(rect, LLColor4(0.f, 0.f, 0.f, 1.f));
				rect.stretch(-1);
			}
			{
				LLGLSUIDefault gls_ui;
				glColor3f(1.f, 1.f, 1.f);
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
											  TRUE,
											  FALSE, FALSE);
	gForceRenderLandFence = FALSE;

	if (success)
	{
		self->mTransactionID.generate();
		self->mImageID = self->mTransactionID.makeAssetID(gAgent.getSecureSessionID());

		gViewerWindow->playSnapshotAnimAndSound();
		llinfos << "Writing TGA..." << llendl;

		LLPointer<LLImageTGA> tga = new LLImageTGA;
		tga->encode(raw);
		LLVFile::writeFile(tga->getData(), tga->getDataSize(), gVFS, self->mImageID, LLAssetType::AT_IMAGE_TGA);
		
		raw->biasedScaleToPowerOfTwo(LLViewerImage::MAX_IMAGE_SIZE_DEFAULT);

		llinfos << "Writing J2C..." << llendl;

		LLPointer<LLImageJ2C> j2c = new LLImageJ2C;
		j2c->encode(raw);
		LLVFile::writeFile(j2c->getData(), j2c->getDataSize(), gVFS, self->mImageID, LLAssetType::AT_TEXTURE);

		self->mImage = new LLImageGL((LLImageRaw*)raw, FALSE);
		self->mImage->bind();
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
	bool is_auction = false;
	LLComboBox* combo = LLUICtrlFactory::getComboBoxByName(self, "saletype_combo");
	if (combo 
		&& combo->getCurrentIndex() == 0)
	{
		is_auction = true;
	}
	if(self->mImageID.notNull())
	{
		LLSD parcel_name = self->childGetValue("parcel_text");

		// create the asset
		if(is_auction)
		{
			// only need the tga if it is an auction.
			LLString* name = new LLString(parcel_name.asString());
			gAssetStorage->storeAssetData(self->mTransactionID, LLAssetType::AT_IMAGE_TGA,
									   &auction_tga_upload_done,
									   (void*)name,
									   FALSE);
			self->getWindow()->incBusyCount();
		}

		LLString* j2c_name = new LLString(parcel_name.asString());
		gAssetStorage->storeAssetData(self->mTransactionID, LLAssetType::AT_TEXTURE,
								   &auction_j2c_upload_done,
								   (void*)j2c_name,
								   FALSE);
		self->getWindow()->incBusyCount();

		if(is_auction)
		{
			LLNotifyBox::showXml("UploadingAuctionSnapshot");
		}
		else
		{
			LLNotifyBox::showXml("UploadingSnapshot");
		}
	}
	LLMessageSystem* msg = gMessageSystem;
	if(is_auction)
	{
		msg->newMessage("ViewerStartAuction");
	}
	else
	{
		msg->newMessage("ParcelGodReserveForNewbie");
	}
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

void auction_tga_upload_done(const LLUUID& asset_id, void* user_data, S32 status) // StoreAssetData callback (fixed)
{
	LLString* name = (LLString*)(user_data);
	llinfos << "Upload of asset '" << *name << "' " << asset_id
			<< " returned " << status << llendl;
	delete name;

	gViewerWindow->getWindow()->decBusyCount();

	if (0 == status)
	{
		LLNotifyBox::showXml("UploadWebSnapshotDone");
	}
	else
	{
		LLStringBase<char>::format_map_t args;
		args["[REASON]"] = std::string(LLAssetStorage::getErrorString(status));
		gViewerWindow->alertXml("UploadAuctionSnapshotFail", args);
	}
}

void auction_j2c_upload_done(const LLUUID& asset_id, void* user_data, S32 status) // StoreAssetData callback (fixed)
{
	LLString* name = (LLString*)(user_data);
	llinfos << "Upload of asset '" << *name << "' " << asset_id
			<< " returned " << status << llendl;
	delete name;

	gViewerWindow->getWindow()->decBusyCount();

	if (0 == status)
	{
		LLNotifyBox::showXml("UploadSnapshotDone");
	}
	else
	{
		LLStringBase<char>::format_map_t args;
		args["[REASON]"] = std::string(LLAssetStorage::getErrorString(status));
		gViewerWindow->alertXml("UploadAuctionSnapshotFail", args);
	}
}
