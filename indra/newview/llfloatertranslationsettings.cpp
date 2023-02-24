/** 
 * @file llfloatertranslationsettings.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloatertranslationsettings.h"

// Viewer includes
#include "llfloaterimnearbychat.h"
#include "lltranslate.h"
#include "llviewercontrol.h" // for gSavedSettings

// Linden library includes
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfloaterreg.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llradiogroup.h"

LLFloaterTranslationSettings::LLFloaterTranslationSettings(const LLSD& key)
:	LLFloater(key)
,	mMachineTranslationCB(NULL)
,	mAzureKeyVerified(false)
,	mGoogleKeyVerified(false)
{
}

// virtual
BOOL LLFloaterTranslationSettings::postBuild()
{
	mMachineTranslationCB = getChild<LLCheckBoxCtrl>("translate_chat_checkbox");
	mLanguageCombo = getChild<LLComboBox>("translate_language_combo");
	mTranslationServiceRadioGroup = getChild<LLRadioGroup>("translation_service_rg");
    mAzureAPIEndpointEditor = getChild<LLComboBox>("azure_api_endpoint_combo");
	mAzureAPIKeyEditor = getChild<LLLineEditor>("azure_api_key");
    mAzureAPIRegionEditor = getChild<LLLineEditor>("azure_api_region");
	mGoogleAPIKeyEditor = getChild<LLLineEditor>("google_api_key");
	mAzureVerifyBtn = getChild<LLButton>("verify_azure_api_key_btn");
	mGoogleVerifyBtn = getChild<LLButton>("verify_google_api_key_btn");
	mOKBtn = getChild<LLButton>("ok_btn");

	mMachineTranslationCB->setCommitCallback(boost::bind(&LLFloaterTranslationSettings::updateControlsEnabledState, this));
	mTranslationServiceRadioGroup->setCommitCallback(boost::bind(&LLFloaterTranslationSettings::updateControlsEnabledState, this));
	mOKBtn->setClickedCallback(boost::bind(&LLFloaterTranslationSettings::onBtnOK, this));
	getChild<LLButton>("cancel_btn")->setClickedCallback(boost::bind(&LLFloater::closeFloater, this, false));
	mAzureVerifyBtn->setClickedCallback(boost::bind(&LLFloaterTranslationSettings::onBtnAzureVerify, this));
	mGoogleVerifyBtn->setClickedCallback(boost::bind(&LLFloaterTranslationSettings::onBtnGoogleVerify, this));

	mAzureAPIKeyEditor->setFocusReceivedCallback(boost::bind(&LLFloaterTranslationSettings::onEditorFocused, this, _1));
	mAzureAPIKeyEditor->setKeystrokeCallback(boost::bind(&LLFloaterTranslationSettings::onAzureKeyEdited, this), NULL);
    mAzureAPIRegionEditor->setFocusReceivedCallback(boost::bind(&LLFloaterTranslationSettings::onEditorFocused, this, _1));
    mAzureAPIRegionEditor->setKeystrokeCallback(boost::bind(&LLFloaterTranslationSettings::onAzureKeyEdited, this), NULL);

    mAzureAPIEndpointEditor->setFocusLostCallback(boost::bind(&LLFloaterTranslationSettings::onAzureKeyEdited, this));
    mAzureAPIEndpointEditor->setCommitCallback(boost::bind(&LLFloaterTranslationSettings::onAzureKeyEdited, this));

	mGoogleAPIKeyEditor->setFocusReceivedCallback(boost::bind(&LLFloaterTranslationSettings::onEditorFocused, this, _1));
	mGoogleAPIKeyEditor->setKeystrokeCallback(boost::bind(&LLFloaterTranslationSettings::onGoogleKeyEdited, this), NULL);

	center();
	return TRUE;
}

// virtual
void LLFloaterTranslationSettings::onOpen(const LLSD& key)
{
	mMachineTranslationCB->setValue(gSavedSettings.getBOOL("TranslateChat"));
	mLanguageCombo->setSelectedByValue(gSavedSettings.getString("TranslateLanguage"), TRUE);
	mTranslationServiceRadioGroup->setSelectedByValue(gSavedSettings.getString("TranslationService"), TRUE);

	LLSD azure_key = gSavedSettings.getLLSD("AzureTranslateAPIKey");
	if (azure_key.isMap())
	{
		mAzureAPIKeyEditor->setText(azure_key["id"].asString());
		mAzureAPIKeyEditor->setTentative(false);
        if (azure_key.has("region"))
        {
            mAzureAPIRegionEditor->setText(azure_key["region"].asString());
            mAzureAPIRegionEditor->setTentative(false);
        }
        else
        {
            mAzureAPIRegionEditor->setTentative(true);
        }
        mAzureAPIEndpointEditor->setValue(azure_key["endpoint"]);
		verifyKey(LLTranslate::SERVICE_AZURE, azure_key, false);
	}
	else
	{
		mAzureAPIKeyEditor->setTentative(TRUE);
		mAzureKeyVerified = FALSE;
	}

	std::string google_key = gSavedSettings.getString("GoogleTranslateAPIKey");
	if (!google_key.empty())
	{
		mGoogleAPIKeyEditor->setText(google_key);
		mGoogleAPIKeyEditor->setTentative(FALSE);
		verifyKey(LLTranslate::SERVICE_GOOGLE, google_key, false);
	}
	else
	{
		mGoogleAPIKeyEditor->setTentative(TRUE);
		mGoogleKeyVerified = FALSE;
	}

	updateControlsEnabledState();
}

void LLFloaterTranslationSettings::setAzureVerified(bool ok, bool alert)
{
	if (alert)
	{
		showAlert(ok ? "azure_api_key_verified" : "azure_api_key_not_verified");
	}

	mAzureKeyVerified = ok;
	updateControlsEnabledState();
}

void LLFloaterTranslationSettings::setGoogleVerified(bool ok, bool alert)
{
	if (alert)
	{
		showAlert(ok ? "google_api_key_verified" : "google_api_key_not_verified");
	}

	mGoogleKeyVerified = ok;
	updateControlsEnabledState();
}

std::string LLFloaterTranslationSettings::getSelectedService() const
{
	return mTranslationServiceRadioGroup->getSelectedValue().asString();
}

LLSD LLFloaterTranslationSettings::getEnteredAzureKey() const
{
    LLSD key;
    if (!mAzureAPIKeyEditor->getTentative())
    {
        key["endpoint"] = mAzureAPIEndpointEditor->getValue();
        key["id"] = mAzureAPIKeyEditor->getText();
        if (!mAzureAPIRegionEditor->getTentative())
        {
            key["region"] = mAzureAPIRegionEditor->getText();
        }
    }
	return key;
}

std::string LLFloaterTranslationSettings::getEnteredGoogleKey() const
{
	return mGoogleAPIKeyEditor->getTentative() ? LLStringUtil::null : mGoogleAPIKeyEditor->getText();
}

void LLFloaterTranslationSettings::showAlert(const std::string& msg_name) const
{
	LLSD args;
	args["MESSAGE"] = getString(msg_name);
	LLNotificationsUtil::add("GenericAlert", args);
}

void LLFloaterTranslationSettings::updateControlsEnabledState()
{
	// Enable/disable controls based on the checkbox value.
	bool on = mMachineTranslationCB->getValue().asBoolean();
	std::string service = getSelectedService();
	bool azure_selected = service == "azure";
	bool google_selected = service == "google";

	mTranslationServiceRadioGroup->setEnabled(on);
	mLanguageCombo->setEnabled(on);

	getChild<LLTextBox>("azure_api_endoint_label")->setEnabled(on);
	mAzureAPIEndpointEditor->setEnabled(on);
    getChild<LLTextBox>("azure_api_key_label")->setEnabled(on);
    mAzureAPIKeyEditor->setEnabled(on);
    getChild<LLTextBox>("azure_api_region_label")->setEnabled(on);
    mAzureAPIRegionEditor->setEnabled(on);

	getChild<LLTextBox>("google_api_key_label")->setEnabled(on);
	mGoogleAPIKeyEditor->setEnabled(on);

	mAzureAPIKeyEditor->setEnabled(on && azure_selected);
	mGoogleAPIKeyEditor->setEnabled(on && google_selected);

	mAzureVerifyBtn->setEnabled(on && azure_selected &&
		!mAzureKeyVerified && getEnteredAzureKey().isMap());
	mGoogleVerifyBtn->setEnabled(on && google_selected &&
		!mGoogleKeyVerified && !getEnteredGoogleKey().empty());

	bool service_verified = (azure_selected && mAzureKeyVerified) || (google_selected && mGoogleKeyVerified);
	gSavedPerAccountSettings.setBOOL("TranslatingEnabled", service_verified);

	mOKBtn->setEnabled(!on || service_verified);
}

/*static*/
void LLFloaterTranslationSettings::setVerificationStatus(int service, bool ok, bool alert)
{
    LLFloaterTranslationSettings* floater =
        LLFloaterReg::getTypedInstance<LLFloaterTranslationSettings>("prefs_translation");

    if (!floater)
    {
        LL_WARNS() << "Cannot find translation settings floater" << LL_ENDL;
        return;
    }

    switch (service)
    {
    case LLTranslate::SERVICE_AZURE:
        floater->setAzureVerified(ok, alert);
        break;
    case LLTranslate::SERVICE_GOOGLE:
        floater->setGoogleVerified(ok, alert);
        break;
    }
}


void LLFloaterTranslationSettings::verifyKey(int service, const LLSD& key, bool alert)
{
    LLTranslate::verifyKey(static_cast<LLTranslate::EService>(service), key,
        boost::bind(&LLFloaterTranslationSettings::setVerificationStatus, _1, _2, alert));
}

void LLFloaterTranslationSettings::onEditorFocused(LLFocusableElement* control)
{
	LLLineEditor* editor = dynamic_cast<LLLineEditor*>(control);
	if (editor && editor->hasTabStop()) // if enabled. getEnabled() doesn't work
	{
		if (editor->getTentative())
		{
			editor->setText(LLStringUtil::null);
			editor->setTentative(FALSE);
		}
	}
}

void LLFloaterTranslationSettings::onAzureKeyEdited()
{
	if (mAzureAPIKeyEditor->isDirty()
        || mAzureAPIRegionEditor->isDirty()
        || mAzureAPIEndpointEditor->getValue().isString())
	{
        // todo: verify mAzureAPIEndpointEditor url
		setAzureVerified(false, false);
	}
}

void LLFloaterTranslationSettings::onGoogleKeyEdited()
{
	if (mGoogleAPIKeyEditor->isDirty())
	{
		setGoogleVerified(false, false);
	}
}

void LLFloaterTranslationSettings::onBtnAzureVerify()
{
	LLSD key = getEnteredAzureKey();
	if (key.isMap())
	{
		verifyKey(LLTranslate::SERVICE_AZURE, key);
	}
}

void LLFloaterTranslationSettings::onBtnGoogleVerify()
{
	std::string key = getEnteredGoogleKey();
	if (!key.empty())
	{
		verifyKey(LLTranslate::SERVICE_GOOGLE, LLSD(key));
	}
}
void LLFloaterTranslationSettings::onClose(bool app_quitting)
{
	std::string service = gSavedSettings.getString("TranslationService");
	bool azure_selected = service == "azure";
	bool google_selected = service == "google";

	bool service_verified = (azure_selected && mAzureKeyVerified) || (google_selected && mGoogleKeyVerified);
	gSavedPerAccountSettings.setBOOL("TranslatingEnabled", service_verified);

}
void LLFloaterTranslationSettings::onBtnOK()
{
	gSavedSettings.setBOOL("TranslateChat", mMachineTranslationCB->getValue().asBoolean());
	gSavedSettings.setString("TranslateLanguage", mLanguageCombo->getSelectedValue().asString());
	gSavedSettings.setString("TranslationService", getSelectedService());
	gSavedSettings.setLLSD("AzureTranslateAPIKey", getEnteredAzureKey());
	gSavedSettings.setString("GoogleTranslateAPIKey", getEnteredGoogleKey());

	closeFloater(false);
}
