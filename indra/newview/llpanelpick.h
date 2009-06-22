/** 
 * @file llpanelpick.h
 * @brief LLPanelPick class definition
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
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

// Display of a "Top Pick" used both for the global top picks in the 
// Find directory, and also for each individual user's picks in their
// profile.

#ifndef LL_LLPANELPICK_H
#define LL_LLPANELPICK_H

#include "llpanel.h"

class LLTextureCtrl;
class LLMessageSystem;
class LLPanelMeProfile;
class LLAvatarPropertiesObserver;

class LLPanelPick : public LLPanel, public LLAvatarPropertiesObserver
{
	LOG_CLASS(LLPanelPick);
public:
	LLPanelPick(BOOL edit_mode = FALSE);
	/*virtual*/ ~LLPanelPick();

	void reset();

	/*virtual*/ BOOL postBuild();

	// Create a new pick, including creating an id, giving a sane
	// initial position, etc.
	void createNewPick();

	void init(LLUUID creator_id, LLUUID pick_id);

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	void setEditMode(BOOL edit_mode);

	//TODO redo panel toggling
	void setPanelMeProfile(LLPanelMeProfile* meProfilePanel);

protected:

	void setName(std::string name);
	void setDesc(std::string desc);
	void setLocation(std::string location);

	std::string getName();
	std::string getDesc();
	std::string getLocation();

	void sendUpdate();
	void init(LLPickData *pick_data);

	//-----------------------------------------
	// "PICK INFO" (VIEW MODE) BUTTON HANDLERS
	//-----------------------------------------
	static void onClickEdit(void* data);
	static void onClickTeleport(void* data);
	static void onClickMap(void* data);
	static void onClickBack(void* data);

	//-----------------------------------------
	// "EDIT PICK" (EDIT MODE) BUTTON HANDLERS
	//-----------------------------------------
	static void onClickSet(void* data);
	static void onClickSave(void* data);
	static void onClickCancel(void* data);

protected:
	BOOL mEditMode;
	LLTextureCtrl*	mSnapshotCtrl;
	BOOL mDataRequested;
	BOOL mDataReceived;

	LLUUID mPickId;
	LLUUID mCreatorId;
	LLVector3d mPosGlobal;
	LLUUID mParcelId;
	std::string mSimName;

	//TODO redo panel toggling
	LLPanelMeProfile* mMeProfilePanel;
};

#endif // LL_LLPANELPICK_H
