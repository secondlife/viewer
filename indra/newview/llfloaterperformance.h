/**
 * @file llfloaterperformance.h
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

#ifndef LL_LLFLOATERPERFORMANCE_H
#define LL_LLFLOATERPERFORMANCE_H

#include "llfloater.h"
#include "lllistcontextmenu.h"

class LLCharacter;
class LLCheckBoxCtrl;
class LLNameListCtrl;
class LLTextBox;

class LLFloaterPerformance : public LLFloater
{
public:
    LLFloaterPerformance(const LLSD& key);
    virtual ~LLFloaterPerformance();

    /*virtual*/ bool postBuild();
    /*virtual*/ void draw();

    void showSelectedPanel(LLPanel* selected_panel);
    void showMainPanel();
    void hidePanels();
    void showAutoadjustmentsPanel();

    void detachObject(const LLUUID& obj_id);

    void onAvatarListRightClick(LLUICtrl* ctrl, S32 x, S32 y);

    void onCustomAction (const LLSD& userdata, const LLUUID& av_id);
    bool isActionChecked(const LLSD& userdata, const LLUUID& av_id);

private:
    void initBackBtn(LLPanel* panel);
    void populateHUDList();
    void populateObjectList();
    void populateNearbyList();
    void setFPSText();

    void onClickAdvanced();
    void onClickDefaults();
    void onChangeQuality(const LLSD& data);
    void onClickHideAvatars();
    void onClickExceptions();
    void onClickShadows();
    void onClickAdvancedLighting();

    void startAutotune();
    void stopAutotune();
    void updateAutotuneCtrls(bool autotune_enabled);
    void enableAutotuneWarning();

    void updateMaxRenderTime();

    static void changeQualityLevel(const std::string& notif);

    LLPanel* mMainPanel;
    LLPanel* mNearbyPanel;
    LLPanel* mComplexityPanel;
    LLPanel* mHUDsPanel;
    LLPanel* mSettingsPanel;
    LLPanel* mAutoadjustmentsPanel;
    LLNameListCtrl* mHUDList;
    LLNameListCtrl* mObjectList;
    LLNameListCtrl* mNearbyList;

    LLButton* mStartAutotuneBtn;
    LLButton* mStopAutotuneBtn;

    LLTextBox* mTextWIPDesc = nullptr;
    LLTextBox* mTextDisplayDesc = nullptr;
    LLTextBox* mTextFPSLabel = nullptr;
    LLTextBox* mTextFPSValue = nullptr;

    LLCheckBoxCtrl* mCheckTuneContinous = nullptr;

    LLListContextMenu* mContextMenu;

    LLTimer* mUpdateTimer;

    // maximum GPU time of nearby avatars in ms according to LLWorld::getNearbyAvatarsAndMaxGPUTime
    // -1.f if no profile has happened yet
    F32 mNearbyMaxGPUTime = -1.f;

    boost::signals2::connection mMaxARTChangedSignal;
};

#endif // LL_LLFLOATERPERFORMANCE_H
