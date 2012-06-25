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
#include "llhandle.h"
#include "llpathfindingcharacter.h"
#include "llpathfindingcharacterlist.h"
#include "llpathfindingmanager.h"
#include "llpathfindingobjectlist.h"
#include "llsd.h"
#include "lluicolortable.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "pipeline.h"

LLHandle<LLFloaterPathfindingCharacters> LLFloaterPathfindingCharacters::sInstanceHandle;

//---------------------------------------------------------------------------
// LLFloaterPathfindingCharacters
//---------------------------------------------------------------------------

void LLFloaterPathfindingCharacters::onClose(bool pIsAppQuitting)
{
	// Hide any capsule that might be showing on floater close
	hideCapsule();
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

BOOL LLFloaterPathfindingCharacters::isPhysicsCapsuleEnabled(LLUUID& id, LLVector3& pos, LLQuaternion& rot) const
{
	id = mSelectedCharacterId;
	// Physics capsule is enable if the checkbox is enabled and if we can get the required render 
	// parameters for any selected object
	return (isShowPhysicsCapsule() &&  getCapsuleRenderData(pos, rot ));
}

void LLFloaterPathfindingCharacters::openCharactersWithSelectedObjects()
{
	LLFloaterPathfindingCharacters *charactersFloater = LLFloaterReg::getTypedInstance<LLFloaterPathfindingCharacters>("pathfinding_characters");
	charactersFloater->showFloaterWithSelectionObjects();
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
	mSelectedCharacterId(),
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

	LLSD scrollListData = LLSD::emptyArray();

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

void LLFloaterPathfindingCharacters::updateControlsOnScrollListChange()
{
	LLFloaterPathfindingObjects::updateControlsOnScrollListChange();
	updateStateOnDisplayControls();
	showSelectedCharacterCapsules();
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
	if (mSelectedCharacterId.notNull() && isShowPhysicsCapsule())
	{
		showCapsule();
	}
	else
	{
		hideCapsule();
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

void LLFloaterPathfindingCharacters::updateStateOnDisplayControls()
{
	int numSelectedItems = getNumSelectedObjects();;
	bool isEditEnabled = (numSelectedItems == 1);

	mShowPhysicsCapsuleCheckBox->setEnabled(isEditEnabled);
	if (!isEditEnabled)
	{
		setShowPhysicsCapsule(FALSE);
	}
}

void LLFloaterPathfindingCharacters::showSelectedCharacterCapsules()
{
	// Hide any previous capsule
	hideCapsule();

	// Get the only selected object, or set the selected object to null if we do not have exactly
	// one object selected
	if (getNumSelectedObjects() == 1)
	{
		LLPathfindingObjectPtr selectedObjectPtr = getFirstSelectedObject();
		mSelectedCharacterId = selectedObjectPtr->getUUID();
	}
	else
	{
		mSelectedCharacterId.setNull();
	}

	// Show any capsule if enabled
	showCapsule();
}

void LLFloaterPathfindingCharacters::showCapsule() const
{
	if (mSelectedCharacterId.notNull() && isShowPhysicsCapsule())
	{
		gPipeline.hideObject(mSelectedCharacterId);
	}
}

void LLFloaterPathfindingCharacters::hideCapsule() const
{
	if (mSelectedCharacterId.notNull())
	{
		gPipeline.restoreHiddenObject(mSelectedCharacterId);
	}
}

bool LLFloaterPathfindingCharacters::getCapsuleRenderData(LLVector3& pPosition, LLQuaternion& rot) const
{
	bool result = false;

	// If we have a selected object, find the object on the viewer object list and return its
	// position.  Else, return false indicating that we either do not have a selected object
	// or we cannot find the selected object on the viewer object list
	if (mSelectedCharacterId.notNull())
	{
		LLViewerObject *viewerObject = gObjectList.findObject(mSelectedCharacterId);
		if ( viewerObject != NULL )
		{
			rot			= viewerObject->getRotation() ;
			pPosition	= viewerObject->getRenderPosition();		
			result		= true;
		}
	}

	return result;
}
