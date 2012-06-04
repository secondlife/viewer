/** 
 * @file llfloaterpathfindingcharacters.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterpathfindingcharacters.h"

#include "llcheckboxctrl.h"
#include "llfloaterreg.h"
#include "llfloaterpathfindingobjects.h"
#include "llpathfindingcharacter.h"
#include "llpathfindingcharacterlist.h"
#include "llpathfindingmanager.h"
#include "llpathfindingobjectlist.h"
#include "llsd.h"
#include "lluicolortable.h"

#include "pipeline.h"
#include "llviewerobjectlist.h"

LLHandle<LLFloaterPathfindingCharacters> LLFloaterPathfindingCharacters::sInstanceHandle;

//---------------------------------------------------------------------------
// LLFloaterPathfindingCharacters
//---------------------------------------------------------------------------

void LLFloaterPathfindingCharacters::onClose(bool pIsAppQuitting)
{
	unhideAnyCharacters();
	LLFloaterPathfindingObjects::onClose( pIsAppQuitting );
}

BOOL LLFloaterPathfindingCharacters::isShowPhysicsCapsule() const
{
	return mShowPhysicsCapsuleCheckBox->get();
}

void LLFloaterPathfindingCharacters::setShowPhysicsCapsule(BOOL pIsShowPhysicsCapsule)
{
	mShowPhysicsCapsuleCheckBox->set(pIsShowPhysicsCapsule);
}

void LLFloaterPathfindingCharacters::openCharactersViewer()
{
	LLFloaterReg::toggleInstanceOrBringToFront("pathfinding_characters");
}

LLHandle<LLFloaterPathfindingCharacters> LLFloaterPathfindingCharacters::getInstanceHandle()
{
	if ( sInstanceHandle.isDead() )
	{
		LLFloaterPathfindingCharacters *floaterInstance = LLFloaterReg::getTypedInstance<LLFloaterPathfindingCharacters>("pathfinding_characters");
		if (floaterInstance != NULL)
		{
			sInstanceHandle = floaterInstance->mSelfHandle;
		}
	}

	return sInstanceHandle;
}

LLFloaterPathfindingCharacters::LLFloaterPathfindingCharacters(const LLSD& pSeed)
	: LLFloaterPathfindingObjects(pSeed),
	mShowPhysicsCapsuleCheckBox(NULL),
	mBeaconColor(),
	mSelfHandle()
{
	mSelfHandle.bind(this);
}

LLFloaterPathfindingCharacters::~LLFloaterPathfindingCharacters()
{
}

BOOL LLFloaterPathfindingCharacters::postBuild()
{
	mBeaconColor = LLUIColorTable::getInstance()->getColor("PathfindingCharacterBeaconColor");

	mShowPhysicsCapsuleCheckBox = findChild<LLCheckBoxCtrl>("show_physics_capsule");
	llassert(mShowPhysicsCapsuleCheckBox != NULL);
	mShowPhysicsCapsuleCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingCharacters::onShowPhysicsCapsuleClicked, this));

	return LLFloaterPathfindingObjects::postBuild();
}

void LLFloaterPathfindingCharacters::requestGetObjects()
{
	LLPathfindingManager::getInstance()->requestGetCharacters(getNewRequestId(), boost::bind(&LLFloaterPathfindingCharacters::handleNewObjectList, this, _1, _2, _3));
}

LLSD LLFloaterPathfindingCharacters::convertObjectsIntoScrollListData(const LLPathfindingObjectListPtr pObjectListPtr)
{
	llassert(pObjectListPtr != NULL);
	llassert(!pObjectListPtr->isEmpty());

	LLSD scrollListData;

	for (LLPathfindingObjectList::const_iterator objectIter = pObjectListPtr->begin();	objectIter != pObjectListPtr->end(); ++objectIter)
	{
		const LLPathfindingCharacter *characterPtr = dynamic_cast<const LLPathfindingCharacter *>(objectIter->second.get());
		LLSD element = buildCharacterScrollListData(characterPtr);
		scrollListData.append(element);

		if (characterPtr->hasOwner() && !characterPtr->hasOwnerName())
		{
			rebuildScrollListAfterAvatarNameLoads(characterPtr->getUUID());
		}
	}

	return scrollListData;
}

void LLFloaterPathfindingCharacters::updateControls()
{
	LLFloaterPathfindingObjects::updateControls();
	updateStateOnEditFields();
}

S32 LLFloaterPathfindingCharacters::getNameColumnIndex() const
{
	return 0;
}

const LLColor4 &LLFloaterPathfindingCharacters::getBeaconColor() const
{
	return mBeaconColor;
}

LLPathfindingObjectListPtr LLFloaterPathfindingCharacters::getEmptyObjectList() const
{
	LLPathfindingObjectListPtr objectListPtr(new LLPathfindingCharacterList());
	return objectListPtr;
}

void LLFloaterPathfindingCharacters::onShowPhysicsCapsuleClicked()
{
	LLVector3 pos;
	LLUUID id = getUUIDFromSelection( pos );
	if ( id.notNull() )
	{
		if ( isShowPhysicsCapsule() )
		{
			//We want to hide the VO and display the the objects physics capsule		
			gPipeline.hideObject( id );
		}
		else
		{
			gPipeline.restoreHiddenObject( id );
		}
	}
}

LLSD LLFloaterPathfindingCharacters::buildCharacterScrollListData(const LLPathfindingCharacter *pCharacterPtr) const
{
	LLSD columns;

	columns[0]["column"] = "name";
	columns[0]["value"] = pCharacterPtr->getName();
	columns[0]["font"] = "SANSSERIF";

	columns[1]["column"] = "description";
	columns[1]["value"] = pCharacterPtr->getDescription();
	columns[1]["font"] = "SANSSERIF";

	columns[2]["column"] = "owner";
	columns[2]["value"] = (pCharacterPtr->hasOwner() ?
		(pCharacterPtr->hasOwnerName() ? pCharacterPtr->getOwnerName() : getString("character_owner_loading")) :
		getString("character_owner_unknown"));
	columns[2]["font"] = "SANSSERIF";

	S32 cpuTime = llround(pCharacterPtr->getCPUTime());
	std::string cpuTimeString = llformat("%d", cpuTime);
	LLStringUtil::format_map_t string_args;
	string_args["[CPU_TIME]"] = cpuTimeString;

	columns[3]["column"] = "cpu_time";
	columns[3]["value"] = getString("character_cpu_time", string_args);
	columns[3]["font"] = "SANSSERIF";

	columns[4]["column"] = "altitude";
	columns[4]["value"] = llformat("%1.0f m", pCharacterPtr->getLocation()[2]);
	columns[4]["font"] = "SANSSERIF";

	LLSD element;
	element["id"] = pCharacterPtr->getUUID().asString();
	element["column"] = columns;

	return element;
}

void LLFloaterPathfindingCharacters::updateStateOnEditFields()
{
	int numSelectedItems = getNumSelectedObjects();;
	bool isEditEnabled = (numSelectedItems == 1);

	mShowPhysicsCapsuleCheckBox->setEnabled(isEditEnabled);
	if (!isEditEnabled)
	{
		setShowPhysicsCapsule(FALSE);
	}
}

LLUUID LLFloaterPathfindingCharacters::getUUIDFromSelection( LLVector3& pos )
{
	LLUUID uuid = LLUUID::null;

	if (getNumSelectedObjects() == 1)
	{
		LLPathfindingObjectPtr selectedObjectPtr = getFirstSelectedObject();
		uuid = selectedObjectPtr->getUUID();
		LLViewerObject *viewerObject = gObjectList.findObject(uuid);
		if ( viewerObject != NULL )
		{
			pos = viewerObject->getRenderPosition();
		}
	}

	return uuid;
}

void LLFloaterPathfindingCharacters::unhideAnyCharacters()
{
	LLPathfindingObjectListPtr objectListPtr = getSelectedObjects();
	for (LLPathfindingObjectList::const_iterator objectIter = objectListPtr->begin();
		objectIter != objectListPtr->end(); ++objectIter)
	{
		LLPathfindingObjectPtr objectPtr = objectIter->second;
		gPipeline.restoreHiddenObject(objectPtr->getUUID());
	}
}

BOOL LLFloaterPathfindingCharacters::isPhysicsCapsuleEnabled( LLUUID& id, LLVector3& pos )
{
	BOOL result = false;
	if ( isShowPhysicsCapsule() )
	{	
		 id = getUUIDFromSelection( pos );
		 result = true;
	}
	else
	{
		id.setNull();
	}
	return result;
}
