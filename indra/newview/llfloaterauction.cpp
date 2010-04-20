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
#include "llfloaterregioninfo.h"

#include "llgl.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "llparcel.h"
#include "llvfile.h"
#include "llvfs.h"
#include "llwindow.h"
#include "message.h"

#include "llagent.h"
#include "llcombobox.h"
#include "llmimetypes.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llsavedsettingsglue.h"
#include "llviewertexturelist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llviewerdisplay.h"
#include "llviewercontrol.h"
#include "llui.h"
#include "llrender.h"
#include "llsdutil.h"
#include "llsdutil_math.h"

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

// Default constructor
LLFloaterAuction::LLFloaterAuction(const LLSD& key)
  : LLFloater(key),
	mParcelID(-1)
{
//	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_auction.xml");
	mCommitCallbackRegistrar.add("ClickSnapshot",	boost::bind(&LLFloaterAuction::onClickSnapshot, this));
	mCommitCallbackRegistrar.add("ClickSellToAnyone",		boost::bind(&LLFloaterAuction::onClickSellToAnyone, this));
	mCommitCallbackRegistrar.add("ClickStartAuction",		boost::bind(&LLFloaterAuction::onClickStartAuction, this));
	mCommitCallbackRegistrar.add("ClickResetParcel",		boost::bind(&LLFloaterAuction::onClickResetParcel, this));
}

// Destroys the object
LLFloaterAuction::~LLFloaterAuction()
{
}

BOOL LLFloaterAuction::postBuild()
{
	return TRUE;
}

void LLFloaterAuction::onOpen(const LLSD& key)
{
	initialize();
}

void LLFloaterAuction::initialize()
{
	mParcelUpdateCapUrl.clear();

	mParcelp = LLViewerParcelMgr::getInstance()->getParcelSelection();
	LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();
	LLParcel* parcelp = mParcelp->getParcel();
	if(parcelp && region && !parcelp->getForSale())
	{
		mParcelHost = region->getHost();
		mParcelID = parcelp->getLocalID();
		mParcelUpdateCapUrl = region->getCapability("ParcelPropertiesUpdate");

		childSetText("parcel_text", parcelp->getName());
		childEnable("snapshot_btn");
		childEnable("reset_parcel_btn");
		childEnable("start_auction_btn");

		LLPanelEstateInfo* panel = LLFloaterRegionInfo::getPanelEstate();
		if (panel)
		{	// Only enable "Sell to Anyone" on Teen grid or if we don't know the ID yet
			U32 estate_id = panel->getEstateID();
			childSetEnabled("sell_to_anyone_btn", (estate_id == ESTATE_TEEN || estate_id == 0));
		}
		else
		{	// Don't have the panel up, so don't know if we're on the teen grid or not.  Default to enabling it
			childEnable("sell_to_anyone_btn");
		}
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
		childSetEnabled("reset_parcel_btn", false);
		childSetEnabled("sell_to_anyone_btn", false);
		childSetEnabled("start_auction_btn", false);
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
											  gViewerWindow->getWindowWidthScaled(),
											  gViewerWindow->getWindowHeightScaled(),
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
		
		raw->biasedScaleToPowerOfTwo(LLViewerTexture::MAX_IMAGE_SIZE_DEFAULT);

		llinfos << "Writing J2C..." << llendl;

		LLPointer<LLImageJ2C> j2c = new LLImageJ2C;
		j2c->encode(raw, 0.0f);
		LLVFile::writeFile(j2c->getData(), j2c->getDataSize(), gVFS, self->mImageID, LLAssetType::AT_TEXTURE);

		self->mImage = LLViewerTextureManager::getLocalTexture((LLImageRaw*)raw, FALSE);
		gGL.getTexUnit(0)->bind(self->mImage);
		self->mImage->setAddressMode(LLTexUnit::TAM_CLAMP);
	}
	else
	{
		llwarns << "Unable to take snapshot" << llendl;
	}
}

// static
void LLFloaterAuction::onClickStartAuction(void* data)
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

		LLNotificationsUtil::add("UploadingAuctionSnapshot");

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
	self->cleanupAndClose();
}


void LLFloaterAuction::cleanupAndClose()
{
	mImageID.setNull();
	mImage = NULL;
	mParcelID = -1;
	mParcelHost.invalidate();
	closeFloater();
}



// static glue
void LLFloaterAuction::onClickResetParcel(void* data)
{
	LLFloaterAuction* self = (LLFloaterAuction*)(data);
	if (self)
	{
		self->doResetParcel();
	}
}


// Reset all the values for the parcel in preparation for a sale
void LLFloaterAuction::doResetParcel()
{
	LLParcel* parcelp = mParcelp->getParcel();
	LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();

	if (parcelp
		&& region
		&& !mParcelUpdateCapUrl.empty())
	{
		LLSD body;
		std::string empty;

		// request new properties update from simulator
		U32 message_flags = 0x01;
		body["flags"] = ll_sd_from_U32(message_flags);

		// Set all the default parcel properties for auction
		body["local_id"] = parcelp->getLocalID();

		U32 parcel_flags = PF_ALLOW_LANDMARK |
						   PF_ALLOW_FLY	|
						   PF_CREATE_GROUP_OBJECTS |
						   PF_ALLOW_ALL_OBJECT_ENTRY |
						   PF_ALLOW_GROUP_OBJECT_ENTRY |
						   PF_ALLOW_GROUP_SCRIPTS |
						   PF_RESTRICT_PUSHOBJECT |
						   PF_SOUND_LOCAL |
						   PF_ALLOW_VOICE_CHAT |
						   PF_USE_ESTATE_VOICE_CHAN;

		body["parcel_flags"] = ll_sd_from_U32(parcel_flags);
		
		// Build a parcel name like "Ahern (128,128) PG 4032m"
		std::ostringstream parcel_name;
		LLVector3 center_point( parcelp->getCenterpoint() );
		center_point.snap(0);		// Get rid of fractions
		parcel_name << region->getName() 
					<< " ("
					<< (S32) center_point.mV[VX]
					<< ","
					<< (S32) center_point.mV[VY]						
					<< ") "
					<< region->getSimAccessString()
					<< " "
					<< parcelp->getArea()
					<< "m";

		std::string new_name(parcel_name.str().c_str());
		body["name"] = new_name;
		childSetText("parcel_text", new_name);	// Set name in dialog as well, since it won't get updated otherwise

		body["sale_price"] = (S32) 0;
		body["description"] = empty;
		body["music_url"] = empty;
		body["media_url"] = empty;
		body["media_desc"] = empty;
		body["media_type"] = LLMIMETypes::getDefaultMimeType();
		body["media_width"] = (S32) 0;
		body["media_height"] = (S32) 0;
		body["auto_scale"] = (S32) 0;
		body["media_loop"] = (S32) 0;
		body["obscure_media"] = (S32) 0;
		body["obscure_music"] = (S32) 0;
		body["media_id"] = LLUUID::null;
		body["group_id"] = MAINTENANCE_GROUP_ID;	// Use maintenance group
		body["pass_price"] = (S32) 10;		// Defaults to $10
		body["pass_hours"] = 0.0f;
		body["category"] = (U8) LLParcel::C_NONE;
		body["auth_buyer_id"] = LLUUID::null;
		body["snapshot_id"] = LLUUID::null;
		body["user_location"] = ll_sd_from_vector3( LLVector3::zero );
		body["user_look_at"] = ll_sd_from_vector3( LLVector3::zero );
		body["landing_type"] = (U8) LLParcel::L_DIRECT;

		llinfos << "Sending parcel update to reset for auction via capability to: "
			<< mParcelUpdateCapUrl << llendl;
		LLHTTPClient::post(mParcelUpdateCapUrl, body, new LLHTTPClient::Responder());

		// Send a message to clear the object return time
		LLMessageSystem *msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_ParcelSetOtherCleanTime);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ParcelData);
		msg->addS32Fast(_PREHASH_LocalID, parcelp->getLocalID());
		msg->addS32Fast(_PREHASH_OtherCleanTime, 5);			// 5 minute object auto-return

		msg->sendReliable(region->getHost());

		// Clear the access lists
		clearParcelAccessLists(parcelp, region);
	}
}



void LLFloaterAuction::clearParcelAccessLists(LLParcel* parcel, LLViewerRegion* region)
{
	if (!region || !parcel) return;

	LLUUID transactionUUID;
	transactionUUID.generate();

	LLMessageSystem* msg = gMessageSystem;

	// Clear access list
	//	parcel->mAccessList.clear();

	msg->newMessageFast(_PREHASH_ParcelAccessListUpdate);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->nextBlockFast(_PREHASH_Data);
	msg->addU32Fast(_PREHASH_Flags, AL_ACCESS);
	msg->addS32(_PREHASH_LocalID, parcel->getLocalID() );
	msg->addUUIDFast(_PREHASH_TransactionID, transactionUUID);
	msg->addS32Fast(_PREHASH_SequenceID, 1);			// sequence_id
	msg->addS32Fast(_PREHASH_Sections, 0);				// num_sections

	// pack an empty block since there will be no data
	msg->nextBlockFast(_PREHASH_List);
	msg->addUUIDFast(_PREHASH_ID,  LLUUID::null );
	msg->addS32Fast(_PREHASH_Time, 0 );
	msg->addU32Fast(_PREHASH_Flags,	0 );

	msg->sendReliable( region->getHost() );

	// Send message for empty ban list
	//parcel->mBanList.clear();
	msg->newMessageFast(_PREHASH_ParcelAccessListUpdate);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->nextBlockFast(_PREHASH_Data);
	msg->addU32Fast(_PREHASH_Flags, AL_BAN);
	msg->addS32(_PREHASH_LocalID, parcel->getLocalID() );
	msg->addUUIDFast(_PREHASH_TransactionID, transactionUUID);
	msg->addS32Fast(_PREHASH_SequenceID, 1);		// sequence_id
	msg->addS32Fast(_PREHASH_Sections, 0);			// num_sections

	// pack an empty block since there will be no data
	msg->nextBlockFast(_PREHASH_List);
	msg->addUUIDFast(_PREHASH_ID,  LLUUID::null );
	msg->addS32Fast(_PREHASH_Time, 0 );
	msg->addU32Fast(_PREHASH_Flags,	0 );

	msg->sendReliable( region->getHost() );
}



// static - 'Sell to Anyone' clicked, throw up a confirmation dialog
void LLFloaterAuction::onClickSellToAnyone(void* data)
{
	LLFloaterAuction* self = (LLFloaterAuction*)(data);
	if (self)
	{
		LLParcel* parcelp = self->mParcelp->getParcel();

		// Do a confirmation
		S32 sale_price = parcelp->getArea();	// Selling for L$1 per meter
		S32 area = parcelp->getArea();

		LLSD args;
		args["LAND_SIZE"] = llformat("%d", area);
		args["SALE_PRICE"] = llformat("%d", sale_price);
		args["NAME"] = "Anyone";

		LLNotification::Params params("ConfirmLandSaleChange");	// Re-use existing dialog
		params.substitutions(args)
			.functor.function(boost::bind(&LLFloaterAuction::onSellToAnyoneConfirmed, self, _1, _2));

		params.name("ConfirmLandSaleToAnyoneChange");
		
		// ask away
		LLNotifications::instance().add(params);
	}
}


// Sell confirmation clicked
bool LLFloaterAuction::onSellToAnyoneConfirmed(const LLSD& notification, const LLSD& response)	
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		doSellToAnyone();
	}

	return false;
}



// Reset all the values for the parcel in preparation for a sale
void LLFloaterAuction::doSellToAnyone()
{
	LLParcel* parcelp = mParcelp->getParcel();
	LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();

	if (parcelp
		&& region
		&& !mParcelUpdateCapUrl.empty())
	{
		LLSD body;
		std::string empty;

		// request new properties update from simulator
		U32 message_flags = 0x01;
		body["flags"] = ll_sd_from_U32(message_flags);

		// Set all the default parcel properties for auction
		body["local_id"] = parcelp->getLocalID();

		// Set 'for sale' flag
		U32 parcel_flags = parcelp->getParcelFlags() | PF_FOR_SALE;
		// Ensure objects not included
		parcel_flags &= ~PF_FOR_SALE_OBJECTS;
		body["parcel_flags"] = ll_sd_from_U32(parcel_flags);
		
		body["sale_price"] = parcelp->getArea();	// Sell for L$1 per square meter
		body["auth_buyer_id"] = LLUUID::null;		// To anyone

		llinfos << "Sending parcel update to sell to anyone for L$1 via capability to: "
			<< mParcelUpdateCapUrl << llendl;
		LLHTTPClient::post(mParcelUpdateCapUrl, body, new LLHTTPClient::Responder());

		// clean up floater, and get out
		cleanupAndClose();
	}
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
		LLNotificationsUtil::add("UploadWebSnapshotDone");
	}
	else
	{
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotificationsUtil::add("UploadAuctionSnapshotFail", args);
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
		LLNotificationsUtil::add("UploadSnapshotDone");
	}
	else
	{
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotificationsUtil::add("UploadAuctionSnapshotFail", args);
	}
}
