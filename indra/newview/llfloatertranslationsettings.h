/**
 * @file llfloatertranslationsettings.h
 * @brief Machine translation settings for chat
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLFLOATERTRANSLATIONSETTINGS_H
#define LL_LLFLOATERTRANSLATIONSETTINGS_H

#include "llfloater.h"

class LLButton;
class LLCheckBoxCtrl;
class LLComboBox;
class LLLineEditor;
class LLRadioGroup;

class LLFloaterTranslationSettings : public LLFloater
{
public:
    LLFloaterTranslationSettings(const LLSD& key);
    bool postBuild() override;
    void onOpen(const LLSD& key) override;

    void setAzureVerified(bool ok, bool alert, S32 status);
    void setGoogleVerified(bool ok, bool alert, S32 status);
    void setDeepLVerified(bool ok, bool alert, S32 status);
    void onClose(bool app_quitting) override;

private:
    std::string getSelectedService() const;
    LLSD getEnteredAzureKey() const;
    std::string getEnteredGoogleKey() const;
    LLSD getEnteredDeepLKey() const;
    void showAlert(const std::string& msg_name, S32 status) const;
    void updateControlsEnabledState();
    void verifyKey(int service, const LLSD& key, bool alert = true);

    void onEditorFocused(LLFocusableElement* control);
    void onAzureKeyEdited();
    void onGoogleKeyEdited();
    void onDeepLKeyEdited();
    void onBtnAzureVerify();
    void onBtnGoogleVerify();
    void onBtnDeepLVerify();
    void onBtnOK();

    static void setVerificationStatus(int service, bool alert, bool ok, S32 status);

    LLCheckBoxCtrl* mMachineTranslationCB;
    LLComboBox* mLanguageCombo;
    LLComboBox* mAzureAPIEndpointEditor;
    LLLineEditor* mAzureAPIKeyEditor;
    LLLineEditor* mAzureAPIRegionEditor;
    LLLineEditor* mGoogleAPIKeyEditor;
    LLComboBox* mDeepLAPIDomainCombo;
    LLLineEditor* mDeepLAPIKeyEditor;
    LLRadioGroup* mTranslationServiceRadioGroup;
    LLButton* mAzureVerifyBtn;
    LLButton* mGoogleVerifyBtn;
    LLButton* mDeepLVerifyBtn;
    LLButton* mOKBtn;

    bool mAzureKeyVerified;
    bool mGoogleKeyVerified;
    bool mDeepLKeyVerified;
};

#endif // LL_LLFLOATERTRANSLATIONSETTINGS_H
