/** 
 * @file llinspectavatar.cpp
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llinspectavatar.h"

// viewer files
#include "llavatariconctrl.h"
#include "llavatarnamecache.h"
#include "llavatarpropertiesprocessor.h"
#include "lldateutil.h"
#include "llinspect.h"
#include "llslurl.h"
#include "llstartup.h"
#include "lltransientfloatermgr.h"

// Linden libraries
#include "llfloater.h"
#include "llfloaterreg.h"
#include "lltextbox.h"
#include "lltooltip.h"	// positionViewNearMouse()
#include "lltrans.h"

class LLFetchAvatarData;


//////////////////////////////////////////////////////////////////////////////
// LLInspectAvatar
//////////////////////////////////////////////////////////////////////////////

// Avatar Inspector, a small information window used when clicking
// on avatar names in the 2D UI and in the ambient inspector widget for
// the 3D world.
class LLInspectAvatar : public LLInspect, LLTransientFloater
{
	friend class LLFloaterReg;
	
public:
	// avatar_id - Avatar ID for which to show information
	// Inspector will be positioned relative to current mouse position
	LLInspectAvatar(const LLSD& avatar_id);
	virtual ~LLInspectAvatar();
	
	// Because floater is single instance, need to re-parse data on each spawn
	// (for example, inspector about same avatar but in different position)
	/*virtual*/ void onOpen(const LLSD& avatar_id);

	// Update view based on information from avatar properties processor
	void processAvatarData(LLAvatarData* data);
	
	virtual LLTransientFloaterMgr::ETransientGroup getGroup() { return LLTransientFloaterMgr::GLOBAL; }

private:
	// Make network requests for all the data to display in this view.
	// Used on construction and if avatar id changes.
	void requestUpdate();
	
	void onAvatarNameCache(const LLUUID& agent_id,
						   const LLAvatarName& av_name);
	
private:
	LLUUID				mAvatarID;
	// Need avatar name information to spawn friend add request
	LLAvatarName		mAvatarName;
	// an in-flight request for avatar properties from LLAvatarPropertiesProcessor
	// is represented by this object
	LLFetchAvatarData*	mPropertiesRequest;
};

//////////////////////////////////////////////////////////////////////////////
// LLFetchAvatarData
//////////////////////////////////////////////////////////////////////////////

// This object represents a pending request for avatar properties information
class LLFetchAvatarData : public LLAvatarPropertiesObserver
{
public:
	// If the inspector closes it will delete the pending request object, so the
	// inspector pointer will be valid for the lifetime of this object
	LLFetchAvatarData(const LLUUID& avatar_id, LLInspectAvatar* inspector)
	:	mAvatarID(avatar_id),
		mInspector(inspector)
	{
		LLAvatarPropertiesProcessor* processor = 
			LLAvatarPropertiesProcessor::getInstance();
		// register ourselves as an observer
		processor->addObserver(mAvatarID, this);
		// send a request (duplicates will be suppressed inside the avatar
		// properties processor)
		processor->sendAvatarPropertiesRequest(mAvatarID);
	}
	
	~LLFetchAvatarData()
	{
		// remove ourselves as an observer
		LLAvatarPropertiesProcessor::getInstance()->
		removeObserver(mAvatarID, this);
	}
	
	void processProperties(void* data, EAvatarProcessorType type)
	{
		// route the data to the inspector
		if (data
			&& type == APT_PROPERTIES)
		{
			LLAvatarData* avatar_data = static_cast<LLAvatarData*>(data);
			mInspector->processAvatarData(avatar_data);
		}
	}
	
	// Store avatar ID so we can un-register the observer on destruction
	LLUUID mAvatarID;
	LLInspectAvatar* mInspector;
};

LLInspectAvatar::LLInspectAvatar(const LLSD& sd)
:	LLInspect( LLSD() ),	// single_instance, doesn't really need key
	mAvatarID(),			// set in onOpen()  *Note: we used to show partner's name but we dont anymore --angela 3rd Dec* 
	mAvatarName(),
	mPropertiesRequest(NULL)
{
	// can't make the properties request until the widgets are constructed
	// as it might return immediately, so do it in onOpen.

	LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::GLOBAL, this);
	LLTransientFloater::init(this);
}

LLInspectAvatar::~LLInspectAvatar()
{
	// clean up any pending requests so they don't call back into a deleted
	// view
	delete mPropertiesRequest;
	mPropertiesRequest = NULL;

	LLTransientFloaterMgr::getInstance()->removeControlView(this);
}

// Multiple calls to showInstance("inspect_avatar", foo) will provide different
// LLSD for foo, which we will catch here.
//virtual
void LLInspectAvatar::onOpen(const LLSD& data)
{
	// Start open animation
	LLInspect::onOpen(data);

	// Extract appropriate avatar id
	mAvatarID = data["avatar_id"];

	// Position the inspector relative to the mouse cursor
	// Similar to how tooltips are positioned
	// See LLToolTipMgr::createToolTip
	if (data.has("pos"))
	{
		LLUI::positionViewNearMouse(this, data["pos"]["x"].asInteger(), data["pos"]["y"].asInteger());
	}
	else
	{
		LLUI::positionViewNearMouse(this);
	}

	// Generate link to avatar profile.
	getChild<LLUICtrl>("avatar_profile_link")->setTextArg("[LINK]", LLSLURL("agent", mAvatarID, "about").getSLURLString());

	// can't call from constructor as widgets are not built yet
	requestUpdate();
}

void LLInspectAvatar::requestUpdate()
{
	// Don't make network requests when spawning from the debug menu at the
	// login screen (which is useful to work on the layout).
	if (mAvatarID.isNull())
	{
		if (LLStartUp::getStartupState() >= STATE_STARTED)
		{
			// once we're running we don't want to show the test floater
			// for bogus LLUUID::null links
			closeFloater();
		}
		return;
	}

	// Clear out old data so it doesn't flash between old and new
	getChild<LLUICtrl>("user_name")->setValue("");
	getChild<LLUICtrl>("user_name_small")->setValue("");
	getChild<LLUICtrl>("user_slid")->setValue("");
	getChild<LLUICtrl>("user_subtitle")->setValue("");
	getChild<LLUICtrl>("user_details")->setValue("");
	
	// Make a new request for properties
	delete mPropertiesRequest;
	mPropertiesRequest = new LLFetchAvatarData(mAvatarID, this);

	// Use an avatar_icon even though the image id will come down with the
	// avatar properties because the avatar_icon code maintains a cache of icons
	// and this may result in the image being visible sooner.
	// *NOTE: This may generate a duplicate avatar properties request, but that
	// will be suppressed internally in the avatar properties processor.
	
	//remove avatar id from cache to get fresh info
	LLAvatarIconIDCache::getInstance()->remove(mAvatarID);

	getChild<LLUICtrl>("avatar_icon")->setValue(LLSD(mAvatarID) );

	LLAvatarNameCache::get(mAvatarID,
			boost::bind(&LLInspectAvatar::onAvatarNameCache,
				this, _1, _2));
}

void LLInspectAvatar::processAvatarData(LLAvatarData* data)
{
	LLStringUtil::format_map_t args;
	{
		std::string birth_date = LLTrans::getString("AvatarBirthDateFormat");
		LLStringUtil::format(birth_date, LLSD().with("datetime", (S32) data->born_on.secondsSinceEpoch()));
		args["[BORN_ON]"] = birth_date;
	}
	args["[AGE]"] = LLDateUtil::ageFromDate(data->born_on, LLDate::now());
	args["[SL_PROFILE]"] = data->about_text;
	args["[RW_PROFILE"] = data->fl_about_text;
	args["[ACCTTYPE]"] = LLAvatarPropertiesProcessor::accountType(data);
	std::string payment_info = LLAvatarPropertiesProcessor::paymentInfo(data);
	args["[PAYMENTINFO]"] = payment_info;
	args["[COMMA]"] = (payment_info.empty() ? "" : ",");

	std::string subtitle = getString("Subtitle", args);
	getChild<LLUICtrl>("user_subtitle")->setValue( LLSD(subtitle) );
	std::string details = getString("Details", args);
	getChild<LLUICtrl>("user_details")->setValue( LLSD(details) );

	// Delete the request object as it has been satisfied
	delete mPropertiesRequest;
	mPropertiesRequest = NULL;
}

void LLInspectAvatar::onAvatarNameCache(
		const LLUUID& agent_id,
		const LLAvatarName& av_name)
{
	if (agent_id == mAvatarID)
	{
		getChild<LLUICtrl>("user_name")->setValue(av_name.mDisplayName);
		getChild<LLUICtrl>("user_name_small")->setValue(av_name.mDisplayName);
		getChild<LLUICtrl>("user_slid")->setValue(av_name.mUsername);
		mAvatarName = av_name;
		
		// show smaller display name if too long to display in regular size
		if (getChild<LLTextBox>("user_name")->getTextPixelWidth() > getChild<LLTextBox>("user_name")->getRect().getWidth())
		{
			getChild<LLUICtrl>("user_name_small")->setVisible( true );
			getChild<LLUICtrl>("user_name")->setVisible( false );
		}
		else
		{
			getChild<LLUICtrl>("user_name_small")->setVisible( false );
			getChild<LLUICtrl>("user_name")->setVisible( true );

		}

	}
}

//////////////////////////////////////////////////////////////////////////////
// LLInspectAvatarUtil
//////////////////////////////////////////////////////////////////////////////
void LLInspectAvatarUtil::registerFloater()
{
	LLFloaterReg::add("inspect_avatar", "inspect_avatar.xml",
					  &LLFloaterReg::build<LLInspectAvatar>);
}
