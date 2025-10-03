/**
 * @file llfloaterpreferencesgraphicsadvanced.h
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

#ifndef LLFLOATERPREFERENCEGRAPHICSADVANCED_H
#define LLFLOATERPREFERENCEGRAPHICSADVANCED_H

#include "llcontrol.h"
#include "llfloater.h"

class LLSliderCtrl;
class LLTextBox;

class LLFloaterPreferenceGraphicsAdvanced : public LLFloater
{
public:
    LLFloaterPreferenceGraphicsAdvanced(const LLSD& key);
    ~LLFloaterPreferenceGraphicsAdvanced();
    /*virtual*/ bool postBuild();
    void onOpen(const LLSD& key);
    void onClickCloseBtn(bool app_quitting);
    void disableUnavailableSettings();
    void refreshEnabledGraphics();
    void refreshEnabledState();
    void updateSliderText(LLSliderCtrl* ctrl, LLTextBox* text_box);
    void updateMaxNonImpostors();
    void updateIndirectMaxNonImpostors(const LLSD& newvalue);
    void setMaxNonImpostorsText(U32 value, LLTextBox* text_box);
    void updateMaxComplexity();
    void updateComplexityMode(const LLSD& newvalue);
    void updateComplexityText();
    void updateObjectMeshDetailText();
    void refresh();
    // callback for when client modifies a render option
    void onRenderOptionEnable();
    void onAdvancedAtmosphericsEnable();
    LOG_CLASS(LLFloaterPreferenceGraphicsAdvanced);

protected:
    void        onBtnOK(const LLSD& userdata);
    void        onBtnCancel(const LLSD& userdata);

    boost::signals2::connection	mImpostorsChangedSignal;
    boost::signals2::connection mComplexityChangedSignal;
    boost::signals2::connection mComplexityModeChangedSignal;
    boost::signals2::connection mLODFactorChangedSignal;
    boost::signals2::connection mNumImpostorsChangedSignal;
};

#endif //LLFLOATERPREFERENCEGRAPHICSADVANCED_H

