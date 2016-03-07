/**
 * @file llpaneloutfitsinventory.h
 * @brief Outfits inventory panel
 * class definition
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

#ifndef LL_LLPANELOUTFITSINVENTORY_H
#define LL_LLPANELOUTFITSINVENTORY_H

#include "llpanel.h"

class LLOutfitsList;
class LLOutfitListGearMenu;
class LLPanelAppearanceTab;
class LLPanelWearing;
class LLMenuGL;
class LLSidepanelAppearance;
class LLTabContainer;
class LLSaveOutfitComboBtn;

class LLPanelOutfitsInventory : public LLPanel
{
	LOG_CLASS(LLPanelOutfitsInventory);
public:
	LLPanelOutfitsInventory();
	virtual ~LLPanelOutfitsInventory();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	
	void onSearchEdit(const std::string& string);
	void onSave();
	
	bool onSaveCommit(const LLSD& notification, const LLSD& response);

	static LLSidepanelAppearance* getAppearanceSP();

	static LLPanelOutfitsInventory* findInstance();

protected:
	void updateVerbs();

private:
	LLTabContainer*			mAppearanceTabs;
	std::string 			mFilterSubString;
	std::auto_ptr<LLSaveOutfitComboBtn> mSaveComboBtn;

	//////////////////////////////////////////////////////////////////////////////////
	// tab panels                                                                   //
protected:
	void 					initTabPanels();
	void 					onTabChange();
	bool 					isCOFPanelActive() const;

private:
	LLPanelAppearanceTab*	mActivePanel;
	LLOutfitsList*			mMyOutfitsPanel;
	LLPanelWearing*			mCurrentOutfitPanel;

	// tab panels                                                                   //
	//////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////
	// List Commands                                                                //
protected:
	void initListCommandsHandlers();
	void updateListCommands();
	void onWearButtonClick();
	void showGearMenu();
	void onTrashButtonClick();
	bool isActionEnabled(const LLSD& userdata);
	void setWearablesLoading(bool val);
	void onWearablesLoaded();
	void onWearablesLoading();
private:
	LLPanel*					mListCommands;
	LLMenuGL*					mMenuAdd;
	// List Commands                                                                //
	//////////////////////////////////////////////////////////////////////////////////

	bool mInitialized;
};

#endif //LL_LLPANELOUTFITSINVENTORY_H
