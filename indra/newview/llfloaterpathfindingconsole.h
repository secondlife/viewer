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
#include "llnavmeshstation.h"
#include "LLPathingLib.h"

class LLSD;
class LLRadioGroup;
class LLSliderCtrl;
class LLLineEditor;
class LLTextBase;
class LLCheckBoxCtrl;

class LLFloaterPathfindingConsole
:	public LLFloater
{
	friend class LLFloaterReg;

public:
	typedef enum
	{
		kCharacterTypeA = 0,
		kCharacterTypeB = 1,
		kCharacterTypeC = 2,
		kCharacterTypeD = 3
	} ECharacterType;

	virtual BOOL postBuild();
	//Populates a data packet that is forwarded onto the LLPathingSystem
	void providePathingData( const LLVector3& point1, const LLVector3& point2 );

    F32                   getCharacterWidth() const;
    void                  setCharacterWidth(F32 pCharacterWidth);

    ECharacterType        getCharacterType() const;
    void                  setCharacterType(ECharacterType pCharacterType);

	void setHasNavMeshReceived();
	void setHasNoNavMesh();

protected:

private:
	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingConsole(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingConsole();

	virtual void onOpen(const LLSD& pKey);
	virtual void onClose(bool app_quitting);

	void onShowNavMeshToggle();
	void onShowWalkablesToggle();
	void onShowStaticObstaclesToggle();
	void onShowMaterialVolumesToggle();
	void onShowExclusionVolumesToggle();
	void onShowWorldToggle();
	void onCharacterWidthSet();
	void onCharacterTypeSwitch();
	void onViewEditLinksetClicked();
	void generatePath();

	LLCheckBoxCtrl *mShowNavMeshCheckBox;
	LLCheckBoxCtrl *mShowWalkablesCheckBox;
	LLCheckBoxCtrl *mShowStaticObstaclesCheckBox;
	LLCheckBoxCtrl *mShowMaterialVolumesCheckBox;
	LLCheckBoxCtrl *mShowExclusionVolumesCheckBox;
	LLCheckBoxCtrl *mShowWorldCheckBox;
	LLSliderCtrl   *mCharacterWidthSlider;
	LLRadioGroup   *mCharacterTypeRadioGroup;
	LLTextBase     *mPathfindingStatus;

	LLNavMeshDownloadObserver	mNavMeshDownloadObserver[10];
	int							mCurrentMDO;
	int							mNavMeshCnt;

	//Container that is populated and subsequently submitted to the LLPathingSystem for processing
	LLPathingLib::PathingPacket		mPathData;
	bool mHasStartPoint;
	bool mHasEndPoint;
};

#endif // LL_LLFLOATERPATHFINDINGCONSOLE_H
