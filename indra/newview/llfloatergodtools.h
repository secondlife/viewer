/** 
 * @file llfloatergodtools.h
 * @brief The on-screen rectangle with tool options.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLFLOATERGODTOOLS_H
#define LL_LLFLOATERGODTOOLS_H

#include "llcoord.h"
#include "llhost.h"
#include "llframetimer.h"

#include "llfloater.h"
#include "llpanel.h"
#include <vector>

class LLAvatarName;
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
	U64 computeRegionFlags() const;

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
	U64 getRegionFlags() const;
	U64 getRegionFlagsMask() const;
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
	void setCheckFlags(U64 flags);
	void setBillableFactor(F32 billable_factor);
	void setPricePerMeter(S32 price);
	void setGridPosX(S32 pos);
	void setGridPosY(S32 pos);
	void setRedirectGridX(S32 pos);
	void setRedirectGridY(S32 pos);

	U64 computeRegionFlags(U64 initial_flags) const;
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
	U64 computeRegionFlags(U64 initial_flags) const;
	void clearAllWidgets();
	void enableAllWidgets();
	void setCheckFlags(U64 flags);

	void onChangeAnything();
	void onApplyChanges();
	void onClickSet();
	void callbackAvatarID(const uuid_vec_t& ids, const std::vector<LLAvatarName> names);
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
