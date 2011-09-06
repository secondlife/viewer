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
,	mLanguageCombo(NULL)
,	mTranslationServiceRadioGroup(NULL)
,	mBingAPIKeyEditor(NULL)
,	mGoogleAPIKeyEditor(NULL)
{
}

// virtual
BOOL LLFloaterTranslationSettings::postBuild()
{
	mMachineTranslationCB = getChild<LLCheckBoxCtrl>("translate_chat_checkbox");
	mLanguageCombo = getChild<LLComboBox>("translate_language_combo");
	mTranslationServiceRadioGroup = getChild<LLRadioGroup>("translation_service_rg");
	mBingAPIKeyEditor = getChild<LLLineEditor>("bing_api_key");
	mGoogleAPIKeyEditor = getChild<LLLineEditor>("google_api_key");

	mMachineTranslationCB->setCommitCallback(boost::bind(&LLFloaterTranslationSettings::updateControlsEnabledState, this));
	mTranslationServiceRadioGroup->setCommitCallback(boost::bind(&LLFloaterTranslationSettings::updateControlsEnabledState, this));
	getChild<LLButton>("ok_btn")->setClickedCallback(boost::bind(&LLFloaterTranslationSettings::onBtnOK, this));
	getChild<LLButton>("cancel_btn")->setClickedCallback(boost::bind(&LLFloater::closeFloater, this, false));

	center();
	return TRUE;
}

// virtual
void LLFloaterTranslationSettings::onOpen(const LLSD& key)
{
	mMachineTranslationCB->setValue(gSavedSettings.getBOOL("TranslateChat"));
	mLanguageCombo->setSelectedByValue(gSavedSettings.getString("TranslateLanguage"), TRUE);
	mTranslationServiceRadioGroup->setSelectedByValue(gSavedSettings.getString("TranslationService"), TRUE);
	mBingAPIKeyEditor->setText(gSavedSettings.getString("BingTranslateAPIKey"));
	mGoogleAPIKeyEditor->setText(gSavedSettings.getString("GoogleTranslateAPIv2Key"));

	updateControlsEnabledState();
}

std::string LLFloaterTranslationSettings::getSelectedService() const
{
	return mTranslationServiceRadioGroup->getSelectedValue().asString();
}

void LLFloaterTranslationSettings::showError(const std::string& err_name)
{
	LLSD args;
	args["MESSAGE"] = getString(err_name);
	LLNotificationsUtil::add("GenericAlert", args);
}

bool LLFloaterTranslationSettings::validate()
{
	bool translate_chat = mMachineTranslationCB->getValue().asBoolean();
	if (!translate_chat) return true;

	std::string service = getSelectedService();
	if (service == "bing" && mBingAPIKeyEditor->getText().empty())
	{
		showError("no_bing_api_key");
		return false;
	}

	if (service == "google_v2" && mGoogleAPIKeyEditor->getText().empty())
	{
		showError("no_google_api_key");
		return false;
	}

	return true;
}

void LLFloaterTranslationSettings::updateControlsEnabledState()
{
	// Enable/disable controls based on the checkbox value.
	bool on = mMachineTranslationCB->getValue().asBoolean();
	std::string service = getSelectedService();

	mTranslationServiceRadioGroup->setEnabled(on);
	mLanguageCombo->setEnabled(on);

	getChild<LLTextBox>("bing_api_key_label")->setEnabled(on);
	mBingAPIKeyEditor->setEnabled(on);

	getChild<LLTextBox>("google_api_key_label")->setEnabled(on);
	mGoogleAPIKeyEditor->setEnabled(on);

	mBingAPIKeyEditor->setEnabled(service == "bing");
	mGoogleAPIKeyEditor->setEnabled(service == "google_v2");
}

void LLFloaterTranslationSettings::onBtnOK()
{
	if (validate())
	{
		gSavedSettings.setBOOL("TranslateChat", mMachineTranslationCB->getValue().asBoolean());
		gSavedSettings.setString("TranslateLanguage", mLanguageCombo->getSelectedValue().asString());
		gSavedSettings.setString("TranslationService", getSelectedService());
		gSavedSettings.setString("BingTranslateAPIKey", mBingAPIKeyEditor->getText());
		gSavedSettings.setString("GoogleTranslateAPIv2Key", mGoogleAPIKeyEditor->getText());
		closeFloater(false);
	}
}
