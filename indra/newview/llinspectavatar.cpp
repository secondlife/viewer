/** 
 * @file llinspectavatar.cpp
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llinspectavatar.h"

// viewer files
#include "llagentdata.h"
#include "llavataractions.h"
#include "llavatarpropertiesprocessor.h"
#include "llcallingcard.h"

// linden libraries
#include "lltooltip.h"	// positionViewNearMouse()
#include "lluictrl.h"

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
:	LLFloater( LLSD() ),	// single_instance, doesn't really need key
	mAvatarID(),			// set in onOpen()
	mFirstName(),
	mLastName(),
	mPropertiesRequest(NULL)
{
	// can't make the properties request until the widgets are constructed
	// as it might return immediately, so do it in postBuild.
}

LLInspectAvatar::~LLInspectAvatar()
{
	// clean up any pending requests so they don't call back into a deleted
	// view
	delete mPropertiesRequest;
	mPropertiesRequest = NULL;
}

/*virtual*/
BOOL LLInspectAvatar::postBuild(void)
{
	getChild<LLUICtrl>("add_friend_btn")->setCommitCallback(
		boost::bind(&LLInspectAvatar::onClickAddFriend, this) );

	getChild<LLUICtrl>("view_profile_btn")->setCommitCallback(
		boost::bind(&LLInspectAvatar::onClickViewProfile, this) );

	return TRUE;
}

void LLInspectAvatar::draw()
{
	static LLCachedControl<F32> FADE_OUT_TIME(*LLUI::sSettingGroups["config"], "InspectorFadeTime", 1.f);
	if (mCloseTimer.getStarted())
	{
		F32 alpha = clamp_rescale(mCloseTimer.getElapsedTimeF32(), 0.f, FADE_OUT_TIME, 1.f, 0.f);
		LLViewDrawContext context(alpha);
		LLFloater::draw();
		if (mCloseTimer.getElapsedTimeF32() > FADE_OUT_TIME)
		{
			closeFloater(false);
		}
	}
	else
	{
		LLFloater::draw();
	}
}


// Multiple calls to showInstance("inspect_avatar", foo) will provide different
// LLSD for foo, which we will catch here.
//virtual
void LLInspectAvatar::onOpen(const LLSD& data)
{
	mCloseTimer.stop();

	// Extract appropriate avatar id
	mAvatarID = data.isUUID() ? data : data["avatar_id"];

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

	// can't call from constructor as widgets are not built yet
	requestUpdate();
}

//virtual
void LLInspectAvatar::onFocusLost()
{
	// Start closing when we lose focus
	mCloseTimer.start();
}

void LLInspectAvatar::requestUpdate()
{
	// Don't make network requests when spawning from the debug menu at the
	// login screen (which is useful to work on the layout).
	if (mAvatarID.isNull())
	{
		getChild<LLUICtrl>("user_subtitle")->
			setValue("Test subtitle");
		getChild<LLUICtrl>("user_details")->
			setValue("Test details\nTest line 2");
		return;
	}

	// Clear out old data so it doesn't flash between old and new
	getChild<LLUICtrl>("user_name")->setValue("");
	getChild<LLUICtrl>("user_subtitle")->setValue("");
	getChild<LLUICtrl>("user_details")->setValue("");
	
	// Make a new request for properties
	delete mPropertiesRequest;
	mPropertiesRequest = new LLFetchAvatarData(mAvatarID, this);

	// You can't re-add someone as a friend if they are already your friend
	bool is_friend = LLAvatarTracker::instance().getBuddyInfo(mAvatarID) != NULL;
	bool is_self = (mAvatarID == gAgentID);
	childSetEnabled("add_friend_btn", !is_friend && !is_self);

	// Use an avatar_icon even though the image id will come down with the
	// avatar properties because the avatar_icon code maintains a cache of icons
	// and this may result in the image being visible sooner.
	// *NOTE: This may generate a duplicate avatar properties request, but that
	// will be suppressed internally in the avatar properties processor.
	childSetValue("avatar_icon", LLSD(mAvatarID) );

	gCacheName->get(mAvatarID, FALSE,
		boost::bind(&LLInspectAvatar::nameUpdatedCallback,
			this, _1, _2, _3, _4));
}

void LLInspectAvatar::processAvatarData(LLAvatarData* data)
{
	LLStringUtil::format_map_t args;
	args["[BORN_ON]"] = data->born_on;
	args["[AGE]"] = LLAvatarPropertiesProcessor::ageFromDate(data->born_on);
	args["[SL_PROFILE]"] = data->about_text;
	args["[RW_PROFILE"] = data->fl_about_text;
	args["[ACCTTYPE]"] = LLAvatarPropertiesProcessor::accountType(data);
	args["[PAYMENTINFO]"] = LLAvatarPropertiesProcessor::paymentInfo(data);

	std::string subtitle = getString("Subtitle", args);
	getChild<LLUICtrl>("user_subtitle")->setValue( LLSD(subtitle) );
	std::string details = getString("Details", args);
	getChild<LLUICtrl>("user_details")->setValue( LLSD(details) );

	// Delete the request object as it has been satisfied
	delete mPropertiesRequest;
	mPropertiesRequest = NULL;
}

void LLInspectAvatar::nameUpdatedCallback(
	const LLUUID& id,
	const std::string& first,
	const std::string& last,
	BOOL is_group)
{
	// Possibly a request for an older inspector
	if (id != mAvatarID) return;

	mFirstName = first;
	mLastName = last;
	std::string name = first + " " + last;

	childSetValue("user_name", LLSD(name) );
}

void LLInspectAvatar::onClickAddFriend()
{
	std::string name;
	name.assign(mFirstName);
	name.append(" ");
	name.append(mLastName);

	LLAvatarActions::requestFriendshipDialog(mAvatarID, name);
}

void LLInspectAvatar::onClickViewProfile()
{
	LLAvatarActions::showProfile(mAvatarID);
}
