/**
 * @file lloutfitobserver.cpp
 * @brief Outfit observer facade.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llappearancemgr.h"
#include "lloutfitobserver.h"
#include "llinventorymodel.h"
#include "llviewerinventory.h"

LLOutfitObserver::LLOutfitObserver() :
    mCOFLastVersion(LLViewerInventoryCategory::VERSION_UNKNOWN)
{
    mItemNameHash.finalize();
    gInventory.addObserver(this);
}

LLOutfitObserver::~LLOutfitObserver()
{
    if (gInventory.containsObserver(this))
    {
        gInventory.removeObserver(this);
    }
}

void LLOutfitObserver::changed(U32 mask)
{
    if (!gInventory.isInventoryUsable())
        return;

    checkCOF();

    checkBaseOutfit();
}

// static
S32 LLOutfitObserver::getCategoryVersion(const LLUUID& cat_id)
{
    LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
    if (!cat)
        return LLViewerInventoryCategory::VERSION_UNKNOWN;

    return cat->getVersion();
}

// static
const std::string& LLOutfitObserver::getCategoryName(const LLUUID& cat_id)
{
    LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
    if (!cat)
        return LLStringUtil::null;

    return cat->getName();
}

bool LLOutfitObserver::checkCOF()
{
    LLUUID cof = LLAppearanceMgr::getInstance()->getCOF();
    if (cof.isNull())
        return false;

    bool cof_changed = false;
    LLMD5 item_name_hash = gInventory.hashDirectDescendentNames(cof);
    if (item_name_hash != mItemNameHash)
    {
        cof_changed = true;
        mItemNameHash = item_name_hash;
    }

    S32 cof_version = getCategoryVersion(cof);
    if (cof_version != mCOFLastVersion)
    {
        cof_changed = true;
        mCOFLastVersion = cof_version;
    }

    if (!cof_changed)
        return false;
    
    // dirtiness state should be updated before sending signal
    LLAppearanceMgr::getInstance()->updateIsDirty();
    mCOFChanged();

    return true;
}

void LLOutfitObserver::checkBaseOutfit()
{
    LLUUID baseoutfit_id =
            LLAppearanceMgr::getInstance()->getBaseOutfitUUID();

    if (baseoutfit_id == mBaseOutfitId)
    {
        if (baseoutfit_id.isNull())
            return;

        const S32 baseoutfit_ver = getCategoryVersion(baseoutfit_id);
        const std::string& baseoutfit_name = getCategoryName(baseoutfit_id);

        if (baseoutfit_ver == mBaseOutfitLastVersion
                // renaming category doesn't change version, so it's need to check it
                && baseoutfit_name == mLastBaseOutfitName)
            return;
    }
    else
    {
        mBaseOutfitId = baseoutfit_id;
        mBOFReplaced();

        if (baseoutfit_id.isNull())
            return;
    }

    mBaseOutfitLastVersion = getCategoryVersion(mBaseOutfitId);
    mLastBaseOutfitName = getCategoryName(baseoutfit_id);

    LLAppearanceMgr& app_mgr = LLAppearanceMgr::instance();
    // dirtiness state should be updated before sending signal
    app_mgr.updateIsDirty();
    mBOFChanged();

    if (mLastOutfitDirtiness != app_mgr.isOutfitDirty())
    {
        if(!app_mgr.isOutfitDirty())
        {
            mCOFSaved();
        }
        mLastOutfitDirtiness = app_mgr.isOutfitDirty();
    }
}
