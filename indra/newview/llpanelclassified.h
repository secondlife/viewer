/** 
 * @file llpanelclassified.h
 * @brief LLPanelClassified class definition
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

// Display of a classified used both for the global view in the
// Find directory, and also for each individual user's classified in their
// profile.
#ifndef LL_LLPANELCLASSIFIED_H
#define LL_LLPANELCLASSIFIED_H

#include "llavatarpropertiesprocessor.h"
#include "llclassifiedinfo.h"
#include "llfloater.h"
#include "llpanel.h"
#include "llrect.h"
#include "lluuid.h"
#include "v3dmath.h"

class LLScrollContainer;
class LLTextureCtrl;
class LLUICtrl;

class LLPublishClassifiedFloater : public LLFloater
{
public:
	LLPublishClassifiedFloater(const LLSD& key);
	virtual ~LLPublishClassifiedFloater();

	/*virtual*/ BOOL postBuild();

	void setPrice(S32 price);
	S32 getPrice();

	void setPublishClickedCallback(const commit_signal_t::slot_type& cb);
	void setCancelClickedCallback(const commit_signal_t::slot_type& cb);

private:
};

class LLPanelClassifiedInfo : public LLPanel, public LLAvatarPropertiesObserver
{
	LOG_CLASS(LLPanelClassifiedInfo);
public:

	static LLPanelClassifiedInfo* create();

	virtual ~LLPanelClassifiedInfo();

	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	void setAvatarId(const LLUUID& avatar_id) { mAvatarId = avatar_id; }

	LLUUID& getAvatarId() { return mAvatarId; }

	void setSnapshotId(const LLUUID& id);

	LLUUID getSnapshotId();

	void setClassifiedId(const LLUUID& id) { mClassifiedId = id; }

	LLUUID& getClassifiedId() { return mClassifiedId; }

	void setClassifiedName(const std::string& name);

	std::string getClassifiedName();

	void setDescription(const std::string& desc);

	std::string getDescription();

	void setClassifiedLocation(const std::string& location);

	std::string getClassifiedLocation();

	void setPosGlobal(const LLVector3d& pos) { mPosGlobal = pos; }

	LLVector3d& getPosGlobal() { return mPosGlobal; }

	void setParcelId(const LLUUID& id) { mParcelId = id; }

	LLUUID getParcelId() { return mParcelId; }

	void setSimName(const std::string& sim_name) { mSimName = sim_name; }

	std::string getSimName() { return mSimName; }

	void setFromSearch(bool val) { mFromSearch = val; }

	bool fromSearch() { return mFromSearch; }

	bool getInfoLoaded() { return mInfoLoaded; }

	void setInfoLoaded(bool loaded) { mInfoLoaded = loaded; }

	static void setClickThrough(
		const LLUUID& classified_id,
		S32 teleport,
		S32 map,
		S32 profile,
		bool from_new_table);

	static void sendClickMessage(
			const std::string& type,
			bool from_search,
			const LLUUID& classified_id,
			const LLUUID& parcel_id,
			const LLVector3d& global_pos,
			const std::string& sim_name);

	void setExitCallback(const commit_callback_t& cb);

	void setEditClassifiedCallback(const commit_callback_t& cb);

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	/*virtual*/ void draw();

protected:

	LLPanelClassifiedInfo();

	virtual void resetData();

	virtual void resetControls();

	static std::string createLocationText(
		const std::string& original_name,
		const std::string& sim_name, 
		const LLVector3d& pos_global);

	void stretchSnapshot();
	void sendClickMessage(const std::string& type);

	LLRect getDefaultSnapshotRect();

	void scrollToTop();

	void onMapClick();
	void onTeleportClick();
	void onExit();

	bool mSnapshotStreched;
	LLRect mSnapshotRect;
	LLTextureCtrl* mSnapshotCtrl;

private:

	LLUUID mAvatarId;
	LLUUID mClassifiedId;
	LLVector3d mPosGlobal;
	LLUUID mParcelId;
	std::string mSimName;
	bool mFromSearch;
	bool mInfoLoaded;

	LLScrollContainer*		mScrollContainer;
	LLPanel*				mScrollingPanel;

	S32 mScrollingPanelMinHeight;
	S32 mScrollingPanelWidth;

	// Needed for stat tracking
	S32 mTeleportClicksOld;
	S32 mMapClicksOld;
	S32 mProfileClicksOld;
	S32 mTeleportClicksNew;
	S32 mMapClicksNew;
	S32 mProfileClicksNew;

	typedef std::list<LLPanelClassifiedInfo*> panel_list_t;
	static panel_list_t sAllPanels;
};

class LLPanelClassifiedEdit : public LLPanelClassifiedInfo
{
	LOG_CLASS(LLPanelClassifiedEdit);
public:

	static LLPanelClassifiedEdit* create();

	virtual ~LLPanelClassifiedEdit();

	/*virtual*/ BOOL postBuild();

	void fillIn(const LLSD& key);

	/*virtual*/ void onOpen(const LLSD& key);

	/*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

	/*virtual*/ BOOL isDirty() const;

	/*virtual*/ void resetDirty();

	void setSaveCallback(const commit_signal_t::slot_type& cb);

	void setCancelCallback(const commit_signal_t::slot_type& cb);

	/*virtual*/ void resetControls();

	bool isNew() { return mIsNew; }

	bool isNewWithErrors() { return mIsNewWithErrors; }

	bool canClose();

	void draw();

	void stretchSnapshot();

	U32 getCategory();

	void setCategory(U32 category);

	U32 getContentType();

	void setContentType(U32 content_type);

	bool getAutoRenew();

	S32 getPriceForListing();

protected:

	LLPanelClassifiedEdit();

	void sendUpdate();

	void enableVerbs(bool enable);

	void enableEditing(bool enable);

	void showEditing(bool show);

	std::string makeClassifiedName();

	void setPriceForListing(S32 price);

	U8 getFlags();

	std::string getLocationNotice();

	bool isValidName();

	void notifyInvalidName();

	void onSetLocationClick();
	void onChange();
	void onSaveClick();

	void doSave();

	void onPublishFloaterPublishClicked();

	void onTexturePickerMouseEnter(LLUICtrl* ctrl);
	void onTexturePickerMouseLeave(LLUICtrl* ctrl);

	void onTextureSelected();

private:
	bool mIsNew;
	bool mIsNewWithErrors;
	bool mCanClose;

	LLPublishClassifiedFloater* mPublishFloater;

	commit_signal_t mSaveButtonClickedSignal;
};

#endif // LL_LLPANELCLASSIFIED_H
