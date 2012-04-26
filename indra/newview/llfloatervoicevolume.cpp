/** 
 * @file llfloatervoicevolume.cpp
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "llfloatervoicevolume.h"

// Linden libraries
#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "lltextbox.h"

// viewer files
#include "llagent.h"
#include "llavataractions.h"
#include "llinspect.h"
#include "lltransientfloatermgr.h"
#include "llvoiceclient.h"

class LLAvatarName;

//////////////////////////////////////////////////////////////////////////////
// LLFloaterVoiceVolume
//////////////////////////////////////////////////////////////////////////////

// Avatar Inspector, a small information window used when clicking
// on avatar names in the 2D UI and in the ambient inspector widget for
// the 3D world.
class LLFloaterVoiceVolume : public LLInspect, LLTransientFloater
{
	friend class LLFloaterReg;
	
public:
	// avatar_id - Avatar ID for which to show information
	// Inspector will be positioned relative to current mouse position
	LLFloaterVoiceVolume(const LLSD& avatar_id);
	virtual ~LLFloaterVoiceVolume();
	
	/*virtual*/ BOOL postBuild(void);
	
	// Because floater is single instance, need to re-parse data on each spawn
	// (for example, inspector about same avatar but in different position)
	/*virtual*/ void onOpen(const LLSD& avatar_id);

	/*virtual*/ LLTransientFloaterMgr::ETransientGroup getGroup() { return LLTransientFloaterMgr::GLOBAL; }

private:
	// Set the volume slider to this user's current client-side volume setting,
	// hiding/disabling if the user is not nearby.
	void updateVolumeControls();

	void onClickMuteVolume();
	void onVolumeChange(const LLSD& data);
	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);
	
private:
	LLUUID				mAvatarID;
	// Need avatar name information to spawn friend add request
	LLAvatarName		mAvatarName;
};

LLFloaterVoiceVolume::LLFloaterVoiceVolume(const LLSD& sd)
:	LLInspect(LLSD())		// single_instance, doesn't really need key
,	mAvatarID()				// set in onOpen()  *Note: we used to show partner's name but we dont anymore --angela 3rd Dec*
,	mAvatarName()
{
	LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::GLOBAL, this);
	LLTransientFloater::init(this);
}

LLFloaterVoiceVolume::~LLFloaterVoiceVolume()
{
	LLTransientFloaterMgr::getInstance()->removeControlView(this);
}

/*virtual*/
BOOL LLFloaterVoiceVolume::postBuild(void)
{
	getChild<LLUICtrl>("mute_btn")->setCommitCallback(
		boost::bind(&LLFloaterVoiceVolume::onClickMuteVolume, this) );

	getChild<LLUICtrl>("volume_slider")->setCommitCallback(
		boost::bind(&LLFloaterVoiceVolume::onVolumeChange, this, _2));

	return TRUE;
}


// Multiple calls to showInstance("floater_voice_volume", foo) will provide different
// LLSD for foo, which we will catch here.
//virtual
void LLFloaterVoiceVolume::onOpen(const LLSD& data)
{
	// Start open animation
	LLInspect::onOpen(data);

	// Extract appropriate avatar id
	mAvatarID = data["avatar_id"];

	LLUI::positionViewNearMouse(this);

	getChild<LLUICtrl>("avatar_name")->setValue("");
	updateVolumeControls();

	LLAvatarNameCache::get(mAvatarID,
		boost::bind(&LLFloaterVoiceVolume::onAvatarNameCache, this, _1, _2));
}

void LLFloaterVoiceVolume::updateVolumeControls()
{
	bool voice_enabled = LLVoiceClient::getInstance()->getVoiceEnabled(mAvatarID);

	LLUICtrl* mute_btn = getChild<LLUICtrl>("mute_btn");
	LLUICtrl* volume_slider = getChild<LLUICtrl>("volume_slider");

	// Do not display volume slider and mute button if it 
	// is ourself or we are not in a voice channel together
	if (!voice_enabled || (mAvatarID == gAgent.getID()))
	{
		mute_btn->setVisible(false);
		volume_slider->setVisible(false);
	}
	else 
	{
		mute_btn->setVisible(true);
		volume_slider->setVisible(true);

		// By convention, we only display and toggle voice mutes, not all mutes
		bool is_muted = LLAvatarActions::isVoiceMuted(mAvatarID);
		bool is_linden = LLStringUtil::endsWith(mAvatarName.getLegacyName(), " Linden");

		mute_btn->setEnabled(!is_linden);
		mute_btn->setValue(is_muted);

		volume_slider->setEnabled(!is_muted);

		F32 volume;
		if (is_muted)
		{
			// it's clearer to display their volume as zero
			volume = 0.f;
		}
		else
		{
			// actual volume
			volume = LLVoiceClient::getInstance()->getUserVolume(mAvatarID);
		}
		volume_slider->setValue((F64)volume);
	}

}

void LLFloaterVoiceVolume::onClickMuteVolume()
{
	LLAvatarActions::toggleMuteVoice(mAvatarID);
	updateVolumeControls();
}

void LLFloaterVoiceVolume::onVolumeChange(const LLSD& data)
{
	F32 volume = (F32)data.asReal();
	LLVoiceClient::getInstance()->setUserVolume(mAvatarID, volume);
}

void LLFloaterVoiceVolume::onAvatarNameCache(
		const LLUUID& agent_id,
		const LLAvatarName& av_name)
{
	if (agent_id != mAvatarID)
	{
		return;
	}

	getChild<LLUICtrl>("avatar_name")->setValue(av_name.getCompleteName());
	mAvatarName = av_name;
}

//////////////////////////////////////////////////////////////////////////////
// LLFloaterVoiceVolumeUtil
//////////////////////////////////////////////////////////////////////////////
void LLFloaterVoiceVolumeUtil::registerFloater()
{
	LLFloaterReg::add("floater_voice_volume", "floater_voice_volume.xml",
					  &LLFloaterReg::build<LLFloaterVoiceVolume>);
}
