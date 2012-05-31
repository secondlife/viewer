/** 
 * @file llfloaterspellchecksettings.h
 * @brief Spell checker settings floater
 *
* $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#include "llcombobox.h"
#include "llfloaterspellchecksettings.h"
#include "llscrolllistctrl.h"
#include "llspellcheck.h"
#include "llviewercontrol.h"

#include <boost/algorithm/string.hpp>

LLFloaterSpellCheckerSettings::LLFloaterSpellCheckerSettings(const LLSD& key)
	: LLFloater(key)
{
}

BOOL LLFloaterSpellCheckerSettings::postBuild(void)
{
	gSavedSettings.getControl("SpellCheck")->getSignal()->connect(boost::bind(&LLFloaterSpellCheckerSettings::refreshDictionaryLists, this, false));
	getChild<LLUICtrl>("spellcheck_main_combo")->setCommitCallback(boost::bind(&LLFloaterSpellCheckerSettings::refreshDictionaryLists, this, false));
	getChild<LLUICtrl>("spellcheck_moveleft_btn")->setCommitCallback(boost::bind(&LLFloaterSpellCheckerSettings::onClickDictMove, this, "spellcheck_active_list", "spellcheck_available_list"));
	getChild<LLUICtrl>("spellcheck_moveright_btn")->setCommitCallback(boost::bind(&LLFloaterSpellCheckerSettings::onClickDictMove, this, "spellcheck_available_list", "spellcheck_active_list"));
	getChild<LLUICtrl>("spellcheck_ok")->setCommitCallback(boost::bind(&LLFloaterSpellCheckerSettings::onOK, this));
	getChild<LLUICtrl>("spellcheck_cancel")->setCommitCallback(boost::bind(&LLFloaterSpellCheckerSettings::onCancel, this));

	return true;
}

void LLFloaterSpellCheckerSettings::onCancel()
{
	closeFloater(false);
}

void LLFloaterSpellCheckerSettings::onClickDictMove(const std::string& from, const std::string& to)
{
	LLScrollListCtrl* from_ctrl = findChild<LLScrollListCtrl>(from);
	LLScrollListCtrl* to_ctrl = findChild<LLScrollListCtrl>(to);

	LLSD row;
	row["columns"][0]["column"] = "name";
	row["columns"][0]["font"]["name"] = "SANSSERIF_SMALL";
	row["columns"][0]["font"]["style"] = "NORMAL";

	std::vector<LLScrollListItem*> sel_items = from_ctrl->getAllSelected();
	for (std::vector<LLScrollListItem*>::const_iterator sel_it = sel_items.begin(); sel_it != sel_items.end(); ++sel_it)
	{
		row["columns"][0]["value"] = (*sel_it)->getColumn(0)->getValue();
		to_ctrl->addElement(row);
	}
	from_ctrl->deleteSelectedItems();
}

void LLFloaterSpellCheckerSettings::onOK()
{
	std::list<std::string> list_dict;

	LLComboBox* dict_combo = findChild<LLComboBox>("spellcheck_main_combo");
	const std::string dict_name = dict_combo->getSelectedItemLabel();
	if (!dict_name.empty())
	{
		list_dict.push_back(dict_name);

		LLScrollListCtrl* list_ctrl = findChild<LLScrollListCtrl>("spellcheck_active_list");
		std::vector<LLScrollListItem*> list_items = list_ctrl->getAllData();
		for (std::vector<LLScrollListItem*>::const_iterator item_it = list_items.begin(); item_it != list_items.end(); ++item_it)
			list_dict.push_back((*item_it)->getColumn(0)->getValue().asString());
	}
	gSavedSettings.setString("SpellCheckDictionary", boost::join(list_dict, ","));

	closeFloater(false);
}

void LLFloaterSpellCheckerSettings::onOpen(const LLSD& key)
{
	refreshDictionaryLists(true);
}

void LLFloaterSpellCheckerSettings::refreshDictionaryLists(bool from_settings)
{
	bool enabled = gSavedSettings.getBOOL("SpellCheck");
	getChild<LLUICtrl>("spellcheck_moveleft_btn")->setEnabled(enabled);
	getChild<LLUICtrl>("spellcheck_moveright_btn")->setEnabled(enabled);

	// Populate the dictionary combobox
	LLComboBox* dict_combo = findChild<LLComboBox>("spellcheck_main_combo");
	std::string dict_cur = dict_combo->getSelectedItemLabel();
	if ((dict_cur.empty() || from_settings) && (LLSpellChecker::getUseSpellCheck()))
		dict_cur = LLSpellChecker::instance().getActiveDictionary();
	dict_combo->clearRows();
	dict_combo->setEnabled(enabled);

	const LLSD& dict_map = LLSpellChecker::getDictionaryMap();
	if (dict_map.size())
	{
		for (LLSD::array_const_iterator dict_it = dict_map.beginArray(); dict_it != dict_map.endArray(); ++dict_it)
		{
			const LLSD& dict = *dict_it;
			if ( (dict["installed"].asBoolean()) && (dict["is_primary"].asBoolean()) && (dict.has("language")) )
				dict_combo->add(dict["language"].asString());
		}
		if (!dict_combo->selectByValue(dict_cur))
			dict_combo->clear();
	}

	// Populate the available and active dictionary list
	LLScrollListCtrl* avail_ctrl = findChild<LLScrollListCtrl>("spellcheck_available_list");
	LLScrollListCtrl* active_ctrl = findChild<LLScrollListCtrl>("spellcheck_active_list");

	LLSpellChecker::dict_list_t active_list;
	if ( ((!avail_ctrl->getItemCount()) && (!active_ctrl->getItemCount())) || (from_settings) )
	{
		if (LLSpellChecker::getUseSpellCheck())
			active_list = LLSpellChecker::instance().getSecondaryDictionaries();
	}
	else
	{
		std::vector<LLScrollListItem*> active_items = active_ctrl->getAllData();
		for (std::vector<LLScrollListItem*>::const_iterator item_it = active_items.begin(); item_it != active_items.end(); ++item_it)
		{
			std::string dict = (*item_it)->getColumn(0)->getValue().asString();
			if (dict_cur != dict)
				active_list.push_back(dict);
		}
	}

	LLSD row;
	row["columns"][0]["column"] = "name";
	row["columns"][0]["font"]["name"] = "SANSSERIF_SMALL";
	row["columns"][0]["font"]["style"] = "NORMAL";

	active_ctrl->clearRows();
	active_ctrl->setEnabled(enabled);
	active_ctrl->sortByColumnIndex(0, true);
	for (LLSpellChecker::dict_list_t::const_iterator it = active_list.begin(); it != active_list.end(); ++it)
	{
		row["columns"][0]["value"] = *it;
		active_ctrl->addElement(row);
	}
	active_list.push_back(dict_cur);

	avail_ctrl->clearRows();
	avail_ctrl->setEnabled(enabled);
	avail_ctrl->sortByColumnIndex(0, true);
	for (LLSD::array_const_iterator dict_it = dict_map.beginArray(); dict_it != dict_map.endArray(); ++dict_it)
	{
		const LLSD& dict = *dict_it;
		if ( (dict["installed"].asBoolean()) && (dict.has("language")) && 
			 (active_list.end() == std::find(active_list.begin(), active_list.end(), dict["language"].asString())) )
		{
			row["columns"][0]["value"] = dict["language"].asString();
			avail_ctrl->addElement(row);
		}
	}
}
