/** 
 * @file llfloateravatarpicker.cpp
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "llfloateravatarpicker.h"

// Viewer includes
#include "llagent.h"
#include "llcallingcard.h"
#include "lldate.h"				// split()
#include "lldateutil.h"			// ageFromDate()
#include "llfocusmgr.h"
#include "llfloaterreg.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"		// getCapability()
#include "llworld.h"

// Linden libraries
#include "llavatarnamecache.h"	// IDEVO
#include "llbutton.h"
#include "llcachename.h"
#include "llhttpclient.h"		// IDEVO
#include "lllineeditor.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "lltabcontainer.h"
#include "lluictrlfactory.h"
#include "message.h"

LLFloaterAvatarPicker* LLFloaterAvatarPicker::show(select_callback_t callback,
												   BOOL allow_multiple,
												   BOOL closeOnSelect)
{
	// *TODO: Use a key to allow this not to be an effective singleton
	LLFloaterAvatarPicker* floater = 
		LLFloaterReg::showTypedInstance<LLFloaterAvatarPicker>("avatar_picker");
	
	floater->mSelectionCallback = callback;
	floater->setAllowMultiple(allow_multiple);
	floater->mNearMeListComplete = FALSE;
	floater->mCloseOnSelect = closeOnSelect;
	
	if (!closeOnSelect)
	{
		// Use Select/Close
		std::string select_string = floater->getString("Select");
		std::string close_string = floater->getString("Close");
		floater->getChild<LLButton>("ok_btn")->setLabel(select_string);
		floater->getChild<LLButton>("cancel_btn")->setLabel(close_string);
	}

	return floater;
}

// Default constructor
LLFloaterAvatarPicker::LLFloaterAvatarPicker(const LLSD& key)
  : LLFloater(key),
	mNumResultsReturned(0),
	mNearMeListComplete(FALSE),
	mCloseOnSelect(FALSE)
{
// 	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_avatar_picker.xml");
	mCommitCallbackRegistrar.add("Refresh.FriendList", boost::bind(&LLFloaterAvatarPicker::populateFriend, this));
}

BOOL LLFloaterAvatarPicker::postBuild()
{
	getChild<LLLineEditor>("Edit")->setKeystrokeCallback( boost::bind(&LLFloaterAvatarPicker::editKeystroke, this, _1, _2),NULL);

	childSetAction("Find", boost::bind(&LLFloaterAvatarPicker::onBtnFind, this));
	childDisable("Find");
	childSetAction("Refresh", boost::bind(&LLFloaterAvatarPicker::onBtnRefresh, this));
	getChild<LLUICtrl>("near_me_range")->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onRangeAdjust, this));
	
	LLScrollListCtrl* searchresults = getChild<LLScrollListCtrl>("SearchResults");
	searchresults->setDoubleClickCallback( boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
	searchresults->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));
	childDisable("SearchResults");
	
	LLScrollListCtrl* nearme = getChild<LLScrollListCtrl>("NearMe");
	nearme->setDoubleClickCallback(boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
	nearme->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));

	LLScrollListCtrl* friends = getChild<LLScrollListCtrl>("Friends");
	friends->setDoubleClickCallback(boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
	getChild<LLUICtrl>("Friends")->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));

	childSetAction("ok_btn", boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
	childDisable("ok_btn");
	childSetAction("cancel_btn", boost::bind(&LLFloaterAvatarPicker::onBtnClose, this));

	childSetFocus("Edit");

	LLPanel* search_panel = getChild<LLPanel>("SearchPanel");
	if (search_panel)
	{
		// Start searching when Return is pressed in the line editor.
		search_panel->setDefaultBtn("Find");
	}

	getChild<LLScrollListCtrl>("SearchResults")->setCommentText(getString("no_results"));

	getChild<LLTabContainer>("ResidentChooserTabs")->setCommitCallback(
		boost::bind(&LLFloaterAvatarPicker::onTabChanged, this));
	
	setAllowMultiple(FALSE);
	
	center();
	
	populateFriend();

	return TRUE;
}

void LLFloaterAvatarPicker::setOkBtnEnableCb(validate_callback_t cb)
{
	mOkButtonValidateSignal.connect(cb);
}

void LLFloaterAvatarPicker::onTabChanged()
{
	childSetEnabled("ok_btn", isSelectBtnEnabled());
}

// Destroys the object
LLFloaterAvatarPicker::~LLFloaterAvatarPicker()
{
	gFocusMgr.releaseFocusIfNeeded( this );
}

void LLFloaterAvatarPicker::onBtnFind()
{
	find();
}

static void getSelectedAvatarData(const LLScrollListCtrl* from, std::vector<std::string>& avatar_names, std::vector<LLUUID>& avatar_ids)
{
	std::vector<LLScrollListItem*> items = from->getAllSelected();
	for (std::vector<LLScrollListItem*>::iterator iter = items.begin(); iter != items.end(); ++iter)
	{
		LLScrollListItem* item = *iter;
		if (item->getUUID().notNull())
		{
			avatar_names.push_back(item->getColumn(0)->getValue().asString());
			avatar_ids.push_back(item->getUUID());
		}
	}
}

void LLFloaterAvatarPicker::onBtnSelect()
{

	// If select btn not enabled then do not callback
	if (!isSelectBtnEnabled())
		return;

	if(mSelectionCallback)
	{
		std::string acvtive_panel_name;
		LLScrollListCtrl* list =  NULL;
		LLPanel* active_panel = childGetVisibleTab("ResidentChooserTabs");
		if(active_panel)
		{
			acvtive_panel_name = active_panel->getName();
		}
		if(acvtive_panel_name == "SearchPanel")
		{
			list = getChild<LLScrollListCtrl>("SearchResults");
		}
		else if(acvtive_panel_name == "NearMePanel")
		{
			list = getChild<LLScrollListCtrl>("NearMe");
		}
		else if (acvtive_panel_name == "FriendsPanel")
		{
			list = getChild<LLScrollListCtrl>("Friends");
		}

		if(list)
		{
			std::vector<std::string>	avatar_names;
			std::vector<LLUUID>			avatar_ids;
			getSelectedAvatarData(list, avatar_names, avatar_ids);
			mSelectionCallback(avatar_names, avatar_ids);
		}
	}
	getChild<LLScrollListCtrl>("SearchResults")->deselectAllItems(TRUE);
	getChild<LLScrollListCtrl>("NearMe")->deselectAllItems(TRUE);
	getChild<LLScrollListCtrl>("Friends")->deselectAllItems(TRUE);
	if(mCloseOnSelect)
	{
		mCloseOnSelect = FALSE;
		closeFloater();		
	}
}

void LLFloaterAvatarPicker::onBtnRefresh()
{
	getChild<LLScrollListCtrl>("NearMe")->deleteAllItems();
	getChild<LLScrollListCtrl>("NearMe")->setCommentText(getString("searching"));
	mNearMeListComplete = FALSE;
}

void LLFloaterAvatarPicker::onBtnClose()
{
	closeFloater();
}

void LLFloaterAvatarPicker::onRangeAdjust()
{
	onBtnRefresh();
}

void LLFloaterAvatarPicker::onList()
{
	childSetEnabled("ok_btn", isSelectBtnEnabled());
}

void LLFloaterAvatarPicker::populateNearMe()
{
	BOOL all_loaded = TRUE;
	BOOL empty = TRUE;
	LLScrollListCtrl* near_me_scroller = getChild<LLScrollListCtrl>("NearMe");
	near_me_scroller->deleteAllItems();

	std::vector<LLUUID> avatar_ids;
	LLWorld::getInstance()->getAvatars(&avatar_ids, NULL, gAgent.getPositionGlobal(), gSavedSettings.getF32("NearMeRange"));
	for(U32 i=0; i<avatar_ids.size(); i++)
	{
		LLUUID& av = avatar_ids[i];
		if(av == gAgent.getID()) continue;
		LLSD element;
		element["id"] = av; // value
		std::string fullname;
		if(!gCacheName->getFullName(av, fullname))
		{
			element["columns"][0]["value"] = LLCacheName::getDefaultName();
			all_loaded = FALSE;
		}			
		else
		{
			element["columns"][0]["value"] = fullname;
		}
		near_me_scroller->addElement(element);
		empty = FALSE;
	}

	if (empty)
	{
		childDisable("NearMe");
		childDisable("ok_btn");
		near_me_scroller->setCommentText(getString("no_one_near"));
	}
	else 
	{
		childEnable("NearMe");
		childEnable("ok_btn");
		near_me_scroller->selectFirstItem();
		onList();
		near_me_scroller->setFocus(TRUE);
	}

	if (all_loaded)
	{
		mNearMeListComplete = TRUE;
	}
}

void LLFloaterAvatarPicker::populateFriend()
{
	LLScrollListCtrl* friends_scroller = getChild<LLScrollListCtrl>("Friends");
	friends_scroller->deleteAllItems();
	LLCollectAllBuddies collector;
	LLAvatarTracker::instance().applyFunctor(collector);
	LLCollectAllBuddies::buddy_map_t::iterator it;
	
	
	for(it = collector.mOnline.begin(); it!=collector.mOnline.end(); it++)
	{
		friends_scroller->addStringUUIDItem(it->first, it->second);
	}
	for(it = collector.mOffline.begin(); it!=collector.mOffline.end(); it++)
	{
			friends_scroller->addStringUUIDItem(it->first, it->second);
	}
	friends_scroller->sortByColumnIndex(0, TRUE);
}

void LLFloaterAvatarPicker::draw()
{
	LLFloater::draw();
	if (!mNearMeListComplete && childGetVisibleTab("ResidentChooserTabs") == getChild<LLPanel>("NearMePanel"))
	{
		populateNearMe();
	}
}

BOOL LLFloaterAvatarPicker::visibleItemsSelected() const
{
	LLPanel* active_panel = childGetVisibleTab("ResidentChooserTabs");

	if(active_panel == getChild<LLPanel>("SearchPanel"))
	{
		return getChild<LLScrollListCtrl>("SearchResults")->getFirstSelectedIndex() >= 0;
	}
	else if(active_panel == getChild<LLPanel>("NearMePanel"))
	{
		return getChild<LLScrollListCtrl>("NearMe")->getFirstSelectedIndex() >= 0;
	}
	else if(active_panel == getChild<LLPanel>("FriendsPanel"))
	{
		return getChild<LLScrollListCtrl>("Friends")->getFirstSelectedIndex() >= 0;
	}
	return FALSE;
}

class LLAvatarPickerResponder : public LLHTTPClient::Responder
{
public:
	LLUUID mQueryID;

	LLAvatarPickerResponder(const LLUUID& id) : mQueryID(id) { }

	/*virtual*/ void result(const LLSD& content)
	{
		LLFloaterAvatarPicker* floater =
			LLFloaterReg::findTypedInstance<LLFloaterAvatarPicker>("avatar_picker");
		if (floater)
		{
			floater->processResponse(mQueryID, content);
		}
	}

	/*virtual*/ void error(U32 status, const std::string& reason)
	{
		llinfos << "JAMESDEBUG avatar picker failed " << status
			<< " reason " << reason << llendl;
	}
};

void LLFloaterAvatarPicker::find()
{
	std::string text = childGetValue("Edit").asString();

	mQueryID.generate();

	std::string url;
	url.reserve(128); // avoid a memory allocation or two

	LLViewerRegion* region = gAgent.getRegion();
	url = region->getCapability("AvatarPickerSearch");
	// Prefer use of capabilities to search on both SLID and display name
	// but allow display name search to be manually turned off for test
	if (!url.empty()
		&& LLAvatarNameCache::useDisplayNames())
	{
		// capability urls don't end in '/', but we need one to parse
		// query parameters correctly
		if (url.size() > 0 && url[url.size()-1] != '/')
		{
			url += "/";
		}
		url += "?name=";
		url += LLURI::escape(text);
		llinfos << "JAMESDEBUG picker " << url << llendl;
		LLHTTPClient::get(url, new LLAvatarPickerResponder(mQueryID));
	}
	else
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("AvatarPickerRequest");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->addUUID("QueryID", mQueryID);	// not used right now
		msg->nextBlock("Data");
		msg->addString("Name", text);
		gAgent.sendReliableMessage();
	}

	getChild<LLScrollListCtrl>("SearchResults")->deleteAllItems();
	getChild<LLScrollListCtrl>("SearchResults")->setCommentText(getString("searching"));
	
	childSetEnabled("ok_btn", FALSE);
	mNumResultsReturned = 0;
}

void LLFloaterAvatarPicker::setAllowMultiple(BOOL allow_multiple)
{
	getChild<LLScrollListCtrl>("SearchResults")->setAllowMultipleSelection(allow_multiple);
	getChild<LLScrollListCtrl>("NearMe")->setAllowMultipleSelection(allow_multiple);
	getChild<LLScrollListCtrl>("Friends")->setAllowMultipleSelection(allow_multiple);
}

// static 
void LLFloaterAvatarPicker::processAvatarPickerReply(LLMessageSystem* msg, void**)
{
	LLUUID	agent_id;
	LLUUID	query_id;
	LLUUID	avatar_id;
	std::string first_name;
	std::string last_name;

	msg->getUUID("AgentData", "AgentID", agent_id);
	msg->getUUID("AgentData", "QueryID", query_id);

	// Not for us
	if (agent_id != gAgent.getID()) return;
	
	LLFloaterAvatarPicker* floater = LLFloaterReg::findTypedInstance<LLFloaterAvatarPicker>("avatar_picker");

	// these are not results from our last request
	if (query_id != floater->mQueryID)
	{
		return;
	}

	LLScrollListCtrl* search_results = floater->getChild<LLScrollListCtrl>("SearchResults");

	// clear "Searching" label on first results
	if (floater->mNumResultsReturned++ == 0)
	{
		search_results->deleteAllItems();
	}

	BOOL found_one = FALSE;
	S32 num_new_rows = msg->getNumberOfBlocks("Data");
	for (S32 i = 0; i < num_new_rows; i++)
	{			
		msg->getUUIDFast(  _PREHASH_Data,_PREHASH_AvatarID,	avatar_id, i);
		msg->getStringFast(_PREHASH_Data,_PREHASH_FirstName, first_name, i);
		msg->getStringFast(_PREHASH_Data,_PREHASH_LastName,	last_name, i);
	
		std::string avatar_name;
		if (avatar_id.isNull())
		{
			LLStringUtil::format_map_t map;
			map["[TEXT]"] = floater->childGetText("Edit");
			avatar_name = floater->getString("not_found", map);
			search_results->setEnabled(FALSE);
			floater->childDisable("ok_btn");
		}
		else
		{
			avatar_name = LLCacheName::buildFullName(first_name, last_name);
			search_results->setEnabled(TRUE);
			found_one = TRUE;
		}
		LLSD element;
		element["id"] = avatar_id; // value
		element["columns"][0]["column"] = "name";
		element["columns"][0]["value"] = avatar_name;
		search_results->addElement(element);
	}

	if (found_one)
	{
		floater->childEnable("ok_btn");
		search_results->selectFirstItem();
		floater->onList();
		search_results->setFocus(TRUE);
	}
}

void LLFloaterAvatarPicker::processResponse(const LLUUID& query_id, const LLSD& content)
{
	// Check for out-of-date query
	if (query_id != mQueryID) return;

	LLScrollListCtrl* search_results = getChild<LLScrollListCtrl>("SearchResults");

	LLSD agents = content["agents"];
	if (agents.size() == 0)
	{
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = childGetText("Edit");
		LLSD item;
		item["id"] = LLUUID::null;
		item["columns"][0]["column"] = "name";
		item["columns"][0]["value"] = getString("not_found", map);
		search_results->addElement(item);
		search_results->setEnabled(FALSE);
		childDisable("ok_btn");
		return;
	}

	// clear "Searching" label on first results
	search_results->deleteAllItems();

	LLSD item;
	LLSD::array_const_iterator it = agents.beginArray();
	for ( ; it != agents.endArray(); ++it)
	{
		const LLSD& row = *it;
		item["id"] = row["agent_id"];
		LLSD& columns = item["columns"];
		columns[0]["column"] = "name";
		columns[0]["value"] = row["display_name"];
		columns[1]["column"] = "slid";
		columns[1]["value"] = row["sl_id"];
		LLDate account_created = row["account_created"].asDate();
		S32 year, month, day;
		account_created.split(&year, &month, &day);
		std::string age =
			LLDateUtil::ageFromDate(year, month, day, LLDate::now());
		columns[2]["column"] = "age";
		columns[2]["value"] = age;
		search_results->addElement(item);
	}

	childEnable("ok_btn");
	search_results->setEnabled(true);
	search_results->selectFirstItem();
	onList();
	search_results->setFocus(TRUE);
}

//static
void LLFloaterAvatarPicker::editKeystroke(LLLineEditor* caller, void* user_data)
{
	childSetEnabled("Find", caller->getText().size() >= 3);
}

// virtual
BOOL LLFloaterAvatarPicker::handleKeyHere(KEY key, MASK mask)
{
	if (key == KEY_RETURN && mask == MASK_NONE)
	{
		if (childHasFocus("Edit"))
		{
			onBtnFind();
		}
		else
		{
			onBtnSelect();
		}
		return TRUE;
	}
	else if (key == KEY_ESCAPE && mask == MASK_NONE)
	{
		closeFloater();
		return TRUE;
	}

	return LLFloater::handleKeyHere(key, mask);
}

bool LLFloaterAvatarPicker::isSelectBtnEnabled()
{
	bool ret_val = visibleItemsSelected();

	if ( ret_val && mOkButtonValidateSignal.num_slots() )
	{
		std::string acvtive_panel_name;
		LLScrollListCtrl* list =  NULL;
		LLPanel* active_panel = childGetVisibleTab("ResidentChooserTabs");

		if(active_panel)
		{
			acvtive_panel_name = active_panel->getName();
		}

		if(acvtive_panel_name == "SearchPanel")
		{
			list = getChild<LLScrollListCtrl>("SearchResults");
		}
		else if(acvtive_panel_name == "NearMePanel")
		{
			list = getChild<LLScrollListCtrl>("NearMe");
		}
		else if (acvtive_panel_name == "FriendsPanel")
		{
			list = getChild<LLScrollListCtrl>("Friends");
		}

		if(list)
		{
			std::vector<LLUUID> avatar_ids;
			std::vector<std::string> avatar_names;
			getSelectedAvatarData(list, avatar_names, avatar_ids);
			return mOkButtonValidateSignal(avatar_ids);
		}
	}

	return ret_val;
}
