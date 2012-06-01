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

#include "llfloaterreg.h"
#include "llfloaterpathfindingobjects.h"
#include "llpathfindingcharacter.h"
#include "llpathfindingcharacterlist.h"
#include "llpathfindingmanager.h"
#include "llpathfindingobjectlist.h"
#include "llsd.h"
#include "lluicolortable.h"

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llfloater.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llselectmgr.h"
#include "pipeline.h"
#include "llviewerobjectlist.h"

LLHandle<LLFloaterPathfindingCharacters> LLFloaterPathfindingCharacters::sInstanceHandle;

//---------------------------------------------------------------------------
// LLFloaterPathfindingCharacters
//---------------------------------------------------------------------------

void LLFloaterPathfindingCharacters::openCharactersViewer()
{
	LLFloaterReg::toggleInstanceOrBringToFront("pathfinding_characters");
}
void LLFloaterPathfindingCharacters::onClose(bool pIsAppQuitting)
{
}

LLFloaterPathfindingCharacters::LLFloaterPathfindingCharacters(const LLSD& pSeed)
	: LLFloaterPathfindingObjects(pSeed),
	mBeaconColor(),
	mSelfHandle(),
	mShowPhysicsCapsuleCheckBox(NULL)
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

LLSD LLFloaterPathfindingCharacters::convertObjectsIntoScrollListData(const LLPathfindingObjectListPtr pObjectListPtr) const
{
	llassert(pObjectListPtr != NULL);
	llassert(!pObjectListPtr->isEmpty());

	LLSD scrollListData;

	for (LLPathfindingObjectList::const_iterator objectIter = pObjectListPtr->begin();	objectIter != pObjectListPtr->end(); ++objectIter)
	{
		const LLPathfindingCharacter *characterPtr = dynamic_cast<const LLPathfindingCharacter *>(objectIter->second.get());
		LLSD element = buildCharacterScrollListData(characterPtr);
		scrollListData.append(element);
	}

	return scrollListData;
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
	columns[2]["value"] = pCharacterPtr->getOwnerName();
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


void LLFloaterPathfindingCharacters::onShowPhysicsCapsuleClicked()
{
	 if ( mShowPhysicsCapsuleCheckBox->get() )
	 {
		//We want to hide the VO and display the the objects physics capsule		
		LLVector3 pos;
		LLUUID id = getUUIDFromSelection( pos );
		if ( id.notNull() )
		{
			gPipeline.hideObject( id );
		}		
	 }
	 else
	 {
		 //We want to restore the selected objects vo and disable the physics capsule rendering	
		  LLVector3 pos;
		  LLUUID id = getUUIDFromSelection( pos );
		  if ( id.notNull() )
		  {	
			gPipeline.restoreHiddenObject( id );
		  }		
	 }
}

BOOL LLFloaterPathfindingCharacters::isPhysicsCapsuleEnabled( LLUUID& id, LLVector3& pos )
{
	BOOL result = false;
	if ( mShowPhysicsCapsuleCheckBox->get() )
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

LLUUID LLFloaterPathfindingCharacters::getUUIDFromSelection( LLVector3& pos )
{ 	
	std::vector<LLScrollListItem*> selectedItems = mObjectsScrollList->getAllSelected();
	if ( selectedItems.size() > 1 )
	{
		return LLUUID::null;
	}
	if (selectedItems.size() == 1)
	{
		std::vector<LLScrollListItem*>::const_reference selectedItemRef = selectedItems.front();
		const LLScrollListItem *selectedItem = selectedItemRef;
		llassert(mObjectList != NULL);	
		LLViewerObject *viewerObject = gObjectList.findObject( selectedItem->getUUID() );
		if ( viewerObject != NULL )
		{
			pos = viewerObject->getRenderPosition();
		}
		//llinfos<<"id : "<<selectedItem->getUUID()<<llendl;
		return selectedItem->getUUID();
	}

	return LLUUID::null;
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

void LLFloaterPathfindingCharacters::updateStateOnEditFields()
{
	int numSelectedItems = mObjectsScrollList->getNumSelected();
	bool isEditEnabled = (numSelectedItems > 0);

	mShowPhysicsCapsuleCheckBox->setEnabled(isEditEnabled);

	LLFloaterPathfindingObjects::updateStateOnEditFields();
}
