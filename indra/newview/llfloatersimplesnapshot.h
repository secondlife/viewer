/**
* @file llfloatersimplesnapshot.h
* @brief Snapshot preview window for saving as a thumbnail
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

#ifndef LL_LLFLOATERSIMPLESNAPSHOT_H
#define LL_LLFLOATERSIMPLESNAPSHOT_H

#include "llfloater.h"
#include "llfloatersnapshot.h"
#include "llsnapshotlivepreview.h"

///----------------------------------------------------------------------------
/// Class LLFloaterSimpleSnapshot
///----------------------------------------------------------------------------

class LLFloaterSimpleSnapshot : public LLFloaterSnapshotBase
{
    LOG_CLASS(LLFloaterSimpleSnapshot);

public:

    LLFloaterSimpleSnapshot(const LLSD& key);
    ~LLFloaterSimpleSnapshot();

    bool postBuild();
    void onOpen(const LLSD& key);
    void draw();

    static void update();

    static LLFloaterSimpleSnapshot* getInstance(const LLSD &key);
    static LLFloaterSimpleSnapshot* findInstance(const LLSD &key);
    void saveTexture();

    const LLRect& getThumbnailPlaceholderRect() { return mThumbnailPlaceholder->getRect(); }

    void setInventoryId(const LLUUID &inventory_id) { mInventoryId = inventory_id; }
    LLUUID getInventoryId() { return mInventoryId; }
    void setTaskId(const LLUUID &task_id) { mTaskId = task_id; }
    void setOwner(LLView *owner_view) { mOwner = owner_view; }

    void postSave();

    typedef boost::function<void(const LLUUID& asset_id)> completion_t;
    void setComplectionCallback(completion_t callback) { mUploadCompletionCallback = callback; }
    static void uploadThumbnail(const std::string &file_path,
                                const LLUUID &inventory_id,
                                const LLUUID &task_id,
                                completion_t callback = completion_t());
    static void uploadThumbnail(LLPointer<LLImageRaw> raw_image,
                                const LLUUID& inventory_id,
                                const LLUUID& task_id,
                                completion_t callback = completion_t());

    class Impl;
    friend class Impl;

    static const S32 THUMBNAIL_SNAPSHOT_DIM_MAX;
    static const S32 THUMBNAIL_SNAPSHOT_DIM_MIN;

private:
    void onSend();
    void onCancel();

    // uploads upload-ready file
    static void uploadImageUploadFile(const std::string &temp_file,
                                      const LLUUID &inventory_id,
                                      const LLUUID &task_id,
                                      completion_t callback);

    LLUUID mInventoryId;
    LLUUID mTaskId;

    LLView* mOwner;
    F32  mContextConeOpacity;
    completion_t mUploadCompletionCallback;
};

///----------------------------------------------------------------------------
/// Class LLFloaterSimpleSnapshot::Impl
///----------------------------------------------------------------------------

class LLFloaterSimpleSnapshot::Impl : public LLFloaterSnapshotBase::ImplBase
{
    LOG_CLASS(LLFloaterSimpleSnapshot::Impl);
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

class LLSimpleSnapshotFloaterView : public LLFloaterView
{
public:
    struct Params
        : public LLInitParam::Block<Params, LLFloaterView::Params>
    {
    };

protected:
    LLSimpleSnapshotFloaterView(const Params& p);
    friend class LLUICtrlFactory;

public:
    virtual ~LLSimpleSnapshotFloaterView();
};

extern LLSimpleSnapshotFloaterView* gSimpleOutfitSnapshotFloaterView;

#endif // LL_LLFLOATERSIMPLESNAPSHOT_H
