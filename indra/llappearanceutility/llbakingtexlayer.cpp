/** 
 * @file llbakingtexlayer.cpp
 * @brief Implementation of LLBakingTexLayer class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "indra_constants.h"
#include "llappappearanceutility.h"
#include "llavatarappearance.h"
#include "llbakingtexlayer.h"
#include "llbakingwearable.h"
#include "llfasttimer.h"
#include "lllocaltextureobject.h"
#include "llimagej2c.h"
#include "llwearabledata.h"
#include "llmd5.h"
#include "llbakingavatar.h"

//#define DEBUG_TEXTURE_IDS 1

static const char* BAKE_HASH_VERSION = "3";

using namespace LLAvatarAppearanceDefines;

LLBakingTexLayerSetBuffer::LLBakingTexLayerSetBuffer(LLTexLayerSet* const owner, S32 width, S32 height)
	: LLTexLayerSetBuffer(owner),
	  LLBakingTexture( width, height, 4 )
{}

// virtual
LLBakingTexLayerSetBuffer::~LLBakingTexLayerSetBuffer()
{
	destroyGLTexture();
}

BOOL LLBakingTexLayerSetBuffer::render()
{
	BOOL result = FALSE;

	preRenderTexLayerSet();
	result = renderTexLayerSet();
	postRenderTexLayerSet(result);

	return result;
}

static LLFastTimer::DeclareTimer FTM_MID_RENDER("midRenderTexLayerSet");
static LLFastTimer::DeclareTimer FTM_CREATE_J2C("Encode J2C image.");
void LLBakingTexLayerSetBuffer::midRenderTexLayerSet(BOOL success)
{
	LL_RECORD_BLOCK_TIME(FTM_MID_RENDER);
	if (!mTexLayerSet->isVisible())
	{
		// Should have used IMG_INVISIBLE during hash id generation?
		LL_ERRS() << "Rendered texture for non-visible tex layer set!" << LL_ENDL;
	}

	if (!success)
	{
		throw LLAppException(RV_UNABLE_TO_BAKE, " Rendering process failed.");
	}

	// Get the COLOR information from our texture
	LL_DEBUGS() << "glReadPixels..." << LL_ENDL;
	LL_DEBUGS() << "getCompositeWidth()" << getCompositeWidth() << "getCompositeHeight()" << getCompositeHeight() << LL_ENDL;

	U8* baked_color_data = new U8[ getCompositeWidth() * getCompositeHeight() * 4 ];
	glReadPixels(getCompositeOriginX(), getCompositeOriginY(),
				 getCompositeWidth(), getCompositeHeight(),
				 GL_RGBA, GL_UNSIGNED_BYTE, baked_color_data );
	stop_glerror();

	// Get the MASK information from our texture
	LLGLSUIDefault gls_ui;
	LL_DEBUGS() << "Creating baked mask image raw.." << LL_ENDL;
	LLPointer<LLImageRaw> baked_mask_image = new LLImageRaw(getCompositeWidth(), getCompositeHeight(), 1 );
	U8* baked_mask_data = baked_mask_image->getData(); 

	LL_DEBUGS() << "Gathering Morph Mask Alpha..." << LL_ENDL;
	mTexLayerSet->gatherMorphMaskAlpha(baked_mask_data,
			getCompositeOriginX(), getCompositeOriginY(),
			getCompositeWidth(), getCompositeHeight());
	
	// Create the baked image from our color and mask information
	const S32 baked_image_components = 5; // red green blue [bump] clothing
	LL_DEBUGS() << "Creating baked image raw.." << LL_ENDL;
	LLPointer<LLImageRaw> baked_image = new LLImageRaw( getCompositeWidth(), getCompositeHeight(), baked_image_components );
	U8* baked_image_data = baked_image->getData();
	S32 i = 0;
	LL_DEBUGS() << "Running ugly loop..." << LL_ENDL;
	for (S32 u=0; u < getCompositeWidth(); u++)
	{
		for (S32 v=0; v < getCompositeHeight(); v++)
		{
			baked_image_data[5*i + 0] = baked_color_data[4*i + 0];
			baked_image_data[5*i + 1] = baked_color_data[4*i + 1];
			baked_image_data[5*i + 2] = baked_color_data[4*i + 2];
			baked_image_data[5*i + 3] = baked_color_data[4*i + 3]; // alpha should be correct for eyelashes.
			baked_image_data[5*i + 4] = baked_mask_data[i];
			i++;
		}
	}

	{
		LL_RECORD_BLOCK_TIME(FTM_CREATE_J2C);
		LL_DEBUGS() << "Creating J2C..." << LL_ENDL;
		mCompressedImage = new LLImageJ2C;
		const char* comment_text = LINDEN_J2C_COMMENT_PREFIX "RGBHM"; // writes into baked_color_data. 5 channels (rgb, heightfield/alpha, mask)
		if (!mCompressedImage->encode(baked_image, comment_text))
		{
			throw LLAppException(RV_UNABLE_TO_ENCODE);
		}
	}

	delete [] baked_color_data;
}

LLBakingTexLayerSet::LLBakingTexLayerSet(LLAvatarAppearance* const appearance) :
	LLTexLayerSet(appearance)
{
}

// virtual
LLBakingTexLayerSet::~LLBakingTexLayerSet()
{
}

// Ignored.
void LLBakingTexLayerSet::requestUpdate()
{
}

void LLBakingTexLayerSet::createComposite()
{
	if(!mComposite)
	{
		const LLBakingAvatar* avatar = dynamic_cast<const LLBakingAvatar*> (mAvatarAppearance);
		S32 width = avatar->bakeTextureSize();
		S32 height = avatar->bakeTextureSize();

		LL_DEBUGS() << "Creating composite with width and height " << width << " " << height << LL_ENDL;

		mComposite = new LLBakingTexLayerSetBuffer(this, width, height);
	}
}

LLSD LLBakingTexLayerSet::computeTextureIDs() const
{
	const LLAvatarAppearanceDictionary::BakedEntry *baked_dict = 
			LLAvatarAppearanceDictionary::getInstance()->getBakedTexture(getBakedTexIndex());
#ifdef DEBUG_TEXTURE_IDS
	const std::string& slot_name = LLAvatarAppearanceDictionary::getInstance()->getTexture(
		LLAvatarAppearanceDictionary::bakedToLocalTextureIndex(slot_name()))->mDefaultImageName;
	const LLAvatarAppearanceDictionary::TextureEntry* entry = LLAvatarAppearanceDictionary::getInstance()->getTexture(baked_dict->mTextureIndex);
	LL_DEBUGS() << "-----------------------------------------------------------------" << LL_ENDL;
	LL_DEBUGS() << "BakedTexIndex = " << (S32) getBakedTexIndex() << " (" << slot_name << ") "
			<< "BakedEntry::mTextureIndex = " << (S32) baked_dict->mTextureIndex 
			<< " (" << entry->mDefaultImageName 
			<< ")" << LL_ENDL;
	//LL_DEBUGS() << "LLAvatarAppearanceDictionary::getTEWearableType(" << baked_dict->mTextureIndex << ") = " << (S32) LLAvatarAppearanceDictionary::getTEWearableType(baked_dict->mTextureIndex) << LL_ENDL;

	texture_vec_t::const_iterator idx_iter = baked_dict->mLocalTextures.begin();
	texture_vec_t::const_iterator idx_end = baked_dict->mLocalTextures.end();
	std::ostringstream idx_str;
	for (; idx_iter != idx_end; ++idx_iter)
	{
		entry = LLAvatarAppearanceDictionary::getInstance()->getTexture((*idx_iter));
		idx_str << (*idx_iter) << " (" << entry->mDefaultImageName 
				<< ";" << LLWearableType::getTypeName( entry->mWearableType )
				<< "), ";
	}
	LL_DEBUGS() << "BakedEntry::mLocalTextures: " << idx_str.str() << LL_ENDL;
	wearables_vec_t::const_iterator type_iter = baked_dict->mWearables.begin();
	wearables_vec_t::const_iterator type_end = baked_dict->mWearables.end();
	std::ostringstream type_str;
	for (; type_iter != type_end; ++type_iter)
	{
		type_str << (S32) (*type_iter)  << " ("
				<< LLWearableType::getTypeName( (*type_iter) )
				<< "), ";
	}
	LL_DEBUGS() << "BakedEntry::mWearables: " << type_str.str() << LL_ENDL;
	//LL_DEBUGS() << "BakedEntry::mWearablesHashID: " << baked_dict->mWearablesHashID << LL_ENDL;
#endif

	LLMD5 hash;
	std::set<LLUUID> texture_ids;

	bool is_visible = true;
	bool hash_computed = computeLayerListTextureIDs(hash, texture_ids, mLayerList, is_visible);
	if (is_visible)
	{
		hash_computed |= computeLayerListTextureIDs(hash, texture_ids, mMaskLayerList, is_visible);
	}

	LLUUID hash_id;
	if (!is_visible)
	{
		hash_id = IMG_INVISIBLE;
		texture_ids.clear();
	}
	else if (hash_computed)
	{
		if ((getBakedTexIndex() == EBakedTextureIndex::BAKED_LEFT_ARM) ||
			(getBakedTexIndex() == EBakedTextureIndex::BAKED_LEFT_LEG) ||
			(getBakedTexIndex() == EBakedTextureIndex::BAKED_AUX1) ||
			(getBakedTexIndex() == EBakedTextureIndex::BAKED_AUX2) ||
			(getBakedTexIndex() == EBakedTextureIndex::BAKED_AUX3))
		{
			hash.update(BAKE_HASH_VERSION);
		}

		hash.update((const unsigned char*)baked_dict->mWearablesHashID.mData, UUID_BYTES);
		hash.finalize();
		hash.raw_digest(hash_id.mData);
	}

	if (hash_id.isNull())
	{
		hash_id = IMG_DEFAULT_AVATAR;
	}
	
	if (getBakedTexIndex() == EBakedTextureIndex::BAKED_SKIRT)
	{
		if (getAvatarAppearance()->getWearableData()->getWearableCount(LLWearableType::WT_SKIRT) == 0)
		{
			hash_id = IMG_DEFAULT_AVATAR;
			texture_ids.clear();
		}
	}
	LLSD sd;
	sd["hash_id"] = hash_id;
	sd["texture_ids"] = LLSD::emptyArray();
	std::set<LLUUID>::iterator id_iter = texture_ids.begin();
	std::set<LLUUID>::iterator id_end  = texture_ids.end();
	for (; id_iter != id_end; ++id_iter)
	{
		sd["texture_ids"].append( *id_iter );
	}
	return sd;
}

bool LLBakingTexLayerSet::computeLayerListTextureIDs(LLMD5& hash,
		std::set<LLUUID>& texture_ids,
		const layer_list_t& layer_list,
		bool& is_visible) const
{
	is_visible = true;
	bool hash_computed = false;
	layer_list_t::const_iterator iter = layer_list.begin();
	layer_list_t::const_iterator end  = layer_list.end();
	for (; iter != end; ++iter)
	{
		const LLTexLayerInterface* layer_template = *iter;
		LLWearableType::EType wearable_type = layer_template->getWearableType();
		ETextureIndex te = layer_template->getLocalTextureIndex();
		U32 num_wearables = getAvatarAppearance()->getWearableData()->getWearableCount(wearable_type);
#ifdef DEBUG_TEXTURE_IDS
		const LLAvatarAppearanceDictionary::TextureEntry* entry = LLAvatarAppearanceDictionary::getInstance()->getTexture(te);
		if (entry)
		{
			LL_DEBUGS() << "LLTexLayerInterface(" << layer_template->getName() << ") type = " 
					<< (S32) wearable_type << " ("
					<< LLWearableType::getTypeName( wearable_type ) << ") te = "
					<< (S32) te << " ("<< entry->mDefaultImageName 
					<< ";" << LLWearableType::getTypeName( entry->mWearableType )
					<< ") wearable_count = " << num_wearables << LL_ENDL;
		}
		else
		{
			LL_DEBUGS() << "LLTexLayerInterface(" << layer_template->getName() << ") type = " 
					<< (S32) wearable_type << " ("
					<< LLWearableType::getTypeName( wearable_type )
					<< ") wearable_count = " << num_wearables << LL_ENDL;
		}
#endif
		if (LLWearableType::WT_INVALID == wearable_type)
		{
			//LL_DEBUGS() << "Skipping invalid wearable type" << LL_ENDL;
			continue;
		}

		for (U32 i = 0; i < num_wearables; i++)
		{
			LLBakingWearable* wearable = dynamic_cast<LLBakingWearable*> (
							getAvatarAppearance()->getWearableData()->getWearable(wearable_type, i));
			if (!wearable)
			{
				//LL_DEBUGS() << "Skipping wearable" << LL_ENDL;
				continue;
			}
#ifdef DEBUG_TEXTURE_IDS
			S32 num_ltos = wearable->getLocalTextureListSeq().size();
			LL_DEBUGS() << "LLWearable (" << wearable->getName() << ") type = " 
					<< (S32) wearable->getAssetType() << " ("
					<< LLAssetType::lookup(wearable->getAssetType())
					<< ") num_ltos = " << num_ltos << LL_ENDL;
#endif
			LLLocalTextureObject *lto = wearable->getLocalTextureObject(te);
			if (lto)
			{
#ifdef DEBUG_TEXTURE_IDS
				LL_DEBUGS() << "LocalTextureObject id = " << lto->getID() << " num texlayers = " << lto->getNumTexLayers() << LL_ENDL;
#endif
				// Hash texture id
				LLUUID texture_id = lto->getID();
				if (IMG_INVISIBLE == texture_id)
				{
					is_visible = false;
				}
				hash.update((const unsigned char*)texture_id.mData, UUID_BYTES);
				hash_computed = true;
				// Add texture to list.
				texture_ids.insert(texture_id);
			}
#ifdef DEBUG_TEXTURE_IDS
			else
			{
				LL_DEBUGS() << "LLWearable -- no LocalTextureObject for te " << (S32) te << LL_ENDL;
			}


			for(S32 i = 0; i < (S32)TEX_NUM_INDICES; i++)
			{
				lto = wearable->getLocalTextureObject(i);
				if (lto)
				{
					LL_DEBUGS() << "LTO (" << i << ") id = " << lto->getID() << " num texlayers = " << lto->getNumTexLayers() << LL_ENDL;
				}
			}
#endif

			for( LLVisualParam* param = getAvatarAppearance()->getFirstVisualParam(); 
				 param; param = getAvatarAppearance()->getNextVisualParam() )
			{
				// cross-wearable parameters are not authoritative, as they are driven by a different wearable.
				if( (((LLViewerVisualParam*)param)->getWearableType() == wearable_type) && 
					(!((LLViewerVisualParam*)param)->getCrossWearable()) )
				{
					std::ostringstream ostr;
					F32 weight = wearable->getVisualParamWeight(param->getID());
					ostr << param->getID() << " " << weight;
					hash.update(ostr.str());
					hash_computed = true;
				}
			}
		}
	}
	return hash_computed;
}

