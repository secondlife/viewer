/** 
 * @file llfloaterpathfindingcharacters.h
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

#ifndef LL_LLFLOATERPATHFINDINGCHARACTERS_H
#define LL_LLFLOATERPATHFINDINGCHARACTERS_H

#include <string>
#include <map>

#include "llhandle.h"
#include "llfloater.h"
#include "llpathfindingcharacter.h"
#include "llselectmgr.h"

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
	friend class CharactersGetResponder;

public:
	typedef enum
	{
		kMessagingInitial,
		kMessagingFetchStarting,
		kMessagingFetchRequestSent,
		kMessagingFetchRequestSent_MultiRequested,
		kMessagingFetchReceived,
		kMessagingFetchError,
		kMessagingComplete,
		kMessagingServiceNotAvailable
	} EMessagingState;

	virtual BOOL postBuild();
	virtual void onOpen(const LLSD& pKey);

	EMessagingState getMessagingState() const;
	BOOL            isMessagingInProgress() const;

protected:

private:
	typedef std::map<std::string, LLPathfindingCharacter> PathfindingCharacterMap;

	LLRootHandle<LLFloaterPathfindingCharacters> mSelfHandle;
	PathfindingCharacterMap                      mPathfindingCharacters;
	EMessagingState                              mMessagingState;
	LLScrollListCtrl                             *mCharactersScrollList;
	LLTextBase                                   *mCharactersStatus;
	LLTextBase                                   *mLabelActions;
	LLCheckBoxCtrl                               *mShowBeaconCheckBox;
	LLButton                                     *mTakeBtn;
	LLButton                                     *mTakeCopyBtn;
	LLButton                                     *mReturnBtn;
	LLButton                                     *mDeleteBtn;
	LLButton                                     *mTeleportBtn;
	LLObjectSelectionHandle                      mSelection;

	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingCharacters(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingCharacters();

	void sendCharactersDataGetRequest();
	void handleCharactersDataGetReply(const LLSD& pCharactersData);
	void handleCharactersDataGetError(const std::string& pURL, const std::string& pErrorReason);

	std::string getRegionName() const;
	std::string getCapabilityURL() const;

	void parseCharactersData(const LLSD &pCharactersData);

	void setMessagingState(EMessagingState pMessagingState);

	void onCharactersSelectionChange();
	void onRefreshCharactersClicked();
	void onSelectAllCharactersClicked();
	void onSelectNoneCharactersClicked();
	void onShowBeaconToggled();
	void onTakeCharactersClicked();
	void onTakeCopyCharactersClicked();
	void onReturnCharactersClicked();
	void onDeleteCharactersClicked();
	void onTeleportCharacterToMeClicked();

	void updateCharactersList();
	void selectAllCharacters();
	void selectNoneCharacters();

	void updateCharactersStatusMessage();

	void updateActionFields();
	void setEnableActionFields(BOOL pEnabled);
};

#endif // LL_LLFLOATERPATHFINDINGCHARACTERS_H
