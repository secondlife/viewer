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
#include "llregex.h"
#include "llsnapshotlivepreview.h"
#include "llviewercontrol.h" // gSavedSettings
#include "llviewerwindow.h"
#include "llviewerregion.h"

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
	/*virtual*/ bool postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

private:
	/*virtual*/ std::string getWidthSpinnerName() const		{ return "postcard_snapshot_width"; }
	/*virtual*/ std::string getHeightSpinnerName() const	{ return "postcard_snapshot_height"; }
	/*virtual*/ std::string getAspectRatioCBName() const	{ return "postcard_keep_aspect_check"; }
	/*virtual*/ std::string getImageSizeComboName() const	{ return "postcard_size_combo"; }
	/*virtual*/ std::string getImageSizePanelName() const	{ return "postcard_image_size_lp"; }
	/*virtual*/ LLSnapshotModel::ESnapshotFormat getImageFormat() const { return LLSnapshotModel::SNAPSHOT_FORMAT_JPEG; }
	/*virtual*/ LLSnapshotModel::ESnapshotType getSnapshotType();
	/*virtual*/ void updateControls(const LLSD& info);

	bool missingSubjMsgAlertCallback(const LLSD& notification, const LLSD& response);
	static void sendPostcardFinished(LLSD result);
	void sendPostcard();

	void onMsgFormFocusRecieved();
	void onFormatComboCommit(LLUICtrl* ctrl);
	void onQualitySliderCommit(LLUICtrl* ctrl);
	void onSend();

	bool mHasFirstMsgFocus;
};

static LLPanelInjector<LLPanelSnapshotPostcard> panel_class("llpanelsnapshotpostcard");

LLPanelSnapshotPostcard::LLPanelSnapshotPostcard()
:	mHasFirstMsgFocus(false)
{
	mCommitCallbackRegistrar.add("Postcard.Send",		boost::bind(&LLPanelSnapshotPostcard::onSend,	this));
	mCommitCallbackRegistrar.add("Postcard.Cancel",		boost::bind(&LLPanelSnapshotPostcard::cancel,	this));

}

// virtual
bool LLPanelSnapshotPostcard::postBuild()
{
	// For the first time a user focuses to .the msg box, all text will be selected.
	getChild<LLUICtrl>("msg_form")->setFocusChangedCallback(boost::bind(&LLPanelSnapshotPostcard::onMsgFormFocusRecieved, this));

	getChild<LLUICtrl>("to_form")->setFocus(true);

	getChild<LLUICtrl>("image_quality_slider")->setCommitCallback(boost::bind(&LLPanelSnapshotPostcard::onQualitySliderCommit, this, _1));

	return LLPanelSnapshot::postBuild();
}

// virtual
void LLPanelSnapshotPostcard::onOpen(const LLSD& key)
{
	LLUICtrl* name_form = getChild<LLUICtrl>("name_form");
	if (name_form && name_form->getValue().asString().empty())
	{
		std::string name_string;
		LLAgentUI::buildFullname(name_string);
		getChild<LLUICtrl>("name_form")->setValue(LLSD(name_string));
	}

	LLPanelSnapshot::onOpen(key);
}

// virtual
void LLPanelSnapshotPostcard::updateControls(const LLSD& info)
{
	getChild<LLUICtrl>("image_quality_slider")->setValue(gSavedSettings.getS32("SnapshotQuality"));
	updateImageQualityLevel();

	const bool have_snapshot = info.has("have-snapshot") ? info["have-snapshot"].asBoolean() : true;
	getChild<LLUICtrl>("send_btn")->setEnabled(have_snapshot);
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


void LLPanelSnapshotPostcard::sendPostcardFinished(LLSD result)
{
    LL_WARNS() << result << LL_ENDL;

    std::string state = result["state"].asString();

    LLPostCard::reportPostResult((state == "complete"));
}


void LLPanelSnapshotPostcard::sendPostcard()
{
    if (!gAgent.getRegion()) return;

    // upload the image
    std::string url = gAgent.getRegion()->getCapability("SendPostcard");
    if (!url.empty())
    {
        LLResourceUploadInfo::ptr_t uploadInfo(std::make_shared<LLPostcardUploadInfo>(
            getChild<LLUICtrl>("name_form")->getValue().asString(),
            getChild<LLUICtrl>("to_form")->getValue().asString(),
            getChild<LLUICtrl>("subject_form")->getValue().asString(),
            getChild<LLUICtrl>("msg_form")->getValue().asString(),
            mSnapshotFloater->getPosTakenGlobal(),
            mSnapshotFloater->getImageData(),
            [](LLUUID, LLUUID, LLUUID, LLSD response) {
                LLPanelSnapshotPostcard::sendPostcardFinished(response);
            }));

        LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
    }
    else
    {
        LL_WARNS() << "Postcards unavailable in this region." << LL_ENDL;
    }


    // Give user feedback of the event.
    gViewerWindow->playSnapshotAnimAndSound();

    mSnapshotFloater->postSave();
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

void LLPanelSnapshotPostcard::onQualitySliderCommit(LLUICtrl* ctrl)
{
	updateImageQualityLevel();

	LLSliderCtrl* slider = (LLSliderCtrl*)ctrl;
	S32 quality_val = llfloor((F32)slider->getValue().asReal());
	LLSD info;
	info["image-quality-change"] = quality_val;
	LLFloaterSnapshot::getInstance()->notify(info); // updates the "SnapshotQuality" setting
}

void LLPanelSnapshotPostcard::onSend()
{
	// Validate input.
	std::string to(getChild<LLUICtrl>("to_form")->getValue().asString());

	boost::regex email_format("[A-Za-z0-9.%+-_]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}(,[ \t]*[A-Za-z0-9.%+-_]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,})*");

	if (to.empty() || !ll_regex_match(to, email_format))
	{
		LLNotificationsUtil::add("PromptRecipientEmail");
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

LLSnapshotModel::ESnapshotType LLPanelSnapshotPostcard::getSnapshotType()
{
    return LLSnapshotModel::SNAPSHOT_POSTCARD;
}
