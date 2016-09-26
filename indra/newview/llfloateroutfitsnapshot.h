/**
 * @file llfloateroutfitsnapshot.h
 * @brief Snapshot preview window for saving as an outfit thumbnail in visual outfit gallery
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2016, Linden Research, Inc.
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

#ifndef LL_LLFLOATEROUTFITSNAPSHOT_H
#define LL_LLFLOATEROUTFITSNAPSHOT_H

#include "llfloater.h"
#include "llfloatersnapshot.h"
#include "lloutfitgallery.h"
#include "llsnapshotlivepreview.h"

///----------------------------------------------------------------------------
/// Class LLFloaterOutfitSnapshot
///----------------------------------------------------------------------------

class LLFloaterOutfitSnapshot : public LLFloaterSnapshotBase
{
    LOG_CLASS(LLFloaterOutfitSnapshot);

public:

    LLFloaterOutfitSnapshot(const LLSD& key);
    /*virtual*/ ~LLFloaterOutfitSnapshot();

    /*virtual*/ BOOL postBuild();
    /*virtual*/ void onOpen(const LLSD& key);

    static void update();

    void onExtendFloater();

    static LLFloaterOutfitSnapshot* getInstance();
    static LLFloaterOutfitSnapshot* findInstance();
    /*virtual*/ void saveTexture();

    const LLRect& getThumbnailPlaceholderRect() { return mThumbnailPlaceholder->getRect(); }

    void setOutfitID(LLUUID id) { mOutfitID = id; }
    LLUUID getOutfitID() { return mOutfitID; }
    void setGallery(LLOutfitGallery* gallery) { mOutfitGallery = gallery; }

    class Impl;
    friend class Impl;
private:

    LLUUID mOutfitID;
    LLOutfitGallery* mOutfitGallery;
};

///----------------------------------------------------------------------------
/// Class LLFloaterOutfitSnapshot::Impl
///----------------------------------------------------------------------------

class LLFloaterOutfitSnapshot::Impl : public LLFloaterSnapshotBase::ImplBase
{
    LOG_CLASS(LLFloaterOutfitSnapshot::Impl);
public:
    Impl(LLFloaterSnapshotBase* floater)
        : LLFloaterSnapshotBase::ImplBase(floater)
    {}
    ~Impl()
    {}
    void updateResolution(void* data);

    static void onSnapshotUploadFinished(LLFloaterSnapshotBase* floater, bool status);

    /*virtual*/ LLPanelSnapshot* getActivePanel(LLFloaterSnapshotBase* floater, bool ok_if_not_found = true);
    /*virtual*/ LLSnapshotModel::ESnapshotFormat getImageFormat(LLFloaterSnapshotBase* floater);
    /*virtual*/ std::string getSnapshotPanelPrefix();

    /*virtual*/ void updateControls(LLFloaterSnapshotBase* floater);

private:
    /*virtual*/ LLSnapshotModel::ESnapshotLayerType getLayerType(LLFloaterSnapshotBase* floater);
    /*virtual*/ void setFinished(bool finished, bool ok = true, const std::string& msg = LLStringUtil::null);
};

///----------------------------------------------------------------------------
/// Class LLOutfitSnapshotFloaterView
///----------------------------------------------------------------------------

class LLOutfitSnapshotFloaterView : public LLFloaterView
{
public:
    struct Params
        : public LLInitParam::Block<Params, LLFloaterView::Params>
    {
    };

protected:
    LLOutfitSnapshotFloaterView(const Params& p);
    friend class LLUICtrlFactory;

public:
    virtual ~LLOutfitSnapshotFloaterView();
};

extern LLOutfitSnapshotFloaterView* gOutfitSnapshotFloaterView;

#endif // LL_LLFLOATEROUTFITSNAPSHOT_H
