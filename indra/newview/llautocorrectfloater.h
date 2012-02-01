/** 
 * @file llautocorrectfloater.h
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

#ifndef AUTOCORRECTFLOATER_H
#define AUTOCORRECTFLOATER_H

#include "llfloater.h"
#include "llmediactrl.h"
#include "llscrolllistctrl.h"

#include "llviewerinventory.h"
#include <boost/bind.hpp>

class AutoCorrectFloater : 
public LLFloater
{
public:
	AutoCorrectFloater(const LLSD& key);

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onClose(bool app_quitting);

	static AutoCorrectFloater* showFloater();

	void setData(void * data);
	void updateEnabledStuff();
	void updateNamesList();
	void updateListControlsEnabled(BOOL selected);
	void updateItemsList();

	LLScrollListCtrl *namesList;
	LLScrollListCtrl *entryList;
	//LLPanel * empanel;
private:
	//static JCInvDropTarget* mNotecardDropTarget;
	static void onBoxCommitEnabled(LLUICtrl* caller, void* user_data);
	static void onEntrySettingChange(LLUICtrl* caller, void* user_data);
	static void onSelectName(LLUICtrl* caller, void* user_data);
	//static void ResponseItemDrop(LLViewerInventoryItem* item);
	//static void onNotecardLoadComplete(LLVFS *vfs,const LLUUID& asset_uuid,LLAssetType::EType type,void* user_data, S32 status, LLExtStat ext_status);


	static void deleteEntry(void* data);
	static void addEntry(void* data);
	static void exportList(void* data);
	static void removeList(void* data);
	static void loadList(void* data);
};

#endif  // AUTOCORRECTFLOATER_H
