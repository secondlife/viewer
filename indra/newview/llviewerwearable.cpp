/** 
 * @file llviewerwearable.cpp
 * @brief LLViewerWearable class implementation
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llfloatersidepanelcontainer.h"
#include "llnotificationsutil.h"
#include "llsidepanelappearance.h"
#include "lltextureentry.h"
#include "llviewertexlayer.h"
#include "llvoavatarself.h"
#include "llavatarappearancedefines.h"
#include "llviewerwearable.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"

using namespace LLAvatarAppearanceDefines;

// support class - remove for 2.1 (hackity hack hack)
class LLOverrideBakedTextureUpdate
{
public:
	LLOverrideBakedTextureUpdate(bool temp_state)
	{
		U32 num_bakes = (U32) LLAvatarAppearanceDefines::BAKED_NUM_INDICES;
		for( U32 index = 0; index < num_bakes; ++index )
		{
			composite_enabled[index] = gAgentAvatarp->isCompositeUpdateEnabled(index);
		}
		gAgentAvatarp->setCompositeUpdatesEnabled(temp_state);
	}

	~LLOverrideBakedTextureUpdate()
	{
		U32 num_bakes = (U32)LLAvatarAppearanceDefines::BAKED_NUM_INDICES;		
		for( U32 index = 0; index < num_bakes; ++index )
		{
			gAgentAvatarp->setCompositeUpdatesEnabled(index, composite_enabled[index]);
		}
	}
private:
	bool composite_enabled[LLAvatarAppearanceDefines::BAKED_NUM_INDICES];
};

// Private local functions
static std::string asset_id_to_filename(const LLUUID &asset_id);

LLViewerWearable::LLViewerWearable(const LLTransactionID& transaction_id) :
	LLWearable()
{
	mTransactionID = transaction_id;
	mAssetID = mTransactionID.makeAssetID(gAgent.getSecureSessionID());
}

LLViewerWearable::LLViewerWearable(const LLAssetID& asset_id) :
	LLWearable()
{
	mAssetID = asset_id;
	mTransactionID.setNull();
}

// virtual
LLViewerWearable::~LLViewerWearable()
{
}

// virtual
LLWearable::EImportResult LLViewerWearable::importStream( std::istream& input_stream, LLAvatarAppearance* avatarp )
{
	// suppress texlayerset updates while wearables are being imported. Layersets will be updated
	// when the wearables are "worn", not loaded. Note state will be restored when this object is destroyed.
	LLOverrideBakedTextureUpdate stop_bakes(false);

	LLWearable::EImportResult result = LLWearable::importStream(input_stream, avatarp);
	if (LLWearable::FAILURE == result) return result;
	if (LLWearable::BAD_HEADER == result)
	{
		// Shouldn't really log the asset id for security reasons, but
		// we need it in this case.
		llwarns << "Bad Wearable asset header: " << mAssetID << llendl;
		//gVFS->dumpMap();
		return result;
	}

	LLStringUtil::truncate(mName, DB_INV_ITEM_NAME_STR_LEN );
	LLStringUtil::truncate(mDescription, DB_INV_ITEM_DESC_STR_LEN );

	te_map_t::const_iterator iter = mTEMap.begin();
	te_map_t::const_iterator end = mTEMap.end();
	for (; iter != end; ++iter)
	{
		S32 te = iter->first;
		LLLocalTextureObject* lto = iter->second;
		LLUUID textureid = LLUUID::null;
		if (lto)
		{
			textureid = lto->getID();
		}

		LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTexture( textureid );
		if(gSavedSettings.getBOOL("DebugAvatarLocalTexLoadedTime"))
		{
			image->setLoadedCallback(LLVOAvatarSelf::debugOnTimingLocalTexLoaded,0,TRUE,FALSE, new LLVOAvatarSelf::LLAvatarTexData(textureid, (LLAvatarAppearanceDefines::ETextureIndex)te), NULL);
		}
	}

	return result;
}


// Avatar parameter and texture definitions can change over time.
// This function returns true if parameters or textures have been added or removed
// since this wearable was created.
BOOL LLViewerWearable::isOldVersion() const
{
	if (!isAgentAvatarValid()) return FALSE;

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
	for( LLViewerVisualParam* param = (LLViewerVisualParam*) gAgentAvatarp->getFirstVisualParam(); 
		param;
		param = (LLViewerVisualParam*) gAgentAvatarp->getNextVisualParam() )
	{
		if( (param->getWearableType() == mType) && (param->isTweakable() ) )
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
		if (LLAvatarAppearanceDictionary::getTEWearableType((ETextureIndex) te) == mType)
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
BOOL LLViewerWearable::isDirty() const
{
	if (!isAgentAvatarValid()) return FALSE;

	for( LLViewerVisualParam* param = (LLViewerVisualParam*) gAgentAvatarp->getFirstVisualParam(); 
		param;
		param = (LLViewerVisualParam*) gAgentAvatarp->getNextVisualParam() )
	{
		if( (param->getWearableType() == mType) 
			&& (param->isTweakable() ) 
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
		if (LLAvatarAppearanceDictionary::getTEWearableType((ETextureIndex) te) == mType)
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

	return FALSE;
}


void LLViewerWearable::setParamsToDefaults()
{
	if (!isAgentAvatarValid()) return;

	for( LLVisualParam* param = gAgentAvatarp->getFirstVisualParam(); param; param = gAgentAvatarp->getNextVisualParam() )
	{
		if( (((LLViewerVisualParam*)param)->getWearableType() == mType ) && (param->isTweakable() ) )
		{
			setVisualParamWeight(param->getID(),param->getDefaultWeight(), FALSE);
		}
	}
}

void LLViewerWearable::setTexturesToDefaults()
{
	for( S32 te = 0; te < TEX_NUM_INDICES; te++ )
	{
		if (LLAvatarAppearanceDictionary::getTEWearableType((ETextureIndex) te) == mType)
		{
			LLUUID id = getDefaultTextureImageID((ETextureIndex) te);
			LLViewerFetchedTexture * image = LLViewerTextureManager::getFetchedTexture( id );
			if( mTEMap.find(te) == mTEMap.end() )
			{
				mTEMap[te] = new LLLocalTextureObject(image, id);
				createLayers(te, gAgentAvatarp);
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


// virtual
LLUUID LLViewerWearable::getDefaultTextureImageID(ETextureIndex index) const
{
	const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = LLAvatarAppearanceDictionary::getInstance()->getTexture(index);
	const std::string &default_image_name = texture_dict->mDefaultImageName;
	if (default_image_name == "")
	{
		return IMG_DEFAULT_AVATAR;
	}
	else
	{
		return LLUUID(gSavedSettings.getString(default_image_name));
	}
}


// Updates the user's avatar's appearance
//virtual
void LLViewerWearable::writeToAvatar(LLAvatarAppearance *avatarp)
{
	LLVOAvatarSelf* viewer_avatar = dynamic_cast<LLVOAvatarSelf*>(avatarp);

	if (!avatarp || !viewer_avatar) return;

	if (!viewer_avatar->isValid()) return;

#if 0
	// FIXME DRANO - kludgy way to avoid overwriting avatar state from wearables.
	// Ideally would avoid calling this func in the first place.
	if (viewer_avatar->isUsingServerBakes() &&
		!viewer_avatar->isUsingLocalAppearance())
	{
		return;
	}
#endif

	ESex old_sex = avatarp->getSex();

	LLWearable::writeToAvatar(avatarp);


	// Pull texture entries
	for( S32 te = 0; te < TEX_NUM_INDICES; te++ )
	{
		if (LLAvatarAppearanceDictionary::getTEWearableType((ETextureIndex) te) == mType)
		{
			te_map_t::const_iterator iter = mTEMap.find(te);
			LLUUID image_id;
			if(iter != mTEMap.end())
			{
				image_id = iter->second->getID();
			}
			else
			{	
				image_id = getDefaultTextureImageID((ETextureIndex) te);
			}
			LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture( image_id, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE );
			// MULTI-WEARABLE: assume index 0 will be used when writing to avatar. TODO: eliminate the need for this.
			viewer_avatar->setLocalTextureTE(te, image, 0);
		}
	}

	ESex new_sex = avatarp->getSex();
	if( old_sex != new_sex )
	{
		viewer_avatar->updateSexDependentLayerSets( FALSE );
	}	
	
//	if( upload_bake )
//	{
//		gAgent.sendAgentSetAppearance();
//	}
}


// Updates the user's avatar's appearance, replacing this wearables' parameters and textures with default values.
// static 
void LLViewerWearable::removeFromAvatar( LLWearableType::EType type, BOOL upload_bake )
{
	if (!isAgentAvatarValid()) return;

	// You can't just remove body parts.
	if( (type == LLWearableType::WT_SHAPE) ||
		(type == LLWearableType::WT_SKIN) ||
		(type == LLWearableType::WT_HAIR) ||
		(type == LLWearableType::WT_EYES) )
	{
		return;
	}

	// Pull params
	for( LLVisualParam* param = gAgentAvatarp->getFirstVisualParam(); param; param = gAgentAvatarp->getNextVisualParam() )
	{
		if( (((LLViewerVisualParam*)param)->getWearableType() == type) && (param->isTweakable() ) )
		{
			S32 param_id = param->getID();
			gAgentAvatarp->setVisualParamWeight( param_id, param->getDefaultWeight(), upload_bake );
		}
	}

	if(gAgentCamera.cameraCustomizeAvatar())
	{
		LLFloaterSidePanelContainer::showPanel("appearance", LLSD().with("type", "edit_outfit"));
	}

	gAgentAvatarp->updateVisualParams();
	gAgentAvatarp->wearableUpdated(type, FALSE);

//	if( upload_bake )
//	{
//		gAgent.sendAgentSetAppearance();
//	}
}

// Does not copy mAssetID.
// Definition version is current: removes obsolete enties and creates default values for new ones.
void LLViewerWearable::copyDataFrom(const LLViewerWearable* src)
{
	if (!isAgentAvatarValid()) return;

	mDefinitionVersion = LLWearable::sCurrentDefinitionVersion;

	mName = src->mName;
	mDescription = src->mDescription;
	mPermissions = src->mPermissions;
	mSaleInfo = src->mSaleInfo;

	setType(src->mType, gAgentAvatarp);

	mSavedVisualParamMap.clear();
	// Deep copy of mVisualParamMap (copies only those params that are current, filling in defaults where needed)
	for (LLViewerVisualParam* param = (LLViewerVisualParam*) gAgentAvatarp->getFirstVisualParam(); 
		param;
		param = (LLViewerVisualParam*) gAgentAvatarp->getNextVisualParam() )
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
	for (S32 te = 0; te < TEX_NUM_INDICES; te++)
	{
		if (LLAvatarAppearanceDictionary::getTEWearableType((ETextureIndex) te) == mType)
		{
			te_map_t::const_iterator iter = src->mTEMap.find(te);
			LLUUID image_id;
			LLViewerFetchedTexture *image = NULL;
			if(iter != src->mTEMap.end())
			{
				image = dynamic_cast<LLViewerFetchedTexture*> (src->getLocalTextureObject(te)->getImage());
				image_id = src->getLocalTextureObject(te)->getID();
				mTEMap[te] = new LLLocalTextureObject(image, image_id);
				mSavedTEMap[te] = new LLLocalTextureObject(image, image_id);
				mTEMap[te]->setBakedReady(src->getLocalTextureObject(te)->getBakedReady());
				mTEMap[te]->setDiscard(src->getLocalTextureObject(te)->getDiscard());
			}
			else
			{
				image_id = getDefaultTextureImageID((ETextureIndex) te);
				image = LLViewerTextureManager::getFetchedTexture( image_id );
				mTEMap[te] = new LLLocalTextureObject(image, image_id);
				mSavedTEMap[te] = new LLLocalTextureObject(image, image_id);
			}
			createLayers(te, gAgentAvatarp);
		}
	}

	// Probably reduntant, but ensure that the newly created wearable is not dirty by setting current value of params in new wearable
	// to be the same as the saved values (which were loaded from src at param->cloneParam(this))
	revertValues();
}

void LLViewerWearable::setItemID(const LLUUID& item_id)
{
	mItemID = item_id;
}

void LLViewerWearable::revertValues()
{
#if 0
	// DRANO avoid overwrite when not in local appearance
	if (isAgentAvatarValid() && gAgentAvatarp->isUsingServerBakes() && !gAgentAvatarp->isUsingLocalAppearance())
	{
		return;
	}
#endif
	LLWearable::revertValues();


	LLSidepanelAppearance *panel = dynamic_cast<LLSidepanelAppearance*>(LLFloaterSidePanelContainer::getPanel("appearance"));
	if( panel )
	{
		panel->updateScrollingPanelList();
	}
}

void LLViewerWearable::saveValues()
{
	LLWearable::saveValues();

	LLSidepanelAppearance *panel = dynamic_cast<LLSidepanelAppearance*>(LLFloaterSidePanelContainer::getPanel("appearance"));
	if( panel )
	{
		panel->updateScrollingPanelList();
	}
}

// virtual
void LLViewerWearable::setUpdated() const
{ 
	gInventory.addChangedMask(LLInventoryObserver::LABEL, getItemID());
}

void LLViewerWearable::refreshName()
{
	LLUUID item_id = getItemID();
	LLInventoryItem* item = gInventory.getItem(item_id);
	if( item )
	{
		mName = item->getName();
	}
}

// virtual
void LLViewerWearable::addToBakedTextureHash(LLMD5& hash) const
{
	LLUUID asset_id = getAssetID();
	hash.update((const unsigned char*)asset_id.mData, UUID_BYTES);
}

struct LLWearableSaveData
{
	LLWearableType::EType mType;
};

void LLViewerWearable::saveNewAsset() const
{
//	llinfos << "LLViewerWearable::saveNewAsset() type: " << getTypeName() << llendl;
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
                                     &LLViewerWearable::onSaveNewAssetComplete,
                                     (void*)data);
	}
}

// static
void LLViewerWearable::onSaveNewAssetComplete(const LLUUID& new_asset_id, void* userdata, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLWearableSaveData* data = (LLWearableSaveData*)userdata;
	const std::string& type_name = LLWearableType::getTypeName(data->mType);
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

std::ostream& operator<<(std::ostream &s, const LLViewerWearable &w)
{
	s << "wearable " << LLWearableType::getTypeName(w.mType) << "\n";
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
	for (LLViewerWearable::te_map_t::const_iterator iter = w.mTEMap.begin();
		 iter != w.mTEMap.end(); ++iter)
	{
		S32 te = iter->first;
		const LLUUID& image_id = iter->second->getID();
		s << "        " << te << " " << image_id << "\n";
	}
	return s;
}

std::string asset_id_to_filename(const LLUUID &asset_id)
{
	std::string asset_id_string;
	asset_id.toString(asset_id_string);
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,asset_id_string) + ".wbl";	
	return filename;
}
