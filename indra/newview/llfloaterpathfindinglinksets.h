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
#include "lluuid.h"
#include "llselectmgr.h"
#include "llpathfindinglinkset.h"
#include "llpathfindinglinksetlist.h"
#include "llpathfindingmanager.h"

#include <boost/signals2.hpp>

class LLSD;
class LLUICtrl;
class LLTextBase;
class LLScrollListCtrl;
class LLScrollListItem;
class LLLineEditor;
class LLComboBox;
class LLCheckBoxCtrl;
class LLButton;

class LLFloaterPathfindingLinksets
:	public LLFloater
{
	friend class LLFloaterReg;

public:
	typedef enum
	{
		kMessagingUnknown,
		kMessagingGetRequestSent,
		kMessagingGetError,
		kMessagingSetRequestSent,
		kMessagingSetError,
		kMessagingComplete,
		kMessagingNotEnabled
	} EMessagingState;

	virtual BOOL postBuild();
	virtual void onOpen(const LLSD& pKey);
	virtual void onClose(bool pAppQuitting);
	virtual void draw();

	static void openLinksetsEditor();

	EMessagingState getMessagingState() const;
	bool            isMessagingInProgress() const;

protected:

private:
	LLLineEditor     *mFilterByName;
	LLLineEditor     *mFilterByDescription;
	LLComboBox       *mFilterByLinksetUse;
	LLScrollListCtrl *mLinksetsScrollList;
	LLTextBase       *mLinksetsStatus;
	LLButton         *mRefreshListButton;
	LLButton         *mSelectAllButton;
	LLButton         *mSelectNoneButton;
	LLCheckBoxCtrl   *mShowBeaconCheckBox;
	LLButton         *mTakeButton;
	LLButton         *mTakeCopyButton;
	LLButton         *mReturnButton;
	LLButton         *mDeleteButton;
	LLButton         *mTeleportButton;
	LLComboBox       *mEditLinksetUse;
	LLScrollListItem *mEditLinksetUseUnset;
	LLScrollListItem *mEditLinksetUseWalkable;
	LLScrollListItem *mEditLinksetUseStaticObstacle;
	LLScrollListItem *mEditLinksetUseDynamicObstacle;
	LLScrollListItem *mEditLinksetUseMaterialVolume;
	LLScrollListItem *mEditLinksetUseExclusionVolume;
	LLScrollListItem *mEditLinksetUseDynamicPhantom;
	LLTextBase       *mLabelWalkabilityCoefficients;
	LLTextBase       *mLabelEditA;
	LLLineEditor     *mEditA;
	LLTextBase       *mLabelEditB;
	LLLineEditor     *mEditB;
	LLTextBase       *mLabelEditC;
	LLLineEditor     *mEditC;
	LLTextBase       *mLabelEditD;
	LLLineEditor     *mEditD;
	LLButton         *mApplyEditsButton;

	EMessagingState                          mMessagingState;
	LLPathfindingLinksetListPtr              mLinksetsListPtr;
	LLObjectSelectionHandle                  mLinksetsSelection;
	LLPathfindingManager::agent_state_slot_t mAgentStateSlot;
	boost::signals2::connection              mSelectionUpdateSlot;

	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingLinksets(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingLinksets();

	void setMessagingState(EMessagingState pMessagingState);
	void requestGetLinksets();
	void requestSetLinksets(LLPathfindingLinksetListPtr pLinksetList, LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD);
	void handleNewLinksets(LLPathfindingManager::ERequestStatus pLinksetsRequestStatus, LLPathfindingLinksetListPtr pLinksetsListPtr);
	void handleUpdateLinksets(LLPathfindingManager::ERequestStatus pLinksetsRequestStatus, LLPathfindingLinksetListPtr pLinksetsListPtr);

	void onApplyAllFilters();
	void onClearFiltersClicked();
	void onLinksetsSelectionChange();
	void onRefreshLinksetsClicked();
	void onSelectAllLinksetsClicked();
	void onSelectNoneLinksetsClicked();
	void onTakeClicked();
	void onTakeCopyClicked();
	void onReturnClicked();
	void onDeleteClicked();
	void onTeleportClicked();
	void onWalkabilityCoefficientEntered(LLUICtrl *pUICtrl);
	void onApplyChangesClicked();
	void onAgentStateCB(LLPathfindingManager::EAgentState pAgentState);

	void applyFilters();
	void clearFilters();

	void selectAllLinksets();
	void selectNoneLinksets();
	void clearLinksets();

	void updateControls();
	void updateEditFieldValues();
	void updateScrollList();
	LLSD buildLinksetScrollListElement(const LLPathfindingLinksetPtr pLinksetPtr, const LLVector3 &pAvatarPosition) const;
	LLSD buildLinksetUseScrollListElement(const std::string &label, S32 value) const;

	bool isShowUnmodifiablePhantomWarning(LLPathfindingLinkset::ELinksetUse linksetUse) const;
	bool isShowCannotBeVolumeWarning(LLPathfindingLinkset::ELinksetUse linksetUse) const;

	void updateStatusMessage();
	void updateEnableStateOnListActions();
	void updateEnableStateOnEditFields();
	void updateEnableStateOnEditLinksetUse();

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
};

#endif // LL_LLFLOATERPATHFINDINGLINKSETS_H
