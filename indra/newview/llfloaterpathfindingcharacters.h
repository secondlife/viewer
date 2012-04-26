/** 
* @file llfloaterpathfindingcharacters.h
* @author William Todd Stinson
* @brief "Pathfinding characters" floater, allowing for identification of pathfinding characters and their cpu usage.
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

#ifndef LL_LLFLOATERPATHFINDINGCHARACTERS_H
#define LL_LLFLOATERPATHFINDINGCHARACTERS_H

#include <string>
#include <map>

#include "llfloater.h"
#include "llpathfindingcharacter.h"
#include "llpathfindingcharacterlist.h"
#include "llpathfindingmanager.h"
#include "llselectmgr.h"

#include <boost/signals2.hpp>

class LLSD;
class LLTextBase;
class LLScrollListCtrl;
class LLCheckBoxCtrl;
class LLRadioGroup;
class LLButton;

class LLFloaterPathfindingCharacters
	:	public LLFloater
{
	friend class LLFloaterReg;

public:
	typedef enum
	{
		kMessagingUnknown,
		kMessagingGetRequestSent,
		kMessagingGetError,
		kMessagingComplete,
		kMessagingNotEnabled
	} EMessagingState;

	virtual BOOL postBuild();
	virtual void onOpen(const LLSD& pKey);
	virtual void onClose(bool pAppQuitting);
	virtual void draw();

	static void openCharactersViewer();

protected:

private:
	LLScrollListCtrl *mCharactersScrollList;
	LLTextBase       *mCharactersStatus;
	LLButton         *mRefreshListButton;
	LLButton         *mSelectAllButton;
	LLButton         *mSelectNoneButton;
	LLCheckBoxCtrl   *mShowBeaconCheckBox;
	LLButton         *mTakeButton;
	LLButton         *mTakeCopyButton;
	LLButton         *mReturnButton;
	LLButton         *mDeleteButton;
	LLButton         *mTeleportButton;

	EMessagingState                    mMessagingState;
	LLPathfindingManager::request_id_t mMessagingRequestId;
	LLPathfindingCharacterListPtr      mCharacterListPtr;
	LLObjectSelectionHandle            mCharacterSelection;
	boost::signals2::connection        mSelectionUpdateSlot;
	boost::signals2::connection        mRegionBoundarySlot;

	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingCharacters(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingCharacters();

	EMessagingState getMessagingState() const;
	void            setMessagingState(EMessagingState pMessagingState);

	void requestGetCharacters();
	void handleNewCharacters(LLPathfindingManager::request_id_t pRequestId, LLPathfindingManager::ERequestStatus pCharacterRequestStatus, LLPathfindingCharacterListPtr pCharacterListPtr);

	void onCharactersSelectionChange();
	void onRefreshCharactersClicked();
	void onSelectAllCharactersClicked();
	void onSelectNoneCharactersClicked();
	void onTakeCharactersClicked();
	void onTakeCopyCharactersClicked();
	void onReturnCharactersClicked();
	void onDeleteCharactersClicked();
	void onTeleportCharacterToMeClicked();
	void onRegionBoundaryCross();

	void selectAllCharacters();
	void selectNoneCharacters();
	void clearCharacters();

	void updateControls();
	void updateScrollList();
	LLSD buildCharacterScrollListElement(const LLPathfindingCharacterPtr pCharacterPtr) const;

	void updateStatusMessage();
	void updateEnableStateOnListActions();
	void updateEnableStateOnEditFields();
};

#endif // LL_LLFLOATERPATHFINDINGCHARACTERS_H
