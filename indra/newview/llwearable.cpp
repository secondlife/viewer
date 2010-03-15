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
#include "lllocaltextureobject.h"
#include "llnotificationsutil.h"
#include "llviewertexturelist.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llvoavatardefines.h"
#include "llwearable.h"
#include "lldictionary.h"
#include "lltrans.h"
#include "lltexlayer.h"
#include "llvisualparam.h"
#include "lltexglobalcolor.h"

using namespace LLVOAvatarDefines;

// static
S32 LLWearable::sCurrentDefinitionVersion = 1;

// support class - remove for 2.1 (hackity hack hack)
class LLOverrideBakedTextureUpdate
{
public:
	LLOverrideBakedTextureUpdate(bool temp_state)
	{
		mAvatar = gAgent.getAvatarObject();
		U32 num_bakes = (U32) LLVOAvatarDefines::BAKED_NUM_INDICES;
		for( U32 index = 0; index < num_bakes; ++index )
		{
			composite_enabled[index] = mAvatar->isCompositeUpdateEnabled(index);
		}
		mAvatar->setCompositeUpdatesEnabled(temp_state);
		llinfos << "suprress baked texture update initialized to " << temp_state << llendl;
	}

	~LLOverrideBakedTextureUpdate()
	{
		U32 num_bakes = (U32)LLVOAvatarDefines::BAKED_NUM_INDICES;		
		for( U32 index = 0; index < num_bakes; ++index )
		{
			mAvatar->setCompositeUpdatesEnabled(index, composite_enabled[index]);
		}		
		llinfos << "suppress baked texture update reverted " << llendl;
	}

private:
	bool composite_enabled[LLVOAvatarDefines::BAKED_NUM_INDICES];
	LLVOAvatarSelf *mAvatar;
};

// Private local functions
static std::string terse_F32_to_string(F32 f);
static std::string asset_id_to_filename(const LLUUID &asset_id);

LLWearable::LLWearable(const LLTransactionID& transaction_id) :
	mDefinitionVersion(LLWearable::sCurrentDefinitionVersion),
	mType(WT_INVALID)
{
	mTransactionID = transaction_id;
	mAssetID = mTransactionID.makeAssetID(gAgent.getSecureSessionID());
}

LLWearable::LLWearable(const LLAssetID& asset_id) :
	mDefinitionVersion( LLWearable::sCurrentDefinitionVersion ),
	mType(WT_INVALID)
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
	S32 num_parameters = mVisualParamIndexMap.size();
	if( fprintf( file, "parameters %d\n", num_parameters ) < 0 )
	{
		return FALSE;
	}

	for (visual_param_index_map_t::const_iterator iter = mVisualParamIndexMap.begin();
		 iter != mVisualParamIndexMap.end(); 
		 ++iter)
	{
		S32 param_id = iter->first;
		const LLVisualParam* param = iter->second;
		F32 param_weight = param->getWeight();
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
		const LLUUID& image_id = iter->second->getID();
		if( fprintf( file, "%d %s\n", te, image_id.asString().c_str()) < 0 )
		{
			return FALSE;
		}
	}
	return TRUE;
}


void LLWearable::createVisualParams()
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	for (LLViewerVisualParam* param = (LLViewerVisualParam*) avatar->getFirstVisualParam(); 
		 param;
		 param = (LLViewerVisualParam*) avatar->getNextVisualParam())
	{
		if (param->getWearableType() == mType)
		{
			addVisualParam(param->cloneParam(this));
		}
	}

	// resync driver parameters to point to the newly cloned driven parameters
	for (visual_param_index_map_t::iterator param_iter = mVisualParamIndexMap.begin(); 
		 param_iter != mVisualParamIndexMap.end(); 
		 ++param_iter)
	{
		LLVisualParam* param = param_iter->second;
		LLVisualParam*(LLWearable::*wearable_function)(S32)const = &LLWearable::getVisualParam; 
		// need this line to disambiguate between versions of LLCharacter::getVisualParam()
		LLVisualParam*(LLVOAvatarSelf::*avatar_function)(S32)const = &LLVOAvatarSelf::getVisualParam; 
		param->resetDrivenParams();
		if(!param->linkDrivenParams(boost::bind(wearable_function,(LLWearable*)this, _1), false))
		{
			if( !param->linkDrivenParams(boost::bind(avatar_function,(LLVOAvatarSelf*)avatar,_1 ), true))
			{
				llwarns << "could not link driven params for wearable " << getName() << " id: " << param->getID() << llendl;
				continue;
			}
		}
	}
}

BOOL LLWearable::importFile( LLFILE* file )
{
	// *NOTE: changing the type or size of this buffer will require
	// changes in the fscanf() code below. You would be better off
	// rewriting this to use streams and not require an open FILE.
	char text_buffer[2048];		/* Flawfinder: ignore */
	S32 fields_read = 0;

	// suppress texlayerset updates while wearables are being imported. Layersets will be updated
	// when the wearables are "worn", not loaded. Note state will be restored when this object is destroyed.
	LLOverrideBakedTextureUpdate stop_bakes(false);

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


	// Temoprary hack to allow wearables with definition version 24 to still load.
	// This should only affect lindens and NDA'd testers who have saved wearables in 2.0
	// the extra check for version == 24 can be removed before release, once internal testers
	// have loaded these wearables again. See hack pt 2 at bottom of function to ensure that
	// these wearables get re-saved with version definition 22.
	if( mDefinitionVersion > LLWearable::sCurrentDefinitionVersion && mDefinitionVersion != 24 )
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
		setType((EWearableType)type);
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

	if( num_parameters != mVisualParamIndexMap.size() )
	{
		llwarns << "Wearable parameter mismatch. Reading in " << num_parameters << " from file, but created " << mVisualParamIndexMap.size() << " from avatar parameters. type: " <<  mType << llendl;
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
		mSavedVisualParamMap[param_id] = param_weight;
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
		LLUUID id = LLUUID(text_buffer);
		LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTexture( id );
		if( mTEMap.find(te) != mTEMap.end() )
		{
			delete mTEMap[te];
		}
		if( mSavedTEMap.find(te) != mSavedTEMap.end() )
		{
			delete mSavedTEMap[te];
		}

		LLUUID textureid(text_buffer);
		mTEMap[te] = new LLLocalTextureObject(image, textureid);
		mSavedTEMap[te] = new LLLocalTextureObject(image, textureid);
		createLayers(te);
	}

	// copy all saved param values to working params
	revertValues();

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
			if( !is_in_map(mVisualParamIndexMap, param->getID() ) )
			{
				return TRUE;
			}
		}
	}
	if( param_count != mVisualParamIndexMap.size() )
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
		if( (param->getWearableType() == mType) 
			&& (param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE ) 
			&& !param->getCrossWearable())
		{
			F32 current_weight = getVisualParamWeight(param->getID());
			current_weight = llclamp( current_weight, param->getMinWeight(), param->getMaxWeight() );
			F32 saved_weight = get_if_there(mSavedVisualParamMap, param->getID(), param->getDefaultWeight());
			saved_weight = llclamp( saved_weight, param->getMinWeight(), param->getMaxWeight() );
			
			U8 a = F32_to_U8( saved_weight, param->getMinWeight(), param->getMaxWeight() );
			U8 b = F32_to_U8( current_weight, param->getMinWeight(), param->getMaxWeight() );
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
			te_map_t::const_iterator current_iter = mTEMap.find(te);
			if(current_iter != mTEMap.end())
			{
 				const LLUUID& current_image_id = current_iter->second->getID();
				te_map_t::const_iterator saved_iter = mSavedTEMap.find(te);
				if(saved_iter != mSavedTEMap.end())
				{
					const LLUUID& saved_image_id = saved_iter->second->getID();
					if (saved_image_id != current_image_id)
					{
						// saved vs current images are different, wearable is dirty
						return TRUE;
					}
				}
				else
				{
					// image found in current image list but not saved image list
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

	for( LLVisualParam* param = avatar->getFirstVisualParam(); param; param = avatar->getNextVisualParam() )
	{
		if( (((LLViewerVisualParam*)param)->getWearableType() == mType ) && (param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE ) )
		{
			setVisualParamWeight(param->getID(),param->getDefaultWeight(), FALSE);
		}
	}
}

void LLWearable::setTexturesToDefaults()
{
	for( S32 te = 0; te < TEX_NUM_INDICES; te++ )
	{
		if (LLVOAvatarDictionary::getTEWearableType((ETextureIndex) te) == mType)
		{
			LLUUID id = LLVOAvatarDictionary::getDefaultTextureImageID((ETextureIndex) te);
			LLViewerFetchedTexture * image = LLViewerTextureManager::getFetchedTexture( id );
			if( mTEMap.find(te) == mTEMap.end() )
			{
				mTEMap[te] = new LLLocalTextureObject(image, id);
				createLayers(te);
			}
			else
			{
				// Local Texture Object already created, just set image and UUID
				LLLocalTextureObject *lto = mTEMap[te];
				lto->setID(id);
				lto->setImage(image);
			}
		}
	}
}

// Updates the user's avatar's appearance
void LLWearable::writeToAvatar()
{
	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	llassert( avatar );
	if( !avatar )
	{
		llerrs << "could not get avatar object to write to for wearable " << this->getName() << llendl;
		return;
	}

	ESex old_sex = avatar->getSex();

	// Pull params
	for( LLVisualParam* param = avatar->getFirstVisualParam(); param; param = avatar->getNextVisualParam() )
	{
		// cross-wearable parameters are not authoritative, as they are driven by a different wearable. So don't copy the values to the
		// avatar object if cross wearable. Cross wearable params get their values from the avatar, they shouldn't write the other way.
		if( (((LLViewerVisualParam*)param)->getWearableType() == mType) && (!((LLViewerVisualParam*)param)->getCrossWearable()) )
		{
			S32 param_id = param->getID();
			F32 weight = getVisualParamWeight(param_id);

			avatar->setVisualParamWeight( param_id, weight, FALSE );
		}
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
				image_id = iter->second->getID();
			}
			else
			{	
				image_id = LLVOAvatarDictionary::getDefaultTextureImageID((ETextureIndex) te);
			}
			LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture( image_id, TRUE, LLViewerTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE );
			// MULTI-WEARABLE: replace hard-coded 0
			avatar->setLocalTextureTE(te, image, 0);
		}
	}

	ESex new_sex = avatar->getSex();
	if( old_sex != new_sex )
	{
		avatar->updateSexDependentLayerSets( FALSE );
	}	
	
//	if( upload_bake )
//	{
//		gAgent.sendAgentSetAppearance();
//	}
}


// Updates the user's avatar's appearance, replacing this wearables' parameters and textures with default values.
// static 
void LLWearable::removeFromAvatar( EWearableType type, BOOL upload_bake )
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
			avatar->setVisualParamWeight( param_id, param->getDefaultWeight(), upload_bake );
		}
	}

	if( gFloaterCustomize )
	{
		gFloaterCustomize->setWearable(type, NULL, PERM_ALL, TRUE);
	}

	avatar->updateVisualParams();
	avatar->wearableUpdated(type, TRUE);

//	if( upload_bake )
//	{
//		gAgent.sendAgentSetAppearance();
//	}
}

// Does not copy mAssetID.
// Definition version is current: removes obsolete enties and creates default values for new ones.
void LLWearable::copyDataFrom(const LLWearable* src)
{
	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
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

	setType(src->mType);

	mSavedVisualParamMap.clear();
	// Deep copy of mVisualParamMap (copies only those params that are current, filling in defaults where needed)
	for( LLViewerVisualParam* param = (LLViewerVisualParam*) avatar->getFirstVisualParam(); 
		param;
		param = (LLViewerVisualParam*) avatar->getNextVisualParam() )
	{
		if( (param->getWearableType() == mType) )
		{
			S32 id = param->getID();
			F32 weight = src->getVisualParamWeight(id);
			mSavedVisualParamMap[id] = weight;
		}
	}

	destroyTextures();
	// Deep copy of mTEMap (copies only those tes that are current, filling in defaults where needed)
	for( S32 te = 0; te < TEX_NUM_INDICES; te++ )
	{
		if (LLVOAvatarDictionary::getTEWearableType((ETextureIndex) te) == mType)
		{
			te_map_t::const_iterator iter = src->mTEMap.find(te);
			LLUUID image_id;
			LLViewerFetchedTexture *image = NULL;
			if(iter != src->mTEMap.end())
			{
				image = src->getConstLocalTextureObject(te)->getImage();
				image_id = src->getConstLocalTextureObject(te)->getID();
				mTEMap[te] = new LLLocalTextureObject(image, image_id);
				mSavedTEMap[te] = new LLLocalTextureObject(image, image_id);
				mTEMap[te]->setBakedReady(src->getConstLocalTextureObject(te)->getBakedReady());
				mTEMap[te]->setDiscard(src->getConstLocalTextureObject(te)->getDiscard());
			}
			else
			{
				image_id = LLVOAvatarDictionary::getDefaultTextureImageID((ETextureIndex) te);
				image = LLViewerTextureManager::getFetchedTexture( image_id );
				mTEMap[te] = new LLLocalTextureObject(image, image_id);
				mSavedTEMap[te] = new LLLocalTextureObject(image, image_id);
			}
			createLayers(te);
		}
	}

	// Probably reduntant, but ensure that the newly created wearable is not dirty by setting current value of params in new wearable
	// to be the same as the saved values (which were loaded from src at param->cloneParam(this))
	revertValues();
}

void LLWearable::setItemID(const LLUUID& item_id)
{
	mItemID = item_id;
}

const LLUUID& LLWearable::getItemID() const
{
	return mItemID;
}

void LLWearable::setType(EWearableType type) 
{ 
	mType = type; 
	createVisualParams();
}

LLLocalTextureObject* LLWearable::getLocalTextureObject(S32 index)
{
	te_map_t::iterator iter = mTEMap.find(index);
	if( iter != mTEMap.end() )
	{
		LLLocalTextureObject* lto = iter->second;
		return lto;
	}
	return NULL;
}

const LLLocalTextureObject* LLWearable::getConstLocalTextureObject(S32 index) const
{
	te_map_t::const_iterator iter = mTEMap.find(index);
	if( iter != mTEMap.end() )
	{
		const LLLocalTextureObject* lto = iter->second;
		return lto;
	}
	return NULL;
}

void LLWearable::setLocalTextureObject(S32 index, LLLocalTextureObject &lto)
{
	if( mTEMap.find(index) != mTEMap.end() )
	{
		mTEMap.erase(index);
	}
	mTEMap[index] = new LLLocalTextureObject(lto);
}


void LLWearable::addVisualParam(LLVisualParam *param)
{
	if( mVisualParamIndexMap[param->getID()] )
	{
		delete mVisualParamIndexMap[param->getID()];
	}
	param->setIsDummy(FALSE);
	mVisualParamIndexMap[param->getID()] = param;
	mSavedVisualParamMap[param->getID()] = param->getDefaultWeight();
}

void LLWearable::setVisualParams()
{
	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();

	for (visual_param_index_map_t::const_iterator iter = mVisualParamIndexMap.begin(); iter != mVisualParamIndexMap.end(); iter++)
	{
		S32 id = iter->first;
		LLVisualParam *wearable_param = iter->second;
		F32 value = wearable_param->getWeight();
		avatar->setVisualParamWeight(id, value, FALSE);
	}
}


void LLWearable::setVisualParamWeight(S32 param_index, F32 value, BOOL upload_bake)
{
	if( is_in_map(mVisualParamIndexMap, param_index ) )
	{
		LLVisualParam *wearable_param = mVisualParamIndexMap[param_index];
		wearable_param->setWeight(value, upload_bake);
	}
	else
	{
		llerrs << "LLWearable::setVisualParam passed invalid parameter index: " << param_index << " for wearable type: " << this->getName() << llendl;
	}
}

F32 LLWearable::getVisualParamWeight(S32 param_index) const
{
	if( is_in_map(mVisualParamIndexMap, param_index ) )
	{
		const LLVisualParam *wearable_param = mVisualParamIndexMap.find(param_index)->second;
		return wearable_param->getWeight();
	}
	else
	{
		llwarns << "LLWerable::getVisualParam passed invalid parameter index: "  << param_index << " for wearable type: " << this->getName() << llendl;
	}
	return (F32)-1.0;
}

LLVisualParam* LLWearable::getVisualParam(S32 index) const
{
	visual_param_index_map_t::const_iterator iter = mVisualParamIndexMap.find(index);
	return (iter == mVisualParamIndexMap.end()) ? NULL : iter->second;
}


void LLWearable::getVisualParams(visual_param_vec_t &list)
{
	visual_param_index_map_t::iterator iter = mVisualParamIndexMap.begin();
	visual_param_index_map_t::iterator end = mVisualParamIndexMap.end();

	// add all visual params to the passed-in vector
	for( ; iter != end; ++iter )
	{
		list.push_back(iter->second);
	}
}

void LLWearable::animateParams(F32 delta, BOOL upload_bake)
{
	for(visual_param_index_map_t::iterator iter = mVisualParamIndexMap.begin();
		 iter != mVisualParamIndexMap.end();
		 ++iter)
	{
		LLVisualParam *param = (LLVisualParam*) iter->second;
		param->animate(delta, upload_bake);
	}
}

LLColor4 LLWearable::getClothesColor(S32 te) const
{
	LLColor4 color;
	U32 param_name[3];
	if( LLVOAvatar::teToColorParams( (LLVOAvatarDefines::ETextureIndex)te, param_name ) )
	{
		for( U8 index = 0; index < 3; index++ )
		{
			color.mV[index] = getVisualParamWeight(param_name[index]);
		}
	}
	return color;
}

void LLWearable::setClothesColor( S32 te, const LLColor4& new_color, BOOL upload_bake )
{
	U32 param_name[3];
	if( LLVOAvatar::teToColorParams( (LLVOAvatarDefines::ETextureIndex)te, param_name ) )
	{
		for( U8 index = 0; index < 3; index++ )
		{
			setVisualParamWeight(param_name[index], new_color.mV[index], upload_bake);
		}
	}
}

void LLWearable::revertValues()
{
	//update saved settings so wearable is no longer dirty
	// non-driver params first
	for (param_map_t::const_iterator iter = mSavedVisualParamMap.begin(); iter != mSavedVisualParamMap.end(); iter++)
	{
		S32 id = iter->first;
		F32 value = iter->second;
		LLVisualParam *param = getVisualParam(id);
		if(param &&  !dynamic_cast<LLDriverParam*>(param) )
		{
			setVisualParamWeight(id, value, TRUE);
		}
	}

	//then driver params
	for (param_map_t::const_iterator iter = mSavedVisualParamMap.begin(); iter != mSavedVisualParamMap.end(); iter++)
	{
		S32 id = iter->first;
		F32 value = iter->second;
		LLVisualParam *param = getVisualParam(id);
		if(param &&  dynamic_cast<LLDriverParam*>(param) )
		{
			setVisualParamWeight(id, value, TRUE);
		}
	}

	// make sure that saved values are sane
	for (param_map_t::const_iterator iter = mSavedVisualParamMap.begin(); iter != mSavedVisualParamMap.end(); iter++)
	{
		S32 id = iter->first;
		LLVisualParam *param = getVisualParam(id);
		if( param )
		{
			mSavedVisualParamMap[id] = param->getWeight();
		}
	}

	syncImages(mSavedTEMap, mTEMap);

	if( gFloaterCustomize )
	{
		gFloaterCustomize->updateScrollingPanelList(TRUE);
	}
}

BOOL LLWearable::isOnTop() const
{ 
	return (this == gAgentWearables.getTopWearable(mType));
}

void LLWearable::createLayers(S32 te)
{
	LLVOAvatarSelf* avatar = gAgent.getAvatarObject();
	LLTexLayerSet *layer_set = avatar->getLayerSet((ETextureIndex)te);
	if( layer_set )
	{
		layer_set->cloneTemplates(mTEMap[te], (ETextureIndex)te, this);
	}
	else
	{
		llerrs << "could not find layerset for LTO in wearable!" << llendl;
	}
}

void LLWearable::saveValues()
{
	//update saved settings so wearable is no longer dirty
	mSavedVisualParamMap.clear();
	for (visual_param_index_map_t::const_iterator iter = mVisualParamIndexMap.begin(); iter != mVisualParamIndexMap.end(); ++iter)
	{
		S32 id = iter->first;
		LLVisualParam *wearable_param = iter->second;
		F32 value = wearable_param->getWeight();
		mSavedVisualParamMap[id] = value;
	}

	// Deep copy of mTEMap (copies only those tes that are current, filling in defaults where needed)
	syncImages(mTEMap, mSavedTEMap);

	if( gFloaterCustomize )
	{
		gFloaterCustomize->updateScrollingPanelList(TRUE);
	}
}

void LLWearable::syncImages(te_map_t &src, te_map_t &dst)
{
	// Deep copy of src (copies only those tes that are current, filling in defaults where needed)
	for( S32 te = 0; te < TEX_NUM_INDICES; te++ )
	{
		if (LLVOAvatarDictionary::getTEWearableType((ETextureIndex) te) == mType)
		{
			te_map_t::const_iterator iter = src.find(te);
			LLUUID image_id;
			LLViewerFetchedTexture *image = NULL;
			LLLocalTextureObject *lto = NULL;
			if(iter != src.end())
			{
				// there's a Local Texture Object in the source image map. Use this to populate the values to store in the destination image map.
				lto = iter->second;
				image = lto->getImage();
				image_id = lto->getID();
			}
			else
			{
				// there is no Local Texture Object in the source image map. Get defaults values for populating the destination image map.
				image_id = LLVOAvatarDictionary::getDefaultTextureImageID((ETextureIndex) te);
				image = LLViewerTextureManager::getFetchedTexture( image_id );
			}

			if( dst.find(te) != dst.end() )
			{
				// there's already an entry in the destination map for the texture. Just update its values.
				dst[te]->setImage(image);
				dst[te]->setID(image_id);
			}
			else
			{
				// no entry found in the destination map, we need to create a new Local Texture Object
				dst[te] = new LLLocalTextureObject(image, image_id);
			}

			if( lto )
			{
				// If we pulled values from a Local Texture Object in the source map, make sure the proper flags are set in the new (or updated) entry in the destination map.
				dst[te]->setBakedReady(lto->getBakedReady());
				dst[te]->setDiscard(lto->getDiscard());
			}
		}
	}
}

void LLWearable::destroyTextures()
{
	for( te_map_t::iterator iter = mTEMap.begin(); iter != mTEMap.end(); ++iter )
	{
		LLLocalTextureObject *lto = iter->second;
		delete lto;
	}
	mTEMap.clear();
	for( te_map_t::iterator iter = mSavedTEMap.begin(); iter != mSavedTEMap.end(); ++iter )
	{
		LLLocalTextureObject *lto = iter->second;
		delete lto;
	}
	mSavedTEMap.clear();
}

void LLWearable::pullCrossWearableValues()
{
	// scan through all of the avatar's visual parameters
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	for (LLViewerVisualParam* param = (LLViewerVisualParam*) avatar->getFirstVisualParam(); 
		 param;
		 param = (LLViewerVisualParam*) avatar->getNextVisualParam())
	{
		if( param )
		{
			LLDriverParam *driver_param = dynamic_cast<LLDriverParam*>(param);
			if(driver_param)
			{
				// parameter is a driver parameter, have it update its 
				driver_param->updateCrossDrivenParams(getType());
			}
		}
	}
}


void LLWearable::setLabelUpdated() const
{ 
	gInventory.addChangedMask(LLInventoryObserver::LABEL, getItemID());
}

void LLWearable::refreshName()
{
	LLUUID item_id = getItemID();
	LLInventoryItem* item = gInventory.getItem(item_id);
	if( item )
	{
		mName = item->getName();
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
		LLNotificationsUtil::add("CannotSaveWearableOutOfSpace", args);
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
			body["folder_id"] = gInventory.findCategoryUUIDForType(LLFolderType::assetToFolderType(getAssetType()));
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
		LLNotificationsUtil::add("CannotSaveToAssetStore", args);
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
	for (LLWearable::visual_param_index_map_t::const_iterator iter = w.mVisualParamIndexMap.begin();
		 iter != w.mVisualParamIndexMap.end(); ++iter)
	{
		S32 param_id = iter->first;
		LLVisualParam *wearable_param = iter->second;
		F32 param_weight = wearable_param->getWeight();
		s << "        " << param_id << " " << param_weight << "\n";
	}

	s << "    Textures:" << "\n";
	for (LLWearable::te_map_t::const_iterator iter = w.mTEMap.begin();
		 iter != w.mTEMap.end(); ++iter)
	{
		S32 te = iter->first;
		const LLUUID& image_id = iter->second->getID();
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
