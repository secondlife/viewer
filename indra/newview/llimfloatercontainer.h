/** 
 * @file llimfloatercontainer.h
 * @brief Multifloater containing active IM sessions in separate tab container tabs
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLIMFLOATERCONTAINER_H
#define LL_LLIMFLOATERCONTAINER_H

#include <map>
#include <vector>

#include "llfloater.h"
#include "llmultifloater.h"
#include "llavatarpropertiesprocessor.h"
#include "llgroupmgr.h"

class LLTabContainer;

class LLIMFloaterContainer : public LLMultiFloater
{
public:
	LLIMFloaterContainer(const LLSD& seed);
	virtual ~LLIMFloaterContainer();
	
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	void onCloseFloater(LLUUID& id);

	/*virtual*/ void addFloater(LLFloater* floaterp, 
								BOOL select_added_floater, 
								LLTabContainer::eInsertionPoint insertion_point = LLTabContainer::END);

	static LLFloater* getCurrentVoiceFloater();

	static LLIMFloaterContainer* findInstance();

	static LLIMFloaterContainer* getInstance();

private:
	typedef std::map<LLUUID,LLFloater*> avatarID_panel_map_t;
	avatarID_panel_map_t mSessions;


	void onNewMessageReceived(const LLSD& data);
};

#endif // LL_LLIMFLOATERCONTAINER_H
