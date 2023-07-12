/**
* @file llfloatersimpleoutfitsnapshot.h
* @brief Snapshot preview window for saving as an outfit thumbnail in visual outfit gallery
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

#ifndef LL_LLFLOATERSIMPLEOUTFITSNAPSHOT_H
#define LL_LLFLOATERSIMPLEOUTFITSNAPSHOT_H

#include "llfloater.h"
#include "llfloatersnapshot.h"
#include "lloutfitgallery.h"
#include "llsnapshotlivepreview.h"

///----------------------------------------------------------------------------
/// Class LLFloaterSimpleOutfitSnapshot
///----------------------------------------------------------------------------

class LLFloaterSimpleOutfitSnapshot : public LLFloaterSnapshotBase
{
    LOG_CLASS(LLFloaterSimpleOutfitSnapshot);

public:

    LLFloaterSimpleOutfitSnapshot(const LLSD& key);
    ~LLFloaterSimpleOutfitSnapshot();

    BOOL postBuild();
    void onOpen(const LLSD& key);
    void draw();

    static void update();

    static LLFloaterSimpleOutfitSnapshot* getInstance();
    static LLFloaterSimpleOutfitSnapshot* findInstance();
    void saveTexture();

    const LLRect& getThumbnailPlaceholderRect() { return mThumbnailPlaceholder->getRect(); }

    void setOutfitID(LLUUID id) { mOutfitID = id; }
    LLUUID getOutfitID() { return mOutfitID; }
    void setGallery(LLOutfitGallery* gallery) { mOutfitGallery = gallery; }

    void postSave();

    class Impl;
    friend class Impl;

private:
    void onSend();
    void onCancel();

    LLUUID mOutfitID;
    LLOutfitGallery* mOutfitGallery;
};

///----------------------------------------------------------------------------
/// Class LLFloaterSimpleOutfitSnapshot::Impl
///----------------------------------------------------------------------------

class LLFloaterSimpleOutfitSnapshot::Impl : public LLFloaterSnapshotBase::ImplBase
{
    LOG_CLASS(LLFloaterSimpleOutfitSnapshot::Impl);
public:
    Impl(LLFloaterSnapshotBase* floater)
        : LLFloaterSnapshotBase::ImplBase(floater)
    {}
    ~Impl()
    {}
    void updateResolution(void* data);

    static void onSnapshotUploadFinished(LLFloaterSnapshotBase* floater, bool status);

    LLPanelSnapshot* getActivePanel(LLFloaterSnapshotBase* floater, bool ok_if_not_found = true) { return NULL; }
    LLSnapshotModel::ESnapshotFormat getImageFormat(LLFloaterSnapshotBase* floater);
    std::string getSnapshotPanelPrefix();

    void updateControls(LLFloaterSnapshotBase* floater);

    void setStatus(EStatus status, bool ok = true, const std::string& msg = LLStringUtil::null);

private:
    LLSnapshotModel::ESnapshotLayerType getLayerType(LLFloaterSnapshotBase* floater);
    void setFinished(bool finished, bool ok = true, const std::string& msg = LLStringUtil::null) {};
};

///----------------------------------------------------------------------------
/// Class LLSimpleOutfitSnapshotFloaterView
///----------------------------------------------------------------------------

class LLSimpleOutfitSnapshotFloaterView : public LLFloaterView
{
public:
    struct Params
        : public LLInitParam::Block<Params, LLFloaterView::Params>
    {
    };

protected:
    LLSimpleOutfitSnapshotFloaterView(const Params& p);
    friend class LLUICtrlFactory;

public:
    virtual ~LLSimpleOutfitSnapshotFloaterView();
};

extern LLSimpleOutfitSnapshotFloaterView* gSimpleOutfitSnapshotFloaterView;

#endif // LL_LLFLOATERSIMPLEOUTFITSNAPSHOT_H
