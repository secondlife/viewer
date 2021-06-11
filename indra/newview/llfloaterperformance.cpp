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

#include "llappearancemgr.h"
#include "llavatarrendernotifier.h"
#include "llfeaturemanager.h"
#include "llfloaterreg.h"
#include "llnamelistctrl.h"
#include "lltextbox.h"
#include "llvoavatar.h"


LLFloaterPerformance::LLFloaterPerformance(const LLSD& key)
    : LLFloater(key)
{

}

LLFloaterPerformance::~LLFloaterPerformance()
{
}

BOOL LLFloaterPerformance::postBuild()
{
    mMainPanel = getChild<LLPanel>("panel_performance_main");
    mTroubleshootingPanel = getChild<LLPanel>("panel_performance_troubleshooting");
    mNearbyPanel = getChild<LLPanel>("panel_performance_nearby");
    mScriptsPanel = getChild<LLPanel>("panel_performance_scripts");
    mPreferencesPanel = getChild<LLPanel>("panel_performance_preferences");
    mHUDsPanel = getChild<LLPanel>("panel_performance_huds");

    getChild<LLPanel>("troubleshooting_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mTroubleshootingPanel));
    getChild<LLPanel>("nearby_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mNearbyPanel));
    getChild<LLPanel>("scripts_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mScriptsPanel));
    getChild<LLPanel>("preferences_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mPreferencesPanel));
    getChild<LLPanel>("huds_subpanel")->setMouseDownCallback(boost::bind(&LLFloaterPerformance::showSelectedPanel, this, mHUDsPanel));

    initBackBtn(mTroubleshootingPanel);
    initBackBtn(mNearbyPanel);
    initBackBtn(mScriptsPanel);
    initBackBtn(mPreferencesPanel);
    initBackBtn(mHUDsPanel);


    mHUDsPanel->getChild<LLButton>("refresh_list_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::populateHUDList, this));
 
    mHUDList = mHUDsPanel->getChild<LLNameListCtrl>("hud_list");
    mHUDList->setNameListType(LLNameListCtrl::SPECIAL);
    mHUDList->setHoverIconName("StopReload_Off");
    mHUDList->setIconClickedCallback(boost::bind(&LLFloaterPerformance::detachItem, this, _1));

    mPreferencesPanel->getChild<LLButton>("advanced_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickAdvanced, this));
    mPreferencesPanel->getChild<LLButton>("defaults_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickRecommended, this));

    mNearbyPanel->getChild<LLButton>("exceptions_btn")->setCommitCallback(boost::bind(&LLFloaterPerformance::onClickExceptions, this));
    mNearbyList = mNearbyPanel->getChild<LLNameListCtrl>("nearby_list");
 
    return TRUE;
}

void LLFloaterPerformance::showSelectedPanel(LLPanel* selected_panel)
{
    selected_panel->setVisible(TRUE);
    mMainPanel->setVisible(FALSE);

    if (mHUDsPanel == selected_panel)
    {
        populateHUDList();
    }
    else if (mNearbyPanel == selected_panel)
    {
        populateNearbyList();
    }
}

void LLFloaterPerformance::showMainPanel()
{
    mTroubleshootingPanel->setVisible(FALSE);
    mNearbyPanel->setVisible(FALSE);
    mScriptsPanel->setVisible(FALSE);
    mHUDsPanel->setVisible(FALSE);
    mPreferencesPanel->setVisible(FALSE);
    mMainPanel->setVisible(TRUE);
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
    mHUDList->clearRows();
    mHUDList->updateColumns(true);

    hud_complexity_list_t complexity_list = LLHUDRenderNotifier::getInstance()->getHUDComplexityList();

    hud_complexity_list_t::iterator iter = complexity_list.begin();
    hud_complexity_list_t::iterator end = complexity_list.end();
   
    for (; iter != end; ++iter)
    {
        LLHUDComplexity hud_object_complexity = *iter;        

        LLSD item;
        item["special_id"] = hud_object_complexity.objectId;
        item["target"] = LLNameListCtrl::SPECIAL;
        LLSD& row = item["columns"];
        row[0]["column"] = "complex_visual";
        row[0]["type"] = "text";
        row[0]["value"] = "*";

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

    mHUDsPanel->getChild<LLTextBox>("huds_value")->setValue(std::to_string(complexity_list.size()));
}

void LLFloaterPerformance::populateNearbyList()
{
    mNearbyList->clearRows();
    mNearbyList->updateColumns(true);

    std::vector<LLCharacter*>::iterator char_iter = LLCharacter::sInstances.begin();
    while (char_iter != LLCharacter::sInstances.end())
    {
        LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(*char_iter);
        if (avatar && !avatar->isDead() && !avatar->isControlAvatar())
        {
            avatar->calculateUpdateRenderComplexity(); 

            LLSD item;
            item["id"] = avatar->getID();
            LLSD& row = item["columns"];
            row[0]["column"] = "complex_visual";
            row[0]["type"] = "text";
            row[0]["value"] = "*";

            row[1]["column"] = "complex_value";
            row[1]["type"] = "text";
            row[1]["value"] = std::to_string( avatar->getVisualComplexity());
            row[1]["font"]["name"] = "SANSSERIF";

            row[2]["column"] = "name";
            row[2]["type"] = "text";
            row[2]["value"] = avatar->getFullname();
            row[2]["font"]["name"] = "SANSSERIF";

            mNearbyList->addElement(item);
        }
        char_iter++;
    }

}

void LLFloaterPerformance::detachItem(const LLUUID& item_id)
{
    LLAppearanceMgr::instance().removeItemFromAvatar(item_id);
    mHUDList->removeNameItem(item_id);
}

void LLFloaterPerformance::onClickRecommended()
{
    LLFeatureManager::getInstance()->applyRecommendedSettings();
}

void LLFloaterPerformance::onClickAdvanced()
{
    LLFloaterReg::showInstance("prefs_graphics_advanced");
}

void LLFloaterPerformance::onClickExceptions()
{
    LLFloaterReg::showInstance("avatar_render_settings");
}

// EOF
