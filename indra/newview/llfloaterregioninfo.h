/** 
 * @file llfloaterregioninfo.h
 * @author Aaron Brashears
 * @brief Declaration of the region info and controls floater and panels.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLFLOATERREGIONINFO_H
#define LL_LLFLOATERREGIONINFO_H

#include <vector>
#include "llfloater.h"
#include "llpanel.h"

class LLLineEditor;
class LLMessageSystem;
class LLPanelRegionInfo;
class LLTabContainer;
class LLViewerRegion;
class LLViewerTextEditor;
class LLInventoryItem;
class LLCheckBoxCtrl;
class LLComboBox;
class LLNameListCtrl;
class LLSliderCtrl;
class LLSpinCtrl;
class LLTextBox;

class LLPanelRegionGeneralInfo;
class LLPanelRegionDebugInfo;
class LLPanelRegionTextureInfo;
class LLPanelRegionTerrainInfo;
class LLPanelEstateInfo;
class LLPanelEstateCovenant;

class LLFloaterRegionInfo : public LLFloater, public LLFloaterSingleton<LLFloaterRegionInfo>
{
	friend class LLUISingleton<LLFloaterRegionInfo, VisibilityPolicy<LLFloater> >;
public:
	~LLFloaterRegionInfo();

	/*virtual*/ void onOpen();
	/*virtual*/ BOOL postBuild();

	static void processEstateOwnerRequest(LLMessageSystem* msg, void**);

	// get and process region info if necessary.
	static void processRegionInfo(LLMessageSystem* msg);

	static const LLUUID& getLastInvoice() { return sRequestInvoice; }
	static void nextInvoice() { sRequestInvoice.generate(); }
	//static S32 getSerial() { return sRequestSerial; }
	//static void incrementSerial() { sRequestSerial++; }

	static LLPanelEstateInfo* getPanelEstate();
	static LLPanelEstateCovenant* getPanelCovenant();

	// from LLPanel
	virtual void refresh();
	
	static void requestRegionInfo();

protected:
	LLFloaterRegionInfo(const LLSD& seed);
	void refreshFromRegion(LLViewerRegion* region);

	// member data
	LLTabContainer* mTab;
	typedef std::vector<LLPanelRegionInfo*> info_panels_t;
	info_panels_t mInfoPanels;
	//static S32 sRequestSerial;	// serial # of last EstateOwnerRequest
	static LLUUID sRequestInvoice;
};


// Base class for all region information panels.
class LLPanelRegionInfo : public LLPanel
{
public:
	LLPanelRegionInfo() : LLPanel(std::string("Region Info Panel")) {}
	static void onBtnSet(void* user_data);
	static void onChangeChildCtrl(LLUICtrl* ctrl, void* user_data);
	static void onChangeAnything(LLUICtrl* ctrl, void* user_data);
	static void onChangeText(LLLineEditor* caller, void* user_data);
	
	virtual bool refreshFromRegion(LLViewerRegion* region);
	virtual bool estateUpdate(LLMessageSystem* msg) { return true; }
	
	virtual BOOL postBuild();
	virtual void updateChild(LLUICtrl* child_ctrl);
	
	void enableButton(const std::string& btn_name, BOOL enable = TRUE);
	void disableButton(const std::string& btn_name);
	
protected:
	void initCtrl(const std::string& name);
	void initTextCtrl(const std::string& name);
	void initHelpBtn(const std::string& name, const std::string& xml_alert);

	// Callback for all help buttons, data is name of XML alert to show.
	static void onClickHelp(void* data);
	
	// Returns TRUE if update sent and apply button should be
	// disabled.
	virtual BOOL sendUpdate() { return TRUE; }
	
	typedef std::vector<std::string> strings_t;
	//typedef std::vector<U32> integers_t;
	void sendEstateOwnerMessage(
					 LLMessageSystem* msg,
					 const std::string& request,
					 const LLUUID& invoice,
					 const strings_t& strings);
	
	// member data
	LLHost mHost;
};

/////////////////////////////////////////////////////////////////////////////
// Actual panels start here
/////////////////////////////////////////////////////////////////////////////

class LLPanelRegionGeneralInfo : public LLPanelRegionInfo
{
public:
	LLPanelRegionGeneralInfo()
		:	LLPanelRegionInfo()	{}
	~LLPanelRegionGeneralInfo() {}
	
	virtual bool refreshFromRegion(LLViewerRegion* region);
	
	// LLPanel
	virtual BOOL postBuild();
protected:
	virtual BOOL sendUpdate();
	
	static void onClickKick(void* userdata);
	static void onKickCommit(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* userdata);
	static void onClickKickAll(void* userdata);
	static void onKickAllCommit(S32 option, void* userdata);
	static void onClickMessage(void* userdata);
	static void onMessageCommit(S32 option, const std::string& text, void* userdata);
	static void onClickManageTelehub(void* data);
};

/////////////////////////////////////////////////////////////////////////////

class LLPanelRegionDebugInfo : public LLPanelRegionInfo
{
public:
	LLPanelRegionDebugInfo()
		:	LLPanelRegionInfo(), mTargetAvatar() {}
	~LLPanelRegionDebugInfo() {}
	// LLPanel
	virtual BOOL postBuild();
	
	virtual bool refreshFromRegion(LLViewerRegion* region);
	
protected:
	virtual BOOL sendUpdate();

	static void onClickChooseAvatar(void*);
	static void callbackAvatarID(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* data);
	static void onClickReturnScriptedOtherLand(void*);
	static void callbackReturnScriptedOtherLand(S32 option, void*);
	static void onClickReturnScriptedAll(void*);
	static void callbackReturnScriptedAll(S32 option, void*);
	static void onClickTopColliders(void*);
	static void onClickTopScripts(void*);
	static void onClickRestart(void* data);
	static void callbackRestart(S32 option, void* data);
	static void onClickCancelRestart(void* data);
	
private:
	LLUUID mTargetAvatar;
};

/////////////////////////////////////////////////////////////////////////////

class LLPanelRegionTextureInfo : public LLPanelRegionInfo
{
public:
	LLPanelRegionTextureInfo();
	~LLPanelRegionTextureInfo() {}
	
	virtual bool refreshFromRegion(LLViewerRegion* region);
	
	// LLPanel && LLView
	virtual BOOL postBuild();
	
protected:
	virtual BOOL sendUpdate();
	
	static void onClickDump(void* data);
	BOOL validateTextureSizes();
};

/////////////////////////////////////////////////////////////////////////////

class LLPanelRegionTerrainInfo : public LLPanelRegionInfo
{
public:
	LLPanelRegionTerrainInfo()
		:	LLPanelRegionInfo() {}
	~LLPanelRegionTerrainInfo() {}
	// LLPanel
	virtual BOOL postBuild();
	
	virtual bool refreshFromRegion(LLViewerRegion* region);
	
protected:
	virtual BOOL sendUpdate();

	static void onChangeUseEstateTime(LLUICtrl* ctrl, void* user_data);
	static void onChangeFixedSun(LLUICtrl* ctrl, void* user_data);
	static void onChangeSunHour(LLUICtrl* ctrl, void*);

	static void onClickDownloadRaw(void*);
	static void onClickUploadRaw(void*);
	static void onClickBakeTerrain(void*);
	static void callbackBakeTerrain(S32 option, void* data);
};

/////////////////////////////////////////////////////////////////////////////

class LLPanelEstateInfo : public LLPanelRegionInfo
{
public:
	static void initDispatch(LLDispatcher& dispatch);
	
	static void onChangeFixedSun(LLUICtrl* ctrl, void* user_data);
	static void onChangeUseGlobalTime(LLUICtrl* ctrl, void* user_data);
	
	static void onClickEditSky(void* userdata);
	static void onClickEditSkyHelp(void* userdata);	
	static void onClickEditDayCycle(void* userdata);
	static void onClickEditDayCycleHelp(void* userdata);	

	static void onClickAddAllowedAgent(void* user_data);
	static void onClickRemoveAllowedAgent(void* user_data);
	static void onClickAddAllowedGroup(void* user_data);
	static void onClickRemoveAllowedGroup(void* user_data);
	static void onClickAddBannedAgent(void* user_data);
	static void onClickRemoveBannedAgent(void* user_data);
	static void onClickAddEstateManager(void* user_data);
	static void onClickRemoveEstateManager(void* user_data);
	static void onClickKickUser(void* userdata);

	// Group picker callback is different, can't use core methods below
	static void addAllowedGroup(S32 option, void* data);
	static void addAllowedGroup2(LLUUID id, void* data);

	// Core methods for all above add/remove button clicks
	static void accessAddCore(U32 operation_flag, const std::string& dialog_name);
	static void accessAddCore2(S32 option, void* data);
	static void accessAddCore3(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* data);

	static void accessRemoveCore(U32 operation_flag, const std::string& dialog_name, const std::string& list_ctrl_name);
	static void accessRemoveCore2(S32 option, void* data);

	// used for both add and remove operations
	static void accessCoreConfirm(S32 option, void* data);
	static void kickUserConfirm(S32 option, void* userdata);

	// Send the actual EstateOwnerRequest "estateaccessdelta" message
	static void sendEstateAccessDelta(U32 flags, const LLUUID& agent_id);

	static void onKickUserCommit(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* userdata);
	static void onClickMessageEstate(void* data);
	static void onMessageCommit(S32 option, const std::string& text, void* data);
	
	LLPanelEstateInfo();
	~LLPanelEstateInfo() {}
	
	virtual bool refreshFromRegion(LLViewerRegion* region);
	virtual bool estateUpdate(LLMessageSystem* msg);
	
	// LLPanel
	virtual BOOL postBuild();
	virtual void updateChild(LLUICtrl* child_ctrl);
	virtual void refresh();
	
	U32 computeEstateFlags();
	void setEstateFlags(U32 flags);
	
	BOOL getGlobalTime();
	void setGlobalTime(bool b);

	BOOL getFixedSun();

	F32 getSunHour();
	void setSunHour(F32 sun_hour);
	
	const std::string getEstateName() const;
	void setEstateName(const std::string& name);

	U32 getEstateID() const { return mEstateID; }
	void setEstateID(U32 estate_id) { mEstateID = estate_id; }
	static bool isLindenEstate();
	
	const std::string getOwnerName() const;
	void setOwnerName(const std::string& name);

	const std::string getAbuseEmailAddress() const;
	void setAbuseEmailAddress(const std::string& address);

	// If visible from mainland, allowed agent and allowed groups
	// are ignored, so must disable UI.
	void setAccessAllowedEnabled(bool enable_agent, bool enable_group, bool enable_ban);

	// this must have the same function signature as
	// llmessage/llcachename.h:LLCacheNameCallback
	static void callbackCacheName(
		const LLUUID& id,
		const std::string& first,
		const std::string& last,
		BOOL is_group,
		void*);

protected:
	virtual BOOL sendUpdate();
	// confirmation dialog callback
	static void callbackChangeLindenEstate(S32 opt, void* data);

	void commitEstateInfoDataserver();
	bool commitEstateInfoCaps();
	void commitEstateAccess();
	void commitEstateManagers();
	
	void clearAccessLists();
	BOOL checkRemovalButton(std::string name);
	BOOL checkSunHourSlider(LLUICtrl* child_ctrl);

	U32 mEstateID;
};

/////////////////////////////////////////////////////////////////////////////

class LLPanelEstateCovenant : public LLPanelRegionInfo
{
public:
	LLPanelEstateCovenant();
	~LLPanelEstateCovenant() {}
	
	// LLPanel
	virtual BOOL postBuild();
	virtual void updateChild(LLUICtrl* child_ctrl);
	virtual bool refreshFromRegion(LLViewerRegion* region);
	virtual bool estateUpdate(LLMessageSystem* msg);

	// LLView overrides
	BOOL handleDragAndDrop(S32 x, S32 y, MASK mask,
						   BOOL drop, EDragAndDropType cargo_type,
						   void *cargo_data, EAcceptance *accept,
						   std::string& tooltip_msg);
	static void confirmChangeCovenantCallback(S32 option, void* userdata);
	static void resetCovenantID(void* userdata);
	static void confirmResetCovenantCallback(S32 option, void* userdata);
	void sendChangeCovenantID(const LLUUID &asset_id);
	void loadInvItem(LLInventoryItem *itemp);
	static void onLoadComplete(LLVFS *vfs,
							   const LLUUID& asset_uuid,
							   LLAssetType::EType type,
							   void* user_data, S32 status, LLExtStat ext_status);

	// Accessor functions
	static void updateCovenantText(const std::string& string, const LLUUID& asset_id);
	static void updateEstateName(const std::string& name);
	static void updateLastModified(const std::string& text);
	static void updateEstateOwnerName(const std::string& name);

	const LLUUID& getCovenantID() const { return mCovenantID; }
	void setCovenantID(const LLUUID& id) { mCovenantID = id; }
	const std::string& getEstateName() const;
	void setEstateName(const std::string& name);
	const std::string& getOwnerName() const;
	void setOwnerName(const std::string& name);
	void setCovenantTextEditor(const std::string& text);

	typedef enum e_asset_status
	{
		ASSET_ERROR,
		ASSET_UNLOADED,
		ASSET_LOADING,
		ASSET_LOADED
	} EAssetStatus;

protected:
	virtual BOOL sendUpdate();
	LLTextBox*				mEstateNameText;
	LLTextBox*				mEstateOwnerText;
	LLTextBox*				mLastModifiedText;
	// CovenantID from sim
	LLUUID					mCovenantID;
	LLViewerTextEditor*		mEditor;
	EAssetStatus			mAssetStatus;
};

#endif
