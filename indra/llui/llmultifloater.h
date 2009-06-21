/** 
 * @file llmultifloater.h
 * @brief LLFloater that hosts other floaters
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

// Floating "windows" within the GL display, like the inventory floater,
// mini-map floater, etc.


#ifndef LL_MULTI_FLOATER_H
#define LL_MULTI_FLOATER_H

#include "llfloater.h"
#include "lltabcontainer.h" // for LLTabContainer::eInsertionPoint

// https://wiki.lindenlab.com/mediawiki/index.php?title=LLMultiFloater&oldid=81376
class LLMultiFloater : public LLFloater
{
public:
	LLMultiFloater(const LLFloater::Params& params = LLFloater::Params());
	virtual ~LLMultiFloater() {};
	
	void buildTabContainer();
	
	virtual BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void draw();
	/*virtual*/ void setVisible(BOOL visible);
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask);
	/*virtual*/ bool addChild(LLView* view, S32 tab_group = 0);

	virtual void setCanResize(BOOL can_resize);
	virtual void growToFit(S32 content_width, S32 content_height);
	virtual void addFloater(LLFloater* floaterp, BOOL select_added_floater, LLTabContainer::eInsertionPoint insertion_point = LLTabContainer::END);

	virtual void showFloater(LLFloater* floaterp, LLTabContainer::eInsertionPoint insertion_point = LLTabContainer::END);
	virtual void removeFloater(LLFloater* floaterp);

	virtual void tabOpen(LLFloater* opened_floater, bool from_click);
	virtual void tabClose();

	virtual BOOL selectFloater(LLFloater* floaterp);
	virtual void selectNextFloater();
	virtual void selectPrevFloater();

	virtual LLFloater*	getActiveFloater();
	virtual BOOL		isFloaterFlashing(LLFloater* floaterp);
	virtual S32			getFloaterCount();

	virtual void setFloaterFlashing(LLFloater* floaterp, BOOL flashing);
	virtual BOOL closeAllFloaters();	//Returns FALSE if the floater could not be closed due to pending confirmation dialogs
	void setTabContainer(LLTabContainer* tab_container) { if (!mTabContainer) mTabContainer = tab_container; }
	void onTabSelected();

	virtual void updateResizeLimits();

protected:
	struct LLFloaterData
	{
		S32		mWidth;
		S32		mHeight;
		BOOL	mCanMinimize;
		BOOL	mCanResize;
	};

	LLTabContainer*		mTabContainer;
	
	typedef std::map<LLHandle<LLFloater>, LLFloaterData> floater_data_map_t;
	floater_data_map_t	mFloaterDataMap;
	
	LLTabContainer::TabPosition mTabPos;
	BOOL				mAutoResize;
	S32					mOrigMinWidth, mOrigMinHeight;  // logically const but initialized late
};

#endif  // LL_MULTI_FLOATER_H



