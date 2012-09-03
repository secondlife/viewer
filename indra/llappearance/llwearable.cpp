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
#include "llvoavatardefines.h"
#include "llwearable.h"

using namespace LLVOAvatarDefines;

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

// virtual
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
	S32 num_textures = mTextureIDMap.size();
	if( fprintf( file, "textures %d\n", num_textures ) < 0 )
	{
		return FALSE;
	}
	
	for (texture_id_map_t::const_iterator iter = mTextureIDMap.begin(); iter != mTextureIDMap.end(); ++iter)
	{
		S32 te = iter->first;
		const LLUUID& image_id = iter->second;
		if( fprintf( file, "%d %s\n", te, image_id.asString().c_str()) < 0 )
		{
			return FALSE;
		}
	}
	return TRUE;
}


// virtual
LLWearable::EImportResult LLWearable::importFile( LLFILE* file )
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
		return LLWearable::BAD_HEADER;
	}


	// Temporary hack to allow wearables with definition version 24 to still load.
	// This should only affect lindens and NDA'd testers who have saved wearables in 2.0
	// the extra check for version == 24 can be removed before release, once internal testers
	// have loaded these wearables again. See hack pt 2 at bottom of function to ensure that
	// these wearables get re-saved with version definition 22.
	if( mDefinitionVersion > LLWearable::sCurrentDefinitionVersion && mDefinitionVersion != 24 )
	{
		llwarns << "Wearable asset has newer version (" << mDefinitionVersion << ") than XML (" << LLWearable::sCurrentDefinitionVersion << ")" << llendl;
		return LLWearable::FAILURE;
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
			return LLWearable::FAILURE;
		}
		mName = text_buffer;
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
			return LLWearable::FAILURE;
		}
		mDescription = text_buffer;
	}

	// permissions
	S32 perm_version;
	fields_read = fscanf( file, " permissions %d\n", &perm_version );
	if( (fields_read != 1) || (perm_version != 0) )
	{
		llwarns << "Bad Wearable asset: missing permissions" << llendl;
		return LLWearable::FAILURE;
	}
	if( !mPermissions.importFile( file ) )
	{
		return LLWearable::FAILURE;
	}

	// sale info
	S32 sale_info_version;
	fields_read = fscanf( file, " sale_info %d\n", &sale_info_version );
	if( (fields_read != 1) || (sale_info_version != 0) )
	{
		llwarns << "Bad Wearable asset: missing sale_info" << llendl;
		return LLWearable::FAILURE;
	}
	// Sale info used to contain next owner perm. It is now in the
	// permissions. Thus, we read that out, and fix legacy
	// objects. It's possible this op would fail, but it should pick
	// up the vast majority of the tasks.
	BOOL has_perm_mask = FALSE;
	U32 perm_mask = 0;
	if( !mSaleInfo.importFile(file, has_perm_mask, perm_mask) )
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
	S32 type = -1;
	fields_read = fscanf( file, "type %d\n", &type );
	if( fields_read != 1 )
	{
		llwarns << "Bad Wearable asset: bad type" << llendl;
		return LLWearable::FAILURE;
	}
	if( 0 <= type && type < LLWearableType::WT_COUNT )
	{
		setType((LLWearableType::EType)type);
	}
	else
	{
		mType = LLWearableType::WT_COUNT;
		llwarns << "Bad Wearable asset: bad type #" << type <<  llendl;
		return LLWearable::FAILURE;
	}

	// parameters header
	S32 num_parameters = 0;
	fields_read = fscanf( file, "parameters %d\n", &num_parameters );
	if( fields_read != 1 )
	{
		llwarns << "Bad Wearable asset: missing parameters block" << llendl;
		return LLWearable::FAILURE;
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
			return LLWearable::FAILURE;
		}
		mSavedVisualParamMap[param_id] = param_weight;
	}

	// textures header
	S32 num_textures = 0;
	fields_read = fscanf( file, "textures %d\n", &num_textures);
	if( fields_read != 1 )
	{
		llwarns << "Bad Wearable asset: missing textures block" << llendl;
		return LLWearable::FAILURE;
	}

	// textures
	mTextureIDMap.clear();
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
			return LLWearable::FAILURE;
		}

		if( !LLUUID::validate( text_buffer ) )
		{
			llwarns << "Bad Wearable asset: bad texture uuid: " << text_buffer << llendl;
			return LLWearable::FAILURE;
		}
		LLUUID id = LLUUID(text_buffer);
		mTextureIDMap[te] = id;
	}

	return LLWearable::SUCCESS;
}


void LLWearable::setType(LLWearableType::EType type) 
{ 
	mType = type; 
	createVisualParams();
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
	if( LLAvatarAppearance::teToColorParams( (LLVOAvatarDefines::ETextureIndex)te, param_name ) )
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
	if( LLAvatarAppearance::teToColorParams( (LLVOAvatarDefines::ETextureIndex)te, param_name ) )
	{
		for( U8 index = 0; index < 3; index++ )
		{
			setVisualParamWeight(param_name[index], new_color.mV[index], upload_bake);
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

