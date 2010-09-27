/**
 * @file llpaneloutfitsinventory.h
 * @brief Outfits inventory panel
 * class definition
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
	void onOutfitsRemovalConfirmation(const LLSD& notification, const LLSD& response);
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
