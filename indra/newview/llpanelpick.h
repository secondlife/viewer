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
#include "llremoteparcelrequest.h"
#include "llavatarpropertiesprocessor.h"

class LLIconCtrl;
class LLTextureCtrl;
class LLScrollContainer;
class LLMessageSystem;
class LLAvatarPropertiesObserver;

/**
 * Panel for displaying Pick Information - snapshot, name, description, etc.
 */
class LLPanelPickInfo : public LLPanel, public LLAvatarPropertiesObserver, LLRemoteParcelInfoObserver
{
	LOG_CLASS(LLPanelPickInfo);
public:
	
	// Creates new panel
	static LLPanelPickInfo* create();

	virtual ~LLPanelPickInfo();

	/**
	 * Initializes panel properties
	 *
	 * By default Pick will be created for current Agent location.
	 * Use setPickData to change Pick properties.
	 */
	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	/**
	 * Sends remote parcel info request to resolve parcel name from its ID.
	 */
	void sendParcelInfoRequest();

	/**
	 * Sets "Back" button click callback
	 */
	virtual void setExitCallback(const commit_callback_t& cb);

	/**
	 * Sets "Edit" button click callback
	 */
	virtual void setEditPickCallback(const commit_callback_t& cb);

	//This stuff we got from LLRemoteParcelObserver, in the last one we intentionally do nothing
	/*virtual*/ void processParcelInfo(const LLParcelData& parcel_data);
	/*virtual*/ void setParcelID(const LLUUID& parcel_id) { mParcelId = parcel_id; }
	/*virtual*/ void setErrorStatus(U32 status, const std::string& reason) {};

protected:

	LLPanelPickInfo();
	
	/**
	 * Resets Pick information
	 */
	virtual void resetData();

	/**
	 * Resets UI controls (visibility, values)
	 */
	virtual void resetControls();

	/** 
	* "Location text" is actually the owner name, the original
	* name that owner gave the parcel, and the location.
	*/
	static std::string createLocationText(
		const std::string& owner_name, 
		const std::string& original_name,
		const std::string& sim_name, 
		const LLVector3d& pos_global);

	virtual void setAvatarId(const LLUUID& avatar_id) { mAvatarId = avatar_id; }
	virtual LLUUID& getAvatarId() { return mAvatarId; }

	/**
	 * Sets snapshot id.
	 *
	 * Will mark snapshot control as valid if id is not null.
	 * Will mark snapshot control as invalid if id is null. If null id is a valid value,
	 * you have to manually mark snapshot is valid.
	 */
	virtual void setSnapshotId(const LLUUID& id);
	
	virtual void setPickId(const LLUUID& id) { mPickId = id; }
	virtual LLUUID& getPickId() { return mPickId; }
	
	virtual void setPickName(const std::string& name);
	
	virtual void setPickDesc(const std::string& desc);
	
	virtual void setPickLocation(const std::string& location);
	
	virtual void setPosGlobal(const LLVector3d& pos) { mPosGlobal = pos; }
	virtual LLVector3d& getPosGlobal() { return mPosGlobal; }

	/**
	 * Callback for "Map" button, opens Map
	 */
	void onClickMap();

	/**
	 * Callback for "Teleport" button, teleports user to Pick location.
	 */
	void onClickTeleport();

	void onClickBack();

protected:

	S32						mScrollingPanelMinHeight;
	S32						mScrollingPanelWidth;
	LLScrollContainer*		mScrollContainer;
	LLPanel*				mScrollingPanel;
	LLTextureCtrl*			mSnapshotCtrl;

	LLUUID mAvatarId;
	LLVector3d mPosGlobal;
	LLUUID mParcelId;
	LLUUID mPickId;
	LLUUID mRequestedId;
};

/**
 * Panel for creating/editing Pick.
 */
class LLPanelPickEdit : public LLPanelPickInfo
{
	LOG_CLASS(LLPanelPickEdit);
public:

	/**
	 * Creates new panel
	 */
	static LLPanelPickEdit* create();

	/*virtual*/ ~LLPanelPickEdit();

	/*virtual*/ void onOpen(const LLSD& key);

	virtual void setPickData(const LLPickData* pick_data);

	/*virtual*/ BOOL postBuild();

	/**
	 * Sets "Save" button click callback
	 */
	virtual void setSaveCallback(const commit_callback_t& cb);

	/**
	 * Sets "Cancel" button click callback
	 */
	virtual void setCancelCallback(const commit_callback_t& cb);

	/**
	 * Resets panel and all cantrols to unedited state
	 */
	/*virtual*/ void resetDirty();

	/**
	 * Returns true if any of Pick properties was changed by user.
	 */
	/*virtual*/ BOOL isDirty() const;

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

protected:

	LLPanelPickEdit();

	/**
	 * Sends Pick properties to server.
	 */
	void sendUpdate();

	/**
	 * Called when snapshot image changes.
	 */
	void onSnapshotChanged();
	
	/**
	 * Callback for Pick snapshot, name and description changed event.
	 */
	void onPickChanged(LLUICtrl* ctrl);

	/*virtual*/ void resetData();

	/**
	 * Enables/disables "Save" button
	 */
	void enableSaveButton(bool enable);

	/**
	 * Callback for "Set Location" button click
	 */
	void onClickSetLocation();

	/**
	 * Callback for "Save" button click
	 */
	void onClickSave();

	std::string getLocationNotice();

protected:

	bool mLocationChanged;
	bool mNeedData;
	bool mNewPick;

private:

	void initTexturePickerMouseEvents();
        void onTexturePickerMouseEnter(LLUICtrl* ctrl);
	void onTexturePickerMouseLeave(LLUICtrl* ctrl);

private:

	LLIconCtrl* text_icon;
};

#endif // LL_LLPANELPICK_H
