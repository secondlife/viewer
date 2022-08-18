/** 
 * @file lllocalrendermaterials.cpp
 * @brief Local GLTF materials source
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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
#include "lllocalrendermaterials.h"

/* boost: will not compile unless equivalent is undef'd, beware. */
#include "fix_macros.h"
#include <boost/filesystem.hpp>

/* time headers */
#include <time.h>
#include <ctime>

/* misc headers */
#include "llagentwearables.h"
#include "llface.h"
#include "llfilepicker.h"
#include "llgltfmateriallist.h"
#include "llimagedimensionsinfo.h"
#include "lllocaltextureobject.h"
#include "llmaterialmgr.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "lltexlayerparams.h"
#include "lltinygltfhelper.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewerdisplay.h"
#include "llviewerobjectlist.h"
#include "llviewerobject.h"
#include "llviewerwearable.h"
#include "llvoavatarself.h"
#include "llvovolume.h"
#include "pipeline.h"
#include "tinygltf/tiny_gltf.h"

/*=======================================*/
/*  Formal declarations, constants, etc. */
/*=======================================*/ 

static const F32 LL_LOCAL_TIMER_HEARTBEAT   = 3.0;
static const BOOL LL_LOCAL_USE_MIPMAPS      = true;
static const S32 LL_LOCAL_DISCARD_LEVEL     = 0;
static const bool LL_LOCAL_SLAM_FOR_DEBUG   = true;
static const bool LL_LOCAL_REPLACE_ON_DEL   = true;
static const S32 LL_LOCAL_UPDATE_RETRIES    = 5;

/*=======================================*/
/*  LLLocalGLTFMaterial: unit class            */
/*=======================================*/ 
LLLocalGLTFMaterial::LLLocalGLTFMaterial(std::string filename)
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

	if (temp_exten == "gltf")
	{ 
		mExtension = ET_MATERIAL_GLTF;
	}
	else if (temp_exten == "glb")
	{
		mExtension = ET_MATERIAL_GLB;
	}
	else
	{
		LL_WARNS() << "File of no valid extension given, local material creation aborted." << "\n"
			    << "Filename: " << mFilename << LL_ENDL;
		return; // no valid extension.
	}

	/* next phase of unit creation is nearly the same as an update cycle.
	   we're running updateSelf as a special case with the optional UT_FIRSTUSE
	   which omits the parts associated with removing the outdated texture */
	mValid = updateSelf(UT_FIRSTUSE);
}

LLLocalGLTFMaterial::~LLLocalGLTFMaterial()
{
	// replace IDs with defaults, if set to do so.
	if(LL_LOCAL_REPLACE_ON_DEL && mValid && gAgentAvatarp) // fix for STORM-1837
	{
		replaceIDs(mWorldID, IMG_DEFAULT);
		LLLocalGLTFMaterialMgr::getInstance()->doRebake();
	}

	// delete self from material list
    gGLTFMaterialList.removeMaterial(mWorldID);
}

/* accessors */
std::string LLLocalGLTFMaterial::getFilename()
{
	return mFilename;
}

std::string LLLocalGLTFMaterial::getShortName()
{
	return mShortName;
}

LLUUID LLLocalGLTFMaterial::getTrackingID()
{
	return mTrackingID;
}

LLUUID LLLocalGLTFMaterial::getWorldID()
{
	return mWorldID;
}

bool LLLocalGLTFMaterial::getValid()
{
	return mValid;
}

/* update functions */
bool LLLocalGLTFMaterial::updateSelf(EUpdateType optional_firstupdate)
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
				LLPointer<LLGLTFMaterial> raw_material = new LLGLTFMaterial();
				if (loadMaterial(raw_material))
				{
					// decode is successful, we can safely proceed.
					LLUUID old_id = LLUUID::null;
					if ((optional_firstupdate != UT_FIRSTUSE) && !mWorldID.isNull())
					{
						old_id = mWorldID;
					}
					mWorldID.generate();
					mLastModified = new_last_modified;

                    gGLTFMaterialList.addMaterial(mWorldID, raw_material);
			
					if (optional_firstupdate != UT_FIRSTUSE)
					{
						// seek out everything old_id uses and replace it with mWorldID
						replaceIDs(old_id, mWorldID);

						// remove old_id from material list
                        gGLTFMaterialList.removeMaterial(old_id);
					}

					mUpdateRetries = LL_LOCAL_UPDATE_RETRIES;
					updated = true;
				}

				// if decoding failed, we get here and it will attempt to decode it in the next cycles
				// until mUpdateRetries runs out. this is done because some software lock the material while writing to it
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

bool LLLocalGLTFMaterial::loadMaterial(LLPointer<LLGLTFMaterial> mat)
{
    bool decode_successful = false;

    switch (mExtension)
    {
    case ET_MATERIAL_GLTF:
    case ET_MATERIAL_GLB:
    {
            tinygltf::TinyGLTF loader;
            std::string        error_msg;
            std::string        warn_msg;

            tinygltf::Model model_in;

            std::string filename_lc = mFilename;
            LLStringUtil::toLower(filename_lc);

            // Load a tinygltf model fom a file. Assumes that the input filename has already been
            // been sanitized to one of (.gltf , .glb) extensions, so does a simple find to distinguish.
            if (std::string::npos == filename_lc.rfind(".gltf"))
            {  // file is binary
                decode_successful = loader.LoadBinaryFromFile(&model_in, &error_msg, &warn_msg, filename_lc);
            }
            else
            {  // file is ascii
                decode_successful = loader.LoadASCIIFromFile(&model_in, &error_msg, &warn_msg, filename_lc);
            }

            if (!decode_successful)
            {
                LL_WARNS() << "Cannot Upload Material, error: " << error_msg
                    << ", warning:" << warn_msg
                    << " file: " << mFilename
                    << LL_ENDL;
                break;
            }

            if (model_in.materials.empty())
            {
                // materials are missing
                LL_WARNS() << "Cannot Upload Material, Material missing, " << mFilename << LL_ENDL;
                decode_successful = false;
                break;
            }

            // sets everything, but textures will have inaccurate ids
            LLTinyGLTFHelper::setFromModel(mat, model_in);

            std::string folder = gDirUtilp->getDirName(filename_lc);
            tinygltf::Material material_in = model_in.materials[0];

            // get albedo texture
            LLPointer<LLImageRaw> albedo_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.pbrMetallicRoughness.baseColorTexture.index);
            // get normal map
            LLPointer<LLImageRaw> normal_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.normalTexture.index);
            // get metallic-roughness texture
            LLPointer<LLImageRaw> mr_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.pbrMetallicRoughness.metallicRoughnessTexture.index);
            // get emissive texture
            LLPointer<LLImageRaw> emissive_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.emissiveTexture.index);
            // get occlusion map if needed
            LLPointer<LLImageRaw> occlusion_img;
            if (material_in.occlusionTexture.index != material_in.pbrMetallicRoughness.metallicRoughnessTexture.index)
            {
                occlusion_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.occlusionTexture.index);
            }

            // todo: pass it into local bitmaps?
            LLTinyGLTFHelper::initFetchedTextures(material_in,
                albedo_img, normal_img, mr_img, emissive_img, occlusion_img,
                mAlbedoFetched, mNormalFetched, mMRFetched, mEmissiveFetched);

            mat->mAlbedoId = mAlbedoFetched->getID();
            mat->mNormalId = mNormalFetched->getID();
            mat->mMetallicRoughnessId = mMRFetched->getID();
            mat->mEmissiveId = mEmissiveFetched->getID();

            break;
        }

        default:
        {
            // separating this into -several- LL_WARNS() calls because in the extremely unlikely case that this happens
            // accessing mFilename and any other object properties might very well crash the viewer.
            // getting here should be impossible, or there's been a pretty serious bug.

            LL_WARNS() << "During a decode attempt, the following local material had no properly assigned extension." << LL_ENDL;
            LL_WARNS() << "Filename: " << mFilename << LL_ENDL;
            LL_WARNS() << "Disabling further update attempts for this file." << LL_ENDL;
            mLinkStatus = LS_BROKEN;
        }
    }

    return decode_successful;
}

void LLLocalGLTFMaterial::replaceIDs(LLUUID old_id, LLUUID new_id)
{
    // checking for misuse.
    if (old_id == new_id)
    {
        LL_INFOS() << "An attempt was made to replace a texture with itself. (matching UUIDs)" << "\n"
            << "Texture UUID: " << old_id.asString() << LL_ENDL;
        return;
    }

    // processing updates per channel; makes the process scalable.
    // the only actual difference is in SetTE* call i.e. SetTETexture, SetTENormal, etc.
    updateUserPrims(old_id, new_id, LLRender::DIFFUSE_MAP);
    updateUserPrims(old_id, new_id, LLRender::NORMAL_MAP);
    updateUserPrims(old_id, new_id, LLRender::SPECULAR_MAP);

    // default safeguard image for layers
    if (new_id == IMG_DEFAULT)
    {
        new_id = IMG_DEFAULT_AVATAR;
    }
}

// this function sorts the faces from a getFaceList[getNumFaces] into a list of objects
// in order to prevent multiple sendTEUpdate calls per object during updateUserPrims
std::vector<LLViewerObject*> LLLocalGLTFMaterial::prepUpdateObjects(LLUUID old_id, U32 channel)
{
    std::vector<LLViewerObject*> obj_list;
    // todo: find a way to update materials
    /*
    LLGLTFMaterial* old_material = gGLTFMaterialList.getMaterial(old_id);

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

    */
    return obj_list;
}

void LLLocalGLTFMaterial::updateUserPrims(LLUUID old_id, LLUUID new_id, U32 channel)
{
    /*std::vector<LLViewerObject*> objectlist = prepUpdateObjects(old_id, channel);
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
    */
}

/*=======================================*/
/*  LLLocalGLTFMaterialTimer: timer class      */
/*=======================================*/
LLLocalGLTFMaterialTimer::LLLocalGLTFMaterialTimer() : LLEventTimer(LL_LOCAL_TIMER_HEARTBEAT)
{
}

LLLocalGLTFMaterialTimer::~LLLocalGLTFMaterialTimer()
{
}

void LLLocalGLTFMaterialTimer::startTimer()
{
    mEventTimer.start();
}

void LLLocalGLTFMaterialTimer::stopTimer()
{
    mEventTimer.stop();
}

bool LLLocalGLTFMaterialTimer::isRunning()
{
    return mEventTimer.getStarted();
}

BOOL LLLocalGLTFMaterialTimer::tick()
{
    // todo: do on idle? No point in timer 
    LLLocalGLTFMaterialMgr::getInstance()->doUpdates();
    return FALSE;
}

/*=======================================*/
/*  LLLocalGLTFMaterialMgr: manager class      */
/*=======================================*/
LLLocalGLTFMaterialMgr::LLLocalGLTFMaterialMgr()
{
}

LLLocalGLTFMaterialMgr::~LLLocalGLTFMaterialMgr()
{
    std::for_each(mMaterialList.begin(), mMaterialList.end(), DeletePointer());
    mMaterialList.clear();
}

bool LLLocalGLTFMaterialMgr::addUnit()
{
    bool add_successful = false;

    LLFilePicker& picker = LLFilePicker::instance();
    if (picker.getMultipleOpenFiles(LLFilePicker::FFLOAD_IMAGE))
    {
        mTimer.stopTimer();

        std::string filename = picker.getFirstFile();
        while (!filename.empty())
        {
            if (!checkTextureDimensions(filename))
            {
                filename = picker.getNextFile();
                continue;
            }

            LLLocalGLTFMaterial* unit = new LLLocalGLTFMaterial(filename);

            if (unit->getValid())
            {
                mMaterialList.push_back(unit);
                add_successful = true;
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

            filename = picker.getNextFile();
        }

        mTimer.startTimer();
    }

    return add_successful;
}

bool LLLocalGLTFMaterialMgr::checkTextureDimensions(std::string filename)
{
    std::string exten = gDirUtilp->getExtension(filename);
    U32 codec = LLImageBase::getCodecFromExtension(exten);
    std::string mImageLoadError;
    LLImageDimensionsInfo image_info;
    if (!image_info.load(filename, codec))
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

void LLLocalGLTFMaterialMgr::delUnit(LLUUID tracking_id)
{
    if (!mMaterialList.empty())
    {
        std::vector<LLLocalGLTFMaterial*> to_delete;
        for (local_list_iter iter = mMaterialList.begin(); iter != mMaterialList.end(); iter++)
        {   /* finding which ones we want deleted and making a separate list */
            LLLocalGLTFMaterial* unit = *iter;
            if (unit->getTrackingID() == tracking_id)
            {
                to_delete.push_back(unit);
            }
        }

        for (std::vector<LLLocalGLTFMaterial*>::iterator del_iter = to_delete.begin();
            del_iter != to_delete.end(); del_iter++)
        {   /* iterating over a temporary list, hence preserving the iterator validity while deleting. */
            LLLocalGLTFMaterial* unit = *del_iter;
            mMaterialList.remove(unit);
            delete unit;
            unit = NULL;
        }
    }
}

LLUUID LLLocalGLTFMaterialMgr::getWorldID(LLUUID tracking_id)
{
    LLUUID world_id = LLUUID::null;

    for (local_list_iter iter = mMaterialList.begin(); iter != mMaterialList.end(); iter++)
    {
        LLLocalGLTFMaterial* unit = *iter;
        if (unit->getTrackingID() == tracking_id)
        {
            world_id = unit->getWorldID();
        }
    }

    return world_id;
}

bool LLLocalGLTFMaterialMgr::isLocal(const LLUUID world_id)
{
    for (local_list_iter iter = mMaterialList.begin(); iter != mMaterialList.end(); iter++)
    {
        LLLocalGLTFMaterial* unit = *iter;
        if (unit->getWorldID() == world_id)
        {
            return true;
        }
    }
    return false;
}

std::string LLLocalGLTFMaterialMgr::getFilename(LLUUID tracking_id)
{
    std::string filename = "";

    for (local_list_iter iter = mMaterialList.begin(); iter != mMaterialList.end(); iter++)
    {
        LLLocalGLTFMaterial* unit = *iter;
        if (unit->getTrackingID() == tracking_id)
        {
            filename = unit->getFilename();
        }
    }

    return filename;
}

void LLLocalGLTFMaterialMgr::feedScrollList(LLScrollListCtrl* ctrl)
{
    if (ctrl)
    {
        ctrl->clearRows();

        if (!mMaterialList.empty())
        {
            for (local_list_iter iter = mMaterialList.begin();
                iter != mMaterialList.end(); iter++)
            {
                LLSD element;
                element["columns"][0]["column"] = "unit_name";
                element["columns"][0]["type"] = "text";
                element["columns"][0]["value"] = (*iter)->getShortName();

                element["columns"][1]["column"] = "unit_id_HIDDEN";
                element["columns"][1]["type"] = "text";
                element["columns"][1]["value"] = (*iter)->getTrackingID();

                ctrl->addElement(element);
            }
        }
    }

}

void LLLocalGLTFMaterialMgr::doUpdates()
{
    // preventing theoretical overlap in cases with huge number of loaded images.
    mTimer.stopTimer();
    mNeedsRebake = false;

    for (local_list_iter iter = mMaterialList.begin(); iter != mMaterialList.end(); iter++)
    {
        (*iter)->updateSelf();
    }

    doRebake();
    mTimer.startTimer();
}

void LLLocalGLTFMaterialMgr::setNeedsRebake()
{
    mNeedsRebake = true;
}

void LLLocalGLTFMaterialMgr::doRebake()
{ /* separated that from doUpdates to insure a rebake can be called separately during deletion */
    if (mNeedsRebake)
    {
        gAgentAvatarp->forceBakeAllTextures(LL_LOCAL_SLAM_FOR_DEBUG);
        mNeedsRebake = false;
    }
}

