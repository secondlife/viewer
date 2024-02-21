/** 
 * @file lllocalbitmaps.cpp
 * @author Vaalith Jinn
 * @brief Local Bitmaps source
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

/* precompiled headers */
#include "llviewerprecompiledheaders.h"

/* own header */
#include "lllocalbitmaps.h"

/* boost: will not compile unless equivalent is undef'd, beware. */
#include "fix_macros.h"
#include <boost/filesystem.hpp>

/* image compression headers. */
#include "llimagebmp.h"
#include "llimagetga.h"
#include "llimagejpeg.h"
#include "llimagepng.h"

/* time headers */
#include <time.h>
#include <ctime>

/* misc headers */
#include "llgltfmaterial.h"
#include "llscrolllistctrl.h"
#include "lllocaltextureobject.h"
#include "llviewertexturelist.h"
#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llface.h"
#include "llvoavatarself.h"
#include "llviewerwearable.h"
#include "llagentwearables.h"
#include "lltexlayerparams.h"
#include "llvovolume.h"
#include "llnotificationsutil.h"
#include "pipeline.h"
#include "llmaterialmgr.h"
#include "llimagedimensionsinfo.h"
#include "llinventoryicon.h"
#include "llviewercontrol.h"
#include "lltrans.h"
#include "llviewerdisplay.h"

/*=======================================*/
/*  Formal declarations, constants, etc. */
/*=======================================*/ 

static const F32 LL_LOCAL_TIMER_HEARTBEAT   = 3.0;
static const bool LL_LOCAL_USE_MIPMAPS      = true;
static const S32 LL_LOCAL_DISCARD_LEVEL     = 0;
static const bool LL_LOCAL_SLAM_FOR_DEBUG   = true;
static const bool LL_LOCAL_REPLACE_ON_DEL   = true;
static const S32 LL_LOCAL_UPDATE_RETRIES    = 5;

/*=======================================*/
/*  LLLocalBitmap: unit class            */
/*=======================================*/ 
LLLocalBitmap::LLLocalBitmap(std::string filename)
	: mFilename(filename)
	, mShortName(gDirUtilp->getBaseFileName(filename, true))
	, mValid(false)
	, mLastModified()
	, mLinkStatus(LS_ON)
	, mUpdateRetries(LL_LOCAL_UPDATE_RETRIES)
{
	mTrackingID.generate();

	/* extension */
	std::string temp_exten = gDirUtilp->getExtension(mFilename);

	if (temp_exten == "bmp")
	{ 
		mExtension = ET_IMG_BMP;
	}
	else if (temp_exten == "tga")
	{
		mExtension = ET_IMG_TGA;
	}
	else if (temp_exten == "jpg" || temp_exten == "jpeg")
	{
		mExtension = ET_IMG_JPG;
	}
	else if (temp_exten == "png")
	{
		mExtension = ET_IMG_PNG;
	}
	else
	{
		LL_WARNS() << "File of no valid extension given, local bitmap creation aborted." << "\n"
			    << "Filename: " << mFilename << LL_ENDL;
		return; // no valid extension.
	}

	/* next phase of unit creation is nearly the same as an update cycle.
	   we're running updateSelf as a special case with the optional UT_FIRSTUSE
	   which omits the parts associated with removing the outdated texture */
	mValid = updateSelf(UT_FIRSTUSE);
}

LLLocalBitmap::~LLLocalBitmap()
{
	// replace IDs with defaults, if set to do so.
	if(LL_LOCAL_REPLACE_ON_DEL && mValid && gAgentAvatarp) // fix for STORM-1837
	{
		replaceIDs(mWorldID, IMG_DEFAULT);
		LLLocalBitmapMgr::getInstance()->doRebake();
	}

    for (LLPointer<LLGLTFMaterial> &mat : mGLTFMaterialWithLocalTextures)
    {
        mat->removeLocalTextureTracking(getTrackingID());
    }

    mChangedSignal(getTrackingID(), getWorldID(), LLUUID());
    mChangedSignal.disconnect_all_slots();

	// delete self from gimagelist
	LLViewerFetchedTexture* image = gTextureList.findImage(mWorldID, TEX_LIST_STANDARD);
	gTextureList.deleteImage(image);

	if (image)
	{
		image->unref();
	}
}

/* accessors */
std::string LLLocalBitmap::getFilename() const
{
	return mFilename;
}

std::string LLLocalBitmap::getShortName() const
{
	return mShortName;
}

LLUUID LLLocalBitmap::getTrackingID() const
{
	return mTrackingID;
}

LLUUID LLLocalBitmap::getWorldID() const
{
	return mWorldID;
}

bool LLLocalBitmap::getValid() const
{
	return mValid;
}

/* update functions */
bool LLLocalBitmap::updateSelf(EUpdateType optional_firstupdate)
{
	bool updated = false;
	
	if (mLinkStatus == LS_ON)
	{
		// verifying that the file exists
		if (gDirUtilp->fileExists(mFilename))
		{
			// verifying that the file has indeed been modified

#ifndef LL_WINDOWS
			const std::time_t temp_time = boost::filesystem::last_write_time(boost::filesystem::path(mFilename));
#else
			const std::time_t temp_time = boost::filesystem::last_write_time(boost::filesystem::path(utf8str_to_utf16str(mFilename)));
#endif
			LLSD new_last_modified = asctime(localtime(&temp_time));

			if (mLastModified.asString() != new_last_modified.asString())
			{
				/* loading the image file and decoding it, here is a critical point which,
				   if fails, invalidates the whole update (or unit creation) process. */
				LLPointer<LLImageRaw> raw_image = new LLImageRaw();
				if (decodeBitmap(raw_image))
				{
					// decode is successful, we can safely proceed.
					LLUUID old_id = LLUUID::null;
					if ((optional_firstupdate != UT_FIRSTUSE) && !mWorldID.isNull())
					{
						old_id = mWorldID;
					}
					mWorldID.generate();
					mLastModified = new_last_modified;

					LLPointer<LLViewerFetchedTexture> texture = new LLViewerFetchedTexture
						("file://"+mFilename, FTT_LOCAL_FILE, mWorldID, LL_LOCAL_USE_MIPMAPS);

					texture->createGLTexture(LL_LOCAL_DISCARD_LEVEL, raw_image);
					texture->setCachedRawImage(LL_LOCAL_DISCARD_LEVEL, raw_image);
					texture->ref(); 

					gTextureList.addImage(texture, TEX_LIST_STANDARD);
			
					if (optional_firstupdate != UT_FIRSTUSE)
					{
						// seek out everything old_id uses and replace it with mWorldID
						replaceIDs(old_id, mWorldID);

						// remove old_id from gimagelist
						LLViewerFetchedTexture* image = gTextureList.findImage(old_id, TEX_LIST_STANDARD);
						if (image != NULL)
						{
							gTextureList.deleteImage(image);
							image->unref();
						}
					}

					mUpdateRetries = LL_LOCAL_UPDATE_RETRIES;
					updated = true;
				}

				// if decoding failed, we get here and it will attempt to decode it in the next cycles
				// until mUpdateRetries runs out. this is done because some software lock the bitmap while writing to it
				else
				{
					if (mUpdateRetries)
					{
						mUpdateRetries--;
					}
					else
					{
						LL_WARNS() << "During the update process the following file was found" << "\n"
							    << "but could not be opened or decoded for " << LL_LOCAL_UPDATE_RETRIES << " attempts." << "\n"
								<< "Filename: " << mFilename << "\n"
								<< "Disabling further update attempts for this file." << LL_ENDL;

						LLSD notif_args;
						notif_args["FNAME"] = mFilename;
						notif_args["NRETRIES"] = LL_LOCAL_UPDATE_RETRIES;
						LLNotificationsUtil::add("LocalBitmapsUpdateFailedFinal", notif_args);

						mLinkStatus = LS_BROKEN;
					}
				}		
			}
			
		} // end if file exists

		else
		{
			LL_WARNS() << "During the update process, the following file was not found." << "\n" 
			        << "Filename: " << mFilename << "\n"
				    << "Disabling further update attempts for this file." << LL_ENDL;

			LLSD notif_args;
			notif_args["FNAME"] = mFilename;
			LLNotificationsUtil::add("LocalBitmapsUpdateFileNotFound", notif_args);

			mLinkStatus = LS_BROKEN;
		}
	}

	return updated;
}

boost::signals2::connection LLLocalBitmap::setChangedCallback(const LLLocalTextureCallback& cb)
{
    return mChangedSignal.connect(cb);
}

void LLLocalBitmap::addGLTFMaterial(LLGLTFMaterial* mat)
{
    if (!mat)
    {
        return;
    }

    mat_list_t::iterator end = mGLTFMaterialWithLocalTextures.end();
    for (mat_list_t::iterator it = mGLTFMaterialWithLocalTextures.begin(); it != end;)
    {
        if (it->get() == mat)
        {
            return;
        }

        if ((*it)->getNumRefs() == 1)
        {
            it = mGLTFMaterialWithLocalTextures.erase(it);
            end = mGLTFMaterialWithLocalTextures.end();
        }
        else
        {
            it++;
        }
    }

    mat->addLocalTextureTracking(getTrackingID(), getWorldID());
    mGLTFMaterialWithLocalTextures.push_back(mat);
}

bool LLLocalBitmap::decodeBitmap(LLPointer<LLImageRaw> rawimg)
{
	bool decode_successful = false;

	switch (mExtension)
	{
		case ET_IMG_BMP:
		{
			LLPointer<LLImageBMP> bmp_image = new LLImageBMP;
			if (bmp_image->load(mFilename) && bmp_image->decode(rawimg, 0.0f))
			{
				rawimg->biasedScaleToPowerOfTwo(LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT);
				decode_successful = true;
			}
			break;
		}

		case ET_IMG_TGA:
		{
			LLPointer<LLImageTGA> tga_image = new LLImageTGA;
			if ((tga_image->load(mFilename) && tga_image->decode(rawimg))
			&& ((tga_image->getComponents() == 3) || (tga_image->getComponents() == 4)))
			{
				rawimg->biasedScaleToPowerOfTwo(LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT);
				decode_successful = true;
			}
			break;
		}

		case ET_IMG_JPG:
		{
			LLPointer<LLImageJPEG> jpeg_image = new LLImageJPEG;
			if (jpeg_image->load(mFilename) && jpeg_image->decode(rawimg, 0.0f))
			{
				rawimg->biasedScaleToPowerOfTwo(LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT);
				decode_successful = true;
			}
			break;
		}

		case ET_IMG_PNG:
		{
			LLPointer<LLImagePNG> png_image = new LLImagePNG;
			if (png_image->load(mFilename) && png_image->decode(rawimg, 0.0f))
			{
				rawimg->biasedScaleToPowerOfTwo(LLViewerFetchedTexture::MAX_IMAGE_SIZE_DEFAULT);
				decode_successful = true;
			}
			break;
		}

		default:
		{
			// separating this into -several- LL_WARNS() calls because in the extremely unlikely case that this happens
			// accessing mFilename and any other object properties might very well crash the viewer.
			// getting here should be impossible, or there's been a pretty serious bug.

			LL_WARNS() << "During a decode attempt, the following local bitmap had no properly assigned extension." << LL_ENDL;
			LL_WARNS() << "Filename: " << mFilename << LL_ENDL;
		    LL_WARNS() << "Disabling further update attempts for this file." << LL_ENDL;
			mLinkStatus = LS_BROKEN;
		}
	}

	return decode_successful;
}

void LLLocalBitmap::replaceIDs(const LLUUID& old_id, LLUUID new_id)
{
	// checking for misuse.
	if (old_id == new_id)
	{
		LL_INFOS() << "An attempt was made to replace a texture with itself. (matching UUIDs)" << "\n"
			    << "Texture UUID: " << old_id.asString() << LL_ENDL;
		return;
	}

    mChangedSignal(getTrackingID(), old_id, new_id);

	// processing updates per channel; makes the process scalable.
	// the only actual difference is in SetTE* call i.e. SetTETexture, SetTENormal, etc.
	updateUserPrims(old_id, new_id, LLRender::DIFFUSE_MAP);
	updateUserPrims(old_id, new_id, LLRender::NORMAL_MAP);
	updateUserPrims(old_id, new_id, LLRender::SPECULAR_MAP);

	updateUserVolumes(old_id, new_id, LLRender::LIGHT_TEX);	
	updateUserVolumes(old_id, new_id, LLRender::SCULPT_TEX); // isn't there supposed to be an IMG_DEFAULT_SCULPT or something?
	
	// default safeguard image for layers
	if( new_id == IMG_DEFAULT )
	{
		new_id = IMG_DEFAULT_AVATAR;
	}

	/* It doesn't actually update all of those, it merely checks if any of them
		contain the referenced ID and if so, updates. */
	updateUserLayers(old_id, new_id, LLWearableType::WT_ALPHA);
	updateUserLayers(old_id, new_id, LLWearableType::WT_EYES);
	updateUserLayers(old_id, new_id, LLWearableType::WT_GLOVES);
	updateUserLayers(old_id, new_id, LLWearableType::WT_JACKET);
	updateUserLayers(old_id, new_id, LLWearableType::WT_PANTS);
	updateUserLayers(old_id, new_id, LLWearableType::WT_SHIRT);
	updateUserLayers(old_id, new_id, LLWearableType::WT_SHOES);
	updateUserLayers(old_id, new_id, LLWearableType::WT_SKIN);
	updateUserLayers(old_id, new_id, LLWearableType::WT_SKIRT);
	updateUserLayers(old_id, new_id, LLWearableType::WT_SOCKS);
	updateUserLayers(old_id, new_id, LLWearableType::WT_TATTOO);
	updateUserLayers(old_id, new_id, LLWearableType::WT_UNIVERSAL);
	updateUserLayers(old_id, new_id, LLWearableType::WT_UNDERPANTS);
	updateUserLayers(old_id, new_id, LLWearableType::WT_UNDERSHIRT);

    updateGLTFMaterials(old_id, new_id);
}

// this function sorts the faces from a getFaceList[getNumFaces] into a list of objects
// in order to prevent multiple sendTEUpdate calls per object during updateUserPrims
std::vector<LLViewerObject*> LLLocalBitmap::prepUpdateObjects(LLUUID old_id, U32 channel)
{
	std::vector<LLViewerObject*> obj_list;
	LLViewerFetchedTexture* old_texture = gTextureList.findImage(old_id, TEX_LIST_STANDARD);

	for(U32 face_iterator = 0; face_iterator < old_texture->getNumFaces(channel); face_iterator++)
	{
		// getting an object from a face
		LLFace* face_to_object = (*old_texture->getFaceList(channel))[face_iterator];

		if(face_to_object)
		{
			LLViewerObject* affected_object = face_to_object->getViewerObject();

			if(affected_object)
			{

				// we have an object, we'll take it's UUID and compare it to
				// whatever we already have in the returnable object list.
				// if there is a match - we do not add (to prevent duplicates)
				LLUUID mainlist_obj_id = affected_object->getID();
				bool add_object = true;

				// begin looking for duplicates
				std::vector<LLViewerObject*>::iterator objlist_iter = obj_list.begin();
				for(; (objlist_iter != obj_list.end()) && add_object; objlist_iter++)
				{
					LLViewerObject* obj = *objlist_iter;
					if (obj->getID() == mainlist_obj_id)
					{
						add_object = false; // duplicate found.
					}
				}
				// end looking for duplicates

				if(add_object)
				{
					obj_list.push_back(affected_object);
				}

			}

		}
		
	} // end of face-iterating for()

	return obj_list;
}

void LLLocalBitmap::updateUserPrims(LLUUID old_id, LLUUID new_id, U32 channel)
{
	std::vector<LLViewerObject*> objectlist = prepUpdateObjects(old_id, channel);

	for(std::vector<LLViewerObject*>::iterator object_iterator = objectlist.begin();
		object_iterator != objectlist.end(); object_iterator++)
	{
		LLViewerObject* object = *object_iterator;

		if(object)
		{
			bool update_tex = false;
			bool update_mat = false;
			S32 num_faces = object->getNumFaces();

			for (U8 face_iter = 0; face_iter < num_faces; face_iter++)
			{
				if (object->mDrawable)
				{
					LLFace* face = object->mDrawable->getFace(face_iter);
					if (face && face->getTexture(channel) && face->getTexture(channel)->getID() == old_id)
					{
						// these things differ per channel, unless there already is a universal
						// texture setting function to setTE that takes channel as a param?
						// p.s.: switch for now, might become if - if an extra test is needed to verify before touching normalmap/specmap
						switch(channel)
						{
							case LLRender::DIFFUSE_MAP:
					{
                                object->setTETexture(face_iter, new_id);
                                update_tex = true;
								break;
							}

							case LLRender::NORMAL_MAP:
							{
								object->setTENormalMap(face_iter, new_id);
								update_mat = true;
								update_tex = true;
                                break;
							}

							case LLRender::SPECULAR_MAP:
							{
								object->setTESpecularMap(face_iter, new_id);
                                update_mat = true;
								update_tex = true;
                                break;
							}
						}
						// end switch

					}
				}
			}
			
			if (update_tex)
			{
				object->sendTEUpdate();
			}

			if (update_mat)
			{
                object->mDrawable->getVOVolume()->faceMappingChanged();
			}
		}
	}
}

void LLLocalBitmap::updateUserVolumes(LLUUID old_id, LLUUID new_id, U32 channel)
{
	LLViewerFetchedTexture* old_texture = gTextureList.findImage(old_id, TEX_LIST_STANDARD);
	for (U32 volume_iter = 0; volume_iter < old_texture->getNumVolumes(channel); volume_iter++)
	{
		LLVOVolume* volobjp = (*old_texture->getVolumeList(channel))[volume_iter];
		switch (channel)
	{
			case LLRender::LIGHT_TEX:
			{
				if (volobjp->getLightTextureID() == old_id)
				{
					volobjp->setLightTextureID(new_id);
				}
				break;
			}
			case LLRender::SCULPT_TEX:
			{
				LLViewerObject* object = (LLViewerObject*)volobjp;

				if (object)
		{
			if (object->isSculpted() && object->getVolume() &&
				object->getVolume()->getParams().getSculptID() == old_id)
			{
				LLSculptParams* old_params = (LLSculptParams*)object->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
				LLSculptParams new_params(*old_params);
				new_params.setSculptTexture(new_id, (*old_params).getSculptType());
				object->setParameterEntry(LLNetworkData::PARAMS_SCULPT, new_params, true);
			}
		}
	}
		}
	}
}

void LLLocalBitmap::updateUserLayers(LLUUID old_id, LLUUID new_id, LLWearableType::EType type)
{
	U32 count = gAgentWearables.getWearableCount(type);
	for(U32 wearable_iter = 0; wearable_iter < count; wearable_iter++)
	{
		LLViewerWearable* wearable = gAgentWearables.getViewerWearable(type, wearable_iter);
		if (wearable)
		{
			std::vector<LLLocalTextureObject*> texture_list = wearable->getLocalTextureListSeq();
			for(std::vector<LLLocalTextureObject*>::iterator texture_iter = texture_list.begin();
				texture_iter != texture_list.end(); texture_iter++)
			{
				LLLocalTextureObject* lto = *texture_iter;

				if (lto && lto->getID() == old_id)
				{
					U32 local_texlayer_index = 0; /* can't keep that as static const, gives errors, so i'm leaving this var here */
					LLAvatarAppearanceDefines::EBakedTextureIndex baked_texind =
						lto->getTexLayer(local_texlayer_index)->getTexLayerSet()->getBakedTexIndex();
				
					LLAvatarAppearanceDefines::ETextureIndex reg_texind = getTexIndex(type, baked_texind);
					if (reg_texind != LLAvatarAppearanceDefines::TEX_NUM_INDICES)
					{
						U32 index;
						if (gAgentWearables.getWearableIndex(wearable,index))
						{
							gAgentAvatarp->setLocalTexture(reg_texind, gTextureList.getImage(new_id), false, index);
							gAgentAvatarp->wearableUpdated(type);
							/* telling the manager to rebake once update cycle is fully done */
							LLLocalBitmapMgr::getInstance()->setNeedsRebake();
						}
					}

				}
			}
		}
	}
}

void LLLocalBitmap::updateGLTFMaterials(LLUUID old_id, LLUUID new_id)
{
    // Might be a better idea to hold this in LLGLTFMaterialList
    mat_list_t::iterator end = mGLTFMaterialWithLocalTextures.end();
    for (mat_list_t::iterator it = mGLTFMaterialWithLocalTextures.begin(); it != end;)
    {
        if ((*it)->getNumRefs() == 1)
        {
            // render and override materials are often recreated,
            // clean up any remains
            it = mGLTFMaterialWithLocalTextures.erase(it);
            end = mGLTFMaterialWithLocalTextures.end();
        }
        else if ((*it)->replaceLocalTexture(mTrackingID, old_id, new_id))
        {
            it++;
        }
        else
        {
            // Matching id not found, no longer in use
            // material would clean itself, remove from the list
            it = mGLTFMaterialWithLocalTextures.erase(it);
            end = mGLTFMaterialWithLocalTextures.end();
        }
    }

    // Render material consists of base and override materials, make sure replaceLocalTexture
    // gets called for base and override before applyOverride
    end = mGLTFMaterialWithLocalTextures.end();
    for (mat_list_t::iterator it = mGLTFMaterialWithLocalTextures.begin(); it != end;)
    {
        LLFetchedGLTFMaterial* fetched_mat = dynamic_cast<LLFetchedGLTFMaterial*>((*it).get());
        if (fetched_mat)
        {
            for (LLTextureEntry* entry : fetched_mat->mTextureEntires)
            {
                // Normally a change in applied material id is supposed to
                // drop overrides thus reset material, but local materials
                // currently reuse their existing asset id, and purpose is
                // to preview how material will work in-world, overrides
                // included, so do an override to render update instead.
                LLGLTFMaterial* override_mat = entry->getGLTFMaterialOverride();
                if (override_mat)
                {
                    // do not create a new material, reuse existing pointer
                    LLFetchedGLTFMaterial* render_mat = (LLFetchedGLTFMaterial*)entry->getGLTFRenderMaterial();
                    if (render_mat)
                    {
                        llassert(dynamic_cast<LLFetchedGLTFMaterial*>(entry->getGLTFRenderMaterial()) != nullptr);
                        {
                            *render_mat = *fetched_mat;
                        }
                        render_mat->applyOverride(*override_mat);
                    }
                }
            }
        }
        ++it;
    }
}

LLAvatarAppearanceDefines::ETextureIndex LLLocalBitmap::getTexIndex(
	LLWearableType::EType type, LLAvatarAppearanceDefines::EBakedTextureIndex baked_texind)
{
	LLAvatarAppearanceDefines::ETextureIndex result = LLAvatarAppearanceDefines::TEX_NUM_INDICES; // using as a default/fail return.

	switch(type)
	{
		case LLWearableType::WT_ALPHA:
		{
			switch(baked_texind)
			{
				case LLAvatarAppearanceDefines::BAKED_EYES:
				{
					result = LLAvatarAppearanceDefines::TEX_EYES_ALPHA;
					break;
				}

				case LLAvatarAppearanceDefines::BAKED_HAIR:
				{
					result = LLAvatarAppearanceDefines::TEX_HAIR_ALPHA;
					break;
				}

				case LLAvatarAppearanceDefines::BAKED_HEAD:
				{
					result = LLAvatarAppearanceDefines::TEX_HEAD_ALPHA;
					break;
				}

				case LLAvatarAppearanceDefines::BAKED_LOWER:
				{
					result = LLAvatarAppearanceDefines::TEX_LOWER_ALPHA;
					break;
				}
				case LLAvatarAppearanceDefines::BAKED_UPPER:
				{
					result = LLAvatarAppearanceDefines::TEX_UPPER_ALPHA;
					break;
				}

				default:
				{
					break;
				}

			}
			break;

		}

		case LLWearableType::WT_EYES:
		{
			if (baked_texind == LLAvatarAppearanceDefines::BAKED_EYES)
			{
				result = LLAvatarAppearanceDefines::TEX_EYES_IRIS;
			}

			break;
		}

		case LLWearableType::WT_GLOVES:
		{
			if (baked_texind == LLAvatarAppearanceDefines::BAKED_UPPER)
			{
				result = LLAvatarAppearanceDefines::TEX_UPPER_GLOVES;
			}

			break;
		}

		case LLWearableType::WT_JACKET:
		{
			if (baked_texind == LLAvatarAppearanceDefines::BAKED_LOWER)
			{
				result = LLAvatarAppearanceDefines::TEX_LOWER_JACKET;
			}
			else if (baked_texind == LLAvatarAppearanceDefines::BAKED_UPPER)
			{
				result = LLAvatarAppearanceDefines::TEX_UPPER_JACKET;
			}

			break;
		}

		case LLWearableType::WT_PANTS:
		{
			if (baked_texind == LLAvatarAppearanceDefines::BAKED_LOWER)
			{
				result = LLAvatarAppearanceDefines::TEX_LOWER_PANTS;
			}

			break;
		}

		case LLWearableType::WT_SHIRT:
		{
			if (baked_texind == LLAvatarAppearanceDefines::BAKED_UPPER)
			{
				result = LLAvatarAppearanceDefines::TEX_UPPER_SHIRT;
			}

			break;
		}

		case LLWearableType::WT_SHOES:
		{
			if (baked_texind == LLAvatarAppearanceDefines::BAKED_LOWER)
			{
				result = LLAvatarAppearanceDefines::TEX_LOWER_SHOES;
			}

			break;
		}

		case LLWearableType::WT_SKIN:
		{
			switch(baked_texind)
			{
				case LLAvatarAppearanceDefines::BAKED_HEAD:
				{
					result = LLAvatarAppearanceDefines::TEX_HEAD_BODYPAINT;
					break;
				}

				case LLAvatarAppearanceDefines::BAKED_LOWER:
				{
					result = LLAvatarAppearanceDefines::TEX_LOWER_BODYPAINT;
					break;
				}
				case LLAvatarAppearanceDefines::BAKED_UPPER:
				{
					result = LLAvatarAppearanceDefines::TEX_UPPER_BODYPAINT;
					break;
				}

				default:
				{
					break;
				}

			}
			break;
		}

		case LLWearableType::WT_SKIRT:
		{
			if (baked_texind == LLAvatarAppearanceDefines::BAKED_SKIRT)
			{
				result = LLAvatarAppearanceDefines::TEX_SKIRT;
			}

			break;
		}

		case LLWearableType::WT_SOCKS:
		{
			if (baked_texind == LLAvatarAppearanceDefines::BAKED_LOWER)
			{
				result = LLAvatarAppearanceDefines::TEX_LOWER_SOCKS;
			}

			break;
		}

		case LLWearableType::WT_TATTOO:
		{
			switch (baked_texind)
			{
				case LLAvatarAppearanceDefines::BAKED_HEAD:
				{
					result = LLAvatarAppearanceDefines::TEX_HEAD_TATTOO;
					break;
				}

				case LLAvatarAppearanceDefines::BAKED_LOWER:
				{
					result = LLAvatarAppearanceDefines::TEX_LOWER_TATTOO;
					break;
				}
				case LLAvatarAppearanceDefines::BAKED_UPPER:
				{
					result = LLAvatarAppearanceDefines::TEX_UPPER_TATTOO;
					break;
				}
				default:
				{
					break;
				}
			}
			break;
			
		}
		case LLWearableType::WT_UNIVERSAL:
		{
			switch (baked_texind)
			{
				
				case LLAvatarAppearanceDefines::BAKED_SKIRT:
				{
					result = LLAvatarAppearanceDefines::TEX_SKIRT_TATTOO;
					break;
				}
				case LLAvatarAppearanceDefines::BAKED_EYES:
				{
					result = LLAvatarAppearanceDefines::TEX_EYES_TATTOO;
					break;
				}
				case LLAvatarAppearanceDefines::BAKED_HAIR:
				{
					result = LLAvatarAppearanceDefines::TEX_HAIR_TATTOO;
					break;
				}
				case LLAvatarAppearanceDefines::BAKED_LEFT_ARM:
				{
					result = LLAvatarAppearanceDefines::TEX_LEFT_ARM_TATTOO;
					break;
				}
				case LLAvatarAppearanceDefines::BAKED_LEFT_LEG:
				{
					result = LLAvatarAppearanceDefines::TEX_LEFT_LEG_TATTOO;
					break;
				}
				case LLAvatarAppearanceDefines::BAKED_AUX1:
				{
					result = LLAvatarAppearanceDefines::TEX_AUX1_TATTOO;
					break;
				}
				case LLAvatarAppearanceDefines::BAKED_AUX2:
				{
					result = LLAvatarAppearanceDefines::TEX_AUX2_TATTOO;
					break;
				}
				case LLAvatarAppearanceDefines::BAKED_AUX3:
				{
					result = LLAvatarAppearanceDefines::TEX_AUX3_TATTOO;
					break;
				}
				case LLAvatarAppearanceDefines::BAKED_UPPER:
				{
					result = LLAvatarAppearanceDefines::TEX_UPPER_UNIVERSAL_TATTOO;
					break;
				}
				case LLAvatarAppearanceDefines::BAKED_LOWER:
				{
					result = LLAvatarAppearanceDefines::TEX_LOWER_UNIVERSAL_TATTOO;
					break;
				}
				case LLAvatarAppearanceDefines::BAKED_HEAD:
				{
					result = LLAvatarAppearanceDefines::TEX_HEAD_UNIVERSAL_TATTOO;
					break;
				}


				default:
				{
					break;
				}

			}
			break;
		}

		case LLWearableType::WT_UNDERPANTS:
		{
			if (baked_texind == LLAvatarAppearanceDefines::BAKED_LOWER)
			{
				result = LLAvatarAppearanceDefines::TEX_LOWER_UNDERPANTS;
			}

			break;
		}

		case LLWearableType::WT_UNDERSHIRT:
		{
			if (baked_texind == LLAvatarAppearanceDefines::BAKED_UPPER)
			{
				result = LLAvatarAppearanceDefines::TEX_UPPER_UNDERSHIRT;
			}

			break;
		}

		default:
		{
			LL_WARNS() << "Unknown wearable type: " << (int)type << "\n"
				    << "Baked Texture Index: " << (int)baked_texind << "\n"
					<< "Filename: " << mFilename << "\n"
					<< "TrackingID: " << mTrackingID << "\n"
					<< "InworldID: " << mWorldID << LL_ENDL;
		}

	}
	return result;
}

/*=======================================*/
/*  LLLocalBitmapTimer: timer class      */
/*=======================================*/ 
LLLocalBitmapTimer::LLLocalBitmapTimer() : LLEventTimer(LL_LOCAL_TIMER_HEARTBEAT)
{
}

LLLocalBitmapTimer::~LLLocalBitmapTimer()
{
}

void LLLocalBitmapTimer::startTimer()
{
	mEventTimer.start();
}

void LLLocalBitmapTimer::stopTimer()
{
	mEventTimer.stop();
}

bool LLLocalBitmapTimer::isRunning()
{
	return mEventTimer.getStarted();
}

bool LLLocalBitmapTimer::tick()
{
	LLLocalBitmapMgr::getInstance()->doUpdates();
	return false;
}

/*=======================================*/
/*  LLLocalBitmapMgr: manager class      */
/*=======================================*/ 
LLLocalBitmapMgr::LLLocalBitmapMgr()
{
}

LLLocalBitmapMgr::~LLLocalBitmapMgr()
{
    std::for_each(mBitmapList.begin(), mBitmapList.end(), DeletePointer());
    mBitmapList.clear();
}

bool LLLocalBitmapMgr::addUnit(const std::vector<std::string>& filenames)
{
    bool add_successful = false;
    std::vector<std::string>::const_iterator iter = filenames.begin();
    while (iter != filenames.end())
    {
        if (!iter->empty() && addUnit(*iter).notNull())
        {
            add_successful = true;
        }
        iter++;
    }
    return add_successful;
}

LLUUID LLLocalBitmapMgr::addUnit(const std::string& filename)
{
    if (!checkTextureDimensions(filename))
    {
        return LLUUID::null;
    }

    LLLocalBitmap* unit = new LLLocalBitmap(filename);

    if (unit->getValid())
    {
        mBitmapList.push_back(unit);
        return unit->getTrackingID();
    }
    else
    {
        LL_WARNS() << "Attempted to add invalid or unreadable image file, attempt cancelled.\n"
            << "Filename: " << filename << LL_ENDL;

        LLSD notif_args;
        notif_args["FNAME"] = filename;
        LLNotificationsUtil::add("LocalBitmapsVerifyFail", notif_args);

        delete unit;
        unit = NULL;
    }

    return LLUUID::null;
}

bool LLLocalBitmapMgr::checkTextureDimensions(std::string filename)
{
	std::string exten = gDirUtilp->getExtension(filename);
	U32 codec = LLImageBase::getCodecFromExtension(exten);
	std::string mImageLoadError;
	LLImageDimensionsInfo image_info;
	if (!image_info.load(filename,codec))
	{
		return false;
	}

	S32 max_width = gSavedSettings.getS32("max_texture_dimension_X");
	S32 max_height = gSavedSettings.getS32("max_texture_dimension_Y");

	if ((image_info.getWidth() > max_width) || (image_info.getHeight() > max_height))
	{
		LLStringUtil::format_map_t args;
		args["WIDTH"] = llformat("%d", max_width);
		args["HEIGHT"] = llformat("%d", max_height);
		mImageLoadError = LLTrans::getString("texture_load_dimensions_error", args);

		LLSD notif_args;
		notif_args["REASON"] = mImageLoadError;
		LLNotificationsUtil::add("CannotUploadTexture", notif_args);

		return false;
	}

	return true;
}

void LLLocalBitmapMgr::delUnit(LLUUID tracking_id)
{
	if (!mBitmapList.empty())
	{	
		std::vector<LLLocalBitmap*> to_delete;
		for (local_list_iter iter = mBitmapList.begin(); iter != mBitmapList.end(); iter++)
		{   /* finding which ones we want deleted and making a separate list */
			LLLocalBitmap* unit = *iter;
			if (unit->getTrackingID() == tracking_id)
			{
				to_delete.push_back(unit);
			}
		}

		for(std::vector<LLLocalBitmap*>::iterator del_iter = to_delete.begin();
			del_iter != to_delete.end(); del_iter++)
		{   /* iterating over a temporary list, hence preserving the iterator validity while deleting. */
			LLLocalBitmap* unit = *del_iter;
			mBitmapList.remove(unit);
			delete unit;
			unit = NULL;
		}
	}
}

LLUUID LLLocalBitmapMgr::getWorldID(const LLUUID &tracking_id) const
{
	LLUUID world_id = LLUUID::null;

	for (local_list_citer iter = mBitmapList.begin(); iter != mBitmapList.end(); iter++)
	{
		LLLocalBitmap* unit = *iter;
		if (unit->getTrackingID() == tracking_id)
		{
			world_id = unit->getWorldID();
		}
	}

	return world_id;
}

bool LLLocalBitmapMgr::isLocal(const LLUUID &world_id) const
{
    for (local_list_citer iter = mBitmapList.begin(); iter != mBitmapList.end(); iter++)
    {
        LLLocalBitmap* unit = *iter;
        if (unit->getWorldID() == world_id)
        {
            return true;
        }
    }
    return false;
}

std::string LLLocalBitmapMgr::getFilename(const LLUUID &tracking_id) const
{
	std::string filename = "";

	for (local_list_citer iter = mBitmapList.begin(); iter != mBitmapList.end(); iter++)
	{
		LLLocalBitmap* unit = *iter;
		if (unit->getTrackingID() == tracking_id)
		{
			filename = unit->getFilename();
		}
	}

	return filename;
}

boost::signals2::connection LLLocalBitmapMgr::setOnChangedCallback(const LLUUID tracking_id, const LLLocalBitmap::LLLocalTextureCallback &cb)
{
    for (local_list_iter iter = mBitmapList.begin(); iter != mBitmapList.end(); iter++)
    {
        LLLocalBitmap* unit = *iter;
        if (unit->getTrackingID() == tracking_id)
        {
            return unit->setChangedCallback(cb);
        }
    }

    return boost::signals2::connection();
}

void LLLocalBitmapMgr::associateGLTFMaterial(const LLUUID tracking_id, LLGLTFMaterial* mat)
{
    for (local_list_iter iter = mBitmapList.begin(); iter != mBitmapList.end(); iter++)
    {
        LLLocalBitmap* unit = *iter;
        if (unit->getTrackingID() == tracking_id)
        {
            unit->addGLTFMaterial(mat);
        }
    }
}

void LLLocalBitmapMgr::feedScrollList(LLScrollListCtrl* ctrl)
{
	if (ctrl)
	{
        std::string icon_name = LLInventoryIcon::getIconName(
            LLAssetType::AT_TEXTURE,
            LLInventoryType::IT_NONE);

		if (!mBitmapList.empty())
		{
			for (local_list_iter iter = mBitmapList.begin();
				 iter != mBitmapList.end(); iter++)
			{
				LLSD element;

                element["columns"][0]["column"] = "icon";
                element["columns"][0]["type"] = "icon";
                element["columns"][0]["value"] = icon_name;

                element["columns"][1]["column"] = "unit_name";
                element["columns"][1]["type"] = "text";
                element["columns"][1]["value"] = (*iter)->getShortName();

                LLSD data;
                data["id"] = (*iter)->getTrackingID();
                data["type"] = (S32)LLAssetType::AT_TEXTURE;
                element["value"] = data;

				ctrl->addElement(element);
			}
		}
	}

}

void LLLocalBitmapMgr::doUpdates()
{
	// preventing theoretical overlap in cases with huge number of loaded images.
	mTimer.stopTimer();
	mNeedsRebake = false;

	for (local_list_iter iter = mBitmapList.begin(); iter != mBitmapList.end(); iter++)
	{
		(*iter)->updateSelf();
	}

	doRebake();
	mTimer.startTimer();
}

void LLLocalBitmapMgr::setNeedsRebake()
{
	mNeedsRebake = true;
}

void LLLocalBitmapMgr::doRebake()
{ /* separated that from doUpdates to insure a rebake can be called separately during deletion */
	if (mNeedsRebake)
	{
		gAgentAvatarp->forceBakeAllTextures(LL_LOCAL_SLAM_FOR_DEBUG);
		mNeedsRebake = false;
	}
}

