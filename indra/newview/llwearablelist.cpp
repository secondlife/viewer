/** 
 * @file llwearablelist.cpp
 * @brief LLWearableList class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llwearablelist.h"

#include "message.h"
#include "llassetstorage.h"
#include "llagent.h"
#include "llvoavatar.h"
#include "llviewerinventory.h"
//#include "llfloaterchat.h"
#include "llviewerstats.h"
#include "llnotify.h"

// Globals
LLWearableList gWearableList;


struct LLWearableArrivedData
{
	LLWearableArrivedData( 
		LLAssetType::EType asset_type,
		const LLString& wearable_name,
		void(*asset_arrived_callback)(LLWearable*, void* userdata),
		void* userdata ) 
		:
		mAssetType( asset_type ),
		mCallback( asset_arrived_callback ), 
		mUserdata( userdata ),
		mName( wearable_name )
		{}

	LLAssetType::EType mAssetType;
	void	(*mCallback)(LLWearable*, void* userdata);
	void*	mUserdata;
	LLString mName;
};



////////////////////////////////////////////////////////////////////////////
// LLWearableList

LLWearableList::~LLWearableList()
{
	mList.deleteAllData();
}

void LLWearableList::getAsset( const LLAssetID& assetID, const LLString& wearable_name, LLAssetType::EType asset_type, void(*asset_arrived_callback)(LLWearable*, void* userdata), void* userdata )
{
	llassert( (asset_type == LLAssetType::AT_CLOTHING) || (asset_type == LLAssetType::AT_BODYPART) );
	LLWearable* instance = mList.getIfThere( assetID );
	if( instance )
	{
		asset_arrived_callback( instance, userdata );
	}
	else
	{
		gAssetStorage->getAssetData(
			assetID,
			asset_type,
			LLWearableList::processGetAssetReply,
			(void*)new LLWearableArrivedData( asset_type, wearable_name, asset_arrived_callback, userdata ),
			TRUE);
	}
}

// static
void LLWearableList::processGetAssetReply( const char* filename, const LLAssetID& uuid, void* userdata, S32 status )
{
	BOOL success = FALSE;
	LLWearableArrivedData* data = (LLWearableArrivedData*) userdata;

	if( !filename )
	{
		llinfos << "Bad Wearable Asset: missing file." << llendl;
	}
	else
	if( status >= 0 )
	{
		// read the file
		FILE* fp = LLFile::fopen(filename, "rb");
		if( !fp )
		{
			llinfos << "Bad Wearable Asset: unable to open file: '" << filename << "'" << llendl;
		}
		else
		{
			LLWearable *wearable = new LLWearable(uuid);
			if( wearable->importFile( fp ) )
			{
//				llinfos << "processGetAssetReply()" << llendl;
//				wearable->dump();

				gWearableList.mList[ uuid ] = wearable;
				if( data->mCallback )
				{
					data->mCallback( wearable, data->mUserdata );
				}
				success = TRUE;
			}
			else
			{
				LLString::format_map_t args;
				// XUI:translate
				args["[TYPE]"] = LLAssetType::lookupHumanReadable(data->mAssetType);
				if (data->mName.empty())
				{
					LLNotifyBox::showXml("FailedToLoadWearableUnnamed", args);
				}
				else
				{
					args["[DESC]"] = data->mName;
					LLNotifyBox::showXml("FailedToLoadWearable", args);
				}
				delete wearable;
			}

			fclose( fp );
			if(filename)
			{
				LLFile::remove(filename);
			}
		}
	}
	else
	{
		if(filename)
		{
			LLFile::remove(filename);
		}
		if( gViewerStats )
		{
			gViewerStats->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
		}

		llwarns << "Wearable download failed: " << LLAssetStorage::getErrorString( status ) << " " << uuid << llendl;
		switch( status )
		{
		  case LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE:
		  {
			  LLString::format_map_t args;
			  // XUI:translate
			  args["[TYPE]"] = LLAssetType::lookupHumanReadable(data->mAssetType);
			  if (data->mName.empty())
			  {
				  LLNotifyBox::showXml("FailedToFindWearableUnnamed", args);
			  }
			  else
			  {
				  args["[DESC]"] = data->mName;
				  LLNotifyBox::showXml("FailedToFindWearable", args);
			  }

			  // Asset does not exist in the database.
			  // Can't load asset, so return NULL
			  if( data->mCallback )
			  {
				  data->mCallback( NULL, data->mUserdata );
			  }
			  break;
		  }
		  default:
		  {
			  // Try again
			  gAssetStorage->getAssetData(uuid,
										  data->mAssetType,
										  LLWearableList::processGetAssetReply,
										  userdata);  // re-use instead of deleting.
			  return;
		  }
		}
	}

	delete data;
}


LLWearable* LLWearableList::createLegacyWearableFromAvatar( EWearableType type )
{
	llinfos << "LLWearableList::createLegacyWearableFromAvatar" << llendl;
	
	LLTransactionID tid;
	LLAssetID new_asset_id;
	tid.generate();
	new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	
	LLWearable* wearable = new LLWearable( tid );
	wearable->setType( type );
	wearable->setName( wearable->getTypeLabel() );
	wearable->setDescription( "Recovered from lost asset." );

	LLVOAvatar* avatar = gAgent.getAvatarObject();
	LLPermissions perm;
	perm.init( avatar->getID(), avatar->getID(), LLUUID::null, LLUUID::null );
	perm.initMasks(PERM_TRANSFER, PERM_TRANSFER, PERM_NONE, PERM_NONE, PERM_MOVE | PERM_TRANSFER);
	wearable->setPermissions( perm );
	
	// Save info is the default.

	wearable->readFromAvatar();
	
	mList[ new_asset_id ] = wearable;

	// Send to the dataserver
	wearable->saveNewAsset();

	return wearable;
}


// Creates a new wearable just like the old_wearable but with data copied over from item
LLWearable* LLWearableList::createWearableMatchedToInventoryItem( LLWearable* old_wearable, LLViewerInventoryItem* item )
{
	lldebugs << "LLWearableList::createWearableMatchedToInventoryItem()" << llendl;

	LLTransactionID tid;
	LLAssetID new_asset_id;
	new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());

	LLWearable* wearable = new LLWearable( tid );
	wearable->copyDataFrom( old_wearable );

	wearable->setName( item->getName() );
	wearable->setDescription( item->getDescription() );
	wearable->setPermissions( item->getPermissions() );
	wearable->setSaleInfo( item->getSaleInfo() );

	mList[ new_asset_id ] = wearable;

	// Send to the dataserver
	wearable->saveNewAsset();

	return wearable;
}

LLWearable* LLWearableList::createCopyFromAvatar( LLWearable* old_wearable, const std::string& new_name )
{
	lldebugs << "LLWearableList::createCopyFromAvatar()" << llendl;

	LLTransactionID tid;
	LLAssetID new_asset_id;
	tid.generate();
	new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	
	LLWearable* wearable = new LLWearable( tid );
	wearable->copyDataFrom( old_wearable );
	LLPermissions perm(old_wearable->getPermissions());
	perm.setOwnerAndGroup(LLUUID::null, gAgent.getID(), LLUUID::null, true);
	wearable->setPermissions(perm);
	wearable->readFromAvatar();  // update from the avatar
   
	if (!new_name.empty()) wearable->setName(new_name);

	mList[ new_asset_id ] = wearable;

	// Send to the dataserver
	wearable->saveNewAsset();

	return wearable;
}


LLWearable* LLWearableList::createCopy( LLWearable* old_wearable )
{
	lldebugs << "LLWearableList::createCopy()" << llendl;

	LLTransactionID tid;
	LLAssetID new_asset_id;
	tid.generate();
	new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	
	LLWearable* wearable = new LLWearable( tid );
	wearable->copyDataFrom( old_wearable );
	LLPermissions perm(old_wearable->getPermissions());
	perm.setOwnerAndGroup(LLUUID::null, gAgent.getID(), LLUUID::null, true);
	wearable->setPermissions(perm);
	mList[ new_asset_id ] = wearable;

	// Send to the dataserver
	wearable->saveNewAsset();

	return wearable;
}

LLWearable* LLWearableList::createNewWearable( EWearableType type )
{
	lldebugs << "LLWearableList::createNewWearable()" << llendl;

	LLTransactionID tid;
	LLAssetID new_asset_id;
	tid.generate();
	new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	
	LLWearable* wearable = new LLWearable( tid );
	wearable->setType( type );
	
	LLString name = "New ";
	name.append( wearable->getTypeLabel() );
	wearable->setName( name );

	LLPermissions perm;
	perm.init(gAgent.getID(), gAgent.getID(), LLUUID::null, LLUUID::null);
	perm.initMasks(PERM_ALL, PERM_ALL, PERM_NONE, PERM_NONE, PERM_MOVE | PERM_TRANSFER);
	wearable->setPermissions(perm);

	// Description and sale info have default values.
	
	wearable->setParamsToDefaults();
	wearable->setTexturesToDefaults();

	mList[ new_asset_id ] = wearable;

	// Send to the dataserver
	wearable->saveNewAsset();

	return wearable;
}
