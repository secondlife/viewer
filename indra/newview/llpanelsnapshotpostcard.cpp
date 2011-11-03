/** 
 * @file llpanelsnapshotpostcard.cpp
 * @brief Postcard sending panel.
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

#include "llcombobox.h"
#include "llnotificationsutil.h"
#include "llsidetraypanelcontainer.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "lltexteditor.h"

#include "llagent.h"
#include "llagentui.h"
#include "llfloatersnapshot.h" // FIXME: replace with a snapshot storage model
#include "llpanelsnapshot.h"
#include "llpostcard.h"
#include "llviewercontrol.h" // gSavedSettings
#include "llviewerwindow.h"

#include <boost/regex.hpp>

/**
 * Sends postcard via email.
 */
class LLPanelSnapshotPostcard
:	public LLPanelSnapshot
{
	LOG_CLASS(LLPanelSnapshotPostcard);

public:
	LLPanelSnapshotPostcard();
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ S32	notify(const LLSD& info);

private:
	/*virtual*/ std::string getWidthSpinnerName() const		{ return "postcard_snapshot_width"; }
	/*virtual*/ std::string getHeightSpinnerName() const	{ return "postcard_snapshot_height"; }
	/*virtual*/ std::string getAspectRatioCBName() const	{ return "postcard_keep_aspect_check"; }
	/*virtual*/ std::string getImageSizeComboName() const	{ return "postcard_size_combo"; }
	/*virtual*/ void updateControls(const LLSD& info);

	void updateCustomResControls(); ///< Enable/disable custom resolution controls (spinners and checkbox)
	bool missingSubjMsgAlertCallback(const LLSD& notification, const LLSD& response);
	void sendPostcard();

	void onMsgFormFocusRecieved();
	void onFormatComboCommit(LLUICtrl* ctrl);
	void onResolutionComboCommit(LLUICtrl* ctrl);
	void onCustomResolutionCommit(LLUICtrl* ctrl);
	void onKeepAspectRatioCommit(LLUICtrl* ctrl);
	void onQualitySliderCommit(LLUICtrl* ctrl);
	void onTabButtonPress(S32 btn_idx);
	void onSend();

	bool mHasFirstMsgFocus;
};

static LLRegisterPanelClassWrapper<LLPanelSnapshotPostcard> panel_class("llpanelsnapshotpostcard");

LLPanelSnapshotPostcard::LLPanelSnapshotPostcard()
:	mHasFirstMsgFocus(false)
{
	mCommitCallbackRegistrar.add("Postcard.Send",		boost::bind(&LLPanelSnapshotPostcard::onSend,	this));
	mCommitCallbackRegistrar.add("Postcard.Cancel",		boost::bind(&LLPanelSnapshotPostcard::cancel,	this));
	mCommitCallbackRegistrar.add("Postcard.Message",	boost::bind(&LLPanelSnapshotPostcard::onTabButtonPress,	this, 0));
	mCommitCallbackRegistrar.add("Postcard.Settings",	boost::bind(&LLPanelSnapshotPostcard::onTabButtonPress,	this, 1));

}

// virtual
BOOL LLPanelSnapshotPostcard::postBuild()
{
	// pick up the user's up-to-date email address
	gAgent.sendAgentUserInfoRequest();

	getChildView("from_form")->setEnabled(FALSE);

	std::string name_string;
	LLAgentUI::buildFullname(name_string);
	getChild<LLUICtrl>("name_form")->setValue(LLSD(name_string));

	// For the first time a user focuses to .the msg box, all text will be selected.
	getChild<LLUICtrl>("msg_form")->setFocusChangedCallback(boost::bind(&LLPanelSnapshotPostcard::onMsgFormFocusRecieved, this));

	getChild<LLUICtrl>("to_form")->setFocus(TRUE);

	getChild<LLUICtrl>(getImageSizeComboName())->setCommitCallback(boost::bind(&LLPanelSnapshotPostcard::onResolutionComboCommit, this, _1));
	getChild<LLUICtrl>(getWidthSpinnerName())->setCommitCallback(boost::bind(&LLPanelSnapshotPostcard::onCustomResolutionCommit, this, _1));
	getChild<LLUICtrl>(getHeightSpinnerName())->setCommitCallback(boost::bind(&LLPanelSnapshotPostcard::onCustomResolutionCommit, this, _1));
	getChild<LLUICtrl>(getAspectRatioCBName())->setCommitCallback(boost::bind(&LLPanelSnapshotPostcard::onKeepAspectRatioCommit, this, _1));
	getChild<LLUICtrl>("image_quality_slider")->setCommitCallback(boost::bind(&LLPanelSnapshotPostcard::onQualitySliderCommit, this, _1));

	getChild<LLButton>("message_btn")->setToggleState(TRUE);

	return LLPanelSnapshot::postBuild();
}

// virtual
void LLPanelSnapshotPostcard::onOpen(const LLSD& key)
{
	gSavedSettings.setS32("SnapshotFormat", getImageFormat());
	updateCustomResControls();
	LLPanelSnapshot::onOpen(key);
}

// virtual
S32 LLPanelSnapshotPostcard::notify(const LLSD& info)
{
	if (!info.has("agent-email"))
	{
		llassert(info.has("agent-email"));
		return 0;
	}

	LLUICtrl* from_input = getChild<LLUICtrl>("from_form");
	const std::string& text = from_input->getValue().asString();
	if (text.empty())
	{
		// there's no text in this field yet, pre-populate
		from_input->setValue(info["agent-email"]);
	}

	return 1;
}

// virtual
void LLPanelSnapshotPostcard::updateControls(const LLSD& info)
{
	getChild<LLUICtrl>("image_quality_slider")->setValue(gSavedSettings.getS32("SnapshotQuality"));
	updateImageQualityLevel();

	const bool have_snapshot = info.has("have-snapshot") ? info["have-snapshot"].asBoolean() : true;
	getChild<LLUICtrl>("send_btn")->setEnabled(have_snapshot);
}

void LLPanelSnapshotPostcard::updateCustomResControls()
{
	LLComboBox* combo = getChild<LLComboBox>(getImageSizeComboName());
	S32 selected_idx = combo->getFirstSelectedIndex();
	bool enable = selected_idx == 0 || selected_idx == (combo->getItemCount() - 1); // Current Window or Custom selected

	getChild<LLUICtrl>(getWidthSpinnerName())->setEnabled(enable);
	getChild<LLSpinCtrl>(getWidthSpinnerName())->setAllowEdit(enable);
	getChild<LLUICtrl>(getHeightSpinnerName())->setEnabled(enable);
	getChild<LLSpinCtrl>(getHeightSpinnerName())->setAllowEdit(enable);
	getChild<LLUICtrl>(getAspectRatioCBName())->setEnabled(enable);
}

bool LLPanelSnapshotPostcard::missingSubjMsgAlertCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if(0 == option)
	{
		// User clicked OK
		if((getChild<LLUICtrl>("subject_form")->getValue().asString()).empty())
		{
			// Stuff the subject back into the form.
			getChild<LLUICtrl>("subject_form")->setValue(getString("default_subject"));
		}

		if (!mHasFirstMsgFocus)
		{
			// The user never switched focus to the message window.
			// Using the default string.
			getChild<LLUICtrl>("msg_form")->setValue(getString("default_message"));
		}

		sendPostcard();
	}
	return false;
}


void LLPanelSnapshotPostcard::sendPostcard()
{
	std::string from(getChild<LLUICtrl>("from_form")->getValue().asString());
	std::string to(getChild<LLUICtrl>("to_form")->getValue().asString());
	std::string subject(getChild<LLUICtrl>("subject_form")->getValue().asString());

	LLSD postcard = LLSD::emptyMap();
	postcard["pos-global"] = LLFloaterSnapshot::getPosTakenGlobal().getValue();
	postcard["to"] = to;
	postcard["from"] = from;
	postcard["name"] = getChild<LLUICtrl>("name_form")->getValue().asString();
	postcard["subject"] = subject;
	postcard["msg"] = getChild<LLUICtrl>("msg_form")->getValue().asString();
	LLPostCard::send(LLFloaterSnapshot::getImageData(), postcard);

	// Give user feedback of the event.
	gViewerWindow->playSnapshotAnimAndSound();

	LLFloaterSnapshot::postSave();
}

void LLPanelSnapshotPostcard::onMsgFormFocusRecieved()
{
	LLTextEditor* msg_form = getChild<LLTextEditor>("msg_form");
	if (msg_form->hasFocus() && !mHasFirstMsgFocus)
	{
		mHasFirstMsgFocus = true;
		msg_form->setText(LLStringUtil::null);
	}
}

void LLPanelSnapshotPostcard::onFormatComboCommit(LLUICtrl* ctrl)
{
	// will call updateControls()
	LLFloaterSnapshot::getInstance()->notify(LLSD().with("image-format-change", true));
}

void LLPanelSnapshotPostcard::onResolutionComboCommit(LLUICtrl* ctrl)
{
	updateCustomResControls();

	LLSD info;
	info["combo-res-change"]["control-name"] = ctrl->getName();
	LLFloaterSnapshot::getInstance()->notify(info);
}

void LLPanelSnapshotPostcard::onCustomResolutionCommit(LLUICtrl* ctrl)
{
	LLSD info;
	info["w"] = getChild<LLUICtrl>(getWidthSpinnerName())->getValue().asInteger();
	info["h"] = getChild<LLUICtrl>(getHeightSpinnerName())->getValue().asInteger();
	LLFloaterSnapshot::getInstance()->notify(LLSD().with("custom-res-change", info));
}

void LLPanelSnapshotPostcard::onKeepAspectRatioCommit(LLUICtrl* ctrl)
{
	LLFloaterSnapshot::getInstance()->notify(LLSD().with("keep-aspect-change", ctrl->getValue().asBoolean()));
}

void LLPanelSnapshotPostcard::onQualitySliderCommit(LLUICtrl* ctrl)
{
	updateImageQualityLevel();

	LLSliderCtrl* slider = (LLSliderCtrl*)ctrl;
	S32 quality_val = llfloor((F32)slider->getValue().asReal());
	LLSD info;
	info["image-quality-change"] = quality_val;
	LLFloaterSnapshot::getInstance()->notify(info); // updates the "SnapshotQuality" setting
}

void LLPanelSnapshotPostcard::onTabButtonPress(S32 btn_idx)
{
	static LLButton* sButtons[2] = {
			getChild<LLButton>("message_btn"),
			getChild<LLButton>("settings_btn"),
	};

	// Switch between Message and Settings tabs.
	LLButton* clicked_btn = sButtons[btn_idx];
	LLButton* other_btn = sButtons[!btn_idx];
	LLSideTrayPanelContainer* container =
		getChild<LLSideTrayPanelContainer>("postcard_panel_container");

	container->selectTab(clicked_btn->getToggleState() ? btn_idx : !btn_idx);
	//clicked_btn->setEnabled(FALSE);
	other_btn->toggleState();
	//other_btn->setEnabled(TRUE);

	lldebugs << "Button #" << btn_idx << " (" << clicked_btn->getName() << ") clicked" << llendl;
}

void LLPanelSnapshotPostcard::onSend()
{
	// Validate input.
	std::string from(getChild<LLUICtrl>("from_form")->getValue().asString());
	std::string to(getChild<LLUICtrl>("to_form")->getValue().asString());

	boost::regex email_format("[A-Za-z0-9.%+-_]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}(,[ \t]*[A-Za-z0-9.%+-_]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,})*");

	if (to.empty() || !boost::regex_match(to, email_format))
	{
		LLNotificationsUtil::add("PromptRecipientEmail");
		return;
	}

	if (from.empty() || !boost::regex_match(from, email_format))
	{
		LLNotificationsUtil::add("PromptSelfEmail");
		return;
	}

	std::string subject(getChild<LLUICtrl>("subject_form")->getValue().asString());
	if(subject.empty() || !mHasFirstMsgFocus)
	{
		LLNotificationsUtil::add("PromptMissingSubjMsg", LLSD(), LLSD(), boost::bind(&LLPanelSnapshotPostcard::missingSubjMsgAlertCallback, this, _1, _2));
		return;
	}

	// Send postcard.
	sendPostcard();
}
