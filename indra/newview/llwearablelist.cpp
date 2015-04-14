/** 
 * @file llwearablelist.cpp
 * @brief LLWearableList class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llwearablelist.h"

#include "message.h"
#include "llassetstorage.h"
#include "llagent.h"
#include "llvoavatar.h"
#include "llviewerstats.h"
#include "llnotificationsutil.h"
#include "llinventorymodel.h"
#include "lltrans.h"

// Callback struct
struct LLWearableArrivedData
{
	LLWearableArrivedData(LLAssetType::EType asset_type,
		const std::string& wearable_name,
		LLAvatarAppearance* avatarp,
		void(*asset_arrived_callback)(LLViewerWearable*, void* userdata),
						  void* userdata) :
		mAssetType( asset_type ),
		mCallback( asset_arrived_callback ), 
		mUserdata( userdata ),
		mName( wearable_name ),
		mRetries(0),
		mAvatarp(avatarp)
		{}

	LLAssetType::EType mAssetType;
	void	(*mCallback)(LLViewerWearable*, void* userdata);
	void*	mUserdata;
	std::string mName;
	S32	mRetries;
	LLAvatarAppearance *mAvatarp;
};

////////////////////////////////////////////////////////////////////////////
// LLWearableList

LLWearableList::~LLWearableList()
{
	cleanup();
}

void LLWearableList::cleanup() 
{
	for_each(mList.begin(), mList.end(), DeletePairedPointer());
	mList.clear();
}

void LLWearableList::getAsset(const LLAssetID& assetID, const std::string& wearable_name, LLAvatarAppearance* avatarp, LLAssetType::EType asset_type, void(*asset_arrived_callback)(LLViewerWearable*, void* userdata), void* userdata)
{
	llassert( (asset_type == LLAssetType::AT_CLOTHING) || (asset_type == LLAssetType::AT_BODYPART) );
	LLViewerWearable* instance = get_if_there(mList, assetID, (LLViewerWearable*)NULL );
	if( instance )
	{
		LL_DEBUGS("Avatar") << "wearable " << assetID << " found in LLWearableList" << LL_ENDL;
		asset_arrived_callback( instance, userdata );
	}
	else
	{
		gAssetStorage->getAssetData(assetID,
			asset_type,
			LLWearableList::processGetAssetReply,
			(void*)new LLWearableArrivedData( asset_type, wearable_name, avatarp, asset_arrived_callback, userdata ),
			TRUE);
	}
}

// static
void LLWearableList::processGetAssetReply( const char* filename, const LLAssetID& uuid, void* userdata, S32 status, LLExtStat ext_status )
{
	BOOL isNewWearable = FALSE;
	LLWearableArrivedData* data = (LLWearableArrivedData*) userdata;
	LLViewerWearable* wearable = NULL; // NULL indicates failure
	LLAvatarAppearance *avatarp = data->mAvatarp;
	
	if( !filename )
	{
		LL_WARNS("Wearable") << "Bad Wearable Asset: missing file." << LL_ENDL;
	}
	else if(!avatarp)
	{
		LL_WARNS("Wearable") << "Bad asset request: missing avatar pointer." << LL_ENDL;
	}
	else if (status >= 0)
	{
		// read the file
		llifstream ifs(filename, llifstream::binary);
		if( !ifs.is_open() )
		{
			LL_WARNS("Wearable") << "Bad Wearable Asset: unable to open file: '" << filename << "'" << LL_ENDL;
		}
		else
		{
			wearable = new LLViewerWearable(uuid);
			LLWearable::EImportResult result = wearable->importStream(
												ifs, avatarp );
			if (LLWearable::SUCCESS != result)
			{
				if (wearable->getType() == LLWearableType::WT_COUNT)
				{
					isNewWearable = TRUE;
				}
				delete wearable;
				wearable = NULL;
			}

			if(filename)
			{
				if (ifs.is_open())
				{
					ifs.close();
				}
				LLFile::remove(std::string(filename));
			}
		}
	}
	else
	{
		if(filename)
		{
			LLFile::remove(std::string(filename));
		}

		LL_WARNS("Wearable") << "Wearable download failed: " << LLAssetStorage::getErrorString( status ) << " " << uuid << LL_ENDL;
		switch( status )
		{
		  case LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE:
		  {
			  // Fail
			  break;
		}
		  default:
		{
			  static const S32 MAX_RETRIES = 3;
			  if (data->mRetries < MAX_RETRIES)
			  {
			  // Try again
				  data->mRetries++;
			  gAssetStorage->getAssetData(uuid,
										  data->mAssetType,
										  LLWearableList::processGetAssetReply,
										  userdata);  // re-use instead of deleting.
			  return;
		}
			  else
			  {
				  // Fail
				  break;
			  }
		  }
	}
	}

	if (wearable) // success
	{
		LLWearableList::instance().mList[ uuid ] = wearable;
		LL_DEBUGS("Wearable") << "processGetAssetReply()" << LL_ENDL;
		LL_DEBUGS("Wearable") << wearable << LL_ENDL;
	}
	else
	{
		LLSD args;
		args["TYPE"] =LLTrans::getString(LLAssetType::lookupHumanReadable(data->mAssetType));
		if (isNewWearable)
		{
			LLNotificationsUtil::add("InvalidWearable");
		}
		else if (data->mName.empty())
		{
			LLNotificationsUtil::add("FailedToFindWearableUnnamed", args);
		}
		else
		{
			args["DESC"] = data->mName;
			LLNotificationsUtil::add("FailedToFindWearable", args);
		}
	}
	// Always call callback; wearable will be NULL if we failed
	{
		if( data->mCallback )
		{
			data->mCallback( wearable, data->mUserdata );
		}
	}
	delete data;
}


LLViewerWearable* LLWearableList::createCopy(const LLViewerWearable* old_wearable, const std::string& new_name)
{
	LL_DEBUGS() << "LLWearableList::createCopy()" << LL_ENDL;

	LLViewerWearable *wearable = generateNewWearable();
	wearable->copyDataFrom(old_wearable);

	LLPermissions perm(old_wearable->getPermissions());
	perm.setOwnerAndGroup(LLUUID::null, gAgent.getID(), LLUUID::null, true);
	wearable->setPermissions(perm);

	if (!new_name.empty()) wearable->setName(new_name);

	// Send to the dataserver
	wearable->saveNewAsset();

	return wearable;
}

LLViewerWearable* LLWearableList::createNewWearable( LLWearableType::EType type, LLAvatarAppearance *avatarp )
{
	LL_DEBUGS() << "LLWearableList::createNewWearable()" << LL_ENDL;

	LLViewerWearable *wearable = generateNewWearable();
	wearable->setType( type, avatarp );
	
	std::string name = LLTrans::getString( LLWearableType::getTypeDefaultNewName(wearable->getType()) );
	wearable->setName( name );

	LLPermissions perm;
	perm.init(gAgent.getID(), gAgent.getID(), LLUUID::null, LLUUID::null);
	perm.initMasks(PERM_ALL, PERM_ALL, PERM_NONE, PERM_NONE, PERM_MOVE | PERM_TRANSFER);
	wearable->setPermissions(perm);

	wearable->setDefinitionVersion(LLWearable::getCurrentDefinitionVersion());

	// Description and sale info have default values.
	wearable->setParamsToDefaults();
	wearable->setTexturesToDefaults();

	//mark all values (params & images) as saved
	wearable->saveValues();

	// Send to the dataserver
	wearable->saveNewAsset();


	return wearable;
}

LLViewerWearable *LLWearableList::generateNewWearable()
{
	LLTransactionID tid;
	tid.generate();
	LLAssetID new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());

	LLViewerWearable* wearable = new LLViewerWearable(tid);
	mList[new_asset_id] = wearable;
	return wearable;
}
