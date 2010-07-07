/** 
 * @file llinspectremoteobject.cpp
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

#include "llfloaterreg.h"
#include "llinspectremoteobject.h"
#include "llinspect.h"
#include "llmutelist.h"
#include "llpanelblockedlist.h"
#include "llslurl.h"
#include "lltrans.h"
#include "llui.h"
#include "lluictrl.h"
#include "llurlaction.h"

//////////////////////////////////////////////////////////////////////////////
// LLInspectRemoteObject
//////////////////////////////////////////////////////////////////////////////

// Remote Object Inspector, a small information window used to
// display information about potentially-remote objects. Used
// to display details about objects sending messages to the user.
class LLInspectRemoteObject : public LLInspect
{
	friend class LLFloaterReg;
	
public:
	LLInspectRemoteObject(const LLSD& object_id);
	virtual ~LLInspectRemoteObject() {};

	/*virtual*/ BOOL postBuild(void);
	/*virtual*/ void onOpen(const LLSD& avatar_id);

	void onClickMap();
	void onClickBlock();
	void onClickClose();
	
private:
	void update();
	void onNameCache(const LLUUID& id, const std::string& name, bool is_group);
	
private:
	LLUUID		 mObjectID;
	LLUUID		 mOwnerID;
	std::string  mOwnerLegacyName;
	std::string  mSLurl;
	std::string  mName;
	bool         mGroupOwned;
};

LLInspectRemoteObject::LLInspectRemoteObject(const LLSD& sd) :
	LLInspect(LLSD()),
	mObjectID(NULL),
	mOwnerID(NULL),
	mOwnerLegacyName(),
	mSLurl(""),
	mName(""),
	mGroupOwned(false)
{
}

/*virtual*/
BOOL LLInspectRemoteObject::postBuild(void)
{
	// hook up the inspector's buttons
	getChild<LLUICtrl>("map_btn")->setCommitCallback(
		boost::bind(&LLInspectRemoteObject::onClickMap, this));
	getChild<LLUICtrl>("block_btn")->setCommitCallback(
		boost::bind(&LLInspectRemoteObject::onClickBlock, this));
	getChild<LLUICtrl>("close_btn")->setCommitCallback(
		boost::bind(&LLInspectRemoteObject::onClickClose, this));

	return TRUE;
}

/*virtual*/
void LLInspectRemoteObject::onOpen(const LLSD& data)
{
	// Start animation
	LLInspect::onOpen(data);

	// Extract appropriate object information from input LLSD
	// (Eventually, it might be nice to query server for details
	// rather than require caller to pass in the information.)
	mObjectID   = data["object_id"].asUUID();
	mName       = data["name"].asString();
	mOwnerID    = data["owner_id"].asUUID();
	mGroupOwned = data["group_owned"].asBoolean();
	mSLurl      = data["slurl"].asString();

	// work out the owner's name
	mOwnerLegacyName = "";
	if (gCacheName)
	{
		gCacheName->get(mOwnerID, mGroupOwned,  // muting
			boost::bind(&LLInspectRemoteObject::onNameCache, this, _1, _2, _3));
	}

	// update the inspector with the current object state
	update();

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
}

void LLInspectRemoteObject::onClickMap()
{
	std::string url = "secondlife://" + mSLurl;
	LLUrlAction::showLocationOnMap(url);
	closeFloater();
}

void LLInspectRemoteObject::onClickBlock()
{
	LLMute::EType mute_type = mGroupOwned ? LLMute::GROUP : LLMute::AGENT;
	LLMute mute(mOwnerID, mOwnerLegacyName, mute_type);
	LLMuteList::getInstance()->add(mute);
	LLPanelBlockedList::showPanelAndSelect(mute.mID);
	closeFloater();
}

void LLInspectRemoteObject::onClickClose()
{
	closeFloater();
}

void LLInspectRemoteObject::onNameCache(const LLUUID& id, const std::string& name, bool is_group)
{
	mOwnerLegacyName = name;
	update();
}

void LLInspectRemoteObject::update()
{
	// show the object name as the inspector's title
	// (don't hyperlink URLs in object names)
	getChild<LLUICtrl>("object_name")->setValue("<nolink>" + mName + "</nolink>");

	// show the object's owner - click it to show profile
	std::string owner;
	if (! mOwnerID.isNull())
	{
		if (mGroupOwned)
		{
			owner = LLSLURL("group", mOwnerID, "about").getSLURLString();
		}
		else
		{
			owner = LLSLURL("agent", mOwnerID, "about").getSLURLString();
		}
	}
	else
	{
		owner = LLTrans::getString("Unknown");
	}
	getChild<LLUICtrl>("object_owner")->setValue(owner);

	// display the object's SLurl - click it to teleport
	std::string url;
	if (! mSLurl.empty())
	{
		url = "secondlife:///app/teleport/" + mSLurl;
	}
	getChild<LLUICtrl>("object_slurl")->setValue(url);

	// disable the Map button if we don't have a SLurl
	getChild<LLUICtrl>("map_btn")->setEnabled(! mSLurl.empty());

	// disable the Block button if we don't have the owner ID
	getChild<LLUICtrl>("block_btn")->setEnabled(! mOwnerID.isNull());
}

//////////////////////////////////////////////////////////////////////////////
// LLInspectRemoteObjectUtil
//////////////////////////////////////////////////////////////////////////////
void LLInspectRemoteObjectUtil::registerFloater()
{
	LLFloaterReg::add("inspect_remote_object", "inspect_remote_object.xml",
					  &LLFloaterReg::build<LLInspectRemoteObject>);
}
