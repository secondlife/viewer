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

constexpr auto AvType       {LLPerfStats::ObjType_t::OT_AVATAR};
constexpr auto AttType      {LLPerfStats::ObjType_t::OT_ATTACHMENT};
constexpr auto HudType      {LLPerfStats::ObjType_t::OT_HUD};

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
    mUpdateTimer(new LLTimer()),
    mNearbyMaxComplexity(0)
{
    mContextMenu = new LLExceptionsContextMenu(this);
}

LLFloaterPerformance::~LLFloaterPerformance()
{
    mMaxARTChangedSignal.disconnect();
    delete mContextMenu;
    delete mUpdateTimer;
}

BOOL LLFloaterPerformance::postBuild()
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
    mHUDList->setIconClickedCallback(boost::bind(&LLFloaterPerformance::detachItem, this, _1));

    mObjectList = mComplexityPanel->getChild<LLNameListCtrl>("obj_list");
    mObjectList->setNameListType(LLNameListCtrl::SPECIAL);
    mObjectList->setHoverIconName("StopReload_Off");
    mObjectList->setIconClickedCallback(boost::bind(&LLFloaterPerformance::detachItem, this, _1));

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

    // store the current setting as the users desired reflection detail and DD
    gSavedSettings.setS32("UserTargetReflections", LLPipeline::RenderReflectionDetail);
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

    gSavedPerAccountSettings.declareBOOL("HadEnabledAutoFPS", FALSE, "User had enabled AutoFPS at least once", LLControlVariable::PERSIST_ALWAYS);

    return TRUE;
}

void LLFloaterPerformance::showSelectedPanel(LLPanel* selected_panel)
{
    hidePanels();
    mMainPanel->setVisible(FALSE);
    selected_panel->setVisible(TRUE);

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
    mMainPanel->setVisible(TRUE);
}

void LLFloaterPerformance::hidePanels()
{
    mNearbyPanel->setVisible(FALSE);
    mComplexityPanel->setVisible(FALSE);
    mHUDsPanel->setVisible(FALSE);
    mSettingsPanel->setVisible(FALSE);
    mAutoadjustmentsPanel->setVisible(FALSE);
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

    hud_complexity_list_t complexity_list = LLHUDRenderNotifier::getInstance()->getHUDComplexityList();

    hud_complexity_list_t::iterator iter = complexity_list.begin();
    hud_complexity_list_t::iterator end = complexity_list.end();

    auto huds_max_render_time_raw = LLPerfStats::StatsRecorder::getMax(HudType, LLPerfStats::StatType_t::RENDER_GEOMETRY);
    for (iter = complexity_list.begin(); iter != end; ++iter)
    {
        LLHUDComplexity hud_object_complexity = *iter;

        auto hud_render_time_raw = LLPerfStats::StatsRecorder::get(HudType, hud_object_complexity.objectId, LLPerfStats::StatType_t::RENDER_GEOMETRY);

        LLSD item;
        item["special_id"] = hud_object_complexity.objectId;
        item["target"] = LLNameListCtrl::SPECIAL;
        LLSD& row = item["columns"];
        row[0]["column"] = "complex_visual";
        row[0]["type"] = "bar";
        LLSD& value = row[0]["value"];
        value["ratio"] = (F32)hud_render_time_raw / huds_max_render_time_raw;
        value["bottom"] = BAR_BOTTOM_PAD;
        value["left_pad"] = BAR_LEFT_PAD;
        value["right_pad"] = BAR_RIGHT_PAD;

        row[1]["column"] = "complex_value";
        row[1]["type"] = "text";
        row[1]["value"] = llformat( "%.f", llmax(LLPerfStats::raw_to_us(hud_render_time_raw), (double)1));
        row[1]["font"]["name"] = "SANSSERIF";
 
        row[2]["column"] = "name";
        row[2]["type"] = "text";
        row[2]["value"] = hud_object_complexity.objectName;
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
    mHUDList->sortByColumnIndex(1, FALSE);
    mHUDList->setScrollPos(prev_pos);
    mHUDList->selectItemBySpecialId(prev_selected_id);
}

void LLFloaterPerformance::populateObjectList()
{
    S32 prev_pos = mObjectList->getScrollPos();
    LLUUID prev_selected_id = mObjectList->getSelectedSpecialId();
    mObjectList->clearRows();
    mObjectList->updateColumns(true);

    object_complexity_list_t complexity_list = LLAvatarRenderNotifier::getInstance()->getObjectComplexityList();

    object_complexity_list_t::iterator iter = complexity_list.begin();
    object_complexity_list_t::iterator end = complexity_list.end();

    // for consistency we lock the buffer while we build the list. In theory this is uncontended as the buffer should only toggle on end of frame
    {
        std::lock_guard<std::mutex> guard{ LLPerfStats::bufferToggleLock };
        auto att_max_render_time_raw = LLPerfStats::StatsRecorder::getMax(AttType, LLPerfStats::StatType_t::RENDER_COMBINED);

        for (iter = complexity_list.begin(); iter != end; ++iter)
        {
            LLObjectComplexity object_complexity = *iter;

            auto attach_render_time_raw = LLPerfStats::StatsRecorder::get(AttType, object_complexity.objectId, LLPerfStats::StatType_t::RENDER_COMBINED);
            LLSD item;
            item["special_id"] = object_complexity.objectId;
            item["target"] = LLNameListCtrl::SPECIAL;
            LLSD& row = item["columns"];
            row[0]["column"] = "complex_visual";
            row[0]["type"] = "bar";
            LLSD& value = row[0]["value"];
            value["ratio"] = ((F32)attach_render_time_raw) / att_max_render_time_raw;
            value["bottom"] = BAR_BOTTOM_PAD;
            value["left_pad"] = BAR_LEFT_PAD;
            value["right_pad"] = BAR_RIGHT_PAD;

            row[1]["column"] = "complex_value";
            row[1]["type"] = "text";
            row[1]["value"] = llformat("%.f", llmax(LLPerfStats::raw_to_us(attach_render_time_raw), (double)1));
            row[1]["font"]["name"] = "SANSSERIF";

            row[2]["column"] = "name";
            row[2]["type"] = "text";
            row[2]["value"] = object_complexity.objectName;
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
    mObjectList->sortByColumnIndex(1, FALSE);
    mObjectList->setScrollPos(prev_pos);
    mObjectList->selectItemBySpecialId(prev_selected_id);
}

void LLFloaterPerformance::populateNearbyList()
{
    static LLCachedControl<bool> showTunedART(gSavedSettings, "ShowTunedART");
    S32 prev_pos = mNearbyList->getScrollPos();
    LLUUID prev_selected_id = mNearbyList->getStringUUIDSelectedItem();
    mNearbyList->clearRows();
    mNearbyList->updateColumns(true);

    static LLCachedControl<U32> max_render_cost(gSavedSettings, "RenderAvatarMaxComplexity", 0);
    std::vector<LLCharacter*> valid_nearby_avs;
    mNearbyMaxComplexity = LLWorld::getInstance()->getNearbyAvatarsAndCompl(valid_nearby_avs);

    std::vector<LLCharacter*>::iterator char_iter = valid_nearby_avs.begin();

    LLPerfStats::bufferToggleLock.lock();
    auto av_render_max_raw = LLPerfStats::StatsRecorder::getMax(AvType, LLPerfStats::StatType_t::RENDER_COMBINED);
    LLPerfStats::bufferToggleLock.unlock();

    while (char_iter != valid_nearby_avs.end())
    {
        LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(*char_iter);
        if (avatar && (LLVOAvatar::AOA_INVISIBLE != avatar->getOverallAppearance()))
        {
            LLPerfStats::bufferToggleLock.lock();
            auto render_av_raw  = LLPerfStats::StatsRecorder::get(AvType, avatar->getID(),LLPerfStats::StatType_t::RENDER_COMBINED);
            LLPerfStats::bufferToggleLock.unlock();

            auto is_slow = avatar->isTooSlow();
            LLSD item;
            item["id"] = avatar->getID();
            LLSD& row = item["columns"];
            row[0]["column"] = "complex_visual";
            row[0]["type"] = "bar";
            LLSD& value = row[0]["value"];
            // The ratio used in the bar is the current cost, as soon as we take action this changes so we keep the 
            // pre-tune value for the numerical column and sorting.
            value["ratio"] = (double)render_av_raw / av_render_max_raw;
            value["bottom"] = BAR_BOTTOM_PAD;
            value["left_pad"] = BAR_LEFT_PAD;
            value["right_pad"] = BAR_RIGHT_PAD;

            row[1]["column"] = "complex_value";
            row[1]["type"] = "text";
            if (is_slow && !showTunedART)
            {
                row[1]["value"] = llformat( "%.f", LLPerfStats::raw_to_us( avatar->getLastART() ) );
            }
            else
            {
                row[1]["value"] = llformat( "%.f", LLPerfStats::raw_to_us( render_av_raw ) );
            }
            row[1]["font"]["name"] = "SANSSERIF";

            row[2]["column"] = "name";
            row[2]["type"] = "text";
            row[2]["value"] = avatar->getFullname();
            row[2]["font"]["name"] = "SANSSERIF";

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
        char_iter++;
    }
    mNearbyList->sortByColumnIndex(1, FALSE);
    mNearbyList->setScrollPos(prev_pos);
    mNearbyList->selectByID(prev_selected_id);
}

void LLFloaterPerformance::setFPSText()
{
    const S32 NUM_PERIODS = 50;
    S32 current_fps = (S32)llround(LLTrace::get_frame_recording().getPeriodMedianPerSec(LLStatViewer::FPS, NUM_PERIODS));
    getChild<LLTextBox>("fps_value")->setValue(current_fps);

    std::string fps_text = getString("fps_text");
    static LLCachedControl<bool> vsync_enabled(gSavedSettings, "RenderVSyncEnable", true);
    S32 refresh_rate = gViewerWindow->getWindow()->getRefreshRate();
    if (vsync_enabled && (refresh_rate > 0) && (current_fps >= refresh_rate))
    {
        fps_text += getString("max_text");
    }
    getChild<LLTextBox>("fps_lbl")->setValue(fps_text);
}

void LLFloaterPerformance::detachItem(const LLUUID& item_id)
{
    LLAppearanceMgr::instance().removeItemFromAvatar(item_id);
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
    bool bumpshiny = gGLManager.mHasCubeMap && LLCubeMap::sUseCubeMaps && LLFeatureManager::getInstance()->isFeatureAvailable("RenderObjectBump") && gSavedSettings.getBOOL("RenderObjectBump");
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
    getChild<LLCheckBoxCtrl>("AutoTuneContinuous")->setEnabled(!autotune_enabled || (autotune_enabled && auto_tune_locked));

    getChild<LLTextBox>("wip_desc")->setVisible(autotune_enabled && !auto_tune_locked);
    getChild<LLTextBox>("display_desc")->setVisible(LLPerfStats::tunables.vsyncEnabled);
}

void LLFloaterPerformance::enableAutotuneWarning()
{
    if (!gSavedPerAccountSettings.getBOOL("HadEnabledAutoFPS") && LLPerfStats::tunables.userAutoTuneEnabled)
    {
        gSavedPerAccountSettings.setBOOL("HadEnabledAutoFPS", TRUE);

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
