/** 
* @file llfloatersimpleoutfitsnapshot.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloatersimpleoutfitsnapshot.h"

#include "llfloaterreg.h"
#include "llimagefiltersmanager.h"
#include "llstatusbar.h" // can_afford_transaction()
#include "llnotificationsutil.h"
#include "llagentbenefits.h"
#include "llviewercontrol.h"

LLSimpleOutfitSnapshotFloaterView* gSimpleOutfitSnapshotFloaterView = NULL;

const S32 OUTFIT_SNAPSHOT_WIDTH = 256;
const S32 OUTFIT_SNAPSHOT_HEIGHT = 256;

static LLDefaultChildRegistry::Register<LLSimpleOutfitSnapshotFloaterView> r("simple_snapshot_outfit_floater_view");

///----------------------------------------------------------------------------
/// Class LLFloaterSimpleOutfitSnapshot::Impl
///----------------------------------------------------------------------------

LLSnapshotModel::ESnapshotFormat LLFloaterSimpleOutfitSnapshot::Impl::getImageFormat(LLFloaterSnapshotBase* floater)
{
    return LLSnapshotModel::SNAPSHOT_FORMAT_PNG;
}

LLSnapshotModel::ESnapshotLayerType LLFloaterSimpleOutfitSnapshot::Impl::getLayerType(LLFloaterSnapshotBase* floater)
{
    return LLSnapshotModel::SNAPSHOT_TYPE_COLOR;
}

void LLFloaterSimpleOutfitSnapshot::Impl::updateControls(LLFloaterSnapshotBase* floater)
{
    LLSnapshotLivePreview* previewp = getPreviewView();
    updateResolution(floater);
    if (previewp)
    {
        previewp->setSnapshotType(LLSnapshotModel::ESnapshotType::SNAPSHOT_TEXTURE);
        previewp->setSnapshotFormat(LLSnapshotModel::ESnapshotFormat::SNAPSHOT_FORMAT_PNG);
        previewp->setSnapshotBufferType(LLSnapshotModel::ESnapshotLayerType::SNAPSHOT_TYPE_COLOR);
    }
}

std::string LLFloaterSimpleOutfitSnapshot::Impl::getSnapshotPanelPrefix()
{
    return "panel_outfit_snapshot_";
}

void LLFloaterSimpleOutfitSnapshot::Impl::updateResolution(void* data)
{
    LLFloaterSimpleOutfitSnapshot *view = (LLFloaterSimpleOutfitSnapshot *)data;

    if (!view)
    {
        llassert(view);
        return;
    }

    S32 width = OUTFIT_SNAPSHOT_WIDTH;
    S32 height = OUTFIT_SNAPSHOT_HEIGHT;

    LLSnapshotLivePreview* previewp = getPreviewView();
    if (previewp)
    {
        S32 original_width = 0, original_height = 0;
        previewp->getSize(original_width, original_height);

        if (gSavedSettings.getBOOL("RenderHUDInSnapshot"))
        { //clamp snapshot resolution to window size when showing UI HUD in snapshot
            width = llmin(width, gViewerWindow->getWindowWidthRaw());
            height = llmin(height, gViewerWindow->getWindowHeightRaw());
        }

        llassert(width > 0 && height > 0);

        previewp->setSize(width, height);

        if (original_width != width || original_height != height)
        {
            // hide old preview as the aspect ratio could be wrong
            checkAutoSnapshot(previewp, FALSE);
            previewp->updateSnapshot(TRUE);
        }
    }
}

void LLFloaterSimpleOutfitSnapshot::Impl::setStatus(EStatus status, bool ok, const std::string& msg)
{
    switch (status)
    {
    case STATUS_READY:
        mFloater->setCtrlsEnabled(true);
        break;
    case STATUS_WORKING:
        mFloater->setCtrlsEnabled(false);
        break;
    case STATUS_FINISHED:
        mFloater->setCtrlsEnabled(true);
        break;
    }

    mStatus = status;
}

///----------------------------------------------------------------re------------
/// Class LLFloaterSimpleOutfitSnapshot
///----------------------------------------------------------------------------

LLFloaterSimpleOutfitSnapshot::LLFloaterSimpleOutfitSnapshot(const LLSD& key)
    : LLFloaterSnapshotBase(key),
    mOutfitGallery(NULL)
{
    impl = new Impl(this);
}

LLFloaterSimpleOutfitSnapshot::~LLFloaterSimpleOutfitSnapshot()
{
}

BOOL LLFloaterSimpleOutfitSnapshot::postBuild()
{
    getChild<LLUICtrl>("save_btn")->setLabelArg("[UPLOAD_COST]", std::to_string(LLAgentBenefitsMgr::current().getTextureUploadCost()));

    childSetAction("new_snapshot_btn", ImplBase::onClickNewSnapshot, this);
    childSetAction("save_btn", boost::bind(&LLFloaterSimpleOutfitSnapshot::onSend, this));
    childSetAction("cancel_btn", boost::bind(&LLFloaterSimpleOutfitSnapshot::onCancel, this));

    mThumbnailPlaceholder = getChild<LLUICtrl>("thumbnail_placeholder");

    // create preview window
    LLRect full_screen_rect = getRootView()->getRect();
    LLSnapshotLivePreview::Params p;
    p.rect(full_screen_rect);
    LLSnapshotLivePreview* previewp = new LLSnapshotLivePreview(p);
    LLView* parent_view = gSnapshotFloaterView->getParent();

    parent_view->removeChild(gSnapshotFloaterView);
    // make sure preview is below snapshot floater
    parent_view->addChild(previewp);
    parent_view->addChild(gSnapshotFloaterView);

    //move snapshot floater to special purpose snapshotfloaterview
    gFloaterView->removeChild(this);
    gSnapshotFloaterView->addChild(this);

    impl->mPreviewHandle = previewp->getHandle();
    previewp->setContainer(this);
    impl->updateControls(this);
    impl->setAdvanced(true);
    impl->setSkipReshaping(true);

    previewp->mKeepAspectRatio = FALSE;
    previewp->setThumbnailPlaceholderRect(getThumbnailPlaceholderRect());
    previewp->setAllowRenderUI(false);

    return TRUE;
}
const S32 PREVIEW_OFFSET_X = 12;
const S32 PREVIEW_OFFSET_Y = 70;

void LLFloaterSimpleOutfitSnapshot::draw()
{
    LLSnapshotLivePreview* previewp = getPreviewView();

    if (previewp && (previewp->isSnapshotActive() || previewp->getThumbnailLock()))
    {
        // don't render snapshot window in snapshot, even if "show ui" is turned on
        return;
    }

    LLFloater::draw();

    if (previewp && !isMinimized() && mThumbnailPlaceholder->getVisible())
    {		
        if(previewp->getThumbnailImage())
        {
            bool working = impl->getStatus() == ImplBase::STATUS_WORKING;
            const LLRect& thumbnail_rect = getThumbnailPlaceholderRect();
            const S32 thumbnail_w = previewp->getThumbnailWidth();
            const S32 thumbnail_h = previewp->getThumbnailHeight();

            S32 offset_x = PREVIEW_OFFSET_X;
            S32 offset_y = PREVIEW_OFFSET_Y;

            gGL.matrixMode(LLRender::MM_MODELVIEW);
            // Apply floater transparency to the texture unless the floater is focused.
            F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
            LLColor4 color = working ? LLColor4::grey4 : LLColor4::white;
            gl_draw_scaled_image(offset_x, offset_y, 
                thumbnail_w, thumbnail_h,
                previewp->getThumbnailImage(), color % alpha);
#if LL_DARWIN
            std::string alpha_color = getTransparencyType() == TT_ACTIVE ? "OutfitSnapshotMacMask" : "OutfitSnapshotMacMask2";
#else
            std::string alpha_color = getTransparencyType() == TT_ACTIVE ? "FloaterFocusBackgroundColor" : "DkGray";
#endif

            previewp->drawPreviewRect(offset_x, offset_y, LLUIColorTable::instance().getColor(alpha_color));

            gGL.pushUIMatrix();
            LLUI::translate((F32) thumbnail_rect.mLeft, (F32) thumbnail_rect.mBottom);
            mThumbnailPlaceholder->draw();
            gGL.popUIMatrix();
        }
    }
    impl->updateLayout(this);
}

void LLFloaterSimpleOutfitSnapshot::onOpen(const LLSD& key)
{
    LLSnapshotLivePreview* preview = getPreviewView();
    if (preview)
    {
        preview->updateSnapshot(TRUE);
    }
    focusFirstItem(FALSE);
    gSnapshotFloaterView->setEnabled(TRUE);
    gSnapshotFloaterView->setVisible(TRUE);
    gSnapshotFloaterView->adjustToFitScreen(this, FALSE);

    impl->updateControls(this);
    impl->setStatus(ImplBase::STATUS_READY);
}

void LLFloaterSimpleOutfitSnapshot::onCancel()
{
    closeFloater();
}

void LLFloaterSimpleOutfitSnapshot::onSend()
{
    S32 expected_upload_cost = LLAgentBenefitsMgr::current().getTextureUploadCost();
    if (can_afford_transaction(expected_upload_cost))
    {
        saveTexture();
        postSave();
    }
    else
    {
        LLSD args;
        args["COST"] = llformat("%d", expected_upload_cost);
        LLNotificationsUtil::add("ErrorPhotoCannotAfford", args);
        inventorySaveFailed();
    }
}

void LLFloaterSimpleOutfitSnapshot::postSave()
{
    impl->setStatus(ImplBase::STATUS_WORKING);
}

// static 
void LLFloaterSimpleOutfitSnapshot::update()
{
    LLFloaterSimpleOutfitSnapshot* inst = findInstance();
    if (inst != NULL)
    {
        inst->impl->updateLivePreview();
    }
}


// static
LLFloaterSimpleOutfitSnapshot* LLFloaterSimpleOutfitSnapshot::findInstance()
{
    return LLFloaterReg::findTypedInstance<LLFloaterSimpleOutfitSnapshot>("simple_outfit_snapshot");
}

// static
LLFloaterSimpleOutfitSnapshot* LLFloaterSimpleOutfitSnapshot::getInstance()
{
    return LLFloaterReg::getTypedInstance<LLFloaterSimpleOutfitSnapshot>("simple_outfit_snapshot");
}

void LLFloaterSimpleOutfitSnapshot::saveTexture()
{
     LLSnapshotLivePreview* previewp = getPreviewView();
    if (!previewp)
    {
        llassert(previewp != NULL);
        return;
    }

    if (mOutfitGallery)
    {
        mOutfitGallery->onBeforeOutfitSnapshotSave();
    }
    previewp->saveTexture(TRUE, getOutfitID().asString());
    if (mOutfitGallery)
    {
        mOutfitGallery->onAfterOutfitSnapshotSave();
    }
    closeFloater();
}

///----------------------------------------------------------------------------
/// Class LLSimpleOutfitSnapshotFloaterView
///----------------------------------------------------------------------------

LLSimpleOutfitSnapshotFloaterView::LLSimpleOutfitSnapshotFloaterView(const Params& p) : LLFloaterView(p)
{
}

LLSimpleOutfitSnapshotFloaterView::~LLSimpleOutfitSnapshotFloaterView()
{
}
