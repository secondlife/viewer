/** 
 * @file llpanelpicks.h
 * @brief LLPanelPicks and related class definitions
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

#ifndef LL_LLPANELPICKS_H
#define LL_LLPANELPICKS_H

#include "llpanel.h"
#include "v3dmath.h"
#include "lluuid.h"
#include "llavatarpropertiesprocessor.h"
#include "llpanelavatar.h"
#include "llregistry.h"

class LLAccordionCtrlTab;
class LLPanelProfile;
class LLMessageSystem;
class LLVector3d;
class LLPanelProfileTab;
class LLAgent;
class LLMenuGL;
class LLPickItem;
class LLClassifiedItem;
class LLFlatListView;
class LLPanelPickInfo;
class LLPanelPickEdit;
class LLToggleableMenu;
class LLPanelClassifiedInfo;
class LLPanelClassifiedEdit;
class LLClassifiedClickThrough;

// *TODO
// Panel Picks has been consolidated with Classifieds (EXT-2095), give LLPanelPicks
// and corresponding files (cpp, h, xml) a new name. (new name is TBD at the moment)

class LLPanelPicks 
	: public LLPanelProfileTab
{
public:
	LLPanelPicks();
	~LLPanelPicks();

	static void* create(void* data);

	/*virtual*/ BOOL postBuild(void);

	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ void onClosePanel();

	void processProperties(void* data, EAvatarProcessorType type);

	void updateData();

	// returns the selected pick item
	LLPickItem* getSelectedPickItem();
	LLClassifiedItem* getSelectedClassifiedItem();

	//*NOTE top down approch when panel toggling is done only by 
	// parent panels failed to work (picks related code was in my profile panel)
	void setProfilePanel(LLPanelProfile* profile_panel);

protected:
	/*virtual*/void updateButtons();

private:
	void onClickDelete();
	void onClickTeleport();
	void onClickMap();

	void onPlusMenuItemClicked(const LLSD& param);
	bool isActionEnabled(const LLSD& userdata) const;

	void onListCommit(const LLFlatListView* f_list);
	void onAccordionStateChanged(const LLAccordionCtrlTab* acc_tab);

	//------------------------------------------------
	// Callbacks which require panel toggling
	//------------------------------------------------
	void onClickPlusBtn();
	void onClickInfo();
	void onPanelPickClose(LLPanel* panel);
	void onPanelPickSave(LLPanel* panel);
	void onPanelClassifiedSave(LLPanelClassifiedEdit* panel);
	void onPanelClassifiedClose(LLPanelClassifiedInfo* panel);
	void onPanelPickEdit();
	void onPanelClassifiedEdit();
	void onClickMenuEdit();

	void createNewPick();
	void createNewClassified();

	void openPickInfo();
	void openClassifiedInfo();
	void openClassifiedInfo(const LLUUID &classified_id, const LLUUID &avatar_id,
							const LLUUID &snapshot_id, const std::string &name,
							const std::string &desc);
	friend class LLPanelProfile;

	void showAccordion(const std::string& name, bool show);

	void buildPickPanel();

	bool callbackDeletePick(const LLSD& notification, const LLSD& response);
	bool callbackDeleteClassified(const LLSD& notification, const LLSD& response);
	bool callbackTeleport(const LLSD& notification, const LLSD& response);


	virtual void onDoubleClickPickItem(LLUICtrl* item);
	virtual void onDoubleClickClassifiedItem(LLUICtrl* item);
	virtual void onRightMouseUpItem(LLUICtrl* item, S32 x, S32 y, MASK mask);

	LLPanelProfile* getProfilePanel();

	void createPickInfoPanel();
	void createPickEditPanel();
	void createClassifiedInfoPanel();
	void createClassifiedEditPanel();

	LLMenuGL* mPopupMenu;
	LLPanelProfile* mProfilePanel;
	LLPanelPickInfo* mPickPanel;
	LLFlatListView* mPicksList;
	LLFlatListView* mClassifiedsList;
	LLPanelPickInfo* mPanelPickInfo;
	LLPanelClassifiedInfo* mPanelClassifiedInfo;
	LLPanelClassifiedEdit* mPanelClassifiedEdit;
	LLPanelPickEdit* mPanelPickEdit;
	LLToggleableMenu* mPlusMenu;

	LLAccordionCtrlTab* mPicksAccTab;
	LLAccordionCtrlTab* mClassifiedsAccTab;

	LLClassifiedClickThrough* mClickThroughDisp;
	
	//true if picks list is empty after processing picks
	bool mNoPicks;
	//true if classifieds list is empty after processing classifieds
	bool mNoClassifieds;
};

class LLPickItem : public LLPanel, public LLAvatarPropertiesObserver
{
public:

	LLPickItem();

	static LLPickItem* create();

	void init(LLPickData* pick_data);

	void setPickName(const std::string& name);

	void setPickDesc(const std::string& descr);

	void setPickId(const LLUUID& id);

	void setCreatorId(const LLUUID& id) {mCreatorID = id;};

	void setSnapshotId(const LLUUID& id) {mSnapshotID = id;};

	void setNeedData(bool need){mNeedData = need;};

	const LLUUID& getPickId(); 

	const std::string& getPickName();

	const LLUUID& getCreatorId();

	const LLUUID& getSnapshotId();

	const LLVector3d& getPosGlobal();

	const std::string getDescription();

	const std::string& getSimName() { return mSimName; }

	const std::string& getUserName() { return mUserName; }

	const std::string& getOriginalName() { return mOriginalName; }

	const std::string& getPickDesc() { return mPickDescription; }

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	void update();

	~LLPickItem();

	/*virtual*/ BOOL postBuild();

	/** setting on/off background icon to indicate selected state */
	/*virtual*/ void setValue(const LLSD& value);

protected:

	LLUUID mPickID;
	LLUUID mCreatorID;
	LLUUID mParcelID;
	LLUUID mSnapshotID;
	LLVector3d mPosGlobal;
	bool mNeedData;

	std::string mPickName;
	std::string mUserName;
	std::string mOriginalName;
	std::string mPickDescription;
	std::string mSimName;
};

class LLClassifiedItem : public LLPanel, public LLAvatarPropertiesObserver
{
public:

	LLClassifiedItem(const LLUUID& avatar_id, const LLUUID& classified_id);
	
	virtual ~LLClassifiedItem();

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void setValue(const LLSD& value);

	LLUUID getAvatarId() {return mAvatarId;}
	
	void setAvatarId(const LLUUID& avatar_id) {mAvatarId = avatar_id;}

	LLUUID getClassifiedId() {return mClassifiedId;}

	void setClassifiedId(const LLUUID& classified_id) {mClassifiedId = classified_id;}

	void setPosGlobal(const LLVector3d& pos) { mPosGlobal = pos; }

	const LLVector3d& getPosGlobal() { return mPosGlobal; }

	void setClassifiedName (const std::string& name);

	std::string getClassifiedName() { return childGetValue("name").asString(); }

	void setDescription(const std::string& desc);

	std::string getDescription() { return childGetValue("description").asString(); }

	void setSnapshotId(const LLUUID& snapshot_id);

	LLUUID getSnapshotId();

private:
	LLUUID mAvatarId;
	LLUUID mClassifiedId;
	LLVector3d mPosGlobal;
};

#endif // LL_LLPANELPICKS_H
