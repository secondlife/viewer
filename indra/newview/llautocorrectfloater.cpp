/** 
 * @file llautocorrectfloater.cpp
 * @brief Auto Correct List floater
 * @copyright Copyright (c) 2011 LordGregGreg Back
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
 */

#include "llviewerprecompiledheaders.h"

#include "llautocorrectfloater.h"

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
#include "llautocorrect.h"
#include "llfilepicker.h"
#include "llfile.h"
#include "llsdserialize.h"
//#include "llfloaterchat.h"
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


AutoCorrectFloater::AutoCorrectFloater(const LLSD& key) :
LLFloater(key)
{
}
void AutoCorrectFloater::onClose(bool app_quitting)
{
	destroy();
}

BOOL AutoCorrectFloater::postBuild(void)
{

	namesList = getChild<LLScrollListCtrl>("ac_list_name");
	entryList = getChild<LLScrollListCtrl>("ac_list_entry");

	childSetCommitCallback("ac_enable",onBoxCommitEnabled,this);

	childSetCommitCallback("ac_list_enabled",onEntrySettingChange,this);
	childSetCommitCallback("ac_list_show",onEntrySettingChange,this);
	childSetCommitCallback("ac_list_style",onEntrySettingChange,this);
	childSetCommitCallback("ac_priority",onEntrySettingChange,this);
	

	
	updateEnabledStuff();
	updateNamesList();	


	namesList->setCommitOnSelectionChange(TRUE);
	childSetCommitCallback("ac_list_name", onSelectName, this);
	
	childSetAction("ac_deletelist",removeList,this);
	childSetAction("ac_rementry",deleteEntry,this);
	childSetAction("ac_exportlist",exportList,this);
	childSetAction("ac_addentry",addEntry,this);
	childSetAction("ac_loadlist",loadList,this);

	return true;
}

void AutoCorrectFloater::onSelectName(LLUICtrl* ctrl, void* user_data)
{
	if ( user_data )
	{
		AutoCorrectFloater* self = ( AutoCorrectFloater* )user_data;
		if ( self )
			self->updateItemsList();
	}

}
void AutoCorrectFloater::updateItemsList()
{
	entryList->deleteAllItems();
	if((namesList->getAllSelected().size())<=0)
	{

		updateListControlsEnabled(FALSE);
		return;
	}

	updateListControlsEnabled(TRUE);
	std::string listName= namesList->getFirstSelected()->getColumn(0)->getValue().asString();
	
	LLSD listData = AutoCorrect::getInstance()->getAutoCorrectEntries(listName);
	childSetValue("ac_list_enabled",listData["enabled"].asBoolean());
	childSetValue("ac_list_style",listData["wordStyle"].asBoolean());
	childSetValue("ac_list_show",listData["announce"].asBoolean());
	childSetValue("ac_text_name",listName);
	childSetValue("ac_text_author",listData["author"]);
	childSetValue("ac_priority",listData["priority"]);
	static LLCachedControl<S32> countAuto(gSavedSettings, "AutoCorrectCount");
	childSetValue("ac_stats",(S32)countAuto);
	
	LLSD autoCorrects = listData["data"];
	LLSD::map_const_iterator loc_it = autoCorrects.beginMap();
	LLSD::map_const_iterator loc_end = autoCorrects.endMap();
	for ( ; loc_it != loc_end; ++loc_it)
	{
		const std::string& wrong = (*loc_it).first;
		const std::string& right = (*loc_it).second;

		//std::string lentry(wrong+"=>"+right);

		LLSD element;
		element["id"] = wrong;
		LLSD& s_column = element["columns"][0];
		s_column["column"] = "Search";
		s_column["value"] = wrong;
		s_column["font"] = "SANSSERIF";
		LLSD& r_column = element["columns"][1];
		r_column["column"] = "Replace";
		r_column["value"] = right;
		r_column["font"] = "SANSSERIF";

		entryList->addElement(element, ADD_BOTTOM);
	}
	
}
void AutoCorrectFloater::updateNamesList()
{
	namesList->deleteAllItems();
	static LLCachedControl<bool> enabledd(gSavedSettings, "EnableAutoCorrect");
	if(!(enabledd))
	{
		updateItemsList();
		return;
	}
	static LLCachedControl<S32> countAuto(gSavedSettings, "AutoCorrectCount");
	childSetValue("ac_stats",(S32)countAuto);
	LLSD autoCorrects = AutoCorrect::getInstance()->getAutoCorrects();
	LLSD::map_const_iterator loc_it = autoCorrects.beginMap();
	LLSD::map_const_iterator loc_end = autoCorrects.endMap();
	for ( ; loc_it != loc_end; ++loc_it)
	{
		const std::string& listName = (*loc_it).first;

		LLSD element;
		element["id"] = listName;
		LLSD& friend_column = element["columns"][0];
		friend_column["column"] = "Entries";
		friend_column["value"] = listName;
		//friend_column["font"] = "SANSSERIF";
		const LLSD& loc_map = (*loc_it).second;
		if(loc_map["enabled"].asBoolean())
			friend_column["font"] = "SANSSERIF";
			//friend_column["style"] = "BOLD";
		else
			friend_column["font"] = "SANSSERIF_SMALL";
			//friend_column["style"] = "NORMAL";
		if(namesList)
		namesList->addElement(element, ADD_BOTTOM);
	}
	updateItemsList();
}
void AutoCorrectFloater::updateListControlsEnabled(BOOL selected)
{

		childSetEnabled("ac_text1",selected);
		childSetEnabled("ac_text2",selected);
		childSetEnabled("ac_text_name",selected);
		childSetEnabled("ac_text_author",selected);
		childSetEnabled("ac_list_enabled",selected);
		childSetEnabled("ac_list_show",selected);
		childSetEnabled("ac_list_style",selected);
		childSetEnabled("ac_deletelist",selected);
		childSetEnabled("ac_exportlist",selected);
		childSetEnabled("ac_addentry",selected);
		childSetEnabled("ac_rementry",selected);
		childSetEnabled("ac_priority",selected);
	
}
void AutoCorrectFloater::updateEnabledStuff()
{
	static LLCachedControl<bool> enabledd(gSavedSettings, "EnableAutoCorrect");
	if(!(enabledd))
	{
		LLCheckBoxCtrl *enBox = getChild<LLCheckBoxCtrl>("ac_enable");
		enBox->setDisabledColor(LLColor4::red);
		getChild<LLCheckBoxCtrl>("ac_enable")->setEnabledColor(LLColor4(1.0f,0.0f,0.0f,1.0f));		
	}else
	{
		getChild<LLCheckBoxCtrl>("ac_enable")->setEnabledColor(
			LLUIColorTable::instance().getColor( "LabelTextColor" ));
	}

	childSetEnabled("ac_list_name",enabledd);
	childSetEnabled("ac_list_entry",enabledd);
	updateListControlsEnabled(enabledd);
	updateNamesList();
	AutoCorrect::getInstance()->save();

}
void AutoCorrectFloater::setData(void * data)
{
	//empanel = (LLPanel*)data;
}
void AutoCorrectFloater::onBoxCommitEnabled(LLUICtrl* caller, void* user_data)
{
	if ( user_data )
	{
		AutoCorrectFloater* self = ( AutoCorrectFloater* )user_data;
		if ( self )
		{
			self->updateEnabledStuff();
		}
	}
}
void AutoCorrectFloater::onEntrySettingChange(LLUICtrl* caller, void* user_data)
{
	if ( user_data )
	{
		AutoCorrectFloater* self = ( AutoCorrectFloater* )user_data;
		if ( self )
		{
			std::string listName= self->namesList->getFirstSelected()->getColumn(0)->getValue().asString();
			AutoCorrect::getInstance()->setListEnabled(listName,self->childGetValue("ac_list_enabled").asBoolean());
			AutoCorrect::getInstance()->setListAnnounceeState(listName,self->childGetValue("ac_list_show").asBoolean());
			AutoCorrect::getInstance()->setListStyle(listName,self->childGetValue("ac_list_style").asBoolean());
			AutoCorrect::getInstance()->setListPriority(listName,self->childGetValue("ac_priority").asInteger());

			//sInstance->updateEnabledStuff();
			self->updateItemsList();
			AutoCorrect::getInstance()->save();
		}
	}
}
void AutoCorrectFloater::deleteEntry(void* data)
{
	if ( data )
	{
		AutoCorrectFloater* self = ( AutoCorrectFloater* )data;
		if ( self )
		{

			std::string listName=self->namesList->getFirstSelected()->getColumn(0)->getValue().asString();

			if((self->entryList->getAllSelected().size())>0)
			{	
				std::string wrong= self->entryList->getFirstSelected()->getColumn(0)->getValue().asString();
       				AutoCorrect::getInstance()->removeEntryFromList(wrong,listName);
				self->updateItemsList();
				AutoCorrect::getInstance()->save();
			}
		}
	}
}
void AutoCorrectFloater::loadList(void* data)
{
	LLFilePicker& picker = LLFilePicker::instance();

	if(!picker.getOpenFile( LLFilePicker::FFLOAD_XML) )
	{return;
	}	
	llifstream file;
	file.open(picker.getFirstFile().c_str());
	LLSD blankllsd;
	if (file.is_open())
	{
		LLSDSerialize::fromXMLDocument(blankllsd, file);
	}
	file.close();
	gSavedSettings.setBOOL("EnableAutoCorrect",true);
	AutoCorrect::getInstance()->addCorrectionList(blankllsd);
	if ( data )
	{
		AutoCorrectFloater* self = ( AutoCorrectFloater* )data;
		if ( self )
			self->updateEnabledStuff();
	}
}
void AutoCorrectFloater::removeList(void* data)
{
	if ( data )
	{
		AutoCorrectFloater* self = ( AutoCorrectFloater* )data;
		if ( self )
		{
			std::string listName= self->namesList->getFirstSelected()->getColumn(0)->getValue().asString();
			AutoCorrect::getInstance()->removeCorrectionList(listName);
			self->updateEnabledStuff();
		}

	}
}
void AutoCorrectFloater::exportList(void *data)
{
	if ( data )
	{
		AutoCorrectFloater* self = ( AutoCorrectFloater* )data;
		if ( self )
		{
			std::string listName=self->namesList->getFirstSelected()->getColumn(0)->getValue().asString();

			LLFilePicker& picker = LLFilePicker::instance();

			if(!picker.getSaveFile( LLFilePicker::FFSAVE_XML) )
			{return;
			}	
			llofstream file;
			file.open(picker.getFirstFile().c_str());
			LLSDSerialize::toPrettyXML(AutoCorrect::getInstance()->exportList(listName), file);
			file.close();	
		}
	
	}
}
void AutoCorrectFloater::addEntry(void* data)
{
	if ( data )
	{
		AutoCorrectFloater* self = ( AutoCorrectFloater* )data;
		if ( self )
		{
			std::string listName= self->namesList->getFirstSelected()->getColumn(0)->getValue().asString();
			LLChat chat;
			chat.mText ="To add an entry, please type in chat \""+gSavedSettings.getString("CmdLineAutocorrect")+" "+listName+"|wrongWord|rightWord\"";

			chat.mSourceType = CHAT_SOURCE_SYSTEM;
			LLSD args;
			args["type"] = LLNotificationsUI::NT_NEARBYCHAT;
			LLNotificationsUI::LLNotificationManager::instance().onChat(chat, args);
			
		}
	}
	
}
AutoCorrectFloater* AutoCorrectFloater::showFloater()
{
	AutoCorrectFloater *floater = dynamic_cast<AutoCorrectFloater*>(LLFloaterReg::getInstance("autocorrect"));
	if(floater)
	{
		floater->setVisible(true);
		floater->setFrontmost(true);
		floater->center();
		return floater;
	}
	else
	{
		LL_WARNS("AutoCorrect") << "Can't find floater!" << LL_ENDL;
		return NULL;
	}
}
