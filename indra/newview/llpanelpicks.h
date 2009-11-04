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

class LLPanelProfile;
class LLMessageSystem;
class LLVector3d;
class LLPanelProfileTab;
class LLAgent;
class LLMenuGL;
class LLPickItem;
class LLFlatListView;
class LLPanelPickInfo;
class LLPanelPickEdit;
class LLToggleableMenu;

class LLPanelPicks 
	: public LLPanelProfileTab
{
public:
	LLPanelPicks();
	~LLPanelPicks();

	static void* create(void* data);

	/*virtual*/ BOOL postBuild(void);

	/*virtual*/ void onOpen(const LLSD& key);

	void processProperties(void* data, EAvatarProcessorType type);

	void updateData();

	// returns the selected pick item
	LLPickItem* getSelectedPickItem();

	//*NOTE top down approch when panel toggling is done only by 
	// parent panels failed to work (picks related code was in me profile panel)
	void setProfilePanel(LLPanelProfile* profile_panel);

private:
	void onClickDelete();
	void onClickTeleport();
	void onClickMap();

	void onOverflowMenuItemClicked(const LLSD& param);
	void onOverflowButtonClicked();

	//------------------------------------------------
	// Callbacks which require panel toggling
	//------------------------------------------------
	void onClickNew();
	void onClickInfo();
	void onPanelPickClose(LLPanel* panel);
	void onPanelPickSave(LLPanel* panel);
	void onPanelPickEdit();
	void onClickMenuEdit();

	void buildPickPanel();

	bool callbackDelete(const LLSD& notification, const LLSD& response);
	bool callbackTeleport(const LLSD& notification, const LLSD& response);

	void updateButtons();

	virtual void onDoubleClickItem(LLUICtrl* item);
	virtual void onRightMouseUpItem(LLUICtrl* item, S32 x, S32 y, MASK mask);

	LLPanelProfile* getProfilePanel();

	void createPickInfoPanel();
	void createPickEditPanel();
// 	void openPickEditPanel(LLPickItem* pick);
// 	void openPickInfoPanel(LLPickItem* pick);

	LLMenuGL* mPopupMenu;
	LLPanelProfile* mProfilePanel;
	LLPanelPickInfo* mPickPanel;
	LLFlatListView* mPicksList;
	LLPanelPickInfo* mPanelPickInfo;
	LLPanelPickEdit* mPanelPickEdit;
	LLToggleableMenu* mOverflowMenu;
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

#endif // LL_LLPANELPICKS_H
