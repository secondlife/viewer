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
#include "lllocalgltfmaterials.h"

/* boost: will not compile unless equivalent is undef'd, beware. */
#include "fix_macros.h"
#include <boost/filesystem.hpp>

/* time headers */
#include <time.h>
#include <ctime>

/* misc headers */
#include "llgltfmateriallist.h"
#include "llimage.h"
#include "llinventoryicon.h"
#include "llmaterialmgr.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "lltinygltfhelper.h"
#include "llviewertexture.h"
#include "tinygltf/tiny_gltf.h"

/*=======================================*/
/*  Formal declarations, constants, etc. */
/*=======================================*/ 

static const F32 LL_LOCAL_TIMER_HEARTBEAT   = 3.0;
static const S32 LL_LOCAL_UPDATE_RETRIES    = 5;

/*=======================================*/
/*  LLLocalGLTFMaterial: unit class            */
/*=======================================*/ 
LLLocalGLTFMaterial::LLLocalGLTFMaterial(std::string filename, S32 index)
    : mFilename(filename)
    , mShortName(gDirUtilp->getBaseFileName(filename, true))
    , mValid(false)
    , mLastModified()
    , mLinkStatus(LS_ON)
    , mUpdateRetries(LL_LOCAL_UPDATE_RETRIES)
    , mMaterialIndex(index)
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
    mValid = updateSelf();
}

LLLocalGLTFMaterial::~LLLocalGLTFMaterial()
{
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

S32 LLLocalGLTFMaterial::getIndexInFile()
{
    return mMaterialIndex;
}

bool LLLocalGLTFMaterial::getValid()
{
    return mValid;
}

/* update functions */
bool LLLocalGLTFMaterial::updateSelf()
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
				if (loadMaterial(raw_material, mMaterialIndex))
				{
					// decode is successful, we can safely proceed.
                    if (mWorldID.isNull())
                    {
                        mWorldID.generate();
                    }
					mLastModified = new_last_modified;

                    // will replace material if it already exists
                    gGLTFMaterialList.addMaterial(mWorldID, raw_material);

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

bool LLLocalGLTFMaterial::loadMaterial(LLPointer<LLGLTFMaterial> mat, S32 index)
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
                LL_WARNS() << "Cannot load Material, error: " << error_msg
                    << ", warning:" << warn_msg
                    << " file: " << mFilename
                    << LL_ENDL;
                break;
            }

            if (model_in.materials.size() <= index)
            {
                // materials are missing
                LL_WARNS() << "Cannot load Material, Material " << index << " is missing, " << mFilename << LL_ENDL;
                decode_successful = false;
                break;
            }

            // sets everything, but textures will have inaccurate ids
            LLTinyGLTFHelper::setFromModel(mat, model_in, index);

            std::string folder = gDirUtilp->getDirName(filename_lc);
            tinygltf::Material material_in = model_in.materials[index];

            if (!material_in.name.empty())
            {
                mShortName = gDirUtilp->getBaseFileName(filename_lc, true) + " (" + material_in.name + ")";
            }

            // get base color texture
            LLPointer<LLImageRaw> base_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.pbrMetallicRoughness.baseColorTexture.index);
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
                base_img, normal_img, mr_img, emissive_img, occlusion_img,
                mBaseColorFetched, mNormalFetched, mMRFetched, mEmissiveFetched);

            if (mBaseColorFetched)
            {
                mat->mBaseColorId= mBaseColorFetched->getID();
            }
            if (mNormalFetched)
            {
                mat->mNormalId = mNormalFetched->getID();
            }
            if (mMRFetched)
            {
                mat->mMetallicRoughnessId = mMRFetched->getID();
            }
            if (mEmissiveFetched)
            {
                mat->mEmissiveId = mEmissiveFetched->getID();
            }

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

S32 LLLocalGLTFMaterialMgr::addUnit(const std::vector<std::string>& filenames)
{
    S32 add_count = 0;
    std::vector<std::string>::const_iterator iter = filenames.begin();
    while (iter != filenames.end())
    {
        if (!iter->empty())
        {
            add_count += addUnit(*iter);
        }
        iter++;
    }
    return add_count;
}

S32 LLLocalGLTFMaterialMgr::addUnit(const std::string& filename)
{
    std::string exten = gDirUtilp->getExtension(filename);
    S32 materials_in_file = 0;

    if (exten == "gltf" || exten == "glb")
    {
        tinygltf::TinyGLTF loader;
        std::string        error_msg;
        std::string        warn_msg;

        tinygltf::Model model_in;

        std::string filename_lc = filename;
        LLStringUtil::toLower(filename_lc);

        // Load a tinygltf model fom a file. Assumes that the input filename has already been
        // been sanitized to one of (.gltf , .glb) extensions, so does a simple find to distinguish.
        bool decode_successful = false;
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
            LL_WARNS() << "Cannot load, error: Failed to decode" << error_msg
                << ", warning:" << warn_msg
                << " file: " << filename
                << LL_ENDL;
            return 0;
        }

        if (model_in.materials.empty())
        {
            // materials are missing
            LL_WARNS() << "Cannot load. File has no materials " << filename << LL_ENDL;
            return 0;
        }
        materials_in_file = model_in.materials.size();
    }

    S32 loaded_materials = 0;
    for (S32 i = 0; i < materials_in_file; i++)
    {
        // Todo: this is rather inefficient, files will be spammed with
        // separate loads and date checks, find a way to improve this.
        // May be doUpdates() should be checking individual files.
        LLLocalGLTFMaterial* unit = new LLLocalGLTFMaterial(filename, i);

        if (unit->getValid())
        {
            mMaterialList.push_back(unit);
            loaded_materials++;
        }
        else
        {
            LL_WARNS() << "Attempted to add invalid or unreadable image file, attempt cancelled.\n"
                << "Filename: " << filename << LL_ENDL;

            LLSD notif_args;
            notif_args["FNAME"] = filename;
            LLNotificationsUtil::add("LocalGLTFVerifyFail", notif_args);

            delete unit;
            unit = NULL;
        }
    }

    return loaded_materials;
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

void LLLocalGLTFMaterialMgr::getFilenameAndIndex(LLUUID tracking_id, std::string &filename, S32 &index)
{
    filename = "";
    index = 0;

    for (local_list_iter iter = mMaterialList.begin(); iter != mMaterialList.end(); iter++)
    {
        LLLocalGLTFMaterial* unit = *iter;
        if (unit->getTrackingID() == tracking_id)
        {
            filename = unit->getFilename();
            index = unit->getIndexInFile();
        }
    }
}

// probably shouldn't be here, but at the moment this mirrors lllocalbitmaps
void LLLocalGLTFMaterialMgr::feedScrollList(LLScrollListCtrl* ctrl)
{
    if (ctrl)
    {
        if (!mMaterialList.empty())
        {
            std::string icon_name = LLInventoryIcon::getIconName(
                LLAssetType::AT_MATERIAL,
                LLInventoryType::IT_NONE);

            for (local_list_iter iter = mMaterialList.begin();
                iter != mMaterialList.end(); iter++)
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
                data["type"] = (S32)LLAssetType::AT_MATERIAL;
                element["value"] = data;

                ctrl->addElement(element);
            }
        }
    }

}

void LLLocalGLTFMaterialMgr::doUpdates()
{
    // preventing theoretical overlap in cases with huge number of loaded images.
    mTimer.stopTimer();

    for (local_list_iter iter = mMaterialList.begin(); iter != mMaterialList.end(); iter++)
    {
        (*iter)->updateSelf();
    }

    mTimer.startTimer();
}

