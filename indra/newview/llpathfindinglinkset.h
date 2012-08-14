/** 
* @file   llpathfindinglinkset.h
* @brief  Definition of a pathfinding linkset that contains various properties required for havok pathfinding.
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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
#ifndef LL_LLPATHFINDINGLINKSET_H
#define LL_LLPATHFINDINGLINKSET_H

#include <string>

#include "llpathfindingobject.h"

class LLSD;

class LLPathfindingLinkset : public LLPathfindingObject
{
public:
	typedef enum
	{
		kUnknown,
		kWalkable,
		kStaticObstacle,
		kDynamicObstacle,
		kMaterialVolume,
		kExclusionVolume,
		kDynamicPhantom
	} ELinksetUse;

	LLPathfindingLinkset(const LLSD &pTerrainData);
	LLPathfindingLinkset(const std::string &pUUID, const LLSD &pLinksetData);
	LLPathfindingLinkset(const LLPathfindingLinkset& pOther);
	virtual ~LLPathfindingLinkset();

	LLPathfindingLinkset& operator = (const LLPathfindingLinkset& pOther);

	inline bool        isTerrain() const                   {return mIsTerrain;};
	inline U32         getLandImpact() const               {return mLandImpact;};
	BOOL               isModifiable() const                {return mIsModifiable;};
	BOOL               isPhantom() const;
	BOOL               canBeVolume() const                 {return mCanBeVolume;};
	static ELinksetUse getLinksetUseWithToggledPhantom(ELinksetUse pLinksetUse);

	inline ELinksetUse getLinksetUse() const               {return mLinksetUse;};

	inline BOOL        isScripted() const                  {return mIsScripted;};
	inline BOOL        hasIsScripted() const               {return mHasIsScripted;};

	inline S32         getWalkabilityCoefficientA() const  {return mWalkabilityCoefficientA;};
	inline S32         getWalkabilityCoefficientB() const  {return mWalkabilityCoefficientB;};
	inline S32         getWalkabilityCoefficientC() const  {return mWalkabilityCoefficientC;};
	inline S32         getWalkabilityCoefficientD() const  {return mWalkabilityCoefficientD;};

	bool               isShowUnmodifiablePhantomWarning(ELinksetUse pLinksetUse) const;
	bool               isShowCannotBeVolumeWarning(ELinksetUse pLinksetUse) const;
	LLSD               encodeAlteredFields(ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD) const;

	static const S32 MIN_WALKABILITY_VALUE;
	static const S32 MAX_WALKABILITY_VALUE;
	
protected:

private:
	typedef enum
	{
		kNavMeshGenerationIgnore,
		kNavMeshGenerationInclude,
		kNavMeshGenerationExclude
	} ENavMeshGenerationCategory;

	void                              parseLinksetData(const LLSD &pLinksetData);
	void                              parsePathfindingData(const LLSD &pLinksetData);

	static BOOL                       isPhantom(ELinksetUse pLinksetUse);
	static ELinksetUse                getLinksetUse(bool pIsPhantom, ENavMeshGenerationCategory pNavMeshGenerationCategory);
	static ENavMeshGenerationCategory getNavMeshGenerationCategory(ELinksetUse pLinksetUse);
	static LLSD                       convertCategoryToLLSD(ENavMeshGenerationCategory pNavMeshGenerationCategory);
	static ENavMeshGenerationCategory convertCategoryFromLLSD(const LLSD &llsd);

	bool         mIsTerrain;
	U32          mLandImpact;
	BOOL         mIsModifiable;
	BOOL         mCanBeVolume;
	BOOL         mIsScripted;
	BOOL         mHasIsScripted;
	ELinksetUse  mLinksetUse;
	S32          mWalkabilityCoefficientA;
	S32          mWalkabilityCoefficientB;
	S32          mWalkabilityCoefficientC;
	S32          mWalkabilityCoefficientD;
};

#endif // LL_LLPATHFINDINGLINKSET_H
