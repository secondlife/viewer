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

#include "llfloaterpathfindingobjects.h"
#include "llpathfindinglinkset.h"
#include "llpathfindingmanager.h"
#include "llpathfindingobjectlist.h"
#include "v4color.h"

class LLButton;
class LLComboBox;
class LLLineEditor;
class LLScrollListItem;
class LLSD;
class LLTextBase;
class LLUICtrl;
class LLVector3;

class LLFloaterPathfindingLinksets : public LLFloaterPathfindingObjects
{
public:
	static void  openLinksetsEditor();

protected:
	friend class LLFloaterReg;

	LLFloaterPathfindingLinksets(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingLinksets();

	virtual BOOL                       postBuild();

	virtual void                       requestGetObjects();

	virtual LLSD                       convertObjectsIntoScrollListData(const LLPathfindingObjectListPtr pObjectListPtr);

	virtual void                       updateControls();

	virtual S32                        getNameColumnIndex() const;
	virtual const LLColor4             &getBeaconColor() const;

	virtual LLPathfindingObjectListPtr getEmptyObjectList() const;

private:
	void requestSetLinksets(LLPathfindingObjectListPtr pLinksetList, LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD);

	void onApplyAllFilters();
	void onClearFiltersClicked();
	void onWalkabilityCoefficientEntered(LLUICtrl *pUICtrl);
	void onApplyChangesClicked();

	void applyFilters();
	void clearFilters();

	void updateEditFieldValues();
	LLSD buildLinksetScrollListData(const LLPathfindingLinkset *pLinksetPtr, const LLVector3 &pAvatarPosition) const;
	LLSD buildLinksetUseScrollListData(const std::string &pLabel, S32 pValue) const;

	bool isShowUnmodifiablePhantomWarning(LLPathfindingLinkset::ELinksetUse pLinksetUse) const;
	bool isShowCannotBeVolumeWarning(LLPathfindingLinkset::ELinksetUse pLinksetUse) const;

	void updateStateOnEditFields();
	void updateStateOnEditLinksetUse();

	void applyEdit();
	void handleApplyEdit(const LLSD &pNotification, const LLSD &pResponse);
	void doApplyEdit();

	std::string getLinksetUseString(LLPathfindingLinkset::ELinksetUse pLinksetUse) const;

	LLPathfindingLinkset::ELinksetUse getFilterLinksetUse() const;
	void                              setFilterLinksetUse(LLPathfindingLinkset::ELinksetUse pLinksetUse);

	LLPathfindingLinkset::ELinksetUse getEditLinksetUse() const;
	void                              setEditLinksetUse(LLPathfindingLinkset::ELinksetUse pLinksetUse);

	LLPathfindingLinkset::ELinksetUse convertToLinksetUse(LLSD pXuiValue) const;
	LLSD                              convertToXuiValue(LLPathfindingLinkset::ELinksetUse pLinksetUse) const;

	LLLineEditor                             *mFilterByName;
	LLLineEditor                             *mFilterByDescription;
	LLComboBox                               *mFilterByLinksetUse;
	LLComboBox                               *mEditLinksetUse;
	LLScrollListItem                         *mEditLinksetUseUnset;
	LLScrollListItem                         *mEditLinksetUseWalkable;
	LLScrollListItem                         *mEditLinksetUseStaticObstacle;
	LLScrollListItem                         *mEditLinksetUseDynamicObstacle;
	LLScrollListItem                         *mEditLinksetUseMaterialVolume;
	LLScrollListItem                         *mEditLinksetUseExclusionVolume;
	LLScrollListItem                         *mEditLinksetUseDynamicPhantom;
	LLTextBase                               *mLabelWalkabilityCoefficients;
	LLTextBase                               *mLabelEditA;
	LLLineEditor                             *mEditA;
	LLTextBase                               *mLabelEditB;
	LLLineEditor                             *mEditB;
	LLTextBase                               *mLabelEditC;
	LLLineEditor                             *mEditC;
	LLTextBase                               *mLabelEditD;
	LLLineEditor                             *mEditD;
	LLButton                                 *mApplyEditsButton;

	LLColor4                                 mBeaconColor;
};

#endif // LL_LLFLOATERPATHFINDINGLINKSETS_H
