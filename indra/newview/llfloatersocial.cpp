/** 
* @file llfloatersocial.cpp
* @brief Implementation of llfloatersocial
* @author Gilbert@lindenlab.com
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

#include "llfloatersocial.h"

#include "llagentui.h"
#include "llfloaterreg.h"
#include "llslurl.h"

static LLRegisterPanelClassWrapper<LLSocialPhotoPanel> panel_class("llsocialphotopanel");

LLSocialPhotoPanel::LLSocialPhotoPanel()
{
	mCommitCallbackRegistrar.add("PostToFacebook.Send", boost::bind(&LLSocialPhotoPanel::onSend, this));
}

void LLSocialPhotoPanel::onSend()
{
	std::string caption = getChild<LLUICtrl>("caption")->getValue().asString();
	bool add_location = getChild<LLUICtrl>("add_location_cb")->getValue().asBoolean();

	if (add_location)
	{
		LLSLURL slurl;
		LLAgentUI::buildSLURL(slurl);
		if (caption.empty())
			caption = slurl.getSLURLString();
		else
			caption = caption + " " + slurl.getSLURLString();
	}
	//LLFacebookConnect::instance().sharePhoto(LLFloaterSnapshot::getImageData(), caption);
	//LLWebProfile::uploadImage(LLFloaterSnapshot::getImageData(), caption, add_location, boost::bind(&LLPanelSnapshotFacebook::onImageUploaded, this, caption, _1));
	//LLFloaterSnapshot::postSave();
}



LLFloaterSocial::LLFloaterSocial(const LLSD& key) : LLFloater(key)
{
	mCommitCallbackRegistrar.add("SocialSharing.Cancel", boost::bind(&LLFloaterSocial::onCancel, this));
}

void LLFloaterSocial::onCancel()
{
    closeFloater();
}

BOOL LLFloaterSocial::postBuild()
{
	return LLFloater::postBuild();
}
