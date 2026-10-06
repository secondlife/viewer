/**
 * @file llfloatermediamonitor.cpp
 * @author Callum Prentice
 * @brief Debug floater containing a list of active media sources
 *        and data for each such as URL, priority, location etc.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llagent.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llclipboard.h"
#include "lleventtimer.h"
#include "llformat.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llslider.h"
#include "lltoggleablemenu.h"
#include "lluictrlfactory.h"
#include "lluuid.h"
#include "llviewercamera.h"
#include "llviewermedia.h"
#include "llviewermediafocus.h"
#include "llviewermenu.h"
#include "llvovolume.h"

#include "llfloatermediamonitor.h"

namespace
{
    // Column order as declared in floater_media_monitor.xml -- must stay in sync.
    constexpr S32 DISTANCE_COLUMN = 6;

    // "ui"/"prim"/"parcel" (LLViewerMedia::getEmbeddedBrowserDebugInfo()'s "kind") -> display label.
    std::string mediaKindToDisplayLabel(const std::string& kind)
    {
        if (kind == "ui")
        {
            return "UI";
        }
        if (kind == "parcel")
        {
            return "Parcel";
        }
        if (kind == "prim")
        {
            return "Prim";
        }
        return kind;
    }

    constexpr F32 REFRESH_PERIOD_SECONDS = 2.f;
}

LLFloaterMediaMonitor::LLFloaterMediaMonitor(const LLSD& key)
    :   LLFloater("floater_media_monitor")
    // :   LLFloater("floater_inventory_thumbnails_helper")
{
    // Registered here, before postBuild()'s createFromFile() builds the context menu
    // from menu_media_monitor.xml, which references this by name -- matches NMP's own
    // "SelectedMediaCtrl.Action" registration in its own constructor for the same reason.
    mCommitCallbackRegistrar.add("MediaMonitor.Action",
                                 [this](LLUICtrl*, const LLSD& data)
                                 {
                                     onMenuAction(data);
                                 });
}

LLFloaterMediaMonitor::~LLFloaterMediaMonitor()
{
    delete mRefreshTimer;
}

void LLFloaterMediaMonitor::onOpen(const LLSD& key)
{
    updateDisplayList();

    if (!mRefreshTimer)
    {
        mRefreshTimer = LLEventTimer::run_every(REFRESH_PERIOD_SECONDS, boost::bind(&LLFloaterMediaMonitor::updateDisplayList, this));
    }
}

void LLFloaterMediaMonitor::onClose(bool app_quitting)
{
    delete mRefreshTimer;
    mRefreshTimer = nullptr;
}

bool LLFloaterMediaMonitor::postBuild()
{
    mActiveMediaList = getChild<LLScrollListCtrl>("active_media_list");
    mZoomBtn = getChild<LLButton>("zoom_btn");
    mUnzoomBtn = getChild<LLButton>("unzoom_btn");
    mVolumeSlider = getChild<LLSlider>("volume_slider");

    // Refresh list of media manually
    getChild<LLUICtrl>("refresh_btn")->setCommitCallback(boost::bind(&LLFloaterMediaMonitor::onRefreshBtn, this));

    // Close the floater
    getChild<LLUICtrl>("close_btn")->setCommitCallback(boost::bind(&LLFloaterMediaMonitor::onCloseBtn, this));

    mZoomBtn->setCommitCallback(boost::bind(&LLFloaterMediaMonitor::onZoomBtn, this));
    mUnzoomBtn->setCommitCallback(boost::bind(&LLFloaterMediaMonitor::onUnzoomBtn, this));
    mVolumeSlider->setCommitCallback(boost::bind(&LLFloaterMediaMonitor::onVolumeChange, this));

    // Selecting a row (left-click) needs the bottom controls refreshed too, not just
    // right-click -- mirrors NMP's own selection-driven updateControls() pattern.
    mActiveMediaList->setCommitCallback(boost::bind(&LLFloaterMediaMonitor::updateSelectedMediaControls, this));
    mActiveMediaList->setRightMouseDownCallback(boost::bind(&LLFloaterMediaMonitor::onRightClickItem, this, _1, _2, _3));

    mContextMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
        "menu_media_monitor.xml",
        gMenuHolder,
        LLViewerMenuHolderGL::child_registry_t::instance());

    // Distance ascending by default -- nearest/most-relevant media first.
    mActiveMediaList->sortByColumnIndex(DISTANCE_COLUMN, true);

    mBaseTitle = getTitle();

    updateSelectedMediaControls();

    return true;
}

void LLFloaterMediaMonitor::updateDisplayList()
{
    // Selection doesn't survive deleteAllItems() (it destroys every LLScrollListItem and
    // resets the list's own notion of "selected" outright) -- this list fully rebuilds
    // itself every REFRESH_PERIOD_SECONDS, so without this, the bottom controls would
    // silently reset out from under whoever's using them on every single refresh tick.
    LLUUID selected_id = mActiveMediaList->getValue().asUUID();

    mActiveMediaList->deleteAllItems();

    LLSD media_list = LLViewerMedia::getInstance()->getEmbeddedBrowserDebugInfo();

    for (LLSD::array_const_iterator it = media_list.beginArray(); it != media_list.endArray(); ++it)
    {
        const LLSD& media = *it;

        std::string slurl = media["slurl"].asString();
        bool has_distance = media.has("distance");

        LLSD row;
        row["id"] = media["id"];
        row["columns"][0]["column"] = "slot";
        row["columns"][0]["value"] = media["slot"];
        row["columns"][1]["column"] = "priority";
        row["columns"][1]["value"] = media["priority_label"];
        row["columns"][2]["column"] = "media_type";
        row["columns"][2]["value"] = mediaKindToDisplayLabel(media["kind"].asString());
        row["columns"][3]["column"] = "backend";
        row["columns"][3]["value"] = media["backend"];
        row["columns"][4]["column"] = "url";
        row["columns"][4]["value"] = media["name"]; // page title, falling back to the raw URL
        row["columns"][5]["column"] = "location";
        row["columns"][5]["value"] = slurl.empty() ? "-" : slurl;
        row["columns"][6]["column"] = "distance";
        row["columns"][6]["value"] = has_distance ? llformat("%.1f", media["distance"].asReal()) : "-";

        mActiveMediaList->addElement(row);
    }

    mActiveMediaList->selectByID(selected_id); // false (nothing selected) if that media's gone
    updateSelectedMediaControls();

    S32 count = static_cast<S32>(media_list.size());
    S32 max_instances = LLViewerMedia::getInstance()->getMaxInstances();
    setTitle(llformat("%s - %d/%d instances", mBaseTitle.c_str(), count, max_instances));
}

void LLFloaterMediaMonitor::updateSelectedMediaControls()
{
    LLUUID selected_id = mActiveMediaList->getValue().asUUID();
    LLViewerMediaImpl* impl = selected_id.isNull() ? nullptr :
        LLViewerMedia::getInstance()->getMediaImplFromTextureID(selected_id);

    if (!impl)
    {
        mZoomBtn->setEnabled(false);
        mZoomBtn->setVisible(true);
        mUnzoomBtn->setVisible(false);
        mVolumeSlider->setEnabled(false);
        return;
    }

    // Only Prim media resolves to a single object for the camera to zoom onto -- UI
    // floaters and Parcel media have no equivalent, matching how NMP's own zoom
    // control is gated (showBasicControls(..., include_zoom = !impl->isParcelMedia())).
    bool can_zoom = !impl->getUsedInUI() && !impl->isParcelMedia();
    bool is_zoomed = (mZoomedMediaId == selected_id);

    // Zoom/Unzoom swap by visibility, not a single toggling button -- matches NMP's
    // own mZoomCtrl/mUnzoomCtrl pair exactly (llpanelnearbymedia.cpp's
    // showBasicControls()/showTimeBasedControls()).
    mZoomBtn->setEnabled(can_zoom);
    mZoomBtn->setVisible(!is_zoomed);
    mUnzoomBtn->setVisible(can_zoom && is_zoomed);

    mVolumeSlider->setEnabled(true);
    mVolumeSlider->setValue(impl->getVolume());
}

void LLFloaterMediaMonitor::onZoomBtn()
{
    LLUUID selected_id = mActiveMediaList->getValue().asUUID();
    LLViewerMediaImpl* impl = selected_id.isNull() ? nullptr :
        LLViewerMedia::getInstance()->getMediaImplFromTextureID(selected_id);
    LLVOVolume* obj = impl ? impl->getSomeObject() : nullptr;
    if (!impl || !obj)
    {
        return;
    }

    // Same face/normal approximation LLViewerMediaFocus::focusZoomOnMedia() itself
    // uses -- there's no real pick position to work from here either, just an
    // arbitrarily-selected row in a list. Now that LLVOVolume::getApproximateFaceNormal()'s
    // own shadowing bug is fixed, this reliably returns a real, face-aligned normal
    // instead of always falling back to "dolly along the current camera axis."
    S32 face = obj->getFaceIndexWithMediaImpl(impl, -1);
    LLVector3 normal = obj->getApproximateFaceNormal((U8)face);
    if (normal.isNull())
    {
        normal = LLViewerCamera::getInstance()->getAtAxis();
        normal *= -1.0f;
    }

    LLViewerMediaFocus::getInstance()->setFocusFace(obj, face, impl, normal);

    // 1.0 would put the object's bounding box edge exactly at the viewport edge on
    // the constraining axis (see setCameraZoom()'s own distance math) -- confirmed
    // correctly aligned once LLVOVolume::getApproximateFaceNormal()'s own bug was
    // fixed, but still felt too close even at 1.15, hence the larger margin here.
    // Called directly (a static method) rather than through LLPanelPrimMediaControls'
    // shared EZoomLevel state machine, so this stays fully self-contained to Media
    // Monitor -- NMP's and the in-world media controls' own zoom are untouched.
    static const F32 EXACT_FIT_PADDING = 1.5f;
    LLViewerMediaFocus::setCameraZoom(obj, normal, EXACT_FIT_PADDING, true);

    // setCameraPosAndFocusGlobal() (called by setCameraZoom() above) scales its own
    // animation duration by how far the camera's focus point has to travel -- up to
    // 10 seconds for a big jump (see its own ANIM_METERS_PER_SECOND/MAX_ANIM_SECONDS
    // math in llagentcamera.cpp) -- and by the time it returns, mCameraAnimating is
    // already true, so a plain setAnimationDuration() call here would be a no-op:
    // its own "never cut an existing animation short" guard (llagentcamera.cpp
    // setAnimationDuration()) takes the MAX of the requested value and whatever's
    // already running. Toggling mCameraAnimating off and back on is what actually
    // gets a shorter value to stick, still animated (not an instant snap) -- overridden
    // here rather than touching that shared math, so only Media Monitor's own zoom
    // speeds up.
    static const F32 ZOOM_ANIM_SECONDS = 1.5f; // half of the ~3s observed before this fix actually took effect
    gAgentCamera.setCameraAnimating(false);
    gAgentCamera.setAnimationDuration(ZOOM_ANIM_SECONDS);
    gAgentCamera.setCameraAnimating(true);

    mZoomedMediaId = selected_id;
    updateSelectedMediaControls();
}

void LLFloaterMediaMonitor::onUnzoomBtn()
{
    gAgentCamera.setFocusOnAvatar(true, ANIMATE);
    mZoomedMediaId.setNull();
    updateSelectedMediaControls();
}

void LLFloaterMediaMonitor::onVolumeChange()
{
    LLUUID selected_id = mActiveMediaList->getValue().asUUID();
    LLViewerMediaImpl* impl = selected_id.isNull() ? nullptr :
        LLViewerMedia::getInstance()->getMediaImplFromTextureID(selected_id);
    if (!impl)
    {
        return;
    }
    // Sets mRequestedVolume, the ceiling updateVolume()'s own distance-rolloff computation
    // multiplies against every frame -- this doesn't fight that falloff, it caps it.
    impl->setVolume(mVolumeSlider->getValueF32());
}

void LLFloaterMediaMonitor::onRightClickItem(LLUICtrl* ctrl, S32 x, S32 y)
{
    LLScrollListItem* item = mActiveMediaList->hitItem(x, y);
    if (!item)
    {
        return;
    }
    mActiveMediaList->selectByID(item->getUUID());
    updateSelectedMediaControls();

    if (mContextMenu)
    {
        mContextMenu->buildDrawLabels();
        mContextMenu->updateParent(LLMenuGL::sMenuContainer);
        // x/y are relative to ctrl (the list, where this callback fired), not this
        // floater -- pass ctrl as the reference view, matching LLFloaterBump's own
        // identical setRightMouseDownCallback + showPopup pattern.
        LLMenuGL::showPopup(ctrl, mContextMenu, x, y);
    }
}

void LLFloaterMediaMonitor::onMenuAction(const LLSD& userdata)
{
    const std::string command_name = userdata.asString();
    if (command_name == "copy_url")
    {
        // Resolved via the impl, not read from the "url" column's displayed text --
        // that column shows the page title now (see updateDisplayList()), so the
        // displayed string usually isn't the URL at all.
        LLUUID selected_id = mActiveMediaList->getValue().asUUID();
        LLViewerMediaImpl* impl = selected_id.isNull() ? nullptr :
            LLViewerMedia::getInstance()->getMediaImplFromTextureID(selected_id);
        if (!impl)
        {
            return;
        }
        std::string url = impl->getCurrentMediaURL();
        LLClipboard::instance().reset();
        LLClipboard::instance().copyToClipboard(utf8str_to_wstring(url), 0, static_cast<S32>(url.size()));
    }
}

void LLFloaterMediaMonitor::onRefreshBtn()
{
    updateDisplayList();
}

void LLFloaterMediaMonitor::onCloseBtn()
{
    closeFloater();
}
