/** 
* @file   llpathfindinglinksetlist.h
* @brief  Header file for llpathfindinglinksetlist
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
#ifndef LL_LLPATHFINDINGLINKSETLIST_H
#define LL_LLPATHFINDINGLINKSETLIST_H

#include "llpathfindinglinkset.h"
#include "llpathfindingobjectlist.h"

class LLSD;

class LLPathfindingLinksetList : public LLPathfindingObjectList
{
public:
	LLPathfindingLinksetList();
	LLPathfindingLinksetList(const LLSD& pLinksetListData);
	virtual ~LLPathfindingLinksetList();

	LLSD encodeObjectFields(LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD) const;
	LLSD encodeTerrainFields(LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD) const;

	bool isShowUnmodifiablePhantomWarning(LLPathfindingLinkset::ELinksetUse pLinksetUse) const;
	bool isShowPhantomToggleWarning(LLPathfindingLinkset::ELinksetUse pLinksetUse) const;
	bool isShowCannotBeVolumeWarning(LLPathfindingLinkset::ELinksetUse pLinksetUse) const;

	void determinePossibleStates(BOOL &pCanBeWalkable, BOOL &pCanBeStaticObstacle, BOOL &pCanBeDynamicObstacle,
		BOOL &pCanBeMaterialVolume, BOOL &pCanBeExclusionVolume, BOOL &pCanBeDynamicPhantom) const;

protected:

private:
	void parseLinksetListData(const LLSD& pLinksetListData);
};

#endif // LL_LLPATHFINDINGLINKSETLIST_H
