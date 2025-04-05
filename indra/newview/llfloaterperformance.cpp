/**
 * @file llfloaterperformance.cpp
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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
#include "llfloaterperformance.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llappearancemgr.h"
#include "llavataractions.h"
#include "llavatarrendernotifier.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfeaturemanager.h"
#include "llfloaterpreference.h" // LLAvatarComplexityControls
#include "llfloaterreg.h"
#include "llnamelistctrl.h"
#include "llnotificationsutil.h"
#include "llperfstats.h"
#include "llpresetsmanager.h"
#include "llradiogroup.h"
#include "llsliderctrl.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llworld.h"
#include "pipeline.h"

const F32 REFRESH_INTERVAL = 1.0f;
const S32 BAR_LEFT_PAD = 2;
const S32 BAR_RIGHT_PAD = 5;
const S32 BAR_BOTTOM_PAD = 9;

class LLExceptionsContextMenu : public LLListContextMenu
{
public:
    LLExceptionsContextMenu(LLFloaterPerformance* floater_settings)
        :   mFloaterPerformance(floater_settings)
    {}
protected:
    LLContextMenu* createMenu()
    {
        LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
        LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
        registrar.add("Settings.SetRendering", boost::bind(&LLFloaterPerformance::onCustomAction, mFloaterPerformance, _2, mUUIDs.front()));
        enable_registrar.add("Settings.IsSelected", boost::bind(&LLFloaterPerformance::isActionChecked, mFloaterPerformance, _2, mUUIDs.front()));
        LLContextMenu* menu = createFromFile("menu_avatar_rendering_settings.xml");

        return menu;
    }

    LLFloaterPerformance* mFloaterPerformance;
};

LLFloaterPerformance::LLFloaterPerformance(const LLSD& key)
:   LLFloater(key),
    mUpdateTimer(new LLTimer())
{
    mContextMenu = new LLExceptionsContextMenu(this);
}

LLFloaterPerformance::~LLFloaterPerformance()
{
    mMaxARTChangedSignal.disconnect();
    delete mContextMenu;
    delete mUpdateTimer;
}

bool LLFloaterPerformance::postBuild()
{
    mMainPanel = getChild<LLPanel>("panel_performance_main");
    mNearbyPanel = getChild<LLPanel>("panel_performance_nearby");
    mComplexityPanel = getChild<LLPanel>("panel_performance_complexity");
    mSettingsPanel = getChild<LLPanel>("panel_performance_preferences");
    mHUDsPanel = getChild<LLPanel>("panel_performance_huds");
    mAutoadjustmentsPanel = getChild<LLPanel>("panel_performance_autoadjustments");

    getChild<LLPanel>("nearby_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mNearbyPanel));
    getChild<LLPanel>("complexity_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mComplexityPanel));
    getChild<LLPanel>("settings_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mSettingsPanel));
    getChild<LLPanel>("huds_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mHUDsPanel));
    getChild<LLPanel>("autoadjustments_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mAutoadjustmentsPanel));

    initBackBtn(mNearbyPanel);
    initBackBtn(mComplexityPanel);
    initBackBtn(mSettingsPanel);
    initBackBtn(mHUDsPanel);
    initBackBtn(mAutoadjustmentsPanel);

    mHUDList = mHUDsPanel->getChild<LLNameListCtrl>("hud_list");
    mHUDList->setNameListType(LLNameListCtrl::SPECIAL);
    mHUDList->setHoverIconName("StopReload_Off");
    mHUDList->setIconClickedCallback(boost::bind(&LLFloaterPerformance::detachObject, this, _1));

    mObjectList = mComplexityPanel->getChild<LLNameListCtrl>("obj_list");
    mObjectList->setNameListType(LLNameListCtrl::SPECIAL);
    mObjectList->setHoverIconName("StopReload_Off");
    mObjectList->setIconClickedCallback(boost::bind(&LLFloaterPerformance::detachObject, this, _1));

    mSettingsPanel->getChild<LLButton>("advanced_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickAdvanced, this));
    mSettingsPanel->getChild<LLButton>("defaults_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickDefaults, this));
    mSettingsPanel->getChild<LLRadioGroup>("graphics_quality")->setCommitCallback(boost::bind(&LLFloaterPerformance::onChangeQuality, this, _2));
    mSettingsPanel->getChild<LLCheckBoxCtrl>("advanced_lighting_model")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::onClickAdvancedLighting, this));
    mSettingsPanel->getChild<LLComboBox>("ShadowDetail")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::onClickShadows, this));

    mNearbyPanel->getChild<LLButton>("exceptions_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickExceptions, this));
    mNearbyPanel->getChild<LLCheckBoxCtrl>("hide_avatars")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickHideAvatars, this));
    mNearbyPanel->getChild<LLCheckBoxCtrl>("hide_avatars")->set(!LLPipeline::hasRenderTypeControl(LLPipeline::RENDER_TYPE_AVATAR));
    mNearbyList = mNearbyPanel->getChild<LLNameListCtrl>("nearby_list");
    mNearbyList->setRightMouseDownCallback(boost::bind(&LLFloaterPerformance::onAvatarListRightClick, this, _1, _2, _3));

    mMaxARTChangedSignal = gSavedSettings.getControl("RenderAvatarMaxART")->getCommitSignal()->connect(boost::bind(&LLFloaterPerformance::updateMaxRenderTime, this));
    mNearbyPanel->getChild<LLSliderCtrl>("RenderAvatarMaxART")->setCommitCallback(boost::bind(&LLFloaterPerformance::updateMaxRenderTime, this));

    if(!LLPerfStats::tunables.userAutoTuneEnabled)
    {
        gSavedSettings.setF32("AutoTuneRenderFarClipTarget", LLPipeline::RenderFarClip);
    }

    LLStringExplicit fps_limit(llformat("%d", gViewerWindow->getWindow()->getRefreshRate()));
    mAutoadjustmentsPanel->getChild<LLTextBox>("vsync_desc_limit")->setTextArg("[FPS_LIMIT]", fps_limit);
    mAutoadjustmentsPanel->getChild<LLTextBox>("display_desc")->setTextArg("[FPS_LIMIT]", fps_limit);
    mAutoadjustmentsPanel->getChild<LLButton>("defaults_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickDefaults, this));

    mStartAutotuneBtn = mAutoadjustmentsPanel->getChild<LLButton>("start_autotune");
    mStopAutotuneBtn = mAutoadjustmentsPanel->getChild<LLButton>("stop_autotune");
    mStartAutotuneBtn->setCommitCallback(boost::bind(&LLFloaterPerformance::startAutotune, this));
    mStopAutotuneBtn->setCommitCallback(boost::bind(&LLFloaterPerformance::stopAutotune, this));

    mCheckTuneContinous = mAutoadjustmentsPanel->getChild<LLCheckBoxCtrl>("AutoTuneContinuous");
    mTextWIPDesc = mAutoadjustmentsPanel->getChild<LLTextBox>("wip_desc");
    mTextDisplayDesc = mAutoadjustmentsPanel->getChild<LLTextBox>("display_desc");

    mTextFPSLabel = getChild<LLTextBox>("fps_lbl");
    mTextFPSValue = getChild<LLTextBox>("fps_value");

    gSavedPerAccountSettings.declareBOOL("HadEnabledAutoFPS", false, "User had enabled AutoFPS at least once", LLControlVariable::PERSIST_ALWAYS);

    return true;
}

void LLFloaterPerformance::showSelectedPanel(LLPanel* selected_panel)
{
    hidePanels();
    mMainPanel->setVisible(false);
    selected_panel->setVisible(true);

    if (mHUDsPanel == selected_panel)
    {
        populateHUDList();
    }
    else if (mNearbyPanel == selected_panel)
    {
        populateNearbyList();
    }
    else if (mComplexityPanel == selected_panel)
    {
        populateObjectList();
    }
}

void LLFloaterPerformance::showAutoadjustmentsPanel()
{
    showSelectedPanel(mAutoadjustmentsPanel);
}

void LLFloaterPerformance::draw()
{
    enableAutotuneWarning();

    if (mUpdateTimer->hasExpired() &&
        !LLFloaterReg::instanceVisible("save_pref_preset", PRESETS_GRAPHIC)) // give user a chance to save the graphics settings before updating them
    {
        setFPSText();
        if (mHUDsPanel->getVisible())
        {
            populateHUDList();
        }
        else if (mNearbyPanel->getVisible())
        {
            populateNearbyList();
            mNearbyPanel->getChild<LLCheckBoxCtrl>("hide_avatars")->set(!LLPipeline::hasRenderTypeControl(LLPipeline::RENDER_TYPE_AVATAR));
        }
        else if (mComplexityPanel->getVisible())
        {
            populateObjectList();
        }

        mUpdateTimer->setTimerExpirySec(REFRESH_INTERVAL);
    }
    updateAutotuneCtrls(LLPerfStats::tunables.userAutoTuneEnabled);

    LLFloater::draw();
}

void LLFloaterPerformance::showMainPanel()
{
    hidePanels();
    mMainPanel->setVisible(true);
}

void LLFloaterPerformance::hidePanels()
{
    mNearbyPanel->setVisible(false);
    mComplexityPanel->setVisible(false);
    mHUDsPanel->setVisible(false);
    mSettingsPanel->setVisible(false);
    mAutoadjustmentsPanel->setVisible(false);
}

void LLFloaterPerformance::initBackBtn(LLPanel* panel)
{
    panel->getChild<LLButton>("back_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::showMainPanel, this));

    panel->getChild<LLTextBox>("back_lbl")->setShowCursorHand(false);
    panel->getChild<LLTextBox>("back_lbl")->setSoundFlags(LLView::MOUSE_UP);
    panel->getChild<LLTextBox>("back_lbl")->setClickedCallback(boost::bind(&LLFloaterPerformance::showMainPanel, this));
}

void LLFloaterPerformance::populateHUDList()
{
    S32 prev_pos = mHUDList->getScrollPos();
    LLUUID prev_selected_id = mHUDList->getSelectedSpecialId();
    mHUDList->clearRows();
    mHUDList->updateColumns(true);

    LLVOAvatar* avatar = gAgentAvatarp;

    gPipeline.profileAvatar(avatar, true);

    LLVOAvatar::attachment_map_t::iterator iter;
    LLVOAvatar::attachment_map_t::iterator begin = avatar->mAttachmentPoints.begin();
    LLVOAvatar::attachment_map_t::iterator end = avatar->mAttachmentPoints.end();

    // get max gpu render time of all attachments
    F32 max_gpu_time = -1.f;

    for (iter = begin; iter != end; ++iter)
    {
        LLViewerJointAttachment* attachment = iter->second;
        for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
            attachment_iter != attachment->mAttachedObjects.end();
            ++attachment_iter)
        {
            LLViewerObject* attached_object = attachment_iter->get();
            if (attached_object && attached_object->isHUDAttachment())
            {
                max_gpu_time = llmax(max_gpu_time, attached_object->mGPURenderTime);
            }
        }
    }


    for (iter = begin; iter != end; ++iter)
    {
        if (!iter->second)
        {
            continue;
        }

        LLViewerJointAttachment* attachment = iter->second;
        for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
            attachment_iter != attachment->mAttachedObjects.end();
            ++attachment_iter)
        {
            LLViewerObject* attached_object = attachment_iter->get();
            if (attached_object && attached_object->isHUDAttachment())
            {
                F32 gpu_time = attached_object->mGPURenderTime;

                LLSD item;
                item["special_id"] = attached_object->getID();
                item["target"] = LLNameListCtrl::SPECIAL;
                LLSD& row = item["columns"];
                row[0]["column"] = "complex_visual";
                row[0]["type"] = "bar";
                LLSD& value = row[0]["value"];
                value["ratio"] = gpu_time / max_gpu_time;
                value["bottom"] = BAR_BOTTOM_PAD;
                value["left_pad"] = BAR_LEFT_PAD;
                value["right_pad"] = BAR_RIGHT_PAD;

                row[1]["column"] = "complex_value";
                row[1]["type"] = "text";
                // show gpu time in us
                row[1]["value"] = llformat("%.f", gpu_time * 1000.f);
                row[1]["font"]["name"] = "SANSSERIF";

                row[2]["column"] = "name";
                row[2]["type"] = "text";
                row[2]["value"] = attached_object->getAttachmentItemName();
                row[2]["font"]["name"] = "SANSSERIF";

                LLScrollListItem* obj = mHUDList->addElement(item);
                if (obj)
                {
                    LLScrollListText* value_text = dynamic_cast<LLScrollListText*>(obj->getColumn(1));
                    if (value_text)
                    {
                        value_text->setAlignment(LLFontGL::HCENTER);
                    }
                }
            }
        }
    }
    mHUDList->sortByColumnIndex(1, false);
    mHUDList->setScrollPos(prev_pos);
    mHUDList->selectItemBySpecialId(prev_selected_id);
}

void LLFloaterPerformance::populateObjectList()
{
    S32 prev_pos = mObjectList->getScrollPos();
    LLUUID prev_selected_id = mObjectList->getSelectedSpecialId();
    mObjectList->clearRows();
    mObjectList->updateColumns(true);

    LLVOAvatar* avatar = gAgentAvatarp;

    gPipeline.profileAvatar(avatar, true);

    LLVOAvatar::attachment_map_t::iterator iter;
    LLVOAvatar::attachment_map_t::iterator begin = avatar->mAttachmentPoints.begin();
    LLVOAvatar::attachment_map_t::iterator end = avatar->mAttachmentPoints.end();

    // get max gpu render time of all attachments
    F32 max_gpu_time = -1.f;

    for (iter = begin; iter != end; ++iter)
    {
        LLViewerJointAttachment* attachment = iter->second;
        for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
            attachment_iter != attachment->mAttachedObjects.end();
            ++attachment_iter)
        {
            LLViewerObject* attached_object = attachment_iter->get();
            if (attached_object && !attached_object->isHUDAttachment())
            {
                max_gpu_time = llmax(max_gpu_time, attached_object->mGPURenderTime);
            }
        }
    }

    {
        for (iter = begin; iter != end; ++iter)
        {
            if (!iter->second)
            {
                continue;
            }

            LLViewerJointAttachment* attachment = iter->second;
            for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
                attachment_iter != attachment->mAttachedObjects.end();
                ++attachment_iter)
            {
                LLViewerObject* attached_object = attachment_iter->get();
                if (attached_object && !attached_object->isHUDAttachment())
                {
                    F32 gpu_time = attached_object->mGPURenderTime;

                    LLSD item;
                    item["special_id"] = attached_object->getID();
                    item["target"] = LLNameListCtrl::SPECIAL;
                    LLSD& row = item["columns"];
                    row[0]["column"] = "complex_visual";
                    row[0]["type"] = "bar";
                    LLSD& value = row[0]["value"];
                    value["ratio"] = gpu_time / max_gpu_time;
                    value["bottom"] = BAR_BOTTOM_PAD;
                    value["left_pad"] = BAR_LEFT_PAD;
                    value["right_pad"] = BAR_RIGHT_PAD;

                    row[1]["column"] = "complex_value";
                    row[1]["type"] = "text";
                    // show gpu time in us
                    row[1]["value"] = llformat("%.f", gpu_time * 1000.f);
                    row[1]["font"]["name"] = "SANSSERIF";

                    row[2]["column"] = "name";
                    row[2]["type"] = "text";
                    row[2]["value"] = attached_object->getAttachmentItemName();
                    row[2]["font"]["name"] = "SANSSERIF";

                    LLScrollListItem* obj = mObjectList->addElement(item);
                    if (obj)
                    {
                        LLScrollListText* value_text = dynamic_cast<LLScrollListText*>(obj->getColumn(1));
                        if (value_text)
                        {
                            value_text->setAlignment(LLFontGL::HCENTER);
                        }
                    }
                }
            }
        }
    }
    mObjectList->sortByColumnIndex(1, false);
    mObjectList->setScrollPos(prev_pos);
    mObjectList->selectItemBySpecialId(prev_selected_id);
}

void LLFloaterPerformance::populateNearbyList()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_APP;
    static LLCachedControl<bool> showTunedART(gSavedSettings, "ShowTunedART");
    S32 prev_pos = mNearbyList->getScrollPos();
    LLUUID prev_selected_id = mNearbyList->getStringUUIDSelectedItem();
    mNearbyList->clearRows();
    mNearbyList->updateColumns(true);

    static LLCachedControl<U32> max_render_cost(gSavedSettings, "RenderAvatarMaxComplexity", 0);
    std::vector<LLVOAvatar*> valid_nearby_avs;
    mNearbyMaxGPUTime = LLWorld::getInstance()->getNearbyAvatarsAndMaxGPUTime(valid_nearby_avs);

    for (LLVOAvatar* avatar : valid_nearby_avs)
    {
        if (LLVOAvatar::AOA_INVISIBLE != avatar->getOverallAppearance())
        {
            F32 render_av_gpu_ms = avatar->getGPURenderTime();

            auto is_slow = avatar->isTooSlow();
            LLSD item;
            item["id"] = avatar->getID();
            LLSD& row = item["columns"];
            row[0]["column"] = "complex_visual";
            row[0]["type"] = "bar";
            LLSD& value = row[0]["value"];
            // The ratio used in the bar is the current cost, as soon as we take action this changes so we keep the
            // pre-tune value for the numerical column and sorting.
            value["ratio"] = render_av_gpu_ms / mNearbyMaxGPUTime;
            value["bottom"] = BAR_BOTTOM_PAD;
            value["left_pad"] = BAR_LEFT_PAD;
            value["right_pad"] = BAR_RIGHT_PAD;

            row[1]["column"] = "complex_value";
            row[1]["type"] = "text";
            // use GPU time in us
            row[1]["value"] = llformat( "%.f", render_av_gpu_ms * 1000.f);
            row[1]["font"]["name"] = "SANSSERIF";

            row[3]["column"] = "name";
            row[3]["type"] = "text";
            row[3]["value"] = avatar->getFullname();
            row[3]["font"]["name"] = "SANSSERIF";

            LLScrollListItem* av_item = mNearbyList->addElement(item);
            if(av_item)
            {
                LLScrollListText* value_text = dynamic_cast<LLScrollListText*>(av_item->getColumn(1));
                if (value_text)
                {
                    value_text->setAlignment(LLFontGL::HCENTER);
                }
                LLScrollListText* name_text = dynamic_cast<LLScrollListText*>(av_item->getColumn(2));
                if (name_text)
                {
                    if (avatar->isSelf())
                    {
                        name_text->setColor(LLUIColorTable::instance().getColor("DrYellow"));
                    }
                    else
                    {
                        std::string color = "white";
                        if (is_slow || LLVOAvatar::AOA_JELLYDOLL == avatar->getOverallAppearance())
                        {
                            color = "LabelDisabledColor";
                            LLScrollListBar* bar = dynamic_cast<LLScrollListBar*>(av_item->getColumn(0));
                            if (bar)
                            {
                                bar->setColor(LLUIColorTable::instance().getColor(color));
                            }
                        }
                        else if (LLVOAvatar::AOA_NORMAL == avatar->getOverallAppearance())
                        {
                            color = LLAvatarActions::isFriend(avatar->getID()) ? "ConversationFriendColor" : "white";
                        }
                        name_text->setColor(LLUIColorTable::instance().getColor(color));
                    }
                }
            }
        }
    }
    mNearbyList->sortByColumnIndex(1, false);
    mNearbyList->setScrollPos(prev_pos);
    mNearbyList->selectByID(prev_selected_id);
}

void LLFloaterPerformance::setFPSText()
{
    const S32 NUM_PERIODS = 50;
    S32 current_fps = (S32)llround(LLTrace::get_frame_recording().getPeriodMedianPerSec(LLStatViewer::FPS, NUM_PERIODS));
    mTextFPSValue->setValue(current_fps);

    std::string fps_text = getString("fps_text");
    static LLCachedControl<bool> vsync_enabled(gSavedSettings, "RenderVSyncEnable", true);
    S32 refresh_rate = gViewerWindow->getWindow()->getRefreshRate();
    if (vsync_enabled && (refresh_rate > 0) && (current_fps >= refresh_rate))
    {
        fps_text += getString("max_text");
    }
    mTextFPSLabel->setValue(fps_text);
}

void LLFloaterPerformance::detachObject(const LLUUID& obj_id)
{
    LLViewerObject* obj = gObjectList.findObject(obj_id);
    if (obj)
    {
        LLAppearanceMgr::instance().removeItemFromAvatar(obj->getAttachmentItemID());
    }
}

void LLFloaterPerformance::onClickAdvanced()
{
    LLFloaterPreference* instance = LLFloaterReg::getTypedInstance<LLFloaterPreference>("preferences");
    if (instance)
    {
        instance->saveSettings();
    }
    LLFloaterReg::showInstance("prefs_graphics_advanced");
}

void LLFloaterPerformance::onClickDefaults()
{
    LLFloaterPreference* instance = LLFloaterReg::getTypedInstance<LLFloaterPreference>("preferences");
    if (instance)
    {
        instance->setRecommendedSettings();
    }
}

void LLFloaterPerformance::onChangeQuality(const LLSD& data)
{
    LLFloaterPreference* instance = LLFloaterReg::getTypedInstance<LLFloaterPreference>("preferences");
    if (instance)
    {
        instance->onChangeQuality(data);
    }
}

void LLFloaterPerformance::onClickHideAvatars()
{
    LLPipeline::toggleRenderTypeControl(LLPipeline::RENDER_TYPE_AVATAR);
}

void LLFloaterPerformance::onClickExceptions()
{
    LLFloaterReg::showInstance("avatar_render_settings");
}

void LLFloaterPerformance::updateMaxRenderTime()
{
    LLAvatarComplexityControls::updateMaxRenderTime(
        mNearbyPanel->getChild<LLSliderCtrl>("RenderAvatarMaxART"),
        mNearbyPanel->getChild<LLTextBox>("RenderAvatarMaxARTText"),
        true);
}

static LLVOAvatar* find_avatar(const LLUUID& id)
{
    LLViewerObject *obj = gObjectList.findObject(id);
    while (obj && obj->isAttachment())
    {
        obj = (LLViewerObject *)obj->getParent();
    }

    if (obj && obj->isAvatar())
    {
        return (LLVOAvatar*)obj;
    }
    else
    {
        return NULL;
    }
}

void LLFloaterPerformance::onCustomAction(const LLSD& userdata, const LLUUID& av_id)
{
    const std::string command_name = userdata.asString();

    S32 new_setting = 0;
    if ("default" == command_name)
    {
        new_setting = S32(LLVOAvatar::AV_RENDER_NORMALLY);
    }
    else if ("never" == command_name)
    {
        new_setting = S32(LLVOAvatar::AV_DO_NOT_RENDER);
    }
    else if ("always" == command_name)
    {
        new_setting = S32(LLVOAvatar::AV_ALWAYS_RENDER);
    }

    LLVOAvatar *avatarp = find_avatar(av_id);
    if (avatarp)
    {
        avatarp->setVisualMuteSettings(LLVOAvatar::VisualMuteSettings(new_setting));
    }
    else
    {
        LLRenderMuteList::getInstance()->saveVisualMuteSetting(av_id, new_setting);
    }
}


bool LLFloaterPerformance::isActionChecked(const LLSD& userdata, const LLUUID& av_id)
{
    const std::string command_name = userdata.asString();

    S32 visual_setting = LLRenderMuteList::getInstance()->getSavedVisualMuteSetting(av_id);
    if ("default" == command_name)
    {
        return (visual_setting == S32(LLVOAvatar::AV_RENDER_NORMALLY));
    }
    else if ("non_default" == command_name)
    {
        return (visual_setting != S32(LLVOAvatar::AV_RENDER_NORMALLY));
    }
    else if ("never" == command_name)
    {
        return (visual_setting == S32(LLVOAvatar::AV_DO_NOT_RENDER));
    }
    else if ("always" == command_name)
    {
        return (visual_setting == S32(LLVOAvatar::AV_ALWAYS_RENDER));
    }
    return false;
}

void LLFloaterPerformance::onAvatarListRightClick(LLUICtrl* ctrl, S32 x, S32 y)
{
    LLNameListCtrl* list = dynamic_cast<LLNameListCtrl*>(ctrl);
    if (!list) return;
    list->selectItemAt(x, y, MASK_NONE);
    uuid_vec_t selected_uuids;

    if((list->getCurrentID().notNull()) && (list->getCurrentID() != gAgentID))
    {
        selected_uuids.push_back(list->getCurrentID());
        mContextMenu->show(ctrl, selected_uuids, x, y);
    }
}

const U32 RENDER_QUALITY_LEVEL = 3;
void LLFloaterPerformance::changeQualityLevel(const std::string& notif)
{
    LLNotificationsUtil::add(notif, LLSD(), LLSD(),
        [](const LLSD&notif, const LLSD&resp)
    {
        S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
        if (opt == 0)
        {
            LLFloaterPreference* instance = LLFloaterReg::getTypedInstance<LLFloaterPreference>("preferences");
            if (instance)
            {
                gSavedSettings.setU32("RenderQualityPerformance", RENDER_QUALITY_LEVEL);
                instance->onChangeQuality(LLSD((S32)RENDER_QUALITY_LEVEL));
            }
        }
    });
}

bool is_ALM_available()
{
    bool bumpshiny = LLCubeMap::sUseCubeMaps && LLFeatureManager::getInstance()->isFeatureAvailable("RenderObjectBump") && gSavedSettings.getBOOL("RenderObjectBump");
    bool shaders = gSavedSettings.getBOOL("WindLightUseAtmosShaders");

    return LLFeatureManager::getInstance()->isFeatureAvailable("RenderDeferred") &&
        bumpshiny &&
        shaders;
}

void LLFloaterPerformance::onClickAdvancedLighting()
{
    if (!is_ALM_available())
    {
        changeQualityLevel("AdvancedLightingConfirm");
    }
}

void LLFloaterPerformance::onClickShadows()
{
    if (!is_ALM_available() || !gSavedSettings.getBOOL("RenderDeferred"))
    {
        changeQualityLevel("ShadowsConfirm");
    }

}

void LLFloaterPerformance::startAutotune()
{
    LLPerfStats::tunables.userAutoTuneEnabled = true;
}

void LLFloaterPerformance::stopAutotune()
{
    LLPerfStats::tunables.userAutoTuneEnabled = false;
}

void LLFloaterPerformance::updateAutotuneCtrls(bool autotune_enabled)
{
    static LLCachedControl<bool> auto_tune_locked(gSavedSettings, "AutoTuneLock");
    mStartAutotuneBtn->setEnabled(!autotune_enabled && !auto_tune_locked);
    mStopAutotuneBtn->setEnabled(autotune_enabled && !auto_tune_locked);
    mCheckTuneContinous->setEnabled(!autotune_enabled || (autotune_enabled && auto_tune_locked));

    mTextWIPDesc->setVisible(autotune_enabled && !auto_tune_locked);
    mTextDisplayDesc->setVisible(LLPerfStats::tunables.vsyncEnabled);
}

void LLFloaterPerformance::enableAutotuneWarning()
{
    if (!gSavedPerAccountSettings.getBOOL("HadEnabledAutoFPS") && LLPerfStats::tunables.userAutoTuneEnabled)
    {
        gSavedPerAccountSettings.setBOOL("HadEnabledAutoFPS", true);

        LLNotificationsUtil::add("EnableAutoFPSWarning", LLSD(), LLSD(),
            [](const LLSD& notif, const LLSD& resp)
            {
                S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
                if (opt == 0)
                { // offer user to save current graphics settings as a preset
                    LLFloaterReg::showInstance("save_pref_preset", PRESETS_GRAPHIC);
                }
            });
    }
}
// EOF
