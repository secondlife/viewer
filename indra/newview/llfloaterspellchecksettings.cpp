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
#include "llfilepicker.h"
#include "llfloaterreg.h"
#include "llfloaterspellchecksettings.h"
#include "llnotificationsutil.h"
#include "llscrolllistctrl.h"
#include "llsdserialize.h"
#include "llspellcheck.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewermenufile.h" // LLFilePickerReplyThread

#include <boost/algorithm/string.hpp>

///----------------------------------------------------------------------------
/// Class LLFloaterSpellCheckerSettings
///----------------------------------------------------------------------------
LLFloaterSpellCheckerSettings::LLFloaterSpellCheckerSettings(const LLSD& key)
	: LLFloater(key)
{
}

void LLFloaterSpellCheckerSettings::draw()
{
	LLFloater::draw();

	std::vector<LLScrollListItem*> sel_items = getChild<LLScrollListCtrl>("spellcheck_available_list")->getAllSelected();
	bool enable_remove = !sel_items.empty();
	for (std::vector<LLScrollListItem*>::const_iterator sel_it = sel_items.begin(); sel_it != sel_items.end(); ++sel_it)
	{
		enable_remove &= LLSpellChecker::getInstance()->canRemoveDictionary((*sel_it)->getValue().asString());
	}
	getChild<LLUICtrl>("spellcheck_remove_btn")->setEnabled(enable_remove);
}

BOOL LLFloaterSpellCheckerSettings::postBuild(void)
{
	gSavedSettings.getControl("SpellCheck")->getSignal()->connect(boost::bind(&LLFloaterSpellCheckerSettings::refreshDictionaries, this, false));
	LLSpellChecker::setSettingsChangeCallback(boost::bind(&LLFloaterSpellCheckerSettings::onSpellCheckSettingsChange, this));
	getChild<LLUICtrl>("spellcheck_remove_btn")->setCommitCallback(boost::bind(&LLFloaterSpellCheckerSettings::onBtnRemove, this));
	getChild<LLUICtrl>("spellcheck_import_btn")->setCommitCallback(boost::bind(&LLFloaterSpellCheckerSettings::onBtnImport, this));
	getChild<LLUICtrl>("spellcheck_main_combo")->setCommitCallback(boost::bind(&LLFloaterSpellCheckerSettings::refreshDictionaries, this, false));
	getChild<LLUICtrl>("spellcheck_moveleft_btn")->setCommitCallback(boost::bind(&LLFloaterSpellCheckerSettings::onBtnMove, this, "spellcheck_active_list", "spellcheck_available_list"));
	getChild<LLUICtrl>("spellcheck_moveright_btn")->setCommitCallback(boost::bind(&LLFloaterSpellCheckerSettings::onBtnMove, this, "spellcheck_available_list", "spellcheck_active_list"));
	center();
	return true;
}

void LLFloaterSpellCheckerSettings::onBtnImport()
{
	LLFloaterReg::showInstance("prefs_spellchecker_import");
}

void LLFloaterSpellCheckerSettings::onBtnMove(const std::string& from, const std::string& to)
{
	LLScrollListCtrl* from_ctrl = findChild<LLScrollListCtrl>(from);
	LLScrollListCtrl* to_ctrl = findChild<LLScrollListCtrl>(to);

	LLSD row;
	row["columns"][0]["column"] = "name";

	std::vector<LLScrollListItem*> sel_items = from_ctrl->getAllSelected();
	std::vector<LLScrollListItem*>::const_iterator sel_it;
	for ( sel_it = sel_items.begin(); sel_it != sel_items.end(); ++sel_it)
	{
		row["value"] = (*sel_it)->getValue();
		row["columns"][0]["value"] = (*sel_it)->getColumn(0)->getValue();
		to_ctrl->addElement(row);
		to_ctrl->setSelectedByValue( (*sel_it)->getValue(), true );
	}
	from_ctrl->deleteSelectedItems();
}

void LLFloaterSpellCheckerSettings::onClose(bool app_quitting)
{
	if (app_quitting)
	{
		// don't save anything
		return;
	}
	LLFloaterReg::hideInstance("prefs_spellchecker_import");
	
	std::list<std::string> list_dict;

	LLComboBox* dict_combo = findChild<LLComboBox>("spellcheck_main_combo");
	const std::string dict_name = dict_combo->getSelectedItemLabel();
	if (!dict_name.empty())
	{
		list_dict.push_back(dict_name);

		LLScrollListCtrl* list_ctrl = findChild<LLScrollListCtrl>("spellcheck_active_list");
		std::vector<LLScrollListItem*> list_items = list_ctrl->getAllData();
		for (std::vector<LLScrollListItem*>::const_iterator item_it = list_items.begin(); item_it != list_items.end(); ++item_it)
		{
			const std::string language = (*item_it)->getValue().asString();
			if (LLSpellChecker::getInstance()->hasDictionary(language, true))
			{
				list_dict.push_back(language);
			}
		}
	}
	gSavedSettings.setString("SpellCheckDictionary", boost::join(list_dict, ","));
}

void LLFloaterSpellCheckerSettings::onOpen(const LLSD& key)
{
	refreshDictionaries(true);
}

void LLFloaterSpellCheckerSettings::onBtnRemove()
{
	std::vector<LLScrollListItem*> sel_items = getChild<LLScrollListCtrl>("spellcheck_available_list")->getAllSelected();
	for (std::vector<LLScrollListItem*>::const_iterator sel_it = sel_items.begin(); sel_it != sel_items.end(); ++sel_it)
	{
		LLSpellChecker::instance().removeDictionary((*sel_it)->getValue().asString());
	}
}

void LLFloaterSpellCheckerSettings::onSpellCheckSettingsChange()
{
	refreshDictionaries(true);
}

void LLFloaterSpellCheckerSettings::refreshDictionaries(bool from_settings)
{
	bool enabled = gSavedSettings.getBOOL("SpellCheck");
	getChild<LLUICtrl>("spellcheck_moveleft_btn")->setEnabled(enabled);
	getChild<LLUICtrl>("spellcheck_moveright_btn")->setEnabled(enabled);

	// Populate the dictionary combobox
	LLComboBox* dict_combo = findChild<LLComboBox>("spellcheck_main_combo");
	std::string dict_cur = dict_combo->getSelectedItemLabel();
	if ((dict_cur.empty() || from_settings) && (LLSpellChecker::getUseSpellCheck()))
	{
		dict_cur = LLSpellChecker::instance().getPrimaryDictionary();
	}
	dict_combo->clearRows();

	const LLSD& dict_map = LLSpellChecker::getInstance()->getDictionaryMap();
	if (dict_map.size())
	{
		for (LLSD::array_const_iterator dict_it = dict_map.beginArray(); dict_it != dict_map.endArray(); ++dict_it)
		{
			const LLSD& dict = *dict_it;
			if ( (dict["installed"].asBoolean()) && (dict["is_primary"].asBoolean()) && (dict.has("language")) )
			{
				dict_combo->add(dict["language"].asString());
			}
		}
		if (!dict_combo->selectByValue(dict_cur))
		{
			dict_combo->clear();
		}
	}
	dict_combo->sortByName();
	dict_combo->setEnabled(enabled);

	// Populate the available and active dictionary list
	LLScrollListCtrl* avail_ctrl = findChild<LLScrollListCtrl>("spellcheck_available_list");
	LLScrollListCtrl* active_ctrl = findChild<LLScrollListCtrl>("spellcheck_active_list");

	LLSpellChecker::dict_list_t active_list;
	if ( ((!avail_ctrl->getItemCount()) && (!active_ctrl->getItemCount())) || (from_settings) )
	{
		if (LLSpellChecker::getUseSpellCheck())
		{
			active_list = LLSpellChecker::instance().getSecondaryDictionaries();
		}
	}
	else
	{
		std::vector<LLScrollListItem*> active_items = active_ctrl->getAllData();
		for (std::vector<LLScrollListItem*>::const_iterator item_it = active_items.begin(); item_it != active_items.end(); ++item_it)
		{
			std::string dict = (*item_it)->getValue().asString();
			if (dict_cur != dict)
			{
				active_list.push_back(dict);
			}
		}
	}

	LLSD row;
	row["columns"][0]["column"] = "name";

	active_ctrl->clearRows();
	active_ctrl->setEnabled(enabled);
	for (LLSpellChecker::dict_list_t::const_iterator it = active_list.begin(); it != active_list.end(); ++it)
	{
		const std::string language = *it;
		const LLSD dict = LLSpellChecker::getInstance()->getDictionaryData(language);
		row["value"] = language;
		row["columns"][0]["value"] = (!dict["user_installed"].asBoolean()) ? language : language + " " + LLTrans::getString("UserDictionary");
		active_ctrl->addElement(row);
	}
	active_ctrl->sortByColumnIndex(0, true);
	active_list.push_back(dict_cur);

	avail_ctrl->clearRows();
	avail_ctrl->setEnabled(enabled);
	for (LLSD::array_const_iterator dict_it = dict_map.beginArray(); dict_it != dict_map.endArray(); ++dict_it)
	{
		const LLSD& dict = *dict_it;
		const std::string language = dict["language"].asString();
		if ( (dict["installed"].asBoolean()) && (active_list.end() == std::find(active_list.begin(), active_list.end(), language)) )
		{
			row["value"] = language;
			row["columns"][0]["value"] = (!dict["user_installed"].asBoolean()) ? language : language + " " + LLTrans::getString("UserDictionary");
			avail_ctrl->addElement(row);
		}
	}
	avail_ctrl->sortByColumnIndex(0, true);
}

///----------------------------------------------------------------------------
/// Class LLFloaterSpellCheckerImport
///----------------------------------------------------------------------------
LLFloaterSpellCheckerImport::LLFloaterSpellCheckerImport(const LLSD& key)
	: LLFloater(key)
{
}

BOOL LLFloaterSpellCheckerImport::postBuild(void)
{
	getChild<LLUICtrl>("dictionary_path_browse")->setCommitCallback(boost::bind(&LLFloaterSpellCheckerImport::onBtnBrowse, this));
	getChild<LLUICtrl>("ok_btn")->setCommitCallback(boost::bind(&LLFloaterSpellCheckerImport::onBtnOK, this));
	getChild<LLUICtrl>("cancel_btn")->setCommitCallback(boost::bind(&LLFloaterSpellCheckerImport::onBtnCancel, this));
	center();
	return true;
}

void LLFloaterSpellCheckerImport::onBtnBrowse()
{
	(new LLFilePickerReplyThread(boost::bind(&LLFloaterSpellCheckerImport::importSelectedDictionary, this, _1), LLFilePicker::FFLOAD_DICTIONARY, false))->getFile();
}

void LLFloaterSpellCheckerImport::importSelectedDictionary(const std::vector<std::string>& filenames)
{
	std::string filepath = filenames[0];

	const std::string extension = gDirUtilp->getExtension(filepath);
	if ("xcu" == extension)
	{
		filepath = parseXcuFile(filepath);
		if (filepath.empty())
		{
			return;
		}
	}

	getChild<LLUICtrl>("dictionary_path")->setValue(filepath);

	mDictionaryDir = gDirUtilp->getDirName(filepath);
	mDictionaryBasename = gDirUtilp->getBaseFileName(filepath, true);
	getChild<LLUICtrl>("dictionary_name")->setValue(mDictionaryBasename);
}

void LLFloaterSpellCheckerImport::onBtnCancel()
{
	closeFloater(false);
}

void LLFloaterSpellCheckerImport::onBtnOK()
{
	const std::string dict_dic = mDictionaryDir + gDirUtilp->getDirDelimiter() + mDictionaryBasename + ".dic";
	const std::string dict_aff = mDictionaryDir + gDirUtilp->getDirDelimiter() + mDictionaryBasename + ".aff";
	std::string dict_language = getChild<LLUICtrl>("dictionary_language")->getValue().asString();
	LLStringUtil::trim(dict_language);

	bool imported = false;
	if (   dict_language.empty()
		|| mDictionaryDir.empty()
		|| mDictionaryBasename.empty()
		|| ! gDirUtilp->fileExists(dict_dic)
		)
	{
		LLNotificationsUtil::add("SpellingDictImportRequired");
	}
	else
	{
		std::string settings_dic = LLSpellChecker::getDictionaryUserPath() + mDictionaryBasename + ".dic";
		if ( LLFile::copy( dict_dic, settings_dic ) )
		{
			if (gDirUtilp->fileExists(dict_aff))
			{
				std::string settings_aff = LLSpellChecker::getDictionaryUserPath() + mDictionaryBasename + ".aff";
				if ( LLFile::copy( dict_aff, settings_aff ))
				{
					imported = true;
				}
				else
				{
					LLSD args = LLSD::emptyMap();
					args["FROM_NAME"] = dict_aff;
					args["TO_NAME"] = settings_aff;
					LLNotificationsUtil::add("SpellingDictImportFailed", args);
				}
			}
			else
			{
				LLSD args = LLSD::emptyMap();
				args["DIC_NAME"] = dict_dic;
				LLNotificationsUtil::add("SpellingDictIsSecondary", args);

				imported = true;
			}
		}
		else
		{
			LLSD args = LLSD::emptyMap();
			args["FROM_NAME"] = dict_dic;
			args["TO_NAME"] = settings_dic;
			LLNotificationsUtil::add("SpellingDictImportFailed", args);
		}
	}

	if ( imported )
	{
		LLSD custom_dict_info;
		custom_dict_info["is_primary"] = (bool)gDirUtilp->fileExists(dict_aff);
		custom_dict_info["name"] = mDictionaryBasename;
		custom_dict_info["language"] = dict_language;

		LLSD custom_dict_map;
        std::string custom_filename(LLSpellChecker::getDictionaryUserPath() + "user_dictionaries.xml");
		llifstream custom_file_in(custom_filename.c_str());
		if (custom_file_in.is_open())
		{
			LLSDSerialize::fromXMLDocument(custom_dict_map, custom_file_in);
			custom_file_in.close();
		}
	
		LLSD::array_iterator it = custom_dict_map.beginArray();
		for (; it != custom_dict_map.endArray(); ++it)
		{
			LLSD& dict_info = *it;
			if (dict_info["name"].asString() == mDictionaryBasename)
			{
				dict_info = custom_dict_info;
				break;
			}
		}
		if (custom_dict_map.endArray() == it)
		{
			custom_dict_map.append(custom_dict_info);
		}

		llofstream custom_file_out(custom_filename.c_str(), std::ios::trunc);
		if (custom_file_out.is_open())
		{
			LLSDSerialize::toPrettyXML(custom_dict_map, custom_file_out);
			custom_file_out.close();
		}

		LLSpellChecker::getInstance()->refreshDictionaryMap();
	}

	closeFloater(false);
}

std::string LLFloaterSpellCheckerImport::parseXcuFile(const std::string& file_path) const
{
	LLXMLNodePtr xml_root;
	if ( (!LLUICtrlFactory::getLayeredXMLNode(file_path, xml_root)) || (xml_root.isNull()) )
	{
		return LLStringUtil::null;
	}

	// Bury down to the "Dictionaries" parent node
	LLXMLNode* dict_node = NULL;
	for (LLXMLNode* outer_node = xml_root->getFirstChild(); outer_node && !dict_node; outer_node = outer_node->getNextSibling())
	{
		std::string temp;
		if ( (outer_node->getAttributeString("oor:name", temp)) && ("ServiceManager" == temp) )
		{
			for (LLXMLNode* inner_node = outer_node->getFirstChild(); inner_node && !dict_node; inner_node = inner_node->getNextSibling())
			{
				if ( (inner_node->getAttributeString("oor:name", temp)) && ("Dictionaries" == temp) )
				{
					dict_node = inner_node;
					break;
				}
			}
		}
	}

	if (dict_node)
	{
		// Iterate over all child nodes until we find one that has a <value>DICT_SPELL</value> node
		for (LLXMLNode* outer_node = dict_node->getFirstChild(); outer_node; outer_node = outer_node->getNextSibling())
		{
			std::string temp;
			LLXMLNodePtr location_node, format_node;
			for (LLXMLNode* inner_node = outer_node->getFirstChild(); inner_node; inner_node = inner_node->getNextSibling())
			{
				if (inner_node->getAttributeString("oor:name", temp))
				{
					if ("Locations" == temp)
					{
						inner_node->getChild("value", location_node, false);
					}
					else if ("Format" == temp)
					{
						inner_node->getChild("value", format_node, false);
					}
				}
			}
			if ( (format_node.isNull()) || ("DICT_SPELL" != format_node->getValue()) || (location_node.isNull()) )
			{
				continue;
			}

			// Found a list of file locations, return the .dic (if present)
			std::list<std::string> location_list;
			boost::split(location_list, location_node->getValue(), boost::is_any_of(std::string(" ")));
			for (std::list<std::string>::iterator it = location_list.begin(); it != location_list.end(); ++it)
			{
				std::string& location = *it;
				if ("\\" != gDirUtilp->getDirDelimiter())
					LLStringUtil::replaceString(location, "\\", gDirUtilp->getDirDelimiter());
				else
					LLStringUtil::replaceString(location, "/", gDirUtilp->getDirDelimiter());
				LLStringUtil::replaceString(location, "%origin%", gDirUtilp->getDirName(file_path));
				if ("dic" == gDirUtilp->getExtension(location))
				{
					return location;
				}
			}
		}
	}

	return LLStringUtil::null;
}
