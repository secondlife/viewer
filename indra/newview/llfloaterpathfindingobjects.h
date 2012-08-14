/** 
* @file   llfloaterpathfindingobjects.h
* @brief  Base class for both the pathfinding linksets and characters floater.
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
#ifndef LL_LLFLOATERPATHFINDINGOBJECTS_H
#define LL_LLFLOATERPATHFINDINGOBJECTS_H

#include <string>
#include <map>

#include <boost/signals2.hpp>

#include "llagent.h"
#include "llfloater.h"
#include "llpathfindingmanager.h"
#include "llpathfindingobject.h"
#include "llpathfindingobjectlist.h"
#include "llselectmgr.h"
#include "lluuid.h"
#include "v4color.h"

class LLAvatarName;
class LLButton;
class LLCheckBoxCtrl;
class LLScrollListCtrl;
class LLScrollListItem;
class LLSD;
class LLTextBase;

class LLFloaterPathfindingObjects : public LLFloater
{
public:
	virtual void onOpen(const LLSD &pKey);
	virtual void onClose(bool pIsAppQuitting);
	virtual void draw();

protected:
	friend class LLFloaterReg;

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

	LLFloaterPathfindingObjects(const LLSD &pSeed);
	virtual ~LLFloaterPathfindingObjects();

	virtual BOOL                       postBuild();

	virtual void                       requestGetObjects();
	LLPathfindingManager::request_id_t getNewRequestId();
	void                               handleNewObjectList(LLPathfindingManager::request_id_t pRequestId, LLPathfindingManager::ERequestStatus pRequestStatus, LLPathfindingObjectListPtr pObjectList);
	void                               handleUpdateObjectList(LLPathfindingManager::request_id_t pRequestId, LLPathfindingManager::ERequestStatus pRequestStatus, LLPathfindingObjectListPtr pObjectList);

	void                               rebuildObjectsScrollList();
	virtual void                       buildObjectsScrollList(const LLPathfindingObjectListPtr pObjectListPtr);
	void                               addObjectToScrollList(const LLPathfindingObjectPtr pObjectPr, const LLSD &pScrollListItemData);

	virtual void                       updateControlsOnScrollListChange();
	virtual void                       updateControlsOnInWorldSelectionChange();

	virtual S32                        getNameColumnIndex() const;
	virtual S32                        getOwnerNameColumnIndex() const;
	virtual const LLColor4             &getBeaconColor() const;
	virtual const LLColor4             &getBeaconTextColor() const;
	virtual S32                        getBeaconWidth() const;

	void                               showFloaterWithSelectionObjects();

	BOOL                               isShowBeacons() const;
	void                               clearAllObjects();
	void                               selectAllObjects();
	void                               selectNoneObjects();
	void                               teleportToSelectedObject();

	virtual LLPathfindingObjectListPtr getEmptyObjectList() const;
	int                                getNumSelectedObjects() const;
	LLPathfindingObjectListPtr         getSelectedObjects() const;
	LLPathfindingObjectPtr             getFirstSelectedObject() const;

	EMessagingState                    getMessagingState() const;

private:
	LLFloaterPathfindingObjects(const LLFloaterPathfindingObjects &pOther);

	void                   setMessagingState(EMessagingState pMessagingState);

	void                   onRefreshObjectsClicked();
	void                   onSelectAllObjectsClicked();
	void                   onSelectNoneObjectsClicked();
	void                   onTakeClicked();
	void                   onTakeCopyClicked();
	void                   onReturnClicked();
	void                   onDeleteClicked();
	void                   onTeleportClicked();

	void                   onScrollListSelectionChanged();
	void                   onInWorldSelectionListChanged();
	void                   onRegionBoundaryCrossed();
	void                   onGodLevelChange(U8 pGodLevel);

	void                   handleObjectNameResponse(const LLUUID &pObjectUUID, const std::string &pOwnerName);

	void                   updateMessagingStatus();
	void                   updateStateOnListControls();
	void                   updateStateOnActionControls();
	void                   selectScrollListItemsInWorld();

	void                   handleReturnItemsResponse(const LLSD &pNotification, const LLSD &pResponse);
	void                   handleDeleteItemsResponse(const LLSD &pNotification, const LLSD &pResponse);

	LLPathfindingObjectPtr findObject(const LLScrollListItem *pListItem) const;

	LLScrollListCtrl                   *mObjectsScrollList;
	LLTextBase                         *mMessagingStatus;
	LLButton                           *mRefreshListButton;
	LLButton                           *mSelectAllButton;
	LLButton                           *mSelectNoneButton;
	LLCheckBoxCtrl                     *mShowBeaconCheckBox;

	LLButton                           *mTakeButton;
	LLButton                           *mTakeCopyButton;
	LLButton                           *mReturnButton;
	LLButton                           *mDeleteButton;
	LLButton                           *mTeleportButton;

	LLColor4                           mDefaultBeaconColor;
	LLColor4                           mDefaultBeaconTextColor;
	LLColor4                           mErrorTextColor;
	LLColor4                           mWarningTextColor;

	EMessagingState                    mMessagingState;
	LLPathfindingManager::request_id_t mMessagingRequestId;

	typedef std::map<std::string, LLScrollListItem *> scroll_list_item_map;
	scroll_list_item_map               mMissingNameObjectsScrollListItems;

	LLPathfindingObjectListPtr         mObjectList;

	LLObjectSelectionHandle            mObjectsSelection;

	bool                               mHasObjectsToBeSelected;
	uuid_vec_t                         mObjectsToBeSelected;

	boost::signals2::connection        mSelectionUpdateSlot;
	boost::signals2::connection        mRegionBoundaryCrossingSlot;
	LLAgent::god_level_change_slot_t   mGodLevelChangeSlot;
};

#endif // LL_LLFLOATERPATHFINDINGOBJECTS_H
