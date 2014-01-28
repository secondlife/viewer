/** 
* @file llfloaterexperiencepicker.cpp
* @brief Implementation of llfloaterexperiencepicker
* @author dolphin@lindenlab.com
*
* $LicenseInfo:firstyear=2014&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2014, Linden Research, Inc.
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

#include "llfloaterexperiencepicker.h"


#include "lllineeditor.h"
#include "llfloaterreg.h"
#include "llscrolllistctrl.h"
#include "llviewerregion.h"
#include "llagent.h"
#include "llexperiencecache.h"
#include "llslurl.h"
#include "llavatarnamecache.h"
#include "llfloaterexperienceprofile.h"
#include "llcombobox.h"

#define BTN_FIND		"find"
#define BTN_OK			"ok_btn"
#define BTN_CANCEL		"cancel_btn"
#define BTN_PROFILE		"profile_btn"
#define TEXT_EDIT		"edit"
#define TEXT_MATURITY	"maturity"
#define LIST_RESULTS	"search_results"
#define PANEL_SEARCH	"search_panel"

const static std::string columnSpace = " ";

class LLExperiencePickerResponder : public LLHTTPClient::Responder
{
public:
	LLUUID mQueryID;
	LLHandle<LLFloaterExperiencePicker> mParent;

	LLExperiencePickerResponder(const LLUUID& id, const LLHandle<LLFloaterExperiencePicker>& parent) : mQueryID(id), mParent(parent) { }

	void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
			if(mParent.isDead())
				return;

			LLFloaterExperiencePicker* floater =mParent.get();
			if (floater)
			{
				floater->processResponse(mQueryID, content);
			}
		}
		else
		{
			llwarns << "avatar picker failed [status:" << status << "]: " << content << llendl;

		}
	}
};
LLFloaterExperiencePicker* LLFloaterExperiencePicker::show( select_callback_t callback, const LLUUID& key, BOOL allow_multiple, BOOL closeOnSelect )
{
	LLFloaterExperiencePicker* floater =
		LLFloaterReg::showTypedInstance<LLFloaterExperiencePicker>("experience_search", key);
	if (!floater)
	{
		llwarns << "Cannot instantiate experience picker" << llendl;
		return NULL;
	}

	floater->mSelectionCallback = callback;
	floater->mCloseOnSelect = closeOnSelect;
	floater->setAllowMultiple(allow_multiple);
	return floater;
}


LLFloaterExperiencePicker::LLFloaterExperiencePicker( const LLSD& key )
	:LLFloater(key)
{

}

LLFloaterExperiencePicker::~LLFloaterExperiencePicker()
{
	gFocusMgr.releaseFocusIfNeeded( this );
}



BOOL LLFloaterExperiencePicker::postBuild()
{
	getChild<LLLineEditor>(TEXT_EDIT)->setKeystrokeCallback( boost::bind(&LLFloaterExperiencePicker::editKeystroke, this, _1, _2),NULL);

	childSetAction(BTN_FIND, boost::bind(&LLFloaterExperiencePicker::onBtnFind, this));
	getChildView(BTN_FIND)->setEnabled(FALSE);

	LLScrollListCtrl* searchresults = getChild<LLScrollListCtrl>(LIST_RESULTS);
	searchresults->setDoubleClickCallback( boost::bind(&LLFloaterExperiencePicker::onBtnSelect, this));
	searchresults->setCommitCallback(boost::bind(&LLFloaterExperiencePicker::onList, this));
	getChildView(LIST_RESULTS)->setEnabled(FALSE);
	getChild<LLScrollListCtrl>(LIST_RESULTS)->setCommentText(getString("no_results"));

	childSetAction(BTN_OK, boost::bind(&LLFloaterExperiencePicker::onBtnSelect, this));
	getChildView(BTN_OK)->setEnabled(FALSE);
	childSetAction(BTN_CANCEL, boost::bind(&LLFloaterExperiencePicker::onBtnClose, this));
	childSetAction(BTN_PROFILE, boost::bind(&LLFloaterExperiencePicker::onBtnProfile, this));
	getChildView(BTN_PROFILE)->setEnabled(FALSE);

	getChild<LLComboBox>(TEXT_MATURITY)->setCurrentByIndex(2);
	getChild<LLComboBox>(TEXT_MATURITY)->setCommitCallback(boost::bind(&LLFloaterExperiencePicker::onMaturity, this));
	getChild<LLUICtrl>(TEXT_EDIT)->setFocus(TRUE);

	LLPanel* search_panel = getChild<LLPanel>(PANEL_SEARCH);
	if (search_panel)
	{
		// Start searching when Return is pressed in the line editor.
		search_panel->setDefaultBtn(BTN_FIND);
	}
	return TRUE;
}

void LLFloaterExperiencePicker::editKeystroke( class LLLineEditor* caller, void* user_data )
{
	getChildView(BTN_FIND)->setEnabled(caller->getText().size() > 0);
}

void LLFloaterExperiencePicker::onBtnFind()
{
	find();
}

void LLFloaterExperiencePicker::onBtnSelect()
{
	if(!isSelectButtonEnabled())
	{
		return;
	}

	if(mSelectionCallback)
	{
		const LLScrollListCtrl* results = getChild<LLScrollListCtrl>(LIST_RESULTS);
		uuid_vec_t experience_ids;
		
		getSelectedExperienceIds(results, experience_ids);
		mSelectionCallback(experience_ids);
	}
	getChild<LLScrollListCtrl>(LIST_RESULTS)->deselectAllItems(TRUE);
	if(mCloseOnSelect)
	{
		mCloseOnSelect = FALSE;
		closeFloater();
	}
}

void LLFloaterExperiencePicker::onList()
{
	bool enabled = isSelectButtonEnabled();
	getChildView(BTN_OK)->setEnabled(enabled);

	enabled = enabled && getChild<LLScrollListCtrl>(LIST_RESULTS)->getNumSelected() == 1;
	getChildView(BTN_PROFILE)->setEnabled(enabled);
}


void LLFloaterExperiencePicker::onBtnClose()
{
	closeFloater();
}


void LLFloaterExperiencePicker::find()
{
	std::string text = getChild<LLUICtrl>(TEXT_EDIT)->getValue().asString();
	mQueryID.generate();
	std::string url;
	url.reserve(128+text.size());

	LLViewerRegion* region = gAgent.getRegion();
	url = region->getCapability("FindExperienceByName");
	if (!url.empty())
	{
		url+="?query=";
		url+=LLURI::escape(text);
		LLHTTPClient::get(url, new LLExperiencePickerResponder(mQueryID, getDerivedHandle<LLFloaterExperiencePicker>()));

	}


	getChild<LLScrollListCtrl>(LIST_RESULTS)->deleteAllItems();
	getChild<LLScrollListCtrl>(LIST_RESULTS)->setCommentText(getString("searching"));

	getChildView(BTN_OK)->setEnabled(FALSE);
	getChildView(BTN_PROFILE)->setEnabled(FALSE);
}


bool LLFloaterExperiencePicker::isSelectButtonEnabled()
{
	return getChild<LLScrollListCtrl>(LIST_RESULTS)->getFirstSelectedIndex() >=0;
}

void LLFloaterExperiencePicker::getSelectedExperienceIds( const LLScrollListCtrl* results, uuid_vec_t &experience_ids )
{
	std::vector<LLScrollListItem*> items = results->getAllSelected();
	for(std::vector<LLScrollListItem*>::iterator it = items.begin(); it != items.end(); ++it)
	{
		LLScrollListItem* item = *it;
		if (item->getUUID().notNull())
		{
			experience_ids.push_back(item->getUUID());
		}
	}
}

void LLFloaterExperiencePicker::setAllowMultiple( bool allow_multiple )
{
	getChild<LLScrollListCtrl>(LIST_RESULTS)->setAllowMultipleSelection(allow_multiple);
}


void name_callback(const LLHandle<LLFloaterExperiencePicker>& floater, const LLUUID& experience_id, const LLUUID& agent_id, const LLAvatarName& av_name)
{
	if(floater.isDead())
		return;
	LLFloaterExperiencePicker* picker = floater.get();
	LLScrollListCtrl* search_results = picker->getChild<LLScrollListCtrl>(LIST_RESULTS);

	LLScrollListItem* item = search_results->getItem(experience_id);
	if(!item)
		return;

	item->getColumn(2)->setValue(columnSpace+av_name.getDisplayName());

}

void LLFloaterExperiencePicker::processResponse( const LLUUID& query_id, const LLSD& content )
{
	if(query_id != mQueryID)
	{
		return;
	}

	mResponse = content;

	const LLSD& experiences=mResponse["experience_keys"];
	LLSD::array_const_iterator it = experiences.beginArray();
	for ( ; it != experiences.endArray(); ++it)
	{
		LLExperienceCache::insert(*it);
	}

	filterContent();

}

void LLFloaterExperiencePicker::onBtnProfile()
{
	LLScrollListItem* item = getChild<LLScrollListCtrl>(LIST_RESULTS)->getFirstSelected();
	if(item)
	{
		LLFloaterReg::showInstance("experience_profile", item->getUUID(), true);
	}
}

std::string LLFloaterExperiencePicker::getMaturityString(int maturity)
{
	if(maturity <= SIM_ACCESS_PG)
	{
		return getString("maturity_icon_general");
	}
	else if(maturity <= SIM_ACCESS_MATURE)
	{
		return getString("maturity_icon_moderate");
	}
	return getString("maturity_icon_adult");
}

void LLFloaterExperiencePicker::filterContent()
{
	LLScrollListCtrl* search_results = getChild<LLScrollListCtrl>(LIST_RESULTS);

	const LLSD& experiences=mResponse["experience_keys"];

	search_results->deleteAllItems();

	int maturity = getChild<LLComboBox>(TEXT_MATURITY)->getSelectedValue().asInteger();
	LLSD item;
	LLSD::array_const_iterator it = experiences.beginArray();
	for ( ; it != experiences.endArray(); ++it)
	{
		const LLSD& experience = *it;

		if(experience[LLExperienceCache::MATURITY].asInteger() > maturity)
			continue;


		item["id"]=experience[LLExperienceCache::EXPERIENCE_ID];
		LLSD& columns = item["columns"];
		columns[0]["column"] = "maturity";
		columns[0]["value"] = getMaturityString(experience[LLExperienceCache::MATURITY].asInteger());
		columns[0]["type"]="icon";
		columns[0]["halign"]="right";
		columns[1]["column"] = "experience_name";
		columns[1]["value"] = columnSpace+experience[LLExperienceCache::NAME].asString();
		columns[2]["column"] = "owner";
		columns[2]["value"] = columnSpace+getString("loading");
		search_results->addElement(item);
		LLAvatarNameCache::get(experience[LLExperienceCache::AGENT_ID], boost::bind(name_callback, getDerivedHandle<LLFloaterExperiencePicker>(), experience[LLExperienceCache::EXPERIENCE_ID], _1, _2));
	}

	if (search_results->isEmpty())
	{
		LLStringUtil::format_map_t map;
		map["[TEXT]"] = childGetText(TEXT_EDIT);
		LLSD item;
		item["id"] = LLUUID::null;
		item["columns"][1]["column"] = "name";
		item["columns"][1]["value"] = columnSpace+getString("not_found", map);
		search_results->addElement(item);
		search_results->setEnabled(false);
		getChildView(BTN_OK)->setEnabled(false);
		getChildView(BTN_PROFILE)->setEnabled(false);
	}
	else
	{
		getChildView(BTN_OK)->setEnabled(true);
		search_results->setEnabled(true);
		search_results->sortByColumnIndex(1, TRUE);
		std::string text = getChild<LLUICtrl>(TEXT_EDIT)->getValue().asString();
		if (!search_results->selectItemByLabel(text, TRUE, 1))
		{
			search_results->selectFirstItem();
		}			
		onList();
		search_results->setFocus(TRUE);
	}
}

void LLFloaterExperiencePicker::onMaturity()
{
	filterContent();
}
















