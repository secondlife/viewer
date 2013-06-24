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

#include "llagent.h"
#include "llagentui.h"
#include "llfacebookconnect.h"
#include "llfloaterreg.h"
#include "llslurl.h"
#include "llviewerregion.h"
#include "llviewercontrol.h"

static LLRegisterPanelClassWrapper<LLSocialStatusPanel> t_panel_status("llsocialstatuspanel");
static LLRegisterPanelClassWrapper<LLSocialPhotoPanel> t_panel_photo("llsocialphotopanel");
static LLRegisterPanelClassWrapper<LLSocialCheckinPanel> t_panel_checkin("llsocialcheckinpanel");

std::string get_map_url()
{
    LLVector3d center_agent;
    if (gAgent.getRegion())
    {
        center_agent = gAgent.getRegion()->getCenterGlobal();
    }
    int x_pos = center_agent[0] / 256.0;
    int y_pos = center_agent[1] / 256.0;
    std::string map_url = gSavedSettings.getString("CurrentMapServerURL") + llformat("map-1-%d-%d-objects.jpg", x_pos, y_pos);
    return map_url;
}

LLSocialStatusPanel::LLSocialStatusPanel()
{
	mCommitCallbackRegistrar.add("SocialSharing.SendStatus", boost::bind(&LLSocialStatusPanel::onSend, this));
}

void LLSocialStatusPanel::onSend()
{
	std::string message = getChild<LLUICtrl>("message")->getValue().asString();
	LLFacebookConnect::instance().updateStatus(message);
	
	LLFloater* floater = getParentByType<LLFloater>();
    if (floater)
    {
        floater->closeFloater();
    }
}

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


LLSocialCheckinPanel::LLSocialCheckinPanel() :
    mMapUrl("")
{
	mCommitCallbackRegistrar.add("SocialSharing.SendCheckin", boost::bind(&LLSocialCheckinPanel::onSend, this));
}

/*virtual*/
void LLSocialCheckinPanel::setVisible(BOOL visible)
{
    if (visible)
    {
        mMapUrl = get_map_url();
    }
    LLPanel::setVisible(visible);
}

void LLSocialCheckinPanel::onSend()
{
	// Get the location SLURL
	LLSLURL slurl;
	LLAgentUI::buildSLURL(slurl);
	std::string slurl_string = slurl.getSLURLString();
    
	// Get the region name
	std::string region_name = gAgent.getRegion()->getName();
    
	// Get the region description
	std::string description;
	LLAgentUI::buildLocationString(description, LLAgentUI::LOCATION_FORMAT_NORMAL_COORDS, gAgent.getPositionAgent());
    
	// Optionally add the region map view
	bool add_map_view = getChild<LLUICtrl>("add_place_view_cb")->getValue().asBoolean();
    std::string map_url = (add_map_view ? mMapUrl : "");
    
	// Get the caption
	std::string caption = getChild<LLUICtrl>("place_caption")->getValue().asString();

    // Post all that to Facebook
	LLFacebookConnect::instance().postCheckin(slurl_string, region_name, description, map_url, caption);
    
    // Close the floater once "Post" has been pushed
	LLFloater* floater = getParentByType<LLFloater>();
    if (floater)
    {
        floater->closeFloater();
    }
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
