/** 
 * @file llfloatergodtools.h
 * @brief The on-screen rectangle with tool options.
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

#ifndef LL_LLFLOATERGODTOOLS_H
#define LL_LLFLOATERGODTOOLS_H

#include "llcoord.h"
#include "llhost.h"
#include "llframetimer.h"

#include "llfloater.h"
#include "llpanel.h"
#include <vector>

class LLButton;
class LLCheckBoxCtrl;
class LLComboBox;
class LLUICtrl;
class LLLineEditor;
class LLPanelGridTools;
class LLPanelRegionTools;
class LLPanelObjectTools;
class LLPanelRequestTools;
//class LLSliderCtrl;
class LLSpinCtrl;
class LLTabContainer;
class LLTextBox;
class LLMessageSystem;

class LLFloaterGodTools
	: public LLFloater
{
	friend class LLFloaterReg;
public:

	enum EGodPanel
	{
		PANEL_GRID,
		PANEL_REGION,
		PANEL_OBJECT,
		PANEL_REQUEST,
		PANEL_COUNT
	};

	static void* createPanelGrid(void *userdata);
	static void* createPanelRegion(void *userdata);
	static void* createPanelObjects(void *userdata);
	static void* createPanelRequest(void *userdata);

	static void refreshAll();

	void showPanel(const std::string& panel_name);

	virtual void onOpen(const LLSD& key);

	virtual void draw();

	// call this once per frame to handle visibility, rect location,
	// button highlights, etc.
	void updatePopup(LLCoordGL center, MASK mask);

	// Get data to populate UI.
	void sendRegionInfoRequest();

	// get and process region info if necessary.
	static void processRegionInfo(LLMessageSystem* msg);

	// Send possibly changed values to simulator.
	void sendGodUpdateRegionInfo();

private:
	
	LLFloaterGodTools(const LLSD& key);
	~LLFloaterGodTools();
	
protected:
	U32 computeRegionFlags() const;

protected:

	/*virtual*/	BOOL	postBuild();
	// When the floater is going away, reset any options that need to be 
	// cleared.
	void resetToolState();

public:
	LLPanelRegionTools 	*mPanelRegionTools;
	LLPanelObjectTools	*mPanelObjectTools;

	LLHost mCurrentHost;
	LLFrameTimer mUpdateTimer;
};


//-----------------------------------------------------------------------------
// LLPanelRegionTools
//-----------------------------------------------------------------------------

class LLPanelRegionTools 
: public LLPanel
{
public:
	LLPanelRegionTools();
	/*virtual*/ ~LLPanelRegionTools();

	BOOL postBuild();

	/*virtual*/ void refresh();

	static void onSaveState(void* userdata);
	static void onChangeSimName(LLLineEditor* caller, void* userdata);
	
	void onChangeAnything();
	void onChangePrelude();
	void onApplyChanges();
	void onBakeTerrain();
	void onRevertTerrain();
	void onSwapTerrain();
	void onSelectRegion();
	void onRefresh();

	// set internal checkboxes/spinners/combos 
	const std::string getSimName() const;
	U32 getEstateID() const;
	U32 getParentEstateID() const;
	U32 getRegionFlags() const;
	U32 getRegionFlagsMask() const;
	F32 getBillableFactor() const;
	S32 getPricePerMeter() const;
	S32 getGridPosX() const;
	S32 getGridPosY() const;
	S32 getRedirectGridX() const;
	S32 getRedirectGridY() const;

	// set internal checkboxes/spinners/combos 
	void setSimName(const std::string& name);
	void setEstateID(U32 id);
	void setParentEstateID(U32 id);
	void setCheckFlags(U32 flags);
	void setBillableFactor(F32 billable_factor);
	void setPricePerMeter(S32 price);
	void setGridPosX(S32 pos);
	void setGridPosY(S32 pos);
	void setRedirectGridX(S32 pos);
	void setRedirectGridY(S32 pos);

	U32 computeRegionFlags(U32 initial_flags) const;
	void clearAllWidgets();
	void enableAllWidgets();

protected:
	// gets from internal checkboxes/spinners/combos
	void updateCurrentRegion() const;
};


//-----------------------------------------------------------------------------
// LLPanelGridTools
//-----------------------------------------------------------------------------

class LLPanelGridTools
: public LLPanel
{
public:
	LLPanelGridTools();
	virtual ~LLPanelGridTools();

	BOOL postBuild();

	void refresh();

	static void onDragSunPhase(LLUICtrl *ctrl, void *userdata);
	void onClickFlushMapVisibilityCaches();
	static bool flushMapVisibilityCachesConfirm(const LLSD& notification, const LLSD& response);

protected:
	std::string        mKickMessage; // Message to send on kick
};


//-----------------------------------------------------------------------------
// LLPanelObjectTools
//-----------------------------------------------------------------------------

class LLPanelObjectTools 
: public LLPanel
{
public:
	LLPanelObjectTools();
	/*virtual*/ ~LLPanelObjectTools();

	BOOL postBuild();

	/*virtual*/ void refresh();

	void setTargetAvatar(const LLUUID& target_id);
	U32 computeRegionFlags(U32 initial_flags) const;
	void clearAllWidgets();
	void enableAllWidgets();
	void setCheckFlags(U32 flags);

	void onChangeAnything();
	void onApplyChanges();
	void onClickSet();
	void callbackAvatarID(const std::vector<std::string>& names, const std::vector<LLUUID>& ids);
	void onClickDeletePublicOwnedBy();
	void onClickDeleteAllScriptedOwnedBy();
	void onClickDeleteAllOwnedBy();
	static bool callbackSimWideDeletes(const LLSD& notification, const LLSD& response);
	void onGetTopColliders();
	void onGetTopScripts();
	void onGetScriptDigest();
	static void onClickSetBySelection(void* data);

protected:
	LLUUID		mTargetAvatar;

	// For all delete dialogs, store flags here for message.
	U32 mSimWideDeletesFlags;
}; 


//-----------------------------------------------------------------------------
// LLPanelRequestTools
//-----------------------------------------------------------------------------

class LLPanelRequestTools : public LLPanel
{
public:
	LLPanelRequestTools();
	/*virtual*/ ~LLPanelRequestTools();

	BOOL postBuild();

	void refresh();

	static void sendRequest(const std::string& request, 
							const std::string& parameter, 
							const LLHost& host);

protected:
	void onClickRequest();
	void sendRequest(const LLHost& host);
};

// Flags are SWD_ flags.
void send_sim_wide_deletes(const LLUUID& owner_id, U32 flags);

#endif
