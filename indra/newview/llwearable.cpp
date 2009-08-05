/** 
 * @file llwearable.cpp
 * @brief LLWearable class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llagent.h"
#include "llagentwearables.h"
#include "llfloatercustomize.h"
#include "llviewertexturelist.h"
#include "llinventorymodel.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llvoavatardefines.h"
#include "llwearable.h"
#include "lldictionary.h"
#include "lltrans.h"

using namespace LLVOAvatarDefines;

// static
S32 LLWearable::sCurrentDefinitionVersion = 1;

// Private local functions
static std::string terse_F32_to_string(F32 f);
static std::string asset_id_to_filename(const LLUUID &asset_id);

LLWearable::LLWearable(const LLTransactionID& transaction_id) :
	mDefinitionVersion(LLWearable::sCurrentDefinitionVersion),
	mType(WT_SHAPE)
{
	mTransactionID = transaction_id;
	mAssetID = mTransactionID.makeAssetID(gAgent.getSecureSessionID());
}

LLWearable::LLWearable(const LLAssetID& asset_id) :
	mDefinitionVersion( LLWearable::sCurrentDefinitionVersion ),
	mType(WT_SHAPE)
{
	mAssetID = asset_id;
	mTransactionID.setNull();
}

LLWearable::~LLWearable()
{
}

const std::string& LLWearable::getTypeLabel() const
{
	return LLWearableDictionary::getTypeLabel(mType);
}

const std::string& LLWearable::getTypeName() const
{
	return LLWearableDictionary::getTypeName(mType);
}

LLAssetType::EType LLWearable::getAssetType() const
{
	return LLWearableDictionary::getAssetType(mType);
}

BOOL LLWearable::exportFile(LLFILE* file) const
{
	// header and version
	if( fprintf( file, "LLWearable version %d\n", mDefinitionVersion ) < 0 )
	{
		return FALSE;
	}

	// name
	if( fprintf( file, "%s\n", mName.c_str() ) < 0 )
	{
		return FALSE;
	}

	// description
	if( fprintf( file, "%s\n", mDescription.c_str() ) < 0 )
	{
		return FALSE;
	}
	
	// permissions
	if( !mPermissions.exportFile( file ) )
	{
		return FALSE;
	}

	// sale info
	if( !mSaleInfo.exportFile( file ) )
	{
		return FALSE;
	}

	// wearable type
	S32 type = (S32)mType;
	if( fprintf( file, "type %d\n", type ) < 0 )
	{
		return FALSE;
	}

	// parameters
	S32 num_parameters = mVisualParamMap.size();
	if( fprintf( file, "parameters %d\n", num_parameters ) < 0 )
	{
		return FALSE;
	}

	for (param_map_t::const_iterator iter = mVisualParamMap.begin();
		 iter != mVisualParamMap.end(); ++iter)
	{
		S32 param_id = iter->first;
		F32 param_weight = iter->second;
		if( fprintf( file, "%d %s\n", param_id, terse_F32_to_string( param_weight ).c_str() ) < 0 )
		{
			return FALSE;
		}
	}

	// texture entries
	S32 num_textures = mTEMap.size();
	if( fprintf( file, "textures %d\n", num_textures ) < 0 )
	{
		return FALSE;
	}
	
	for (te_map_t::const_iterator iter = mTEMap.begin(); iter != mTEMap.end(); ++iter)
	{
		S32 te = iter->first;
		const LLUUID& image_id = iter->second.getID();
		if( fprintf( file, "%d %s\n", te, image_id.asString().c_str()) < 0 )
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL LLWearable::importFile( LLFILE* file )
{
	// *NOTE: changing the type or size of this buffer will require
	// changes in the fscanf() code below. You would be better off
	// rewriting this to use streams and not require an open FILE.
	char text_buffer[2048];		/* Flawfinder: ignore */
	S32 fields_read = 0;

	// read header and version 
	fields_read = fscanf( file, "LLWearable version %d\n", &mDefinitionVersion );
	if( fields_read != 1 )
	{
		// Shouldn't really log the asset id for security reasons, but
		// we need it in this case.
		llwarns << "Bad Wearable asset header: " << mAssetID << llendl;
		//gVFS->dumpMap();
		return FALSE;
	}

	if( mDefinitionVersion > LLWearable::sCurrentDefinitionVersion )
	{
		llwarns << "Wearable asset has newer version (" << mDefinitionVersion << ") than XML (" << LLWearable::sCurrentDefinitionVersion << ")" << llendl;
		return FALSE;
	}

	// name
	int next_char = fgetc( file );		/* Flawfinder: ignore */
	if( '\n' == next_char )
	{
		// no name
		mName = "";
	}
	else
	{
		ungetc( next_char, file );
		fields_read = fscanf(	/* Flawfinder: ignore */
			file,
			"%2047[^\n]",
			text_buffer);
		if( (1 != fields_read) || (fgetc( file ) != '\n') )		/* Flawfinder: ignore */
		{
			llwarns << "Bad Wearable asset: early end of file" << llendl;
			return FALSE;
		}
		mName = text_buffer;
		LLStringUtil::truncate(mName, DB_INV_ITEM_NAME_STR_LEN );
	}

	// description
	next_char = fgetc( file );		/* Flawfinder: ignore */
	if( '\n' == next_char )
	{
		// no description
		mDescription = "";
	}
	else
	{
		ungetc( next_char, file );
		fields_read = fscanf(	/* Flawfinder: ignore */
			file,
			"%2047[^\n]",
			text_buffer );
		if( (1 != fields_read) || (fgetc( file ) != '\n') )		/* Flawfinder: ignore */
		{
			llwarns << "Bad Wearable asset: early end of file" << llendl;
			return FALSE;
		}
		mDescription = text_buffer;
		LLStringUtil::truncate(mDescription, DB_INV_ITEM_DESC_STR_LEN );
	}

	// permissions
	S32 perm_version;
	fields_read = fscanf( file, " permissions %d\n", &perm_version );
	if( (fields_read != 1) || (perm_version != 0) )
	{
		llwarns << "Bad Wearable asset: missing permissions" << llendl;
		return FALSE;
	}
	if( !mPermissions.importFile( file ) )
	{
		return FALSE;
	}

	// sale info
	S32 sale_info_version;
	fields_read = fscanf( file, " sale_info %d\n", &sale_info_version );
	if( (fields_read != 1) || (sale_info_version != 0) )
	{
		llwarns << "Bad Wearable asset: missing sale_info" << llendl;
		return FALSE;
	}
	// Sale info used to contain next owner perm. It is now in the
	// permissions. Thus, we read that out, and fix legacy
	// objects. It's possible this op would fail, but it should pick
	// up the vast majority of the tasks.
	BOOL has_perm_mask = FALSE;
	U32 perm_mask = 0;
	if( !mSaleInfo.importFile(file, has_perm_mask, perm_mask) )
	{
		return FALSE;
	}
	if(has_perm_mask)
	{
		// fair use fix.
		if(!(perm_mask & PERM_COPY))
		{
			perm_mask |= PERM_TRANSFER;
		}
		mPermissions.setMaskNext(perm_mask);
	}

	// wearable type
	S32 type = -1;
	fields_read = fscanf( file, "type %d\n", &type );
	if( fields_read != 1 )
	{
		llwarns << "Bad Wearable asset: bad type" << llendl;
		return FALSE;
	}
	if( 0 <= type && type < WT_COUNT )
	{
		mType = (EWearableType)type;
	}
	else
	{
		mType = WT_COUNT;
		llwarns << "Bad Wearable asset: bad type #" << type <<  llendl;
		return FALSE;
	}


	// parameters header
	S32 num_parameters = 0;
	fields_read = fscanf( file, "parameters %d\n", &num_parameters );
	if( fields_read != 1 )
	{
		llwarns << "Bad Wearable asset: missing parameters block" << llendl;
		return FALSE;
	}

	// parameters
	S32 i;
	for( i = 0; i < num_parameters; i++ )
	{
		S32 param_id = 0;
		F32 param_weight = 0.f;
		fields_read = fscanf( file, "%d %f\n", &param_id, &param_weight );
		if( fields_read != 2 )
		{
			llwarns << "Bad Wearable asset: bad parameter, #" << i << llendl;
			return FALSE;
		}
		mVisualParamMap[param_id] = param_weight;
	}

	// textures header
	S32 num_textures = 0;
	fields_read = fscanf( file, "textures %d\n", &num_textures);
	if( fields_read != 1 )
	{
		llwarns << "Bad Wearable asset: missing textures block" << llendl;
		return FALSE;
	}

	// textures
	for( i = 0; i < num_textures; i++ )
	{
		S32 te = 0;
		fields_read = fscanf(	/* Flawfinder: ignore */
			file,
			"%d %2047s\n",
			&te, text_buffer);
		if( fields_read != 2 )
		{
			llwarns << "Bad Wearable asset: bad texture, #" << i << llendl;
			return FALSE;
		}

		if( !LLUUID::validate( text_buffer ) )
		{
			llwarns << "Bad Wearable asset: bad texture uuid: " << text_buffer << llendl;
			return FALSE;
		}

		//TODO: check old values
		mTEMap[te] = LLLocalTextureObject(NULL, NULL, NULL, LLUUID(text_buffer));
	}

	return TRUE;
}


// Avatar parameter and texture definitions can change over time.
// This function returns true if parameters or textures have been added or removed
// since this wearable was created.
BOOL LLWearable::isOldVersion() const
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	llassert( avatar );
	if( !avatar )
	{
		return FALSE;
	}

	if( LLWearable::sCurrentDefinitionVersion < mDefinitionVersion )
	{
		llwarns << "Wearable asset has newer version (" << mDefinitionVersion << ") than XML (" << LLWearable::sCurrentDefinitionVersion << ")" << llendl;
		llassert(0);
	}

	if( LLWearable::sCurrentDefinitionVersion != mDefinitionVersion )
	{
		return TRUE;
	}

	S32 param_count = 0;
	for( LLViewerVisualParam* param = (LLViewerVisualParam*) avatar->getFirstVisualParam(); 
		param;
		param = (LLViewerVisualParam*) avatar->getNextVisualParam() )
	{
		if( (param->getWearableType() == mType) && (param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE ) )
		{
			param_count++;
			if( !is_in_map(mVisualParamMap, param->getID() ) )
			{
				return TRUE;
			}
		}
	}
	if( param_count != mVisualParamMap.size() )
	{
		return TRUE;
	}


	S32 te_count = 0;
	for( S32 te = 0; te < TEX_NUM_INDICES; te++ )
	{
		if (LLVOAvatarDictionary::getTEWearableType((ETextureIndex) te) == mType)
		{
			te_count++;
			if( !is_in_map(mTEMap, te ) )
			{
				return TRUE;
			}
		}
	}
	if( te_count != mTEMap.size() )
	{
		return TRUE;
	}

	return FALSE;
}

// Avatar parameter and texture definitions can change over time.
// * If parameters or textures have been REMOVED since the wearable was created,
// they're just ignored, so we consider the wearable clean even though isOldVersion()
// will return true. 
// * If parameters or textures have been ADDED since the wearable was created,
// they are taken to have default values, so we consider the wearable clean
// only if those values are the same as the defaults.
BOOL LLWearable::isDirty() const
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	llassert( avatar );
	if( !avatar )
	{
		return FALSE;
	}


	for( LLViewerVisualParam* param = (LLViewerVisualParam*) avatar->getFirstVisualParam(); 
		param;
		param = (LLViewerVisualParam*) avatar->getNextVisualParam() )
	{
		if( (param->getWearableType() == mType) && (param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE ) )
		{
			F32 weight = get_if_there(mVisualParamMap, param->getID(), param->getDefaultWeight());
			weight = llclamp( weight, param->getMinWeight(), param->getMaxWeight() );
			
			U8 a = F32_to_U8( param->getWeight(), param->getMinWeight(), param->getMaxWeight() );
			U8 b = F32_to_U8( weight,             param->getMinWeight(), param->getMaxWeight() );
			if( a != b  )
			{
				return TRUE;
			}
		}
	}

	for( S32 te = 0; te < TEX_NUM_INDICES; te++ )
	{
		if (LLVOAvatarDictionary::getTEWearableType((ETextureIndex) te) == mType)
		{
			LLViewerTexture* avatar_image = avatar->getTEImage( te );
			if( !avatar_image )
			{
				llassert( 0 );
				continue;
			}
	
			te_map_t::const_iterator iter = mTEMap.find(te);
			if(iter != mTEMap.end())
			{
 				const LLUUID& image_id = iter->second.getID();
 				if (avatar_image->getID() != image_id)
 				{
 					return TRUE;
 				}
			}
		}
	}

	//if( gFloaterCustomize )
	//{
	//	if( mDescription != gFloaterCustomize->getWearableDescription( mType ) )
	//	{
	//		return TRUE;
	//	}
	//}

	return FALSE;
}


void LLWearable::setParamsToDefaults()
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	llassert( avatar );
	if( !avatar )
	{
		return;
	}

	mVisualParamMap.clear();
	for( LLVisualParam* param = avatar->getFirstVisualParam(); param; param = avatar->getNextVisualParam() )
	{
		if( (((LLViewerVisualParam*)param)->getWearableType() == mType ) && (param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE ) )
		{
			mVisualParamMap[param->getID()] = param->getDefaultWeight();
		}
	}
}

void LLWearable::setTexturesToDefaults()
{
	mTEMap.clear();
	for( S32 te = 0; te < TEX_NUM_INDICES; te++ )
	{
		if (LLVOAvatarDictionary::getTEWearableType((ETextureIndex) te) == mType)
		{
			mTEMap[te] = LLLocalTextureObject(NULL, NULL, NULL, LLVOAvatarDictionary::getDefaultTextureImageID((ETextureIndex) te));
		}
	}
}

// Updates the user's avatar's appearance
void LLWearable::writeToAvatar( BOOL set_by_user )
{
	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	llassert( avatar );
	if( !avatar )
	{
		return;
	}

	ESex old_sex = avatar->getSex();

	// Pull params
	for( LLVisualParam* param = avatar->getFirstVisualParam(); param; param = avatar->getNextVisualParam() )
	{
		if( (((LLViewerVisualParam*)param)->getWearableType() == mType) && (param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE ) )
		{
			S32 param_id = param->getID();
			F32 weight = get_if_there(mVisualParamMap, param_id, param->getDefaultWeight());
			// only animate with user-originated changes
			if (set_by_user)
			{
				param->setAnimationTarget(weight, set_by_user);
			}
			else
			{
				avatar->setVisualParamWeight( param_id, weight, set_by_user );
			}
		}
	}

	// only interpolate with user-originated changes
	if (set_by_user)
	{
		avatar->startAppearanceAnimation(TRUE, TRUE);
	}

	// Pull texture entries
	for( S32 te = 0; te < TEX_NUM_INDICES; te++ )
	{
		if (LLVOAvatarDictionary::getTEWearableType((ETextureIndex) te) == mType)
		{
			te_map_t::const_iterator iter = mTEMap.find(te);
			LLUUID image_id;
			if(iter != mTEMap.end())
			{
				image_id = iter->second.getID();
			}
			else
			{	
				image_id = LLVOAvatarDictionary::getDefaultTextureImageID((ETextureIndex) te);
			}
			LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture( image_id, TRUE, FALSE, LLViewerTexture::LOD_TEXTURE );
			avatar->setLocalTextureTE(te, image, set_by_user);
		}
	}

	avatar->updateVisualParams();

	if( gFloaterCustomize )
	{
		LLViewerInventoryItem* item;
		// MULTI_WEARABLE:
		item = (LLViewerInventoryItem*)gInventory.getItem(gAgentWearables.getWearableItemID(mType,0));
		U32 perm_mask = PERM_NONE;
		BOOL is_complete = FALSE;
		if(item)
		{
			perm_mask = item->getPermissions().getMaskOwner();
			is_complete = item->isComplete();
			if(!is_complete)
			{
				item->fetchFromServer();
			}
		}
		gFloaterCustomize->setWearable(mType, this, perm_mask, is_complete);
		gFloaterCustomize->setCurrentWearableType( mType );
	}

	ESex new_sex = avatar->getSex();
	if( old_sex != new_sex )
	{
		avatar->updateSexDependentLayerSets( set_by_user );
	}	
	
	avatar->updateMeshTextures();

//	if( set_by_user )
//	{
//		gAgent.sendAgentSetAppearance();
//	}
}

// Updates the user's avatar's appearance, replacing this wearables' parameters and textures with default values.
// static 
void LLWearable::removeFromAvatar( EWearableType type, BOOL set_by_user )
{
	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	llassert( avatar );
	if( !avatar )
	{
		return;
	}

	// You can't just remove body parts.
	if( (type == WT_SHAPE) ||
		(type == WT_SKIN) ||
		(type == WT_HAIR) ||
		(type == WT_EYES) )
	{
		return;
	}

	// Pull params
	for( LLVisualParam* param = avatar->getFirstVisualParam(); param; param = avatar->getNextVisualParam() )
	{
		if( (((LLViewerVisualParam*)param)->getWearableType() == type) && (param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE ) )
		{
			S32 param_id = param->getID();
			avatar->setVisualParamWeight( param_id, param->getDefaultWeight(), set_by_user );
		}
	}

	// Pull textures
	LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture( IMG_DEFAULT_AVATAR );
	for( S32 te = 0; te < TEX_NUM_INDICES; te++ )
	{
		if (LLVOAvatarDictionary::getTEWearableType((ETextureIndex) te) == type)
		{
			avatar->setLocalTextureTE(te, image, set_by_user);
		}
	}

	if( gFloaterCustomize )
	{
		gFloaterCustomize->setWearable(type, NULL, PERM_ALL, TRUE);
	}

	avatar->updateVisualParams();
	avatar->updateMeshTextures();

//	if( set_by_user )
//	{
//		gAgent.sendAgentSetAppearance();
//	}
}



// Updates asset from the user's avatar
void LLWearable::readFromAvatar()
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	llassert( avatar );
	if( !avatar )
	{
		return;
	}

	mDefinitionVersion = LLWearable::sCurrentDefinitionVersion;

	mVisualParamMap.clear();
	for( LLVisualParam* param = avatar->getFirstVisualParam(); param; param = avatar->getNextVisualParam() )
	{
		if( (((LLViewerVisualParam*)param)->getWearableType() == mType) && (param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE ) )
		{
			mVisualParamMap[param->getID()] = param->getWeight();
		}
	}

	mTEMap.clear();
	for( S32 te = 0; te < TEX_NUM_INDICES; te++ )
	{
		if (LLVOAvatarDictionary::getTEWearableType((ETextureIndex) te) == mType)
		{
			LLViewerTexture* image = avatar->getTEImage( te );
			if( image )
			{
				mTEMap[te] = LLLocalTextureObject(NULL, NULL, NULL, image->getID());
			}
		}
	}

	//if( gFloaterCustomize )
	//{
	//	mDescription = gFloaterCustomize->getWearableDescription( mType );
	//}
}

// Does not copy mAssetID.
// Definition version is current: removes obsolete enties and creates default values for new ones.
void LLWearable::copyDataFrom(const LLWearable* src)
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	llassert( avatar );
	if( !avatar )
	{
		return;
	}

	mDefinitionVersion = LLWearable::sCurrentDefinitionVersion;

	mName = src->mName;
	mDescription = src->mDescription;
	mPermissions = src->mPermissions;
	mSaleInfo = src->mSaleInfo;
	mType = src->mType;

	// Deep copy of mVisualParamMap (copies only those params that are current, filling in defaults where needed)
	for( LLViewerVisualParam* param = (LLViewerVisualParam*) avatar->getFirstVisualParam(); 
		param;
		param = (LLViewerVisualParam*) avatar->getNextVisualParam() )
	{
		if( (param->getWearableType() == mType) && (param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE ) )
		{
			S32 id = param->getID();
			F32 weight = get_if_there(src->mVisualParamMap, id, param->getDefaultWeight() );
			mVisualParamMap[id] = weight;
		}
	}

	// Deep copy of mTEMap (copies only those tes that are current, filling in defaults where needed)
	for( S32 te = 0; te < TEX_NUM_INDICES; te++ )
	{
		if (LLVOAvatarDictionary::getTEWearableType((ETextureIndex) te) == mType)
		{
			te_map_t::const_iterator iter = mTEMap.find(te);
			LLUUID image_id;
			if(iter != mTEMap.end())
			{
				image_id = iter->second.getID();
			}
			else
			{
				image_id = LLVOAvatarDictionary::getDefaultTextureImageID((ETextureIndex) te);
			}
			mTEMap[te] = LLLocalTextureObject(NULL, NULL, NULL, image_id);
		}
	}
}

void LLWearable::setItemID(const LLUUID& item_id)
{
	mItemID = item_id;
}

const LLUUID& LLWearable::getItemID() const
{
	return mItemID;
}

LLLocalTextureObject* LLWearable::getLocalTextureObject(S32 index) const
{
	te_map_t::const_iterator iter = mTEMap.find(index);
	if( iter != mTEMap.end() )
	{
		return (LLLocalTextureObject*) &iter->second;
	}
	return NULL;
}

void LLWearable::setLocalTextureObject(S32 index, LLLocalTextureObject *lto)
{
	if( lto )
	{
		LLLocalTextureObject obj(*lto);
		mTEMap[index] = obj;
	}
	else
	{
		mTEMap.erase(index);
	}
}

struct LLWearableSaveData
{
	EWearableType mType;
};

void LLWearable::saveNewAsset() const
{
//	llinfos << "LLWearable::saveNewAsset() type: " << getTypeName() << llendl;
	//llinfos << *this << llendl;

	const std::string filename = asset_id_to_filename(mAssetID);
	LLFILE* fp = LLFile::fopen(filename, "wb");		/* Flawfinder: ignore */
	BOOL successful_save = FALSE;
	if(fp && exportFile(fp))
	{
		successful_save = TRUE;
	}
	if(fp)
	{
		fclose(fp);
		fp = NULL;
	}
	if(!successful_save)
	{
		std::string buffer = llformat("Unable to save '%s' to wearable file.", mName.c_str());
		llwarns << buffer << llendl;
		
		LLSD args;
		args["NAME"] = mName;
		LLNotifications::instance().add("CannotSaveWearableOutOfSpace", args);
		return;
	}

	// save it out to database
	if( gAssetStorage )
	{
		 /*
		std::string url = gAgent.getRegion()->getCapability("NewAgentInventory");
		if (!url.empty())
		{
			llinfos << "Update Agent Inventory via capability" << llendl;
			LLSD body;
			body["folder_id"] = gInventory.findCategoryUUIDForType(getAssetType());
			body["asset_type"] = LLAssetType::lookup(getAssetType());
			body["inventory_type"] = LLInventoryType::lookup(LLInventoryType::IT_WEARABLE);
			body["name"] = getName();
			body["description"] = getDescription();
			LLHTTPClient::post(url, body, new LLNewAgentInventoryResponder(body, filename));
		}
		else
		{
		}
		 */
		 LLWearableSaveData* data = new LLWearableSaveData;
		 data->mType = mType;
		 gAssetStorage->storeAssetData(filename, mTransactionID, getAssetType(),
                                     &LLWearable::onSaveNewAssetComplete,
                                     (void*)data);
	}
}

// static
void LLWearable::onSaveNewAssetComplete(const LLUUID& new_asset_id, void* userdata, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLWearableSaveData* data = (LLWearableSaveData*)userdata;
	const std::string& type_name = LLWearableDictionary::getTypeName(data->mType);
	if(0 == status)
	{
		// Success
		llinfos << "Saved wearable " << type_name << llendl;
	}
	else
	{
		std::string buffer = llformat("Unable to save %s to central asset store.", type_name.c_str());
		llwarns << buffer << " Status: " << status << llendl;
		LLSD args;
		args["NAME"] = type_name;
		LLNotifications::instance().add("CannotSaveToAssetStore", args);
	}

	// Delete temp file
	const std::string src_filename = asset_id_to_filename(new_asset_id);
	LLFile::remove(src_filename);

	// delete the context data
	delete data;
}

std::ostream& operator<<(std::ostream &s, const LLWearable &w)
{
	s << "wearable " << LLWearableDictionary::getTypeName(w.mType) << "\n";
	s << "    Name: " << w.mName << "\n";
	s << "    Desc: " << w.mDescription << "\n";
	//w.mPermissions
	//w.mSaleInfo

	s << "    Params:" << "\n";
	for (LLWearable::param_map_t::const_iterator iter = w.mVisualParamMap.begin();
		 iter != w.mVisualParamMap.end(); ++iter)
	{
		S32 param_id = iter->first;
		F32 param_weight = iter->second;
		s << "        " << param_id << " " << param_weight << "\n";
	}

	s << "    Textures:" << "\n";
	for (LLWearable::te_map_t::const_iterator iter = w.mTEMap.begin();
		 iter != w.mTEMap.end(); ++iter)
	{
		S32 te = iter->first;
		const LLUUID& image_id = iter->second.getID();
		s << "        " << te << " " << image_id << "\n";
	}
	return s;
}


std::string terse_F32_to_string(F32 f)
{
	std::string r = llformat("%.2f", f);
	S32 len = r.length();

    // "1.20"  -> "1.2"
    // "24.00" -> "24."
	while (len > 0 && ('0' == r[len - 1]))
	{
		r.erase(len-1, 1);
		len--;
	}
	if ('.' == r[len - 1])
	{
		// "24." -> "24"
		r.erase(len-1, 1);
	}
	else if (('-' == r[0]) && ('0' == r[1]))
	{
		// "-0.59" -> "-.59"
		r.erase(1, 1);
	}
	else if ('0' == r[0])
	{
		// "0.59" -> ".59"
		r.erase(0, 1);
	}
	return r;
}

std::string asset_id_to_filename(const LLUUID &asset_id)
{
	std::string asset_id_string;
	asset_id.toString(asset_id_string);
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id_string) + ".wbl";	
	return filename;
}
