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
#include "llfeaturemanager.h"
#include "llfloaterpreference.h" // LLAvatarComplexityControls
#include "llfloaterreg.h"
#include "llnamelistctrl.h"
#include "llnotificationsutil.h"
#include "llradiogroup.h"
#include "llsliderctrl.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "llviewerobjectlist.h"
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
    mUpdateTimer(new LLTimer()),
    mNearbyMaxComplexity(0)
{
    mContextMenu = new LLExceptionsContextMenu(this);

    mCommitCallbackRegistrar.add("Pref.AutoAdjustWarning", boost::bind(&LLFloaterPreference::showAutoAdjustWarning));
}

LLFloaterPerformance::~LLFloaterPerformance()
{
    mComplexityChangedSignal.disconnect();
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

    getChild<LLPanel>("nearby_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mNearbyPanel));
    getChild<LLPanel>("complexity_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mComplexityPanel));
    getChild<LLPanel>("settings_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mSettingsPanel));
    getChild<LLPanel>("huds_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mHUDsPanel));

    initBackBtn(mNearbyPanel);
    initBackBtn(mComplexityPanel);
    initBackBtn(mSettingsPanel);
    initBackBtn(mHUDsPanel);

    mHUDList = mHUDsPanel->getChild<LLNameListCtrl>("hud_list");
    mHUDList->setNameListType(LLNameListCtrl::SPECIAL);
    mHUDList->setHoverIconName("StopReload_Off");
    mHUDList->setIconClickedCallback(boost::bind(&LLFloaterPerformance::detachItem, this, _1));

    mObjectList = mComplexityPanel->getChild<LLNameListCtrl>("obj_list");
    mObjectList->setNameListType(LLNameListCtrl::SPECIAL);
    mObjectList->setHoverIconName("StopReload_Off");
    mObjectList->setIconClickedCallback(boost::bind(&LLFloaterPerformance::detachItem, this, _1));

    mSettingsPanel->getChild<LLButton>("advanced_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickAdvanced, this));
    mSettingsPanel->getChild<LLRadioGroup>("graphics_quality")->setCommitCallback(boost::bind(&LLFloaterPerformance::onChangeQuality, this, _2));

    mNearbyPanel->getChild<LLButton>("exceptions_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickExceptions, this));
    mNearbyPanel->getChild<LLCheckBoxCtrl>("hide_avatars")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickHideAvatars, this));
    mNearbyPanel->getChild<LLCheckBoxCtrl>("hide_avatars")->set(!LLPipeline::hasRenderTypeControl(LLPipeline::RENDER_TYPE_AVATAR));
    mNearbyList = mNearbyPanel->getChild<LLNameListCtrl>("nearby_list");
    mNearbyList->setRightMouseDownCallback(boost::bind(&LLFloaterPerformance::onAvatarListRightClick, this, _1, _2, _3));

    updateComplexityText();
    mComplexityChangedSignal = gSavedSettings.getControl("RenderAvatarMaxComplexity")->getCommitSignal()->connect(boost::bind(&LLFloaterPerformance::updateComplexityText, this));
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
    const S32 NUM_PERIODS = 50;

    if (mUpdateTimer->hasExpired())
    {
        getChild<LLTextBox>("fps_value")->setValue((S32)llround(LLTrace::get_frame_recording().getPeriodMedianPerSec(LLStatViewer::FPS, NUM_PERIODS)));
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

    U32 max_complexity = 0;
    for (; iter != end; ++iter)
    {
        max_complexity = llmax(max_complexity, (*iter).objectsCost);
    }
   
    for (iter = complexity_list.begin(); iter != end; ++iter)
    {
        LLHUDComplexity hud_object_complexity = *iter;        
        S32 obj_cost_short = llmax((S32)hud_object_complexity.objectsCost / 1000, 1);
        LLSD item;
        item["special_id"] = hud_object_complexity.objectId;
        item["target"] = LLNameListCtrl::SPECIAL;
        LLSD& row = item["columns"];
        row[0]["column"] = "complex_visual";
        row[0]["type"] = "bar";
        LLSD& value = row[0]["value"];
        value["ratio"] = (F32)obj_cost_short / max_complexity * 1000;
        value["bottom"] = BAR_BOTTOM_PAD;
        value["left_pad"] = BAR_LEFT_PAD;
        value["right_pad"] = BAR_RIGHT_PAD;

        row[1]["column"] = "complex_value";
        row[1]["type"] = "text";
        row[1]["value"] = std::to_string(obj_cost_short);
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

    U32 max_complexity = 0;
    for (; iter != end; ++iter)
    {
        max_complexity = llmax(max_complexity, (*iter).objectCost);
    }

    for (iter = complexity_list.begin(); iter != end; ++iter)
    {
        LLObjectComplexity object_complexity = *iter;        
        S32 obj_cost_short = llmax((S32)object_complexity.objectCost / 1000, 1);
        LLSD item;
        item["special_id"] = object_complexity.objectId;
        item["target"] = LLNameListCtrl::SPECIAL;
        LLSD& row = item["columns"];
        row[0]["column"] = "complex_visual";
        row[0]["type"] = "bar";
        LLSD& value = row[0]["value"];
        value["ratio"] = (F32)obj_cost_short / max_complexity * 1000;
        value["bottom"] = BAR_BOTTOM_PAD;
        value["left_pad"] = BAR_LEFT_PAD;
        value["right_pad"] = BAR_RIGHT_PAD;

        row[1]["column"] = "complex_value";
        row[1]["type"] = "text";
        row[1]["value"] = std::to_string(obj_cost_short);
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
    mObjectList->sortByColumnIndex(1, FALSE);
    mObjectList->setScrollPos(prev_pos);
    mObjectList->selectItemBySpecialId(prev_selected_id);
}

void LLFloaterPerformance::populateNearbyList()
{
    S32 prev_pos = mNearbyList->getScrollPos();
    LLUUID prev_selected_id = mNearbyList->getStringUUIDSelectedItem();
    mNearbyList->clearRows();
    mNearbyList->updateColumns(true);

    static LLCachedControl<U32> max_render_cost(gSavedSettings, "RenderAvatarMaxComplexity", 0);
    std::vector<LLCharacter*> valid_nearby_avs;
    mNearbyMaxComplexity = LLWorld::getInstance()->getNearbyAvatarsAndCompl(valid_nearby_avs);

    std::vector<LLCharacter*>::iterator char_iter = valid_nearby_avs.begin();
    while (char_iter != valid_nearby_avs.end())
    {
        LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(*char_iter);
        if (avatar && (LLVOAvatar::AOA_INVISIBLE != avatar->getOverallAppearance()))
        {
            S32 complexity_short = llmax((S32)avatar->getVisualComplexity() / 1000, 1);;
            LLSD item;
            item["id"] = avatar->getID();
            LLSD& row = item["columns"];
            row[0]["column"] = "complex_visual";
            row[0]["type"] = "bar";
            LLSD& value = row[0]["value"];
            value["ratio"] = (F32)complexity_short / mNearbyMaxComplexity * 1000;
            value["bottom"] = BAR_BOTTOM_PAD;
            value["left_pad"] = BAR_LEFT_PAD;
            value["right_pad"] = BAR_RIGHT_PAD;

            row[1]["column"] = "complex_value";
            row[1]["type"] = "text";
            row[1]["value"] = std::to_string(complexity_short);
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
                        if (LLVOAvatar::AOA_JELLYDOLL == avatar->getOverallAppearance())
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

void LLFloaterPerformance::updateMaxComplexity()
{
    LLAvatarComplexityControls::updateMax(
        mNearbyPanel->getChild<LLSliderCtrl>("IndirectMaxComplexity"),
        mNearbyPanel->getChild<LLTextBox>("IndirectMaxComplexityText"), 
        true);
}

void LLFloaterPerformance::updateComplexityText()
{
    LLAvatarComplexityControls::setText(gSavedSettings.getU32("RenderAvatarMaxComplexity"),
        mNearbyPanel->getChild<LLTextBox>("IndirectMaxComplexityText", true), 
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

// EOF
