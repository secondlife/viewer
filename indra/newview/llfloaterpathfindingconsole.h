/** 
 * @file llfloaterpathfindingconsole.h
 * @author William Todd Stinson
 * @brief "Pathfinding console" floater, allowing manipulation of the Havok AI pathfinding settings.
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

#ifndef LL_LLFLOATERPATHFINDINGCONSOLE_H
#define LL_LLFLOATERPATHFINDINGCONSOLE_H

#include "llfloater.h"
#include "llhandle.h"
#include "LLPathingLib.h"
#include "llpathfindingmanager.h"
#include "llpathfindingnavmeshzone.h"

#include <boost/signals2.hpp>

class LLSD;
class LLPanel;
class LLSliderCtrl;
class LLLineEditor;
class LLTextBase;
class LLCheckBoxCtrl;
class LLTabContainer;
class LLComboBox;
class LLButton;

class LLFloaterPathfindingConsole
:	public LLFloater
{
	friend class LLFloaterReg;

public:
	typedef enum
	{
		kRenderHeatmapNone,
		kRenderHeatmapA,
		kRenderHeatmapB,
		kRenderHeatmapC,
		kRenderHeatmapD
	} ERenderHeatmapType;

	typedef enum
	{
		kCharacterTypeNone,
		kCharacterTypeA,
		kCharacterTypeB,
		kCharacterTypeC,
		kCharacterTypeD
	} ECharacterType;

	virtual BOOL postBuild();
	virtual void onOpen(const LLSD& pKey);
	virtual void onClose(bool pIsAppQuitting);
	virtual BOOL handleAnyMouseClick(S32 x, S32 y, MASK mask, EClickType clicktype, BOOL down);

	BOOL isGeneratePathMode(MASK mask, EClickType clicktype, BOOL down) const;

	static LLHandle<LLFloaterPathfindingConsole> getInstanceHandle();

	BOOL isRenderPath() const;

	BOOL isRenderNavMesh() const;
	void setRenderNavMesh(BOOL pIsRenderNavMesh);

	BOOL isRenderWalkables() const;
	void setRenderWalkables(BOOL pIsRenderWalkables);

	BOOL isRenderStaticObstacles() const;
	void setRenderStaticObstacles(BOOL pIsRenderStaticObstacles);

	BOOL isRenderMaterialVolumes() const;
	void setRenderMaterialVolumes(BOOL pIsRenderMaterialVolumes);

	BOOL isRenderExclusionVolumes() const;
	void setRenderExclusionVolumes(BOOL pIsRenderExclusionVolumes);

	BOOL isRenderWorld() const;
	void setRenderWorld(BOOL pIsRenderWorld);
	
	BOOL isRenderXRay() const;
	void setRenderXRay(BOOL pIsRenderXRay);
	
	BOOL isRenderAnyShapes() const;
	U32  getRenderShapeFlags();

	ERenderHeatmapType getRenderHeatmapType() const;
	void               setRenderHeatmapType(ERenderHeatmapType pRenderHeatmapType);

    F32            getCharacterWidth() const;
    void           setCharacterWidth(F32 pCharacterWidth);

    ECharacterType getCharacterType() const;
    void           setCharacterType(ECharacterType pCharacterType);

	int getHeatMapType() const;

protected:

private:
	typedef enum
	{
		kConsoleStateUnknown,
		kConsoleStateLibraryNotImplemented,
		kConsoleStateRegionNotEnabled,
		kConsoleStateCheckingVersion,
		kConsoleStateDownloading,
		kConsoleStateHasNavMesh,
		kConsoleStateError
	} EConsoleState;

	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingConsole(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingConsole();

	void onShowWalkabilitySet();
	void onShowWorldToggle();
	void onShowXRayToggle();
	void onCharacterWidthSet();
	void onCharacterTypeSwitch();
	void onViewCharactersClicked();
	void onUnfreezeClicked();
	void onFreezeClicked();
	void onViewEditLinksetClicked();
	void onClearPathClicked();
	void onNavMeshZoneCB(LLPathfindingNavMeshZone::ENavMeshZoneRequestStatus pNavMeshZoneRequestStatus);
	void onAgentStateCB(LLPathfindingManager::EAgentState pAgentState);
	void onRegionBoundaryCross();

	void setConsoleState(EConsoleState pConsoleState);

	void        updateControlsOnConsoleState();
	void        updateStatusOnConsoleState();
	std::string getSimulatorStatusText() const;

	void initializeNavMeshZoneForCurrentRegion();

	void setAgentState(LLPathfindingManager::EAgentState pAgentState);

	void generatePath();
	void updatePathTestStatus();
	void resetShapeRenderFlags() { mShapeRenderFlags = 0; }
	void setShapeRenderFlag( LLPathingLib::LLShapeType type ) { mShapeRenderFlags |= (1<<type); }
	void fillInColorsForNavMeshVisualization();

	LLRootHandle<LLFloaterPathfindingConsole>     mSelfHandle;
	LLCheckBoxCtrl                                *mShowNavMeshCheckBox;
	LLComboBox                                    *mShowNavMeshWalkabilityComboBox;
	LLCheckBoxCtrl                                *mShowWalkablesCheckBox;
	LLCheckBoxCtrl                                *mShowStaticObstaclesCheckBox;
	LLCheckBoxCtrl                                *mShowMaterialVolumesCheckBox;
	LLCheckBoxCtrl                                *mShowExclusionVolumesCheckBox;
	LLCheckBoxCtrl                                *mShowWorldCheckBox;
	LLCheckBoxCtrl								  *mShowXRayCheckBox;
	LLTextBase                                    *mPathfindingViewerStatus;
	LLTextBase                                    *mPathfindingSimulatorStatus;
	LLButton                                      *mViewCharactersButton;
	LLTabContainer                                *mEditTestTabContainer;
	LLPanel                                       *mEditTab;
	LLPanel                                       *mTestTab;
	LLTextBase                                    *mUnfreezeLabel;
	LLButton                                      *mUnfreezeButton;
	LLTextBase                                    *mLinksetsLabel;
	LLButton                                      *mLinksetsButton;
	LLTextBase                                    *mFreezeLabel;
	LLButton                                      *mFreezeButton;
	LLSliderCtrl                                  *mCharacterWidthSlider;
	LLComboBox                                    *mCharacterTypeComboBox;
	LLTextBase                                    *mPathTestingStatus;
	LLButton                                      *mClearPathButton;

	LLPathfindingNavMeshZone::navmesh_zone_slot_t mNavMeshZoneSlot;
	LLPathfindingNavMeshZone                      mNavMeshZone;
	bool                                          mIsNavMeshUpdating;

	LLPathfindingManager::agent_state_slot_t      mAgentStateSlot;
	boost::signals2::connection                   mRegionBoundarySlot;

	EConsoleState                                 mConsoleState;

	//Container that is populated and subsequently submitted to the LLPathingSystem for processing
	LLPathingLib::PathingPacket		mPathData;
	bool							mHasStartPoint;
	bool							mHasEndPoint;
	U32								mShapeRenderFlags;

	static LLHandle<LLFloaterPathfindingConsole> sInstanceHandle;
	
public:
		LLPathingLib::NavMeshColors					mNavMeshColors;
};

#endif // LL_LLFLOATERPATHFINDINGCONSOLE_H
