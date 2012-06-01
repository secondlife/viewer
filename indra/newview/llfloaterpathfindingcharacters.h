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

#include "llfloaterpathfindingobjects.h"
#include "llpathfindingobjectlist.h"
#include "v4color.h"

class LLPathfindingCharacter;
class LLSD;

class LLFloaterPathfindingCharacters : public LLFloaterPathfindingObjects
{
public:
	static void  openCharactersViewer();
	/*virtual*/ void onClose(bool pIsAppQuitting);
	void updateStateOnEditFields();
protected:
	friend class LLFloaterReg;

	LLFloaterPathfindingCharacters(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingCharacters();

	virtual BOOL                       postBuild();

	virtual void                       requestGetObjects();

	virtual LLSD                       convertObjectsIntoScrollListData(const LLPathfindingObjectListPtr pObjectListPtr) const;

	virtual S32                        getNameColumnIndex() const;
	virtual const LLColor4             &getBeaconColor() const;

	virtual LLPathfindingObjectListPtr getEmptyObjectList() const;

	
	void unhideAnyCharacters();


private:
	LLSD buildCharacterScrollListData(const LLPathfindingCharacter *pCharacterPtr) const;

	LLColor4                           mBeaconColor;
	
	LLUUID getUUIDFromSelection( LLVector3& pos );

public:
	BOOL isPhysicsCapsuleEnabled( LLUUID& id, LLVector3& pos );
	void onShowPhysicsCapsuleClicked();
	LLRootHandle<LLFloaterPathfindingCharacters>     mSelfHandle;
	static LLHandle<LLFloaterPathfindingCharacters>  sInstanceHandle;
	static LLHandle<LLFloaterPathfindingCharacters> getInstanceHandle();

public:
	LLCheckBoxCtrl                     *mShowPhysicsCapsuleCheckBox;

};

#endif // LL_LLFLOATERPATHFINDINGCHARACTERS_H
