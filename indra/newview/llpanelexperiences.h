/** 
 * @file llpanelpicks.h
 * @brief LLPanelPicks and related class definitions
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_LLPANELEXPERIENCES_H
#define LL_LLPANELEXPERIENCES_H

#include "llaccordionctrltab.h"
#include "llflatlistview.h"
#include "llpanelavatar.h"

class LLExperienceData;
class LLExperienceItem;
class LLPanelProfile; 

class LLPanelExperienceInfo
	: public LLPanel
{
public:
	static LLPanelExperienceInfo* create();
	
	void onOpen(const LLSD& key);
	void setExperienceName( const std::string& name );
	void setExperienceDesc( const std::string& desc );


	virtual void setExitCallback(const commit_callback_t& cb);
};


class LLPanelExperiences
	: public LLPanel /*LLPanelProfileTab*/
{
public:
	LLPanelExperiences();

	static void* create(void* data);

	/*virtual*/ BOOL postBuild(void);

	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ void onClosePanel();

	void updateData();

	LLExperienceItem* getSelectedExperienceItem();

	void setProfilePanel(LLPanelProfile* profile_panel);

protected:

	void onListCommit(const LLFlatListView* f_list);
	void onAccordionStateChanged(const LLAccordionCtrlTab* acc_tab);


	void openExperienceInfo();
	void createExperienceInfoPanel();
	void onPanelExperienceClose(LLPanel* panel);
	LLPanelProfile* getProfilePanel();
private:
	LLFlatListView* mExperiencesList;
	LLAccordionCtrlTab* mExperiencesAccTab;
	LLPanelProfile* mProfilePanel;
	LLPanelExperienceInfo* mPanelExperienceInfo;
	bool mNoExperiences;
};


class LLExperienceItem 
	: public LLPanel
	//, public LLAvatarPropertiesObserver
{
public:
	LLExperienceItem();
	~LLExperienceItem();

	void init(LLExperienceData* experience_data);
	/*virtual*/ BOOL postBuild();
	void update();

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	void setCreatorID(const LLUUID& val) { mCreatorID = val; }
	void setExperienceDescription(const std::string& val);
	void setExperienceName(const std::string& val);

	const LLUUID& getCreatorID() const { return mCreatorID; }
	const std::string& getExperienceName() const { return mExperienceName; }
	const std::string& getExperienceDescription() const { return mExperienceDescription; }

protected:
	LLUUID mCreatorID;

	std::string mExperienceName;
	std::string mExperienceDescription;
};
#endif // LL_LLPANELEXPERIENCES_H
