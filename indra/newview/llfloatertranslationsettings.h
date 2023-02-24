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
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

	void setAzureVerified(bool ok, bool alert);
	void setGoogleVerified(bool ok, bool alert);
	void onClose(bool app_quitting);

private:
	std::string getSelectedService() const;
	LLSD getEnteredAzureKey() const;
	std::string getEnteredGoogleKey() const;
	void showAlert(const std::string& msg_name) const;
	void updateControlsEnabledState();
    void verifyKey(int service, const LLSD& key, bool alert = true);

	void onEditorFocused(LLFocusableElement* control);
	void onAzureKeyEdited();
	void onGoogleKeyEdited();
	void onBtnAzureVerify();
	void onBtnGoogleVerify();
	void onBtnOK();

    static void setVerificationStatus(int service, bool alert, bool ok);

	LLCheckBoxCtrl* mMachineTranslationCB;
	LLComboBox* mLanguageCombo;
    LLComboBox* mAzureAPIEndpointEditor;;
	LLLineEditor* mAzureAPIKeyEditor;
    LLLineEditor* mAzureAPIRegionEditor;
	LLLineEditor* mGoogleAPIKeyEditor;
	LLRadioGroup* mTranslationServiceRadioGroup;
	LLButton* mAzureVerifyBtn;
	LLButton* mGoogleVerifyBtn;
	LLButton* mOKBtn;

	bool mAzureKeyVerified;
	bool mGoogleKeyVerified;
};

#endif // LL_LLFLOATERTRANSLATIONSETTINGS_H
