/** 
 * @file llpanelsnapshotfacebook.cpp
 * @brief Posts a snapshot to the resident Facebook account.
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

// libs
#include "llcombobox.h"
#include "llfloaterreg.h"
#include "llpanel.h"
#include "llspinctrl.h"

// newview
#include "llfloatersnapshot.h"
#include "llpanelsnapshot.h"
#include "llsidetraypanelcontainer.h"
#include "llwebprofile.h"

#include "llfacebookconnect.h"

/**
 * Posts a snapshot to the resident Facebook account.
 */
class LLPanelSnapshotFacebook
:	public LLPanelSnapshot
{
	LOG_CLASS(LLPanelSnapshotFacebook);

public:
	LLPanelSnapshotFacebook();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

private:
	/*virtual*/ std::string getWidthSpinnerName() const		{ return "facebook_snapshot_width"; }
	/*virtual*/ std::string getHeightSpinnerName() const	{ return "facebook_snapshot_height"; }
	/*virtual*/ std::string getAspectRatioCBName() const	{ return "facebook_keep_aspect_check"; }
	/*virtual*/ std::string getImageSizeComboName() const	{ return "facebook_size_combo"; }
	/*virtual*/ std::string getImageSizePanelName() const	{ return "facebook_image_size_lp"; }
	/*virtual*/ LLFloaterSnapshot::ESnapshotFormat getImageFormat() const { return LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG; }
	/*virtual*/ void updateControls(const LLSD& info);

	void onSend();
	void onImageUploaded(const std::string& caption, const std::string& image_url);
};

static LLRegisterPanelClassWrapper<LLPanelSnapshotFacebook> panel_class("llpanelsnapshotfacebook");

LLPanelSnapshotFacebook::LLPanelSnapshotFacebook()
{
	mCommitCallbackRegistrar.add("PostToFacebook.Send",		boost::bind(&LLPanelSnapshotFacebook::onSend,		this));
	mCommitCallbackRegistrar.add("PostToFacebook.Cancel",	boost::bind(&LLPanelSnapshotFacebook::cancel,		this));
}

// virtual
BOOL LLPanelSnapshotFacebook::postBuild()
{
	return LLPanelSnapshot::postBuild();
}

// virtual
void LLPanelSnapshotFacebook::onOpen(const LLSD& key)
{
	LLPanelSnapshot::onOpen(key);
}

// virtual
void LLPanelSnapshotFacebook::updateControls(const LLSD& info)
{
	const bool have_snapshot = info.has("have-snapshot") ? info["have-snapshot"].asBoolean() : true;
	getChild<LLUICtrl>("post_btn")->setEnabled(have_snapshot);
}

void LLPanelSnapshotFacebook::onSend()
{
	std::string caption = getChild<LLUICtrl>("caption")->getValue().asString();
	bool add_location = getChild<LLUICtrl>("add_location_cb")->getValue().asBoolean();

	LLWebProfile::uploadImage(LLFloaterSnapshot::getImageData(), caption, add_location, boost::bind(&LLPanelSnapshotFacebook::onImageUploaded, this, caption, _1));
	LLFloaterSnapshot::postSave();

	// test with a placeholder image, until we can figure out a way to grab the uploaded image url
	LLFacebookConnect::instance().sharePhoto("http://fc02.deviantart.net/fs43/i/2009/125/a/9/Future_of_Frog_by_axcho.jpg", caption);
}

void LLPanelSnapshotFacebook::onImageUploaded(const std::string& caption, const std::string& image_url)
{
	if (!image_url.empty())
	{
		LLFacebookConnect::instance().sharePhoto(image_url, caption);
	}
}
