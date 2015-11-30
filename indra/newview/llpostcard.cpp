/** 
 * @file llpostcard.cpp
 * @brief Sending postcards.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llpostcard.h"

#include "llvfile.h"
#include "llvfs.h"
#include "llviewerregion.h"

#include "message.h"

#include "llagent.h"
#include "llassetstorage.h"
#include "llassetuploadresponders.h"

///////////////////////////////////////////////////////////////////////////////
// misc

static void postcard_upload_callback(const LLUUID& asset_id, void *user_data, S32 result, LLExtStat ext_status)
{
	LLSD* postcard_data = (LLSD*)user_data;

	if (result)
	{
		// TODO: display the error messages in UI
		LL_WARNS() << "Failed to send postcard: " << LLAssetStorage::getErrorString(result) << LL_ENDL;
		LLPostCard::reportPostResult(false);
	}
	else
	{
		// only create the postcard once the upload succeeds

		// request the postcard
		const LLSD& data = *postcard_data;
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("SendPostcard");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID",			gAgent.getID());
		msg->addUUID("SessionID",		gAgent.getSessionID());
		msg->addUUID("AssetID",			data["asset-id"].asUUID());
		msg->addVector3d("PosGlobal",	LLVector3d(data["pos-global"]));
		msg->addString("To",			data["to"]);
		msg->addString("From",			data["from"]);
		msg->addString("Name",			data["name"]);
		msg->addString("Subject",		data["subject"]);
		msg->addString("Msg",			data["msg"]);
		msg->addBOOL("AllowPublish",	FALSE);
		msg->addBOOL("MaturePublish",	FALSE);
		gAgent.sendReliableMessage();

		LLPostCard::reportPostResult(true);
	}

	delete postcard_data;
}


///////////////////////////////////////////////////////////////////////////////
// LLPostcardSendResponder

class LLPostcardSendResponder : public LLAssetUploadResponder
{
	LOG_CLASS(LLPostcardSendResponder);

public:
	LLPostcardSendResponder(const LLSD &post_data,
							const LLUUID& vfile_id,
							LLAssetType::EType asset_type):
	    LLAssetUploadResponder(post_data, vfile_id, asset_type)
	{
	}

	/*virtual*/ void httpFailure()
	{
		LL_WARNS() << "Sending postcard failed, status: " << getStatus() << LL_ENDL;
		LLPostCard::reportPostResult(false);
	}

	/*virtual*/ void uploadComplete(const LLSD& content)
	{
		LL_INFOS() << "Postcard sent" << LL_ENDL;
		LL_DEBUGS("Snapshots") << "content: " << content << LL_ENDL;
		LLPostCard::reportPostResult(true);
	}

	/*virtual*/ void uploadFailure(const LLSD& content)
	{
		LL_WARNS() << "Sending postcard failed: " << content << LL_ENDL;
		LLPostCard::reportPostResult(false);
	}
};

///////////////////////////////////////////////////////////////////////////////
// LLPostCard

LLPostCard::result_callback_t LLPostCard::mResultCallback;

// static
void LLPostCard::send(LLPointer<LLImageFormatted> image, const LLSD& postcard_data)
{
	LLTransactionID transaction_id;
	LLAssetID asset_id;

	transaction_id.generate();
	asset_id = transaction_id.makeAssetID(gAgent.getSecureSessionID());
	LLVFile::writeFile(image->getData(), image->getDataSize(), gVFS, asset_id, LLAssetType::AT_IMAGE_JPEG);

	// upload the image
	std::string url = gAgent.getRegion()->getCapability("SendPostcard");
	if (!url.empty())
	{
		LL_INFOS() << "Sending postcard via capability" << LL_ENDL;
		// the capability already encodes: agent ID, region ID
		LL_DEBUGS("Snapshots") << "url: " << url << LL_ENDL;
		LL_DEBUGS("Snapshots") << "body: " << postcard_data << LL_ENDL;
		LL_DEBUGS("Snapshots") << "data size: " << image->getDataSize() << LL_ENDL;
		LLHTTPClient::post(url, postcard_data,
			new LLPostcardSendResponder(postcard_data, asset_id, LLAssetType::AT_IMAGE_JPEG));
	}
	else
	{
		LL_INFOS() << "Sending postcard" << LL_ENDL;
		LLSD* data = new LLSD(postcard_data);
		(*data)["asset-id"] = asset_id;
		gAssetStorage->storeAssetData(transaction_id, LLAssetType::AT_IMAGE_JPEG,
			&postcard_upload_callback, (void *)data, FALSE);
	}
}

// static
void LLPostCard::reportPostResult(bool ok)
{
	if (mResultCallback)
	{
		mResultCallback(ok);
	}
}
