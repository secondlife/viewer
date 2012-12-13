/** 
 * @file llfloaterautoreplacesettings.cpp
 * @brief Auto Replace List floater
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterautoreplacesettings.h"

#include "llagentdata.h"
#include "llcommandhandler.h"
#include "llfloater.h"
#include "lluictrlfactory.h"
#include "llagent.h"
#include "llpanel.h"
#include "llbutton.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "llview.h"
#include "llhttpclient.h"
#include "llbufferstream.h"
#include "llcheckboxctrl.h"
#include "llviewercontrol.h"

#include "llui.h"
#include "llcontrol.h"
#include "llscrollingpanellist.h"
#include "llautoreplace.h"
#include "llfilepicker.h"
#include "llfile.h"
#include "llsdserialize.h"
#include "llsdutil.h"

#include "llchat.h"
#include "llinventorymodel.h"
#include "llhost.h"
#include "llassetstorage.h"
#include "roles_constants.h"
#include "llviewertexteditor.h"
#include <boost/tokenizer.hpp>

#include <iosfwd>
#include "llfloaterreg.h"
#include "llinspecttoast.h"
#include "llnotificationhandler.h"
#include "llnotificationmanager.h"
#include "llnotificationsutil.h"


LLFloaterAutoReplaceSettings::LLFloaterAutoReplaceSettings(const LLSD& key)
 : LLFloater(key)
 , mSelectedListName("")
 , mListNames(NULL)
 , mReplacementsList(NULL)
 , mKeyword(NULL)
 , mPreviousKeyword("")
 , mReplacement(NULL)
{
}

void LLFloaterAutoReplaceSettings::onClose(bool app_quitting)
{
	cleanUp();
}

BOOL LLFloaterAutoReplaceSettings::postBuild(void)
{
	// get copies of the current settings that we will operate on
	mEnabled  = gSavedSettings.getBOOL("AutoReplace");
	LL_DEBUGS("AutoReplace") << ( mEnabled ? "enabled" : "disabled") << LL_ENDL;

	mSettings = LLAutoReplace::getInstance()->getSettings();
	
	// global checkbox for whether or not autoreplace is active
	LLUICtrl* enabledCheckbox = getChild<LLUICtrl>("autoreplace_enable");
	enabledCheckbox->setCommitCallback(boost::bind(&LLFloaterAutoReplaceSettings::onAutoReplaceToggled, this));
	enabledCheckbox->setValue(LLSD(mEnabled));

	// top row list creation and deletion
	getChild<LLUICtrl>("autoreplace_import_list")->setCommitCallback(boost::bind(&LLFloaterAutoReplaceSettings::onImportList,this));
	getChild<LLUICtrl>("autoreplace_export_list")->setCommitCallback(boost::bind(&LLFloaterAutoReplaceSettings::onExportList,this));
	getChild<LLUICtrl>("autoreplace_new_list")->setCommitCallback(   boost::bind(&LLFloaterAutoReplaceSettings::onNewList,this));
	getChild<LLUICtrl>("autoreplace_delete_list")->setCommitCallback(boost::bind(&LLFloaterAutoReplaceSettings::onDeleteList,this));

	// the list of keyword->replacement lists
	mListNames = getChild<LLScrollListCtrl>("autoreplace_list_name");
	mListNames->setCommitCallback(boost::bind(&LLFloaterAutoReplaceSettings::onSelectList, this));
	mListNames->setCommitOnSelectionChange(true);
	
	// list ordering
	getChild<LLUICtrl>("autoreplace_list_up")->setCommitCallback(  boost::bind(&LLFloaterAutoReplaceSettings::onListUp,this));
	getChild<LLUICtrl>("autoreplace_list_down")->setCommitCallback(boost::bind(&LLFloaterAutoReplaceSettings::onListDown,this));

	// keyword->replacement entry add / delete
	getChild<LLUICtrl>("autoreplace_add_entry")->setCommitCallback(   boost::bind(&LLFloaterAutoReplaceSettings::onAddEntry,this));
	getChild<LLUICtrl>("autoreplace_delete_entry")->setCommitCallback(boost::bind(&LLFloaterAutoReplaceSettings::onDeleteEntry,this));

	// entry edits
	mKeyword     = getChild<LLLineEditor>("autoreplace_keyword");
	mReplacement = getChild<LLLineEditor>("autoreplace_replacement");
	getChild<LLUICtrl>("autoreplace_save_entry")->setCommitCallback(boost::bind(&LLFloaterAutoReplaceSettings::onSaveEntry, this));

	// dialog termination ( Save Changes / Cancel )
	getChild<LLUICtrl>("autoreplace_save_changes")->setCommitCallback(boost::bind(&LLFloaterAutoReplaceSettings::onSaveChanges, this));
	getChild<LLUICtrl>("autoreplace_cancel")->setCommitCallback(boost::bind(&LLFloaterAutoReplaceSettings::onCancel, this));

	// the list of keyword->replacement pairs
	mReplacementsList = getChild<LLScrollListCtrl>("autoreplace_list_replacements");
	mReplacementsList->setCommitCallback(boost::bind(&LLFloaterAutoReplaceSettings::onSelectEntry, this));
	mReplacementsList->setCommitOnSelectionChange(true);

	center();

	mSelectedListName.clear();
	updateListNames();
	updateListNamesControls();
	updateReplacementsList();

	return true;
}


void LLFloaterAutoReplaceSettings::updateListNames()
{
	mListNames->deleteAllItems(); // start from scratch

	LLSD listNames = mSettings.getListNames(); // Array of Strings
   
	for ( LLSD::array_const_iterator entry = listNames.beginArray(), end = listNames.endArray();
		  entry != end;
		  ++entry
		 )
	{
		const std::string& listName = entry->asString();
		mListNames->addSimpleElement(listName);
	}

	if (!mSelectedListName.empty())
	{
		mListNames->setSelectedByValue( LLSD(mSelectedListName), true );
	}
}

void LLFloaterAutoReplaceSettings::updateListNamesControls()
{
	if ( mSelectedListName.empty() )
	{
		// There is no selected list

		// Disable all controls that operate on the selected list
		getChild<LLButton>("autoreplace_export_list")->setEnabled(false);
		getChild<LLButton>("autoreplace_delete_list")->setEnabled(false);
		getChild<LLButton>("autoreplace_list_up")->setEnabled(false);
		getChild<LLButton>("autoreplace_list_down")->setEnabled(false);

		mReplacementsList->deleteAllItems();
	}
	else
	{
		// Enable the controls that operate on the selected list
		getChild<LLButton>("autoreplace_export_list")->setEnabled(true);
		getChild<LLButton>("autoreplace_delete_list")->setEnabled(true);
		getChild<LLButton>("autoreplace_list_up")->setEnabled(!selectedListIsFirst());
		getChild<LLButton>("autoreplace_list_down")->setEnabled(!selectedListIsLast());
	}
}

void LLFloaterAutoReplaceSettings::onSelectList()
{
	std::string previousSelectedListName = mSelectedListName;
	// only one selection allowed
	LLSD selected = mListNames->getSelectedValue();
	if (selected.isDefined())
	{
		mSelectedListName = selected.asString();
		LL_DEBUGS("AutoReplace")<<"selected list '"<<mSelectedListName<<"'"<<LL_ENDL;
	}
	else
	{
		mSelectedListName.clear();
		LL_DEBUGS("AutoReplace")<<"unselected"<<LL_ENDL;
	}

	updateListNamesControls();

	if ( previousSelectedListName != mSelectedListName )
	{
		updateReplacementsList();
	}
}

void LLFloaterAutoReplaceSettings::onSelectEntry()
{
    LLSD selectedRow = mReplacementsList->getSelectedValue();
	if (selectedRow.isDefined())
	{
		mPreviousKeyword = selectedRow.asString();
		LL_DEBUGS("AutoReplace")<<"selected entry '"<<mPreviousKeyword<<"'"<<LL_ENDL;	
		mKeyword->setValue(selectedRow);
		std::string replacement = mSettings.replacementFor(mPreviousKeyword, mSelectedListName );
		mReplacement->setValue(replacement);
		enableReplacementEntry();		
		mReplacement->setFocus(true);
	}
	else
	{
		// no entry selection, so the entry panel should be off
		disableReplacementEntry();		
		LL_DEBUGS("AutoReplace")<<"no row selected"<<LL_ENDL;
	}
}

void LLFloaterAutoReplaceSettings::updateReplacementsList()
{
	// start from scratch, since this should only be called when the list changes
	mReplacementsList->deleteAllItems();

	if ( mSelectedListName.empty() )
	{
		mReplacementsList->setEnabled(false);
		getChild<LLButton>("autoreplace_add_entry")->setEnabled(false);
		disableReplacementEntry();		
	}
	else
	{
		// Populate the keyword->replacement list from the selected list
		const LLSD* mappings = mSettings.getListEntries(mSelectedListName);
		for ( LLSD::map_const_iterator entry = mappings->beginMap(), end = mappings->endMap();
			  entry != end;
			  entry++
			 )
		{
			LLSD row;
			row["id"] = entry->first;
			row["columns"][0]["column"] = "keyword";
			row["columns"][0]["value"]  = entry->first;
			row["columns"][1]["column"] = "replacement";
			row["columns"][1]["value"]  = entry->second;

			mReplacementsList->addElement(row, ADD_BOTTOM);
		}

		mReplacementsList->deselectAllItems(false /* don't call commit */);
		mReplacementsList->setEnabled(true);

		getChild<LLButton>("autoreplace_add_entry")->setEnabled(true);
		disableReplacementEntry();		
	}
}

void LLFloaterAutoReplaceSettings::enableReplacementEntry()
{
	LL_DEBUGS("AutoReplace")<<LL_ENDL;
	mKeyword->setEnabled(true);
	mReplacement->setEnabled(true);
	getChild<LLButton>("autoreplace_save_entry")->setEnabled(true);
	getChild<LLButton>("autoreplace_delete_entry")->setEnabled(true);
}

void LLFloaterAutoReplaceSettings::disableReplacementEntry()
{
	LL_DEBUGS("AutoReplace")<<LL_ENDL;
	mPreviousKeyword.clear();
	mKeyword->clear();
	mKeyword->setEnabled(false);
	mReplacement->clear();
	mReplacement->setEnabled(false);
	getChild<LLButton>("autoreplace_save_entry")->setEnabled(false);
	getChild<LLButton>("autoreplace_delete_entry")->setEnabled(false);
}

// called when the global settings checkbox is changed
void LLFloaterAutoReplaceSettings::onAutoReplaceToggled()
{
	// set our local copy of the flag, copied to the global preference in onOk
	mEnabled = childGetValue("autoreplace_enable").asBoolean();
	LL_DEBUGS("AutoReplace")<< "autoreplace_enable " << ( mEnabled ? "on" : "off" ) << LL_ENDL;
}

// called when the List Up button is pressed
void LLFloaterAutoReplaceSettings::onListUp()
{
	S32 selectedRow = mListNames->getFirstSelectedIndex();
	LLSD selectedName = mListNames->getSelectedValue().asString();

	if ( mSettings.increaseListPriority(selectedName) )
	{
		updateListNames();
		updateListNamesControls();
	}
	else
	{
		LL_WARNS("AutoReplace")
			<< "invalid row ("<<selectedRow<<") selected '"<<selectedName<<"'"
			<<LL_ENDL;
	}
}

// called when the List Down button is pressed
void LLFloaterAutoReplaceSettings::onListDown()
{
	S32 selectedRow = mListNames->getFirstSelectedIndex();
	std::string selectedName = mListNames->getSelectedValue().asString();

	if ( mSettings.decreaseListPriority(selectedName) )
	{
		updateListNames();
		updateListNamesControls();
	}
	else
	{
		LL_WARNS("AutoReplace")
			<< "invalid row ("<<selectedRow<<") selected '"<<selectedName<<"'"
			<<LL_ENDL;
	}
}

// called when the Delete Entry button is pressed
void LLFloaterAutoReplaceSettings::onDeleteEntry()
{
    LLSD selectedRow = mReplacementsList->getSelectedValue();
	if (selectedRow.isDefined())
	{	
		std::string keyword = selectedRow.asString();
		mReplacementsList->deleteSelectedItems(); // delete from the control
		mSettings.removeEntryFromList(keyword, mSelectedListName); // delete from the local settings copy
		disableReplacementEntry(); // no selection active, so turn off the buttons
	}
}

// called when the Import List button is pressed
void LLFloaterAutoReplaceSettings::onImportList()
{
	LLFilePicker& picker = LLFilePicker::instance();
	if( picker.getOpenFile( LLFilePicker::FFLOAD_XML) )
	{
		llifstream file;
		file.open(picker.getFirstFile().c_str());
		LLSD newList;
		if (file.is_open())
		{
			LLSDSerialize::fromXMLDocument(newList, file);
		}
		file.close();

		switch ( mSettings.addList(newList) )
		{
		case LLAutoReplaceSettings::AddListOk:
			mSelectedListName = LLAutoReplaceSettings::getListName(newList);
			
			updateListNames();
			updateListNamesControls();
			updateReplacementsList();
			break;

		case LLAutoReplaceSettings::AddListDuplicateName:
			{
				std::string newName = LLAutoReplaceSettings::getListName(newList);
				LL_WARNS("AutoReplace")<<"name '"<<newName<<"' is in use; prompting for new name"<<LL_ENDL;
				LLSD newPayload;
				newPayload["list"] = newList;
				LLSD args;
				args["DUPNAME"] = newName;
	
				LLNotificationsUtil::add("RenameAutoReplaceList", args, newPayload,
										 boost::bind(&LLFloaterAutoReplaceSettings::callbackListNameConflict, this, _1, _2));
			}
			break;

		case LLAutoReplaceSettings::AddListInvalidList:
			LLNotificationsUtil::add("InvalidAutoReplaceList");
			LL_WARNS("AutoReplace") << "imported list was invalid" << LL_ENDL;

			mSelectedListName.clear();
			updateListNames();
			updateListNamesControls();
			updateReplacementsList();
			break;

		default:
			LL_ERRS("AutoReplace") << "invalid AddListResult" << LL_ENDL;

		}
		
	}
	else
	{
		LL_DEBUGS("AutoReplace") << "file selection failed for import list" << LL_ENDL;
	}		
}

void LLFloaterAutoReplaceSettings::onNewList()
{
	LLSD payload;
	LLSD emptyList;
	LLAutoReplaceSettings::createEmptyList(emptyList);
	payload["list"] = emptyList;
    LLSD args;
	
	LLNotificationsUtil::add("AddAutoReplaceList", args, payload,
							 boost::bind(&LLFloaterAutoReplaceSettings::callbackNewListName, this, _1, _2));
}

bool LLFloaterAutoReplaceSettings::callbackNewListName(const LLSD& notification, const LLSD& response)
{
	LL_DEBUGS("AutoReplace")<<"called"<<LL_ENDL;
	
	LLSD newList = notification["payload"]["list"];

	if ( response.has("listname") && response["listname"].isString() )
	{
		std::string newName = response["listname"].asString();
		LLAutoReplaceSettings::setListName(newList, newName);

		switch ( mSettings.addList(newList) )
		{
		case LLAutoReplaceSettings::AddListOk:
			LL_INFOS("AutoReplace") << "added new list '"<<newName<<"'"<<LL_ENDL;
			mSelectedListName = newName;
			updateListNames();
			updateListNamesControls();
			updateReplacementsList();
			break;

		case LLAutoReplaceSettings::AddListDuplicateName:
			{
				LL_WARNS("AutoReplace")<<"name '"<<newName<<"' is in use; prompting for new name"<<LL_ENDL;
				LLSD newPayload;
				newPayload["list"] = notification["payload"]["list"];
				LLSD args;
				args["DUPNAME"] = newName;
	
				LLNotificationsUtil::add("RenameAutoReplaceList", args, newPayload,
										 boost::bind(&LLFloaterAutoReplaceSettings::callbackListNameConflict, this, _1, _2));
			}
			break;

		case LLAutoReplaceSettings::AddListInvalidList:
			LLNotificationsUtil::add("InvalidAutoReplaceList");

			mSelectedListName.clear();
			updateListNames();
			updateListNamesControls();
			updateReplacementsList();
			break;

		default:
			LL_ERRS("AutoReplace") << "invalid AddListResult" << LL_ENDL;
		}
	}
	else
	{
		LL_ERRS("AutoReplace") << "adding notification response" << LL_ENDL;
	}
	return false;
}

// callback for the RenameAutoReplaceList notification
bool LLFloaterAutoReplaceSettings::callbackListNameConflict(const LLSD& notification, const LLSD& response)
{
	LLSD newList = notification["payload"]["list"];
	std::string listName = LLAutoReplaceSettings::getListName(newList);
	
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch ( option )
	{
	case 0:
		// Replace current list
		if ( LLAutoReplaceSettings::AddListOk == mSettings.replaceList(newList) )
		{
			LL_INFOS("AutoReplace") << "replaced list '"<<listName<<"'"<<LL_ENDL;
			mSelectedListName = listName;
			updateListNames();
			updateListNamesControls();
			updateReplacementsList();
		}
		else
		{
			LL_WARNS("AutoReplace")<<"failed to replace list '"<<listName<<"'"<<LL_ENDL;
		}
		break;

	case 1:
		// Use New Name
		LL_INFOS("AutoReplace")<<"option 'use new name' selected"<<LL_ENDL;
		callbackNewListName(notification, response);
		break;

	default:
		LL_ERRS("AutoReplace")<<"invalid selected option "<<option<<LL_ENDL;
	}

	return false;
}

void LLFloaterAutoReplaceSettings::onDeleteList()
{
	std::string listName = mListNames->getSelectedValue().asString();
	if ( ! listName.empty() )
	{
		if ( mSettings.removeReplacementList(listName) )
		{
			LL_INFOS("AutoReplace")<<"deleted list '"<<listName<<"'"<<LL_ENDL;
			mReplacementsList->deleteSelectedItems();   // remove from the scrolling list
			mSelectedListName.clear();
			updateListNames();
			updateListNamesControls();
			updateReplacementsList();
		}
		else
		{
			LL_WARNS("AutoReplace")<<"failed to delete list '"<<listName<<"'"<<LL_ENDL;
		}
	}
	else
	{
		LL_DEBUGS("AutoReplace")<<"no list selected for delete"<<LL_ENDL;
	}
}

void LLFloaterAutoReplaceSettings::onExportList()
{
	std::string listName=mListNames->getFirstSelected()->getColumn(0)->getValue().asString();
	const LLSD* list = mSettings.exportList(listName);
	std::string listFileName = listName + ".xml";
	LLFilePicker& picker = LLFilePicker::instance();
	if( picker.getSaveFile( LLFilePicker::FFSAVE_XML, listFileName) )
	{
		llofstream file;
		file.open(picker.getFirstFile().c_str());
		LLSDSerialize::toPrettyXML(*list, file);
		file.close();
	}
}

void LLFloaterAutoReplaceSettings::onAddEntry()
{
	mPreviousKeyword.clear();
	mReplacementsList->deselectAllItems(false /* don't call commit */);
	mKeyword->clear();
	mReplacement->clear();
	enableReplacementEntry();
	mKeyword->setFocus(true);
}

void LLFloaterAutoReplaceSettings::onSaveEntry()
{
	LL_DEBUGS("AutoReplace")<<"called"<<LL_ENDL;
	
	if ( ! mPreviousKeyword.empty() )
	{
		// delete any existing value for the key that was editted
		LL_INFOS("AutoReplace")
			<< "list '" << mSelectedListName << "' "
			<< "removed '" << mPreviousKeyword
			<< "'" << LL_ENDL;
		mSettings.removeEntryFromList( mPreviousKeyword, mSelectedListName );
	}

	LLWString keyword     = mKeyword->getWText();
	LLWString replacement = mReplacement->getWText();
	if ( mSettings.addEntryToList(keyword, replacement, mSelectedListName) )
	{
		// insert the new keyword->replacement pair
		LL_INFOS("AutoReplace")
			<< "list '" << mSelectedListName << "' "
			<< "added '" << wstring_to_utf8str(keyword)
			<< "' -> '" << wstring_to_utf8str(replacement)
			<< "'" << LL_ENDL;

		updateReplacementsList();
	}
	else
	{
		LLNotificationsUtil::add("InvalidAutoReplaceEntry");
		LL_WARNS("AutoReplace")<<"invalid entry "
							   << "keyword '" << wstring_to_utf8str(keyword)
							   << "' replacement '" << wstring_to_utf8str(replacement)
							   << "'" << LL_ENDL;
	}
}

void LLFloaterAutoReplaceSettings::onCancel()
{
	cleanUp();
	closeFloater(false /* not quitting */);
}

void LLFloaterAutoReplaceSettings::onSaveChanges()
{
	// put our local copy of the settings into the active copy
	LLAutoReplace::getInstance()->setSettings( mSettings );
	// save our local copy of the global feature enable/disable value
	gSavedSettings.setBOOL("AutoReplace", mEnabled);
	cleanUp();
	closeFloater(false /* not quitting */);
}

void LLFloaterAutoReplaceSettings::cleanUp()
{

}

bool LLFloaterAutoReplaceSettings::selectedListIsFirst()
{
	bool isFirst = false;
	
	if (!mSelectedListName.empty())
	{
		LLSD lists = mSettings.getListNames(); // an Array of Strings
		LLSD first = lists.get(0);
		if ( first.isString() && first.asString() == mSelectedListName )
		{
			isFirst = true;
		}
	}
	return isFirst;
}

bool LLFloaterAutoReplaceSettings::selectedListIsLast()
{
	bool isLast = false;
	
	if (!mSelectedListName.empty())
	{
		LLSD last;
		LLSD lists = mSettings.getListNames(); // an Array of Strings
		for ( LLSD::array_const_iterator list = lists.beginArray(), listEnd = lists.endArray();
			  list != listEnd;
			  list++
			 )
		{
			last = *list;
		}
		if ( last.isString() && last.asString() == mSelectedListName )
		{
			isLast = true;
		}
	}
	return isLast;
}

/* TBD
mOldText = getChild<LLLineEditor>("autoreplace_old_text");
mNewText = getChild<LLLineEditor>("autoreplace_new_text");
*/
