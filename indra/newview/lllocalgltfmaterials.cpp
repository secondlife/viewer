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
#include "lltextureentry.h"
#include "lltinygltfhelper.h"
#include "llviewertexture.h"

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
        LL_WARNS("GLTF") << "File of no valid extension given, local material creation aborted." << "\n"
            << "Filename: " << mFilename << LL_ENDL;
        return; // no valid extension.
    }
}

LLLocalGLTFMaterial::~LLLocalGLTFMaterial()
{
    // gGLTFMaterialList will clean itself
}

/* accessors */
std::string LLLocalGLTFMaterial::getFilename() const
{
    return mFilename;
}

std::string LLLocalGLTFMaterial::getShortName() const
{
    return mShortName;
}

LLUUID LLLocalGLTFMaterial::getTrackingID() const
{
    return mTrackingID;
}

LLUUID LLLocalGLTFMaterial::getWorldID() const
{
    return mWorldID;
}

S32 LLLocalGLTFMaterial::getIndexInFile() const
{
    return mMaterialIndex;
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
                if (loadMaterial())
                {
                    // decode is successful, we can safely proceed.
                    if (mWorldID.isNull())
                    {
                        mWorldID.generate();
                    }
                    mLastModified = new_last_modified;

                    // addMaterial will replace material witha a new
                    // pointer if value already exists but we are
                    // reusing existing pointer, so it should add only.
                    gGLTFMaterialList.addMaterial(mWorldID, this);

                    mUpdateRetries = LL_LOCAL_UPDATE_RETRIES;

                    for (LLTextureEntry* entry : mTextureEntires)
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
                                *render_mat = *this;
                                render_mat->applyOverride(*override_mat);
                            }
                        }
                    }

                    materialBegin();
                    materialComplete(true);
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
                        LL_WARNS("GLTF") << "During the update process the following file was found" << "\n"
                            << "but could not be opened or decoded for " << LL_LOCAL_UPDATE_RETRIES << " attempts." << "\n"
                            << "Filename: " << mFilename << "\n"
                            << "Disabling further update attempts for this file." << LL_ENDL;

                        LLSD notif_args;
                        notif_args["FNAME"] = mFilename;
                        notif_args["NRETRIES"] = LL_LOCAL_UPDATE_RETRIES;
                        LLNotificationsUtil::add("LocalBitmapsUpdateFailedFinal", notif_args);

                        mLinkStatus = LS_BROKEN;
                        materialBegin();
                        materialComplete(false);
                    }
                }
            }

        } // end if file exists

        else
        {
            LL_WARNS("GLTF") << "During the update process, the following file was not found." << "\n"
                << "Filename: " << mFilename << "\n"
                << "Disabling further update attempts for this file." << LL_ENDL;

            LLSD notif_args;
            notif_args["FNAME"] = mFilename;
            LLNotificationsUtil::add("LocalBitmapsUpdateFileNotFound", notif_args);

            mLinkStatus = LS_BROKEN;
            materialBegin();
            materialComplete(false);
        }
    }

    return updated;
}

bool LLLocalGLTFMaterial::loadMaterial()
{
    bool decode_successful = false;

    switch (mExtension)
    {
    case ET_MATERIAL_GLTF:
    case ET_MATERIAL_GLB:
    {
            std::string filename_lc = mFilename;
            LLStringUtil::toLower(filename_lc);
            std::string material_name;

            tinygltf::Model model;
            decode_successful = LLTinyGLTFHelper::loadModel(mFilename, model);
            if (decode_successful)
            {
                // Might be a good idea to make these textures into local textures
                decode_successful = LLTinyGLTFHelper::getMaterialFromModel(
                    mFilename,
                    model,
                    mMaterialIndex,
                    this,
                    material_name);
            }

            if (!material_name.empty())
            {
                mShortName = gDirUtilp->getBaseFileName(filename_lc, true) + " (" + material_name + ")";
            }

            break;
        }

        default:
        {
            // separating this into -several- LL_WARNS() calls because in the extremely unlikely case that this happens
            // accessing mFilename and any other object properties might very well crash the viewer.
            // getting here should be impossible, or there's been a pretty serious bug.

            LL_WARNS("GLTF") << "During a decode attempt, the following local material had no properly assigned extension." << LL_ENDL;
            LL_WARNS("GLTF") << "Filename: " << mFilename << LL_ENDL;
            LL_WARNS("GLTF") << "Disabling further update attempts for this file." << LL_ENDL;
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

bool LLLocalGLTFMaterialTimer::tick()
{
    // todo: do on idle? No point in timer
    LLLocalGLTFMaterialMgr::getInstance()->doUpdates();
    return false;
}

/*=======================================*/
/*  LLLocalGLTFMaterialMgr: manager class      */
/*=======================================*/
LLLocalGLTFMaterialMgr::LLLocalGLTFMaterialMgr()
{
}

LLLocalGLTFMaterialMgr::~LLLocalGLTFMaterialMgr()
{
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
    tinygltf::Model model;
    LLTinyGLTFHelper::loadModel(filename, model);

    auto materials_in_file = model.materials.size();
    if (materials_in_file <= 0)
    {
        return 0;
    }

    S32 loaded_materials = 0;
    for (size_t i = 0; i < materials_in_file; i++)
    {
        // Todo: this is rather inefficient, files will be spammed with
        // separate loads and date checks, find a way to improve this.
        // May be doUpdates() should be checking individual files.
        LLPointer<LLLocalGLTFMaterial> unit = new LLLocalGLTFMaterial(filename, static_cast<S32>(i));

        // load material from file
        if (unit->updateSelf())
        {
            mMaterialList.emplace_back(unit);
            loaded_materials++;
        }
        else
        {
            LL_WARNS("GLTF") << "Attempted to add invalid or unreadable image file, attempt cancelled.\n"
                << "Filename: " << filename << LL_ENDL;

            LLSD notif_args;
            notif_args["FNAME"] = filename;
            LLNotificationsUtil::add("LocalGLTFVerifyFail", notif_args);

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

