/** 
 * @file llinspectgroup.cpp
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

#include "llinspectgroup.h"

// viewer files
#include "llgroupactions.h"
#include "llgroupmgr.h"
#include "llinspect.h"
#include "llstartup.h"

// Linden libraries
#include "llcontrol.h"	// LLCachedControl
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llresmgr.h"	// getMonetaryString()
#include "lltooltip.h"	// positionViewNearMouse()
#include "lltrans.h"
#include "lluictrl.h"

class LLFetchGroupData;


//////////////////////////////////////////////////////////////////////////////
// LLInspectGroup
//////////////////////////////////////////////////////////////////////////////

/// Group Inspector, a small information window used when clicking
/// on group names in the 2D UI
class LLInspectGroup : public LLInspect
{
	friend class LLFloaterReg;
	
public:
	// key["group_id"] - Group ID for which to show information
	// Inspector will be positioned relative to current mouse position
	LLInspectGroup(const LLSD& key);
	virtual ~LLInspectGroup();
	
	// Because floater is single instance, need to re-parse data on each spawn
	// (for example, inspector about same group but in different position)
	/*virtual*/ void onOpen(const LLSD& group_id);

	// When closing they should close their gear menu 
	/*virtual*/ void onClose(bool app_quitting);
	
	// Update view based on information from group manager
	void processGroupData();
	
	// Make network requests for all the data to display in this view.
	// Used on construction and if avatar id changes.
	void requestUpdate();
		
	// Callback for gCacheName to look up group name
	// Faster than waiting for group properties to return
	void nameUpdatedCallback(const LLUUID& id,
							 const std::string& name,
							 bool is_group);

	// Button/menu callbacks
	void onClickViewProfile();
	void onClickJoin();
	void onClickLeave();
	
private:
	LLUUID				mGroupID;
	// an in-flight network request for group properties 
	// is represented by this object
	LLFetchGroupData*	mPropertiesRequest;
};

//////////////////////////////////////////////////////////////////////////////
// LLFetchGroupData
//////////////////////////////////////////////////////////////////////////////

// This object represents a pending request for avatar properties information
class LLFetchGroupData : public LLGroupMgrObserver
{
public:
	// If the inspector closes it will delete the pending request object, so the
	// inspector pointer will be valid for the lifetime of this object
	LLFetchGroupData(const LLUUID& group_id, LLInspectGroup* inspector)
	:	LLGroupMgrObserver(group_id),
		mInspector(inspector)
	{
		LLGroupMgr* mgr = LLGroupMgr::getInstance();
		// register ourselves as an observer
		mgr->addObserver(this);
		// send a request
		mgr->sendGroupPropertiesRequest(group_id);
	}
	
	~LLFetchGroupData()
	{
		// remove ourselves as an observer
		LLGroupMgr::getInstance()->removeObserver(this);
	}
	
	void changed(LLGroupChange gc)
	{
		if (gc == GC_PROPERTIES)
		{
			mInspector->processGroupData();
		}
	}
	
	LLInspectGroup* mInspector;
};

LLInspectGroup::LLInspectGroup(const LLSD& sd)
:	LLInspect( LLSD() ),	// single_instance, doesn't really need key
	mGroupID(),			// set in onOpen()
	mPropertiesRequest(NULL)
{
	mCommitCallbackRegistrar.add("InspectGroup.ViewProfile",
		boost::bind(&LLInspectGroup::onClickViewProfile, this));
	mCommitCallbackRegistrar.add("InspectGroup.Join",
		boost::bind(&LLInspectGroup::onClickJoin, this));	
	mCommitCallbackRegistrar.add("InspectGroup.Leave",
		boost::bind(&LLInspectGroup::onClickLeave, this));	

	// can't make the properties request until the widgets are constructed
	// as it might return immediately, so do it in postBuild.
}

LLInspectGroup::~LLInspectGroup()
{
	// clean up any pending requests so they don't call back into a deleted
	// view
	delete mPropertiesRequest;
	mPropertiesRequest = NULL;
}


// Multiple calls to showInstance("inspect_avatar", foo) will provide different
// LLSD for foo, which we will catch here.
//virtual
void LLInspectGroup::onOpen(const LLSD& data)
{
	// start fade animation
	LLInspect::onOpen(data);

	mGroupID = data["group_id"];

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

// virtual
void LLInspectGroup::onClose(bool app_quitting)
{
	// *TODO: If we add a gear menu, close it here
}	

void LLInspectGroup::requestUpdate()
{
	// Don't make network requests when spawning from the debug menu at the
	// login screen (which is useful to work on the layout).
	if (mGroupID.isNull())
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
	getChild<LLUICtrl>("group_name")->setValue("");
	getChild<LLUICtrl>("group_subtitle")->setValue("");
	getChild<LLUICtrl>("group_details")->setValue("");
	getChild<LLUICtrl>("group_cost")->setValue("");
	// Must have a visible button so the inspector can take focus
	getChild<LLUICtrl>("view_profile_btn")->setVisible(true);
	getChild<LLUICtrl>("leave_btn")->setVisible(false);
	getChild<LLUICtrl>("join_btn")->setVisible(false);
	
	// Make a new request for properties
	delete mPropertiesRequest;
	mPropertiesRequest = new LLFetchGroupData(mGroupID, this);

	// Name lookup will be faster out of cache, use that
	gCacheName->getGroup(mGroupID,
		boost::bind(&LLInspectGroup::nameUpdatedCallback,
			this, _1, _2, _3));
}

void LLInspectGroup::nameUpdatedCallback(
	const LLUUID& id,
	const std::string& name,
	bool is_group)
{
	if (id == mGroupID)
	{
		getChild<LLUICtrl>("group_name")->setValue( LLSD(name) );
	}
	
	// Otherwise possibly a request for an older inspector, ignore it
}

void LLInspectGroup::processGroupData()
{
	LLGroupMgrGroupData* data =
		LLGroupMgr::getInstance()->getGroupData(mGroupID);

	if (data)
	{
		// Noun pluralization depends on language
		std::string lang = LLUI::getLanguage();
		std::string members =
			LLTrans::getCountString(lang, "GroupMembers", data->mMemberCount);
		getChild<LLUICtrl>("group_subtitle")->setValue( LLSD(members) );

		getChild<LLUICtrl>("group_details")->setValue( LLSD(data->mCharter) );

		getChild<LLUICtrl>("group_icon")->setValue( LLSD(data->mInsigniaID) );

		std::string cost;
		bool is_member = LLGroupActions::isInGroup(mGroupID);
		if (is_member)
		{
			cost = getString("YouAreMember");
		}
		else if (data->mOpenEnrollment)
		{
			if (data->mMembershipFee == 0)
			{
				cost = getString("FreeToJoin");
			}
			else
			{
				std::string amount =
					LLResMgr::getInstance()->getMonetaryString(
						data->mMembershipFee);
				LLStringUtil::format_map_t args;
				args["[AMOUNT]"] = amount;
				cost = getString("CostToJoin", args);
			}
		}
		else
		{
			cost = getString("PrivateGroup");
		}
		getChild<LLUICtrl>("group_cost")->setValue(cost);

		getChild<LLUICtrl>("join_btn")->setVisible(!is_member);
		getChild<LLUICtrl>("leave_btn")->setVisible(is_member);

		// Only enable join button if you are allowed to join
		bool can_join = !is_member && data->mOpenEnrollment;
		getChild<LLUICtrl>("join_btn")->setEnabled(can_join);
	}

	// Delete the request object as it has been satisfied
	delete mPropertiesRequest;
	mPropertiesRequest = NULL;
}

void LLInspectGroup::onClickViewProfile()
{
	closeFloater();
	LLGroupActions::show(mGroupID);
}

void LLInspectGroup::onClickJoin()
{
	closeFloater();
	LLGroupActions::join(mGroupID);
}

void LLInspectGroup::onClickLeave()
{
	closeFloater();
	LLGroupActions::leave(mGroupID);
}

//////////////////////////////////////////////////////////////////////////////
// LLInspectGroupUtil
//////////////////////////////////////////////////////////////////////////////
void LLInspectGroupUtil::registerFloater()
{
	LLFloaterReg::add("inspect_group", "inspect_group.xml",
					  &LLFloaterReg::build<LLInspectGroup>);
}
