/** 
 * @file llwearable.cpp
 * @brief LLWearable class implementation
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

#include "linden_common.h"

#include "llavatarappearance.h"
#include "lllocaltextureobject.h"
#include "lltexlayer.h"
#include "lltexturemanagerbridge.h"
#include "llvisualparam.h"
#include "llavatarappearancedefines.h"
#include "llwearable.h"

using namespace LLAvatarAppearanceDefines;

// static
S32 LLWearable::sCurrentDefinitionVersion = 1;

// Private local functions
static std::string terse_F32_to_string(F32 f);

// virtual
LLWearable::~LLWearable()
{
}

const std::string& LLWearable::getTypeLabel() const
{
	return LLWearableType::getTypeLabel(mType);
}

const std::string& LLWearable::getTypeName() const
{
	return LLWearableType::getTypeName(mType);
}

LLAssetType::EType LLWearable::getAssetType() const
{
	return LLWearableType::getAssetType(mType);
}

BOOL LLWearable::exportFile(LLFILE* fp) const
{
	llofstream ofs(fp);
	return exportStream(ofs);
}

// virtual
BOOL LLWearable::exportStream( std::ostream& output_stream ) const
{
	if (!output_stream.good()) return FALSE;

	// header and version
	output_stream << "LLWearable version " << mDefinitionVersion  << "\n";
	// name
	output_stream << mName << "\n";
	// description
	output_stream << mDescription << "\n";

	// permissions
	if( !mPermissions.exportStream( output_stream ) )
	{
		return FALSE;
	}

	// sale info
	if( !mSaleInfo.exportStream( output_stream ) )
	{
		return FALSE;
	}

	// wearable type
	output_stream << "type " << (S32) getType() << "\n";

	// parameters
	output_stream << "parameters " << mVisualParamIndexMap.size() << "\n";

	for (visual_param_index_map_t::const_iterator iter = mVisualParamIndexMap.begin();
		 iter != mVisualParamIndexMap.end(); 
		 ++iter)
	{
		S32 param_id = iter->first;
		const LLVisualParam* param = iter->second;
		F32 param_weight = param->getWeight();
		output_stream << param_id << " " << terse_F32_to_string( param_weight ) << "\n";
	}

	// texture entries
	output_stream << "textures " << mTEMap.size() << "\n";

	for (te_map_t::const_iterator iter = mTEMap.begin(); iter != mTEMap.end(); ++iter)
	{
			S32 te = iter->first;
			const LLUUID& image_id = iter->second->getID();
			output_stream << te << " " << image_id << "\n";
	}
	return TRUE;
}

void LLWearable::createVisualParams(LLAvatarAppearance *avatarp)
{
	for (LLViewerVisualParam* param = (LLViewerVisualParam*) avatarp->getFirstVisualParam(); 
		 param;
		 param = (LLViewerVisualParam*) avatarp->getNextVisualParam())
	{
		if (param->getWearableType() == mType)
		{
			LLVisualParam *clone_param = param->cloneParam(this);
			clone_param->setParamLocation(LOC_UNKNOWN);
			clone_param->setParamLocation(LOC_WEARABLE);
			addVisualParam(clone_param);
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
		LLVisualParam*(LLAvatarAppearance::*param_function)(S32)const = &LLAvatarAppearance::getVisualParam; 
		param->resetDrivenParams();
		if(!param->linkDrivenParams(boost::bind(wearable_function,(LLWearable*)this, _1), false))
		{
			if( !param->linkDrivenParams(boost::bind(param_function,avatarp,_1 ), true))
			{
				llwarns << "could not link driven params for wearable " << getName() << " id: " << param->getID() << llendl;
				continue;
			}
		}
	}
}

void LLWearable::createLayers(S32 te, LLAvatarAppearance *avatarp)
{
	LLTexLayerSet *layer_set = NULL;
	const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = LLAvatarAppearanceDictionary::getInstance()->getTexture((ETextureIndex)te);
	if (texture_dict->mIsUsedByBakedTexture)
	{
		const EBakedTextureIndex baked_index = texture_dict->mBakedTextureIndex;
		
		 layer_set = avatarp->getAvatarLayerSet(baked_index);
	}

	if (layer_set)
	{
		   layer_set->cloneTemplates(mTEMap[te], (ETextureIndex)te, this);
	}
	else
	{
		   llerrs << "could not find layerset for LTO in wearable!" << llendl;
	}
}

LLWearable::EImportResult LLWearable::importFile(LLFILE* fp, LLAvatarAppearance* avatarp )
{
	llifstream ifs(fp);
	return importStream(ifs, avatarp);
}

// virtual
LLWearable::EImportResult LLWearable::importStream( std::istream& input_stream, LLAvatarAppearance* avatarp )
{
	// *NOTE: changing the type or size of this buffer will require
	// changes in the fscanf() code below.
	// We are using a local max buffer size here to avoid issues
	// if MAX_STRING size changes.
	const U32 PARSE_BUFFER_SIZE = 2048;
	char buffer[PARSE_BUFFER_SIZE];		/* Flawfinder: ignore */
	char uuid_buffer[37];	/* Flawfinder: ignore */

	// This data is being generated on the viewer.
	// Impose some sane limits on parameter and texture counts.
	const S32 MAX_WEARABLE_ASSET_TEXTURES = 100;
	const S32 MAX_WEARABLE_ASSET_PARAMETERS = 1000;

	if(!avatarp)
	{
		return LLWearable::FAILURE;
	}

	// read header and version 
	if (!getNextPopulatedLine(input_stream, buffer, PARSE_BUFFER_SIZE))
	{
		llwarns << "Failed to read wearable asset input stream." << llendl;
		return LLWearable::FAILURE;
	}
	if ( 1 != sscanf( /* Flawfinder: ignore */
				buffer,
				"LLWearable version %d\n",
				&mDefinitionVersion ) )
	{
		return LLWearable::BAD_HEADER;
	}

	// Hack to allow wearables with definition version 24 to still load.
	// This should only affect lindens and NDA'd testers who have saved wearables in 2.0
	// the extra check for version == 24 can be removed before release, once internal testers
	// have loaded these wearables again. See hack pt 2 at bottom of function to ensure that
	// these wearables get re-saved with version definition 22.
	if( mDefinitionVersion > LLWearable::sCurrentDefinitionVersion && mDefinitionVersion != 24 )
	{
		llwarns << "Wearable asset has newer version (" << mDefinitionVersion << ") than XML (" << LLWearable::sCurrentDefinitionVersion << ")" << llendl;
		return LLWearable::FAILURE;
	}

	// name may be empty
    if (!input_stream.good())
	{
		llwarns << "Bad Wearable asset: early end of input stream " 
				<< "while reading name" << llendl;
		return LLWearable::FAILURE;
	}
	input_stream.getline(buffer, PARSE_BUFFER_SIZE);
	mName = buffer;

	// description may be empty
	if (!input_stream.good())
	{
		llwarns << "Bad Wearable asset: early end of input stream " 
				<< "while reading description" << llendl;
		return LLWearable::FAILURE;
	}
	input_stream.getline(buffer, PARSE_BUFFER_SIZE);
	mDescription = buffer;

	// permissions may have extra empty lines before the correct line
	if (!getNextPopulatedLine(input_stream, buffer, PARSE_BUFFER_SIZE))
	{
		llwarns << "Bad Wearable asset: early end of input stream " 
				<< "while reading permissions" << llendl;
		return LLWearable::FAILURE;
	}
	S32 perm_version = -1;
	if ( 1 != sscanf( buffer, " permissions %d\n", &perm_version ) ||
		 perm_version != 0 )
	{
		llwarns << "Bad Wearable asset: missing valid permissions" << llendl;
		return LLWearable::FAILURE;
	}
	if( !mPermissions.importStream( input_stream ) )
	{
		return LLWearable::FAILURE;
	}

	// sale info
	if (!getNextPopulatedLine(input_stream, buffer, PARSE_BUFFER_SIZE))
	{
		llwarns << "Bad Wearable asset: early end of input stream " 
				<< "while reading sale info" << llendl;
		return LLWearable::FAILURE;
	}
	S32 sale_info_version = -1;
	if ( 1 != sscanf( buffer, " sale_info %d\n", &sale_info_version ) ||
		sale_info_version != 0 )
	{
		llwarns << "Bad Wearable asset: missing valid sale_info" << llendl;
		return LLWearable::FAILURE;
	}
	// Sale info used to contain next owner perm. It is now in the
	// permissions. Thus, we read that out, and fix legacy
	// objects. It's possible this op would fail, but it should pick
	// up the vast majority of the tasks.
	BOOL has_perm_mask = FALSE;
	U32 perm_mask = 0;
	if( !mSaleInfo.importStream(input_stream, has_perm_mask, perm_mask) )
	{
		return LLWearable::FAILURE;
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
	if (!getNextPopulatedLine(input_stream, buffer, PARSE_BUFFER_SIZE))
	{
		llwarns << "Bad Wearable asset: early end of input stream " 
				<< "while reading type" << llendl;
		return LLWearable::FAILURE;
	}
	S32 type = -1;
	if ( 1 != sscanf( buffer, "type %d\n", &type ) )
	{
		llwarns << "Bad Wearable asset: bad type" << llendl;
		return LLWearable::FAILURE;
	}
	if( 0 <= type && type < LLWearableType::WT_COUNT )
	{
		setType((LLWearableType::EType)type, avatarp);
	}
	else
	{
		mType = LLWearableType::WT_COUNT;
		llwarns << "Bad Wearable asset: bad type #" << type <<  llendl;
		return LLWearable::FAILURE;
	}

	// parameters header
	if (!getNextPopulatedLine(input_stream, buffer, PARSE_BUFFER_SIZE))
	{
		llwarns << "Bad Wearable asset: early end of input stream " 
				<< "while reading parameters header" << llendl;
		return LLWearable::FAILURE;
	}
	S32 num_parameters = -1;
	if ( 1 != sscanf( buffer, "parameters %d\n", &num_parameters ) )
	{
		llwarns << "Bad Wearable asset: missing parameters block" << llendl;
		return LLWearable::FAILURE;
	}
	if ( num_parameters > MAX_WEARABLE_ASSET_PARAMETERS )
	{
		llwarns << "Bad Wearable asset: too many parameters, "
				<< num_parameters << llendl;
		return LLWearable::FAILURE;
	}
	if( num_parameters != mVisualParamIndexMap.size() )
	{
		llwarns << "Wearable parameter mismatch. Reading in " 
				<< num_parameters << " from file, but created " 
				<< mVisualParamIndexMap.size() 
				<< " from avatar parameters. type: " 
				<<  getType() << llendl;
	}

	// parameters
	S32 i;
	for( i = 0; i < num_parameters; i++ )
	{
		if (!getNextPopulatedLine(input_stream, buffer, PARSE_BUFFER_SIZE))
		{
			llwarns << "Bad Wearable asset: early end of input stream " 
					<< "while reading parameter #" << i << llendl;
			return LLWearable::FAILURE;
		}
		S32 param_id = 0;
		F32 param_weight = 0.f;
		if ( 2 != sscanf( buffer, "%d %f\n", &param_id, &param_weight ) )
		{
			llwarns << "Bad Wearable asset: bad parameter, #" << i << llendl;
			return LLWearable::FAILURE;
		}
		mSavedVisualParamMap[param_id] = param_weight;
	}

	// textures header
	if (!getNextPopulatedLine(input_stream, buffer, PARSE_BUFFER_SIZE))
	{
		llwarns << "Bad Wearable asset: early end of input stream " 
				<< "while reading textures header" << i << llendl;
		return LLWearable::FAILURE;
	}
	S32 num_textures = -1;
	if ( 1 != sscanf( buffer, "textures %d\n", &num_textures) )
	{
		llwarns << "Bad Wearable asset: missing textures block" << llendl;
		return LLWearable::FAILURE;
	}
	if ( num_textures > MAX_WEARABLE_ASSET_TEXTURES )
	{
		llwarns << "Bad Wearable asset: too many textures, "
				<< num_textures << llendl;
		return LLWearable::FAILURE;
	}

	// textures
	for( i = 0; i < num_textures; i++ )
	{
		if (!getNextPopulatedLine(input_stream, buffer, PARSE_BUFFER_SIZE))
		{
			llwarns << "Bad Wearable asset: early end of input stream " 
					<< "while reading textures #" << i << llendl;
			return LLWearable::FAILURE;
		}
		S32 te = 0;
		if ( 2 != sscanf(   /* Flawfinder: ignore */
				buffer,
				"%d %36s\n",
				&te, uuid_buffer) )
		{
				llwarns << "Bad Wearable asset: bad texture, #" << i << llendl;
				return LLWearable::FAILURE;
		}
	
		if( !LLUUID::validate( uuid_buffer ) )
		{
				llwarns << "Bad Wearable asset: bad texture uuid: " 
						<< uuid_buffer << llendl;
				return LLWearable::FAILURE;
		}
		LLUUID id = LLUUID(uuid_buffer);
		LLGLTexture* image = gTextureManagerBridgep->getFetchedTexture( id );
		if( mTEMap.find(te) != mTEMap.end() )
		{
				delete mTEMap[te];
		}
		if( mSavedTEMap.find(te) != mSavedTEMap.end() )
		{
				delete mSavedTEMap[te];
		}
	
		LLUUID textureid(uuid_buffer);
		mTEMap[te] = new LLLocalTextureObject(image, textureid);
		mSavedTEMap[te] = new LLLocalTextureObject(image, textureid);
		createLayers(te, avatarp);
	}

	// copy all saved param values to working params
	revertValues();

	return LLWearable::SUCCESS;
}

BOOL LLWearable::getNextPopulatedLine(std::istream& input_stream, char* buffer, U32 buffer_size)
{
	if (!input_stream.good())
	{
		return FALSE;
	}

	do 
	{
		input_stream.getline(buffer, buffer_size);
	}
	while (input_stream.good() && buffer[0]=='\0');

	return (buffer[0] != '\0'); 
}


void LLWearable::setType(LLWearableType::EType type, LLAvatarAppearance *avatarp) 
{ 
	mType = type; 
	createVisualParams(avatarp);
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

const LLLocalTextureObject* LLWearable::getLocalTextureObject(S32 index) const
{
	te_map_t::const_iterator iter = mTEMap.find(index);
	if( iter != mTEMap.end() )
	{
		const LLLocalTextureObject* lto = iter->second;
		return lto;
	}
	return NULL;
}

std::vector<LLLocalTextureObject*> LLWearable::getLocalTextureListSeq()
{
	std::vector<LLLocalTextureObject*> result;

	for(te_map_t::const_iterator iter = mTEMap.begin();
		iter != mTEMap.end(); iter++)
	{
		LLLocalTextureObject* lto = iter->second;
		result.push_back(lto);
	}

	return result;
}

void LLWearable::setLocalTextureObject(S32 index, LLLocalTextureObject &lto)
{
	if( mTEMap.find(index) != mTEMap.end() )
	{
		mTEMap.erase(index);
	}
	mTEMap[index] = new LLLocalTextureObject(lto);
}

void LLWearable::revertValues()
{
	// FIXME DRANO - this triggers changes to driven params on avatar, potentially clobbering baked appearance.

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
}

void LLWearable::syncImages(te_map_t &src, te_map_t &dst)
{
	// Deep copy of src (copies only those tes that are current, filling in defaults where needed)
	for( S32 te = 0; te < TEX_NUM_INDICES; te++ )
	{
		if (LLAvatarAppearanceDictionary::getTEWearableType((ETextureIndex) te) == mType)
		{
			te_map_t::const_iterator iter = src.find(te);
			LLUUID image_id;
			LLGLTexture *image = NULL;
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
				image_id = getDefaultTextureImageID((ETextureIndex) te);
				image = gTextureManagerBridgep->getFetchedTexture( image_id );
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

void LLWearable::addVisualParam(LLVisualParam *param)
{
	if( mVisualParamIndexMap[param->getID()] )
	{
		delete mVisualParamIndexMap[param->getID()];
	}
	param->setIsDummy(FALSE);
	param->setParamLocation(LOC_WEARABLE);
	mVisualParamIndexMap[param->getID()] = param;
	mSavedVisualParamMap[param->getID()] = param->getDefaultWeight();
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
	if( LLAvatarAppearance::teToColorParams( (LLAvatarAppearanceDefines::ETextureIndex)te, param_name ) )
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
	if( LLAvatarAppearance::teToColorParams( (LLAvatarAppearanceDefines::ETextureIndex)te, param_name ) )
	{
		for( U8 index = 0; index < 3; index++ )
		{
			setVisualParamWeight(param_name[index], new_color.mV[index], upload_bake);
		}
	}
}

void LLWearable::writeToAvatar(LLAvatarAppearance* avatarp)
{
	if (!avatarp) return;

	// Pull params
	for( LLVisualParam* param = avatarp->getFirstVisualParam(); param; param = avatarp->getNextVisualParam() )
	{
		// cross-wearable parameters are not authoritative, as they are driven by a different wearable. So don't copy the values to the
		// avatar object if cross wearable. Cross wearable params get their values from the avatar, they shouldn't write the other way.
		if( (((LLViewerVisualParam*)param)->getWearableType() == mType) && (!((LLViewerVisualParam*)param)->getCrossWearable()) )
		{
			S32 param_id = param->getID();
			F32 weight = getVisualParamWeight(param_id);

			avatarp->setVisualParamWeight( param_id, weight, FALSE );
		}
	}
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

