/** 
 * @file llfloaterpathfindinglinksets.h
 * @author William Todd Stinson
 * @brief "Pathfinding linksets" floater, allowing manipulation of the Havok AI pathfinding settings.
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

#ifndef LL_LLFLOATERPATHFINDINGLINKSETS_H
#define LL_LLFLOATERPATHFINDINGLINKSETS_H

#include "llfloater.h"

class LLSD;
class LLUICtrl;
class LLMessageSystem;
class LLScrollListCtrl;

class LLFloaterPathfindingLinksets
:	public LLFloater
{
	friend class LLFloaterReg;

public:
	virtual BOOL postBuild();
	virtual void onOpen(const LLSD& pKey);

	static void openLinksetsEditor();
	static void handleLandStatReply(LLMessageSystem *pMsg, void **pData);

protected:

private:
	LLScrollListCtrl *mLinksetsScrollList;
	LLUICtrl         *mLinksetsStatus;

	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingLinksets(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingLinksets();

	void sendLandStatRequest();
	void handleLandStatReply(LLMessageSystem *pMsg);

	void onLinksetsSelectionChange();
	void onRefreshLinksetsClicked();
	void onSelectAllLinksetsClicked();
	void onSelectNoneLinksetsClicked();

	void clearLinksetsList();
	void selectAllLinksets();
	void selectNoneLinksets();

	void updateLinksetsStatus();
	void updateLinksetsStatusForFetch();
};

#endif // LL_LLFLOATERPATHFINDINGLINKSETS_H
