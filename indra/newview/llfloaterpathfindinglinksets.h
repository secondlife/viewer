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

#include "llhandle.h"
#include "llfloater.h"
#include "lluuid.h"
#include "llpathfindinglinkset.h"
#include "llfilteredpathfindinglinksets.h"

class LLSD;
class LLTextBase;
class LLScrollListCtrl;
class LLLineEditor;
class LLComboBox;
class LLButton;

class LLFloaterPathfindingLinksets
:	public LLFloater
{
	friend class LLFloaterReg;
	friend class NavMeshDataGetResponder;
	friend class NavMeshDataPutResponder;

public:
	typedef enum
	{
		kMessagingInitial,
		kMessagingFetchStarting,
		kMessagingFetchRequestSent,
		kMessagingFetchRequestSent_MultiRequested,
		kMessagingFetchReceived,
		kMessagingFetchError,
		kMessagingModifyStarting,
		kMessagingModifyRequestSent,
		kMessagingModifyReceived,
		kMessagingModifyError,
		kMessagingComplete,
		kMessagingServiceNotAvailable
	} EMessagingState;

	virtual BOOL postBuild();
	virtual void onOpen(const LLSD& pKey);

	static void openLinksetsEditor();

	EMessagingState getMessagingState() const;
	BOOL            isMessagingInProgress() const;

protected:

private:
	LLRootHandle<LLFloaterPathfindingLinksets> mSelfHandle;
	LLLineEditor                               *mFilterByName;
	LLLineEditor                               *mFilterByDescription;
	LLComboBox                                 *mFilterByLinksetUse;
	LLScrollListCtrl                           *mLinksetsScrollList;
	LLTextBase                                 *mLinksetsStatus;
	LLButton                                   *mRefreshListButton;
	LLButton                                   *mSelectAllButton;
	LLButton                                   *mSelectNoneButton;
	LLButton                                   *mTakeButton;
	LLButton                                   *mTakeCopyButton;
	LLButton                                   *mReturnButton;
	LLButton                                   *mDeleteButton;
	LLButton                                   *mTeleportButton;
	LLComboBox                                 *mEditLinksetUse;
	LLTextBase                                 *mLabelWalkabilityCoefficients;
	LLTextBase                                 *mLabelEditA;
	LLLineEditor                               *mEditA;
	LLTextBase                                 *mLabelEditB;
	LLLineEditor                               *mEditB;
	LLTextBase                                 *mLabelEditC;
	LLLineEditor                               *mEditC;
	LLTextBase                                 *mLabelEditD;
	LLLineEditor                               *mEditD;
	LLButton                                   *mApplyEditsButton;
	LLFilteredPathfindingLinksets              mPathfindingLinksets;
	EMessagingState                            mMessagingState;

	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingLinksets(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingLinksets();

	void sendNavMeshDataGetRequest();
	void sendNavMeshDataPutRequest(const LLSD& pPostData);
	void handleNavMeshDataGetReply(const LLSD& pNavMeshData);
	void handleNavMeshDataGetError(const std::string& pURL, const std::string& pErrorReason);
	void handleNavMeshDataPutReply(const LLSD& pModifiedData);
	void handleNavMeshDataPutError(const std::string& pURL, const std::string& pErrorReason);

	std::string getRegionName() const;
	std::string getCapabilityURL() const;

	void setMessagingState(EMessagingState pMessagingState);

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
	void onApplyChangesClicked();

	void applyFilters();
	void clearFilters();

	void updateLinksetsList();
	void selectAllLinksets();
	void selectNoneLinksets();

	void updateLinksetsStatusMessage();

	void updateEditFields();
	void applyEditFields();
	void setEnableEditFields(BOOL pEnabled);

	LLPathfindingLinkset::ELinksetUse getFilterLinksetUse() const;
	void                              setFilterLinksetUse(LLPathfindingLinkset::ELinksetUse pLinksetUse);

	LLPathfindingLinkset::ELinksetUse getEditLinksetUse() const;
	void                              setEditLinksetUse(LLPathfindingLinkset::ELinksetUse pLinksetUse);

	LLPathfindingLinkset::ELinksetUse convertToLinksetUse(LLSD pXuiValue) const;
	LLSD                              convertToXuiValue(LLPathfindingLinkset::ELinksetUse pLinksetUse) const;
};

#endif // LL_LLFLOATERPATHFINDINGLINKSETS_H
