/** 
 * @file llpanelpicks.h
 * @brief LLPanelPicks and related class definitions
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

class LLMessageSystem;
class LLVector3d;
class LLPanelProfileTab;
class LLPanelMeProfile;
class LLPanelPick;
class LLAgent;
class LLPickItem;


class LLPanelPicks 
	: public LLPanelProfileTab
{
public:
	LLPanelPicks(const LLUUID& avatar_id = LLUUID::null);
	LLPanelPicks(const Params& params );
	~LLPanelPicks();

	static void* create(void* data);

	static void teleport(const LLVector3d& position);

	static void showOnMap(const LLVector3d& position);
	
	/*virtual*/ BOOL postBuild(void);

	/*virtual*/ void onActivate(const LLUUID& id);

	void processProperties(void* data, EAvatarProcessorType type);

	void updateData();

	void setPanelMeProfile(LLPanelMeProfile*);

	void clear();

	//*TODO implement
	//LLPickItem& getSelectedPick();

private:
	static void onClickInfo(void* data);
	static void onClickNew(void* data);
	static void onClickDelete(void* data);
	static void onClickTeleport(void* data);
	static void onClickMap(void* data);

	bool callbackDelete(const LLSD& notification, const LLSD& response);

	void updateButtons();

	typedef std::vector<LLPickItem*> picture_list_t;
	picture_list_t mPickItemList;
	LLPanelMeProfile* mMeProfilePanel;

};

class LLPickItem : public LLPanel, public LLAvatarPropertiesObserver
{
public:

	LLPickItem();

	static LLPickItem* create();

	void init(LLPickData* pick_data);

	void setPictureName(const std::string& name);

	void setPictureDescription(const std::string& descr);

	void setPicture();

	void setPictureId(const LLUUID& id);

	void setCreatorId(const LLUUID& id) {mCreatorID = id;};

	void setSnapshotId(const LLUUID& id) {mSnapshotID = id;};

	void setNeedData(bool need){mNeedData = need;};

	const LLUUID& getPickId(); 

	const std::string& getPickName();

	const LLUUID& getCreatorId();

	const LLUUID& getSnapshotId();

	const LLVector3d& getPosGlobal();

	const std::string& getLocation();

	const std::string getDescription();

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	void update();

	~LLPickItem();

protected:

	LLUUID mPicID;
	LLUUID mCreatorID;
	LLUUID mParcelID;
	LLUUID mSnapshotID;
	LLVector3d mPosGlobal;
	bool mNeedData;

	std::string mPickName;
	std::string mLocation;
};

#endif // LL_LLPANELPICKS_H
