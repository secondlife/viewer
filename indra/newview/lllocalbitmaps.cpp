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
#include "llscrolllistctrl.h"
#include "llfilepicker.h"
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

/*=======================================*/
/*  Formal declarations, constants, etc. */
/*=======================================*/ 
std::list<LLLocalBitmap*>   LLLocalBitmapMgr::sBitmapList;
LLLocalBitmapTimer          LLLocalBitmapMgr::sTimer;
bool                        LLLocalBitmapMgr::sNeedsRebake;

static const F32 LL_LOCAL_TIMER_HEARTBEAT   = 3.0;
static const BOOL LL_LOCAL_USE_MIPMAPS      = true;
static const S32 LL_LOCAL_DISCARD_LEVEL     = 0;
static const U32 LL_LOCAL_TEXLAYER_FOR_IDX  = 0;
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
		llwarns << "File of no valid extension given, local bitmap creation aborted." << "\n"
			    << "Filename: " << mFilename << llendl;
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
	if(LL_LOCAL_REPLACE_ON_DEL && mValid) // fix for STORM-1837
	{
		replaceIDs(mWorldID, IMG_DEFAULT);
		LLLocalBitmapMgr::doRebake();
	}

	// delete self from gimagelist
	LLViewerFetchedTexture* image = gTextureList.findImage(mWorldID);
	gTextureList.deleteImage(image);

	if (image)
	{
		image->unref();
	}
}

/* accessors */
std::string LLLocalBitmap::getFilename()
{
	return mFilename;
}

std::string LLLocalBitmap::getShortName()
{
	return mShortName;
}

LLUUID LLLocalBitmap::getTrackingID()
{
	return mTrackingID;
}

LLUUID LLLocalBitmap::getWorldID()
{
	return mWorldID;
}

bool LLLocalBitmap::getValid()
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
			const std::time_t temp_time = boost::filesystem::last_write_time(boost::filesystem::path(mFilename));
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
					if (!(optional_firstupdate == UT_FIRSTUSE) && !mWorldID.isNull())
					{
						old_id = mWorldID;
					}
					mWorldID.generate();
					mLastModified = new_last_modified;

					LLPointer<LLViewerFetchedTexture> texture = new LLViewerFetchedTexture
						("file://"+mFilename, mWorldID, LL_LOCAL_USE_MIPMAPS);

					texture->createGLTexture(LL_LOCAL_DISCARD_LEVEL, raw_image);
					texture->setCachedRawImage(LL_LOCAL_DISCARD_LEVEL, raw_image);
					texture->ref(); 

					gTextureList.addImage(texture);
			
					if (!optional_firstupdate == UT_FIRSTUSE)
					{
						// seek out everything old_id uses and replace it with mWorldID
						replaceIDs(old_id, mWorldID);

						// remove old_id from gimagelist
						LLViewerFetchedTexture* image = gTextureList.findImage(old_id);
						gTextureList.deleteImage(image);
						image->unref();
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
						llwarns << "During the update process the following file was found" << "\n"
							    << "but could not be opened or decoded for " << LL_LOCAL_UPDATE_RETRIES << " attempts." << "\n"
								<< "Filename: " << mFilename << "\n"
								<< "Disabling further update attempts for this file." << llendl;

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
			llwarns << "During the update process, the following file was not found." << "\n" 
			        << "Filename: " << mFilename << "\n"
				    << "Disabling further update attempts for this file." << llendl;

			LLSD notif_args;
			notif_args["FNAME"] = mFilename;
			LLNotificationsUtil::add("LocalBitmapsUpdateFileNotFound", notif_args);

			mLinkStatus = LS_BROKEN;
		}
	}

	return updated;
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
			// separating this into -several- llwarns calls because in the extremely unlikely case that this happens
			// accessing mFilename and any other object properties might very well crash the viewer.
			// getting here should be impossible, or there's been a pretty serious bug.

			llwarns << "During a decode attempt, the following local bitmap had no properly assigned extension." << llendl;
			llwarns << "Filename: " << mFilename << llendl;
		    llwarns << "Disabling further update attempts for this file." << llendl;
			mLinkStatus = LS_BROKEN;
		}
	}

	return decode_successful;
}

void LLLocalBitmap::replaceIDs(LLUUID old_id, LLUUID new_id)
{
	// checking for misuse.
	if (old_id == new_id)
	{
		llinfos << "An attempt was made to replace a texture with itself. (matching UUIDs)" << "\n"
			    << "Texture UUID: " << old_id.asString() << llendl;
		return;
	}

	updateUserPrims(old_id, new_id);
	updateUserSculpts(old_id, new_id); // isn't there supposed to be an IMG_DEFAULT_SCULPT or something?
	
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
	updateUserLayers(old_id, new_id, LLWearableType::WT_UNDERPANTS);
	updateUserLayers(old_id, new_id, LLWearableType::WT_UNDERSHIRT);
}

// this function sorts the faces from a getFaceList[getNumFaces] into a list of objects
// in order to prevent multiple sendTEUpdate calls per object during updateUserPrims
std::vector<LLViewerObject*> LLLocalBitmap::prepUpdateObjects(LLUUID old_id)
{
	std::vector<LLViewerObject*> obj_list;
	LLViewerFetchedTexture* old_texture = gTextureList.findImage(old_id);

	for(U32 face_iterator = 0; face_iterator < old_texture->getNumFaces(); face_iterator++)
	{
		// getting an object from a face
		LLFace* face_to_object = (*old_texture->getFaceList())[face_iterator];

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

void LLLocalBitmap::updateUserPrims(LLUUID old_id, LLUUID new_id)
{
	std::vector<LLViewerObject*> objectlist = prepUpdateObjects(old_id);

	for(std::vector<LLViewerObject*>::iterator object_iterator = objectlist.begin();
		object_iterator != objectlist.end(); object_iterator++)
	{
		LLViewerObject* object = *object_iterator;

		if(object)
		{
			bool update_obj = false;
			S32 num_faces = object->getNumFaces();

			for (U8 face_iter = 0; face_iter < num_faces; face_iter++)
			{
				if (object->mDrawable)
				{
					LLFace* face = object->mDrawable->getFace(face_iter);
					if (face && face->getTexture() && face->getTexture()->getID() == old_id)
					{
						object->setTEImage(face_iter, LLViewerTextureManager::getFetchedTexture
							(new_id, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE));

						update_obj = true;
					}
				}
			}
			
			if (update_obj)
			{
				object->sendTEUpdate();
			}
		}
	}
	
}

void LLLocalBitmap::updateUserSculpts(LLUUID old_id, LLUUID new_id)
{
	LLViewerFetchedTexture* old_texture = gTextureList.findImage(old_id);
	for(U32 volume_iter = 0; volume_iter < old_texture->getNumVolumes(); volume_iter++)
	{
		LLVOVolume* volume_to_object = (*old_texture->getVolumeList())[volume_iter];
		LLViewerObject* object = (LLViewerObject*)volume_to_object;
	
		if(object)
		{
			if (object->isSculpted() && object->getVolume() &&
				object->getVolume()->getParams().getSculptID() == old_id)
			{
				LLSculptParams* old_params = (LLSculptParams*)object->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
				LLSculptParams new_params(*old_params);
				new_params.setSculptTexture(new_id);
				object->setParameterEntry(LLNetworkData::PARAMS_SCULPT, new_params, TRUE);
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
						U32 index = gAgentWearables.getWearableIndex(wearable);
						gAgentAvatarp->setLocalTexture(reg_texind, gTextureList.getImage(new_id), FALSE, index);
						gAgentAvatarp->wearableUpdated(type, FALSE);

						/* telling the manager to rebake once update cycle is fully done */
						LLLocalBitmapMgr::setNeedsRebake();
					}

				}
			}
		}
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
			switch(baked_texind)
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
			llwarns << "Unknown wearable type: " << (int)type << "\n"
				    << "Baked Texture Index: " << (int)baked_texind << "\n"
					<< "Filename: " << mFilename << "\n"
					<< "TrackingID: " << mTrackingID << "\n"
					<< "InworldID: " << mWorldID << llendl;
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

BOOL LLLocalBitmapTimer::tick()
{
	LLLocalBitmapMgr::doUpdates();
	return FALSE;
}

/*=======================================*/
/*  LLLocalBitmapMgr: manager class      */
/*=======================================*/ 
LLLocalBitmapMgr::LLLocalBitmapMgr()
{
	// The class is all made of static members, should i even bother instantiating?
}

LLLocalBitmapMgr::~LLLocalBitmapMgr()
{
}

bool LLLocalBitmapMgr::addUnit()
{
	bool add_successful = false;

	LLFilePicker& picker = LLFilePicker::instance();
	if (picker.getMultipleOpenFiles(LLFilePicker::FFLOAD_IMAGE))
	{
		sTimer.stopTimer();

		std::string filename = picker.getFirstFile();
		while(!filename.empty())
		{
			LLLocalBitmap* unit = new LLLocalBitmap(filename);

			if (unit->getValid())
			{
				sBitmapList.push_back(unit);
				add_successful = true;
			}
			else
			{
				llwarns << "Attempted to add invalid or unreadable image file, attempt cancelled.\n"
					    << "Filename: " << filename << llendl;

				LLSD notif_args;
				notif_args["FNAME"] = filename;
				LLNotificationsUtil::add("LocalBitmapsVerifyFail", notif_args);

				delete unit;
				unit = NULL;
			}

			filename = picker.getNextFile();
		}
		
		sTimer.startTimer();
	}

	return add_successful;
}

void LLLocalBitmapMgr::delUnit(LLUUID tracking_id)
{
	if (!sBitmapList.empty())
	{	
		std::vector<LLLocalBitmap*> to_delete;
		for (local_list_iter iter = sBitmapList.begin(); iter != sBitmapList.end(); iter++)
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
			sBitmapList.remove(unit);
			delete unit;
			unit = NULL;
		}
	}
}

LLUUID LLLocalBitmapMgr::getWorldID(LLUUID tracking_id)
{
	LLUUID world_id = LLUUID::null;

	for (local_list_iter iter = sBitmapList.begin(); iter != sBitmapList.end(); iter++)
	{
		LLLocalBitmap* unit = *iter;
		if (unit->getTrackingID() == tracking_id)
		{
			world_id = unit->getWorldID();
		}
	}

	return world_id;
}

std::string LLLocalBitmapMgr::getFilename(LLUUID tracking_id)
{
	std::string filename = "";

	for (local_list_iter iter = sBitmapList.begin(); iter != sBitmapList.end(); iter++)
	{
		LLLocalBitmap* unit = *iter;
		if (unit->getTrackingID() == tracking_id)
		{
			filename = unit->getFilename();
		}
	}

	return filename;
}

void LLLocalBitmapMgr::feedScrollList(LLScrollListCtrl* ctrl)
{
	if (ctrl)
	{
		ctrl->clearRows();

		if (!sBitmapList.empty())
		{
			for (local_list_iter iter = sBitmapList.begin();
				 iter != sBitmapList.end(); iter++)
			{
				LLSD element;
				element["columns"][0]["column"] = "unit_name";
				element["columns"][0]["type"]   = "text";
				element["columns"][0]["value"]  = (*iter)->getShortName();

				element["columns"][1]["column"] = "unit_id_HIDDEN";
				element["columns"][1]["type"]   = "text";
				element["columns"][1]["value"]  = (*iter)->getTrackingID();

				ctrl->addElement(element);
			}
		}
	}

}

void LLLocalBitmapMgr::doUpdates()
{
	// preventing theoretical overlap in cases with huge number of loaded images.
	sTimer.stopTimer();
	sNeedsRebake = false;

	for (local_list_iter iter = sBitmapList.begin(); iter != sBitmapList.end(); iter++)
	{
		(*iter)->updateSelf();
	}

	doRebake();
	sTimer.startTimer();
}

void LLLocalBitmapMgr::setNeedsRebake()
{
	sNeedsRebake = true;
}

void LLLocalBitmapMgr::doRebake()
{ /* separated that from doUpdates to insure a rebake can be called separately during deletion */
	if (sNeedsRebake)
	{
		gAgentAvatarp->forceBakeAllTextures(LL_LOCAL_SLAM_FOR_DEBUG);
		sNeedsRebake = false;
	}
}

