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
#include "llfeaturemanager.h"
#include "llfloaterpreference.h" // LLAvatarComplexityControls
#include "llfloaterreg.h"
#include "llnamelistctrl.h"
#include "llsliderctrl.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "llvoavatar.h"
#include "llvoavatarself.h" 

const F32 REFRESH_INTERVAL = 1.0f;
const S32 COMPLEXITY_THRESHOLD_HIGH = 100000;
const S32 COMPLEXITY_THRESHOLD_MEDIUM = 30000;
const S32 BAR_LEFT_PAD = 2;
const S32 BAR_RIGHT_PAD = 5;
const S32 BAR_BOTTOM_PAD = 9;

LLFloaterPerformance::LLFloaterPerformance(const LLSD& key)
:   LLFloater(key),
    mUpdateTimer(new LLTimer()),
    mNearbyMaxComplexity(0)
{
}

LLFloaterPerformance::~LLFloaterPerformance()
{
    mComplexityChangedSignal.disconnect();
    delete mUpdateTimer;
}

BOOL LLFloaterPerformance::postBuild()
{
    mMainPanel = getChild<LLPanel>("panel_performance_main");
    mTroubleshootingPanel = getChild<LLPanel>("panel_performance_troubleshooting");
    mNearbyPanel = getChild<LLPanel>("panel_performance_nearby");
    mComplexityPanel = getChild<LLPanel>("panel_performance_complexity");
    mSettingsPanel = getChild<LLPanel>("panel_performance_preferences");
    mHUDsPanel = getChild<LLPanel>("panel_performance_huds");
    mPresetsPanel = getChild<LLPanel>("panel_performance_presets");

    getChild<LLPanel>("troubleshooting_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mTroubleshootingPanel));
    getChild<LLPanel>("nearby_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mNearbyPanel));
    getChild<LLPanel>("complexity_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mComplexityPanel));
    getChild<LLPanel>("settings_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mSettingsPanel));
    getChild<LLPanel>("huds_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mHUDsPanel));
    getChild<LLPanel>("presets_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mPresetsPanel));

    initBackBtn(mTroubleshootingPanel);
    initBackBtn(mNearbyPanel);
    initBackBtn(mComplexityPanel);
    initBackBtn(mSettingsPanel);
    initBackBtn(mHUDsPanel);
    initBackBtn(mPresetsPanel);

    mHUDList = mHUDsPanel->getChild<LLNameListCtrl>("hud_list");
    mHUDList->setNameListType(LLNameListCtrl::SPECIAL);
    mHUDList->setHoverIconName("StopReload_Off");
    mHUDList->setIconClickedCallback(boost::bind(&LLFloaterPerformance::detachItem, this, _1));

    mObjectList = mComplexityPanel->getChild<LLNameListCtrl>("obj_list");
    mObjectList->setNameListType(LLNameListCtrl::SPECIAL);
    mObjectList->setHoverIconName("StopReload_Off");
    mObjectList->setIconClickedCallback(boost::bind(&LLFloaterPerformance::detachItem, this, _1));

    mSettingsPanel->getChild<LLButton>("advanced_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickAdvanced, this));

    mNearbyPanel->getChild<LLButton>("exceptions_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickExceptions, this));
    mNearbyList = mNearbyPanel->getChild<LLNameListCtrl>("nearby_list");

    mPresetsPanel->getChild<LLTextBox>("avatars_nearby_link")->setURLClickedCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mNearbyPanel));
    mPresetsPanel->getChild<LLTextBox>("settings_link")->setURLClickedCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mSettingsPanel));

    updateComplexityText();
    mComplexityChangedSignal = gSavedSettings.getControl("IndirectMaxComplexity")->getCommitSignal()->connect(boost::bind(&LLFloaterPerformance::updateComplexityText, this));
    mNearbyPanel->getChild<LLSliderCtrl>("IndirectMaxComplexity")->setCommitCallback(boost::bind(&LLFloaterPerformance::updateMaxComplexity, this));

    LLAvatarComplexityControls::setIndirectMaxArc();

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

void LLFloaterPerformance::draw()
{
    if (mUpdateTimer->hasExpired())
    {
        getChild<LLTextBox>("fps_value")->setValue((S32)llround(LLTrace::get_frame_recording().getPeriodMeanPerSec(LLStatViewer::FPS)));
        if (mMainPanel->getVisible())
        {
            mMainPanel->getChild<LLTextBox>("huds_value")->setValue(LLHUDRenderNotifier::getInstance()->getHUDsCount());
            mMainPanel->getChild<LLTextBox>("complexity_value")->setValue((S32)gAgentAvatarp->getVisualComplexity());
            updateNearbyComplexityDesc();
        }
        else if (mHUDsPanel->getVisible())
        {
            populateHUDList();
        }
        else if (mNearbyPanel->getVisible())
        {
            populateNearbyList();
            updateNearbyComplexityDesc();
        }
        else if (mComplexityPanel->getVisible())
        {
            populateObjectList();
        }

        mUpdateTimer->setTimerExpirySec(REFRESH_INTERVAL);
    }
    LLFloater::draw();
}

void LLFloaterPerformance::showMainPanel()
{
    hidePanels();
    mMainPanel->setVisible(TRUE);
}

void LLFloaterPerformance::hidePanels()
{
    mTroubleshootingPanel->setVisible(FALSE);
    mNearbyPanel->setVisible(FALSE);
    mComplexityPanel->setVisible(FALSE);
    mHUDsPanel->setVisible(FALSE);
    mSettingsPanel->setVisible(FALSE);
    mPresetsPanel->setVisible(FALSE);
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
    mHUDList->clearRows();
    mHUDList->updateColumns(true);

    hud_complexity_list_t complexity_list = LLHUDRenderNotifier::getInstance()->getHUDComplexityList();

    hud_complexity_list_t::iterator iter = complexity_list.begin();
    hud_complexity_list_t::iterator end = complexity_list.end();

    U32 max_complexity = 0;
    for (; iter != end; ++iter)
    {
        max_complexity = llmax(max_complexity, (*iter).objectsCost);
    }
   
    for (iter = complexity_list.begin(); iter != end; ++iter)
    {
        LLHUDComplexity hud_object_complexity = *iter;        

        LLSD item;
        item["special_id"] = hud_object_complexity.objectId;
        item["target"] = LLNameListCtrl::SPECIAL;
        LLSD& row = item["columns"];
        row[0]["column"] = "complex_visual";
        row[0]["type"] = "image";
        LLSD& value = row[0]["value"];
        value["ratio"] = (F32)hud_object_complexity.objectsCost / max_complexity;
        value["bottom"] = BAR_BOTTOM_PAD;
        value["left_pad"] = BAR_LEFT_PAD;
        value["right_pad"] = BAR_RIGHT_PAD;

        row[1]["column"] = "complex_value";
        row[1]["type"] = "text";
        row[1]["value"] = std::to_string(hud_object_complexity.objectsCost);
        row[1]["font"]["name"] = "SANSSERIF";
 
        row[2]["column"] = "name";
        row[2]["type"] = "text";
        row[2]["value"] = hud_object_complexity.objectName;
        row[2]["font"]["name"] = "SANSSERIF";

        mHUDList->addElement(item);
    }
    mHUDList->sortByColumnIndex(1, FALSE);
    mHUDList->setScrollPos(prev_pos);

    mHUDsPanel->getChild<LLTextBox>("huds_value")->setValue(std::to_string(complexity_list.size()));
}

void LLFloaterPerformance::populateObjectList()
{
    S32 prev_pos = mObjectList->getScrollPos();
    mObjectList->clearRows();
    mObjectList->updateColumns(true);

    object_complexity_list_t complexity_list = LLAvatarRenderNotifier::getInstance()->getObjectComplexityList();

    object_complexity_list_t::iterator iter = complexity_list.begin();
    object_complexity_list_t::iterator end = complexity_list.end();

    U32 max_complexity = 0;
    for (; iter != end; ++iter)
    {
        max_complexity = llmax(max_complexity, (*iter).objectCost);
    }

    for (iter = complexity_list.begin(); iter != end; ++iter)
    {
        LLObjectComplexity object_complexity = *iter;        

        LLSD item;
        item["special_id"] = object_complexity.objectId;
        item["target"] = LLNameListCtrl::SPECIAL;
        LLSD& row = item["columns"];
        row[0]["column"] = "complex_visual";
        row[0]["type"] = "image";
        LLSD& value = row[0]["value"];
        value["ratio"] = (F32)object_complexity.objectCost / max_complexity;
        value["bottom"] = BAR_BOTTOM_PAD;
        value["left_pad"] = BAR_LEFT_PAD;
        value["right_pad"] = BAR_RIGHT_PAD;

        row[1]["column"] = "complex_value";
        row[1]["type"] = "text";
        row[1]["value"] = std::to_string(object_complexity.objectCost);
        row[1]["font"]["name"] = "SANSSERIF";

        row[2]["column"] = "name";
        row[2]["type"] = "text";
        row[2]["value"] = object_complexity.objectName;
        row[2]["font"]["name"] = "SANSSERIF";

        mObjectList->addElement(item);
    }
    mObjectList->sortByColumnIndex(1, FALSE);
    mObjectList->setScrollPos(prev_pos);
}

void LLFloaterPerformance::populateNearbyList()
{
    S32 prev_pos = mNearbyList->getScrollPos();
    mNearbyList->clearRows();
    mNearbyList->updateColumns(true);

    static LLCachedControl<U32> max_render_cost(gSavedSettings, "RenderAvatarMaxComplexity", 0);
    std::vector<LLCharacter*> valid_nearby_avs;
    getNearbyAvatars(valid_nearby_avs);

    std::vector<LLCharacter*>::iterator char_iter = valid_nearby_avs.begin();
    while (char_iter != valid_nearby_avs.end())
    {
        LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(*char_iter);
        if (avatar)
        {
            LLSD item;
            item["id"] = avatar->getID();
            LLSD& row = item["columns"];
            row[0]["column"] = "complex_visual";
            row[0]["type"] = "image";
            LLSD& value = row[0]["value"];
            value["ratio"] = (F32)avatar->getVisualComplexity() / mNearbyMaxComplexity;
            value["bottom"] = BAR_BOTTOM_PAD;
            value["left_pad"] = BAR_LEFT_PAD;
            value["right_pad"] = BAR_RIGHT_PAD;

            row[1]["column"] = "complex_value";
            row[1]["type"] = "text";
            row[1]["value"] = std::to_string( avatar->getVisualComplexity());
            row[1]["font"]["name"] = "SANSSERIF";

            row[2]["column"] = "name";
            row[2]["type"] = "text";
            row[2]["value"] = avatar->getFullname();
            row[2]["font"]["name"] = "SANSSERIF";

            LLScrollListItem* av_item = mNearbyList->addElement(item);
            if(av_item)
            {
                LLScrollListText* name_text = dynamic_cast<LLScrollListText*>(av_item->getColumn(2));
                if (name_text)
                {
                    std::string color = "white";
                    if (avatar->getVisualComplexity() > max_render_cost)
                    {
                        color = "LabelDisabledColor";
                        LLScrollListBar* bar = dynamic_cast<LLScrollListBar*>(av_item->getColumn(0));
                        if (bar)
                        {
                            bar->setColor(LLUIColorTable::instance().getColor(color));
                        }
                    }
                    else if (LLAvatarActions::isFriend(avatar->getID()))
                    {
                        color = "ConversationFriendColor";
                    }
                    name_text->setColor(LLUIColorTable::instance().getColor(color));
                }
            }
        }
        char_iter++;
    }
    mNearbyList->sortByColumnIndex(1, FALSE);
    mNearbyList->setScrollPos(prev_pos);
}

void LLFloaterPerformance::getNearbyAvatars(std::vector<LLCharacter*> &valid_nearby_avs)
{
    static LLCachedControl<F32> render_far_clip(gSavedSettings, "RenderFarClip", 64);
    F32 radius = render_far_clip * render_far_clip;
    std::vector<LLCharacter*>::iterator char_iter = LLCharacter::sInstances.begin();
    while (char_iter != LLCharacter::sInstances.end())
    {
        LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(*char_iter);
        if (avatar && !avatar->isDead() && !avatar->isControlAvatar() && !avatar->isSelf())
        {
            if ((dist_vec_squared(avatar->getPositionGlobal(), gAgent.getPositionGlobal()) > radius) &&
                (dist_vec_squared(avatar->getPositionGlobal(), gAgentCamera.getCameraPositionGlobal()) > radius))
            {
                char_iter++;
                continue;
            }
            avatar->calculateUpdateRenderComplexity();
            mNearbyMaxComplexity = llmax(mNearbyMaxComplexity, (S32)avatar->getVisualComplexity());
            valid_nearby_avs.push_back(*char_iter);
        }
        char_iter++;
    }
}

void LLFloaterPerformance::updateNearbyComplexityDesc()
{
    std::string desc = getString("low");

    static LLCachedControl<U32> max_render_cost(gSavedSettings, "RenderAvatarMaxComplexity", 0);
    if (mMainPanel->getVisible())
    {
        std::vector<LLCharacter*> valid_nearby_avs;
        getNearbyAvatars(valid_nearby_avs);
    }
    if (mNearbyMaxComplexity > COMPLEXITY_THRESHOLD_HIGH)
    {
        desc = getString("very_high");
    }
    else if (mNearbyMaxComplexity > COMPLEXITY_THRESHOLD_MEDIUM)
    {
        desc = getString("medium");
    }
   
    if (mMainPanel->getVisible())
    {
        mMainPanel->getChild<LLTextBox>("avatars_nearby_value")->setValue(desc);
    }
    mNearbyPanel->getChild<LLTextBox>("av_nearby_value")->setValue(desc);
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

void LLFloaterPerformance::onClickExceptions()
{
    LLFloaterReg::showInstance("avatar_render_settings");
}

void LLFloaterPerformance::updateMaxComplexity()
{
    LLAvatarComplexityControls::updateMax(
        mNearbyPanel->getChild<LLSliderCtrl>("IndirectMaxComplexity"),
        mNearbyPanel->getChild<LLTextBox>("IndirectMaxComplexityText"));
}

void LLFloaterPerformance::updateComplexityText()
{
    LLAvatarComplexityControls::setText(gSavedSettings.getU32("RenderAvatarMaxComplexity"),
        mNearbyPanel->getChild<LLTextBox>("IndirectMaxComplexityText", true));
}

// EOF
