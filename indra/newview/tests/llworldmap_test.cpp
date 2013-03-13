/** 
 * @file llworldmap_test.cpp
 * @author Merov Linden
 * @date 2009-03-09
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

// Dependencies
#include "linden_common.h"
#include "llapr.h"
#include "llsingleton.h"
#include "lltrans.h"
#include "lluistring.h"
#include "../llviewertexture.h"
#include "../llworldmapmessage.h"
// Class to test
#include "../llworldmap.h"
// Tut header
#include "../test/lltut.h"

// -------------------------------------------------------------------------------------------
// Stubbing: Declarations required to link and run the class being tested
// Notes: 
// * Add here stubbed implementation of the few classes and methods used in the class to be tested
// * Add as little as possible (let the link errors guide you)
// * Do not make any assumption as to how those classes or methods work (i.e. don't copy/paste code)
// * A simulator for a class can be implemented here. Please comment and document thoroughly.

// Stub image calls
void LLGLTexture::setBoostLevel(S32 ) { }
void LLGLTexture::setAddressMode(LLTexUnit::eTextureAddressMode ) { }
LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTexture(const LLUUID&, FTType, BOOL, LLGLTexture::EBoostLevel, S8,
																  LLGLint, LLGLenum, LLHost ) { return NULL; }

// Stub related map calls
LLWorldMapMessage::LLWorldMapMessage() { }
LLWorldMapMessage::~LLWorldMapMessage() { }
void LLWorldMapMessage::sendItemRequest(U32 type, U64 handle) { }
void LLWorldMapMessage::sendMapBlockRequest(U16 min_x, U16 min_y, U16 max_x, U16 max_y, bool return_nonexistent) { }

LLWorldMipmap::LLWorldMipmap() { }
LLWorldMipmap::~LLWorldMipmap() { }
void LLWorldMipmap::reset() { }
void LLWorldMipmap::dropBoostLevels() { }
void LLWorldMipmap::equalizeBoostLevels() { }
LLPointer<LLViewerFetchedTexture> LLWorldMipmap::getObjectsTile(U32 grid_x, U32 grid_y, S32 level, bool load) { return NULL; }

// Stub other stuff
std::string LLTrans::getString(const std::string &, const LLStringUtil::format_map_t& ) { return std::string("test_trans"); }
void LLUIString::updateResult() const { }
void LLUIString::setArg(const std::string& , const std::string& ) { }
void LLUIString::assign(const std::string& ) { }

// End Stubbing
// -------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------

const F32 X_WORLD_TEST = 1000.0f * REGION_WIDTH_METERS;
const F32 Y_WORLD_TEST = 2000.0f * REGION_WIDTH_METERS;
const F32 Z_WORLD_TEST = 240.0f;
const std::string ITEM_NAME_TEST = "Item Foo";
const std::string TOOLTIP_TEST = "Tooltip Foo";

const std::string SIM_NAME_TEST = "Sim Foo";

namespace tut
{
	// Test wrapper declarations
	struct iteminfo_test
	{
		// Instance to be tested
		LLItemInfo* mItem;

		// Constructor and destructor of the test wrapper
		iteminfo_test()
		{
			LLUUID id;
			mItem = new LLItemInfo(X_WORLD_TEST, Y_WORLD_TEST, ITEM_NAME_TEST, id);
		}
		~iteminfo_test()
		{
			delete mItem;
		}
	};

	struct siminfo_test
	{
		// Instance to be tested
		LLSimInfo* mSim;

		// Constructor and destructor of the test wrapper
		siminfo_test()
		{
			U64 handle = to_region_handle_global(X_WORLD_TEST, Y_WORLD_TEST);
			mSim = new LLSimInfo(handle);
		}
		~siminfo_test()
		{
			delete mSim;
		}
	};

	struct worldmap_test
	{
		// Instance to be tested
		LLWorldMap* mWorld;

		// Constructor and destructor of the test wrapper
		worldmap_test()
		{
			mWorld = LLWorldMap::getInstance();
		}
		~worldmap_test()
		{
			mWorld = NULL;
		}
	};

	// Tut templating thingamagic: test group, object and test instance
	typedef test_group<iteminfo_test> iteminfo_t;
	typedef iteminfo_t::object iteminfo_object_t;
	tut::iteminfo_t tut_iteminfo("LLItemInfo");

	typedef test_group<siminfo_test> siminfo_t;
	typedef siminfo_t::object siminfo_object_t;
	tut::siminfo_t tut_siminfo("LLSimInfo");

	typedef test_group<worldmap_test> worldmap_t;
	typedef worldmap_t::object worldmap_object_t;
	tut::worldmap_t tut_worldmap("LLWorldMap");

	// ---------------------------------------------------------------------------------------
	// Test functions
	// Notes:
	// * Test as many as you possibly can without requiring a full blown simulation of everything
	// * The tests are executed in sequence so the test instance state may change between calls
	// * Remember that you cannot test private methods with tut
	// ---------------------------------------------------------------------------------------

	// ---------------------------------------------------------------------------------------
	// Test the LLItemInfo interface
	// ---------------------------------------------------------------------------------------
	template<> template<>
	void iteminfo_object_t::test<1>()
	{
		// Test 1 : setCount() / getCount()
		mItem->setCount(10);
		ensure("LLItemInfo::setCount() test failed", mItem->getCount() == 10);
		// Test 2 : setTooltip() / getToolTip()
		std::string tooltip = TOOLTIP_TEST;
		mItem->setTooltip(tooltip);
		ensure("LLItemInfo::setTooltip() test failed", mItem->getToolTip() == TOOLTIP_TEST);
		// Test 3 : setElevation() / getGlobalPosition()
		mItem->setElevation(Z_WORLD_TEST);
		LLVector3d pos = mItem->getGlobalPosition();
		LLVector3d ref(X_WORLD_TEST, Y_WORLD_TEST, Z_WORLD_TEST);
		ensure("LLItemInfo::getGlobalPosition() test failed", pos == ref);
		// Test 4 : getName()
		std::string name = mItem->getName();
		ensure("LLItemInfo::getName() test failed", name == ITEM_NAME_TEST);
		// Test 5 : isName()
		ensure("LLItemInfo::isName() test failed", mItem->isName(name));
		// Test 6 : getUUID()
		LLUUID id;
		ensure("LLItemInfo::getUUID() test failed", mItem->getUUID() == id);
		// Test 7 : getRegionHandle()
		U64 handle = to_region_handle_global(X_WORLD_TEST, Y_WORLD_TEST);
		ensure("LLItemInfo::getRegionHandle() test failed", mItem->getRegionHandle() == handle);
	}
	// ---------------------------------------------------------------------------------------
	// Test the LLSimInfo interface
	// ---------------------------------------------------------------------------------------
	// Test Setters and Accessors methods
	template<> template<>
	void siminfo_object_t::test<1>()
	{
		// Test 1 : setName() / getName()
		std::string name = SIM_NAME_TEST;
		mSim->setName(name);
		ensure("LLSimInfo::setName() test failed", mSim->getName() == SIM_NAME_TEST);
		// Test 2 : isName()
		ensure("LLSimInfo::isName() test failed", mSim->isName(name));
		// Test 3 : getGlobalPos()
		LLVector3 local;
		LLVector3d ref(X_WORLD_TEST, Y_WORLD_TEST, 0.0f);
		LLVector3d pos = mSim->getGlobalPos(local);
		ensure("LLSimInfo::getGlobalPos() test failed", pos == ref);
		// Test 4 : getGlobalOrigin()
		pos = mSim->getGlobalOrigin();
		ensure("LLSimInfo::getGlobalOrigin() test failed", pos == ref);
		// Test 5 : clearImage()
		try {
			mSim->clearImage();
		} catch (...) {
			fail("LLSimInfo::clearImage() test failed");
		}
		// Test 6 : dropImagePriority()
		try {
			mSim->dropImagePriority();
		} catch (...) {
			fail("LLSimInfo::dropImagePriority() test failed");
		}
		// Test 7 : updateAgentCount()
		try {
			mSim->updateAgentCount(0.0f);
		} catch (...) {
			fail("LLSimInfo::updateAgentCount() test failed");
		}
		// Test 8 : getAgentCount()
		S32 agents = mSim->getAgentCount();
		ensure("LLSimInfo::getAgentCount() test failed", agents == 0);
		// Test 9 : setLandForSaleImage() / getLandForSaleImage()
		LLUUID id;
		mSim->setLandForSaleImage(id);
		LLPointer<LLViewerFetchedTexture> image = mSim->getLandForSaleImage();
		ensure("LLSimInfo::getLandForSaleImage() test failed", image.isNull());
		// Test 10 : isPG()
		mSim->setAccess(SIM_ACCESS_PG);
		ensure("LLSimInfo::isPG() test failed", mSim->isPG());
		// Test 11 : isDown()
		mSim->setAccess(SIM_ACCESS_DOWN);
		ensure("LLSimInfo::isDown() test failed", mSim->isDown());
		// Test 12 : Access strings can't be accessed from unit test...
		//ensure("LLSimInfo::getAccessString() test failed", mSim->getAccessString() == "Offline");
		// Test 13 : Region strings can't be accessed from unit test...
		//mSim->setRegionFlags(REGION_FLAGS_SANDBOX);
		//ensure("LLSimInfo::setRegionFlags() test failed", mSim->getFlagsString() == "Sandbox");
	}
	// Test management of LLInfoItem lists
	template<> template<>
	void siminfo_object_t::test<2>()
	{
		// Test 14 : clearItems()
		try {
			mSim->clearItems();
		} catch (...) {
			fail("LLSimInfo::clearItems() at init test failed");
		}

		// Test 15 : Verify that all the lists are empty
		LLSimInfo::item_info_list_t list;
		list = mSim->getTeleHub();
		ensure("LLSimInfo::getTeleHub() empty at init test failed", list.empty());
		list = mSim->getInfoHub();
		ensure("LLSimInfo::getInfoHub() empty at init test failed", list.empty());
		list = mSim->getPGEvent();
		ensure("LLSimInfo::getPGEvent() empty at init test failed", list.empty());
		list = mSim->getMatureEvent();
		ensure("LLSimInfo::getMatureEvent() empty at init test failed", list.empty());
		list = mSim->getLandForSale();
		ensure("LLSimInfo::getLandForSale() empty at init test failed", list.empty());
		list = mSim->getAgentLocation();
		ensure("LLSimInfo::getAgentLocation() empty at init test failed", list.empty());

		// Create an item to be inserted
		LLUUID id;
		LLItemInfo item(X_WORLD_TEST, Y_WORLD_TEST, ITEM_NAME_TEST, id);

		// Insert the item in each list
		mSim->insertTeleHub(item);
		mSim->insertInfoHub(item);
		mSim->insertPGEvent(item);
		mSim->insertMatureEvent(item);
		mSim->insertLandForSale(item);
		mSim->insertAgentLocation(item);

		// Test 16 : Verify that the lists contain 1 item each
		list = mSim->getTeleHub();
		ensure("LLSimInfo::insertTeleHub() test failed", list.size() == 1);
		list = mSim->getInfoHub();
		ensure("LLSimInfo::insertInfoHub() test failed", list.size() == 1);
		list = mSim->getPGEvent();
		ensure("LLSimInfo::insertPGEvent() test failed", list.size() == 1);
		list = mSim->getMatureEvent();
		ensure("LLSimInfo::insertMatureEvent() test failed", list.size() == 1);
		list = mSim->getLandForSale();
		ensure("LLSimInfo::insertLandForSale() test failed", list.size() == 1);
		list = mSim->getAgentLocation();
		ensure("LLSimInfo::insertAgentLocation() test failed", list.size() == 1);

		// Test 17 : clearItems()
		try {
			mSim->clearItems();
		} catch (...) {
			fail("LLSimInfo::clearItems() at end test failed");
		}

		// Test 18 : Verify that all the lists are empty again... *except* agent which is persisted!! (on purpose)
		list = mSim->getTeleHub();
		ensure("LLSimInfo::getTeleHub() empty after clear test failed", list.empty());
		list = mSim->getInfoHub();
		ensure("LLSimInfo::getInfoHub() empty after clear test failed", list.empty());
		list = mSim->getPGEvent();
		ensure("LLSimInfo::getPGEvent() empty after clear test failed", list.empty());
		list = mSim->getMatureEvent();
		ensure("LLSimInfo::getMatureEvent() empty after clear test failed", list.empty());
		list = mSim->getLandForSale();
		ensure("LLSimInfo::getLandForSale() empty after clear test failed", list.empty());
		list = mSim->getAgentLocation();
		ensure("LLSimInfo::getAgentLocation() empty after clear test failed", list.size() == 1);
	}

	// ---------------------------------------------------------------------------------------
	// Test the LLWorldMap interface
	// ---------------------------------------------------------------------------------------
	// Test Setters and Accessors methods
	template<> template<>
	void worldmap_object_t::test<1>()
	{
		// Test 1 : reset()
		try {
			mWorld->reset();
		} catch (...) {
			fail("LLWorldMap::reset() at init test failed");
		}
		// Test 2 : clearImageRefs()
		try {
			mWorld->clearImageRefs();
		} catch (...) {
			fail("LLWorldMap::clearImageRefs() test failed");
		}
		// Test 3 : dropImagePriorities()
		try {
			mWorld->dropImagePriorities();
		} catch (...) {
			fail("LLWorldMap::dropImagePriorities() test failed");
		}
		// Test 4 : reloadItems()
		try {
			mWorld->reloadItems(true);
		} catch (...) {
			fail("LLWorldMap::reloadItems() test failed");
		}
		// Test 5 : updateRegions()
		try {
			mWorld->updateRegions(1000, 1000, 1004, 1004);
		} catch (...) {
			fail("LLWorldMap::updateRegions() test failed");
		}
		// Test 6 : equalizeBoostLevels()
 		try {
 			mWorld->equalizeBoostLevels();
 		} catch (...) {
 			fail("LLWorldMap::equalizeBoostLevels() test failed");
 		}
		// Test 7 : getObjectsTile()
		try {
			LLPointer<LLViewerFetchedTexture> image = mWorld->getObjectsTile((U32)(X_WORLD_TEST/REGION_WIDTH_METERS), (U32)(Y_WORLD_TEST/REGION_WIDTH_METERS), 1);
			ensure("LLWorldMap::getObjectsTile() failed", image.isNull());
		} catch (...) {
			fail("LLWorldMap::getObjectsTile() test failed with exception");
		}
	}
	// Test management of LLSimInfo lists
	template<> template<>
	void worldmap_object_t::test<2>()
	{
		// Test 8 : reset()
		try {
			mWorld->reset();
		} catch (...) {
			fail("LLWorldMap::reset() at init test failed");
		}

		// Test 9 : Verify that all the region list is empty
		LLWorldMap::sim_info_map_t list;
		list = mWorld->getRegionMap();
		ensure("LLWorldMap::getRegionMap() empty at init test failed", list.empty());

		// Test 10 : Insert a region
		bool success;
		LLUUID id;
		std::string name_sim = SIM_NAME_TEST;
		success = mWorld->insertRegion(	U32(X_WORLD_TEST), 
						U32(Y_WORLD_TEST), 
										name_sim,
										id,
										SIM_ACCESS_PG,
										REGION_FLAGS_SANDBOX);
		list = mWorld->getRegionMap();
		ensure("LLWorldMap::insertRegion() failed", success && (list.size() == 1));

		// Test 11 : Insert an item in the same region -> number of regions doesn't increase
		std::string name_item = ITEM_NAME_TEST;
		success = mWorld->insertItem(	U32(X_WORLD_TEST + REGION_WIDTH_METERS/2),
						U32(Y_WORLD_TEST + REGION_WIDTH_METERS/2), 
										name_item,
										id,
										MAP_ITEM_LAND_FOR_SALE,
										0, 0);
		list = mWorld->getRegionMap();
		ensure("LLWorldMap::insertItem() in existing region failed", success && (list.size() == 1));

		// Test 12 : Insert an item in another region -> number of regions increases
		success = mWorld->insertItem(	U32(X_WORLD_TEST + REGION_WIDTH_METERS*2), 
						U32(Y_WORLD_TEST + REGION_WIDTH_METERS*2), 
										name_item,
										id,
										MAP_ITEM_LAND_FOR_SALE,
										0, 0);
		list = mWorld->getRegionMap();
		ensure("LLWorldMap::insertItem() in unexisting region failed", success && (list.size() == 2));

		// Test 13 : simInfoFromPosGlobal() in region
		LLVector3d pos1(	X_WORLD_TEST + REGION_WIDTH_METERS*2 + REGION_WIDTH_METERS/2, 
							Y_WORLD_TEST + REGION_WIDTH_METERS*2 + REGION_WIDTH_METERS/2, 
							0.0f);
		LLSimInfo* sim;
		sim = mWorld->simInfoFromPosGlobal(pos1);
		ensure("LLWorldMap::simInfoFromPosGlobal() test on existing region failed", sim != NULL);

		// Test 14 : simInfoFromPosGlobal() outside region
		LLVector3d pos2(	X_WORLD_TEST + REGION_WIDTH_METERS*4 + REGION_WIDTH_METERS/2, 
							Y_WORLD_TEST + REGION_WIDTH_METERS*4 + REGION_WIDTH_METERS/2, 
							0.0f);
		sim = mWorld->simInfoFromPosGlobal(pos2);
		ensure("LLWorldMap::simInfoFromPosGlobal() test outside region failed", sim == NULL);

		// Test 15 : simInfoFromName()
		sim = mWorld->simInfoFromName(name_sim);
		ensure("LLWorldMap::simInfoFromName() test on existing region failed", sim != NULL);

		// Test 16 : simInfoFromHandle()
		U64 handle = to_region_handle_global(X_WORLD_TEST, Y_WORLD_TEST);
		sim = mWorld->simInfoFromHandle(handle);
		ensure("LLWorldMap::simInfoFromHandle() test on existing region failed", sim != NULL);

		// Test 17 : simNameFromPosGlobal()
		LLVector3d pos3(	X_WORLD_TEST + REGION_WIDTH_METERS/2, 
							Y_WORLD_TEST + REGION_WIDTH_METERS/2, 
							0.0f);
		success = mWorld->simNameFromPosGlobal(pos3, name_sim);
		ensure("LLWorldMap::simNameFromPosGlobal() test on existing region failed", success && (name_sim == SIM_NAME_TEST));
		
		// Test 18 : reset()
		try {
			mWorld->reset();
		} catch (...) {
			fail("LLWorldMap::reset() at end test failed");
		}

		// Test 19 : Verify that all the region list is empty
		list = mWorld->getRegionMap();
		ensure("LLWorldMap::getRegionMap() empty at end test failed", list.empty());
	}
	// Test tracking
	template<> template<>
	void worldmap_object_t::test<3>()
	{
		// Point to track
		LLVector3d pos( X_WORLD_TEST + REGION_WIDTH_METERS/2, Y_WORLD_TEST + REGION_WIDTH_METERS/2, Z_WORLD_TEST);

		// Test 20 : no tracking
		mWorld->cancelTracking();
		ensure("LLWorldMap::cancelTracking() at begin test failed", mWorld->isTracking() == false);

		// Test 21 : set tracking
		mWorld->setTracking(pos);
		ensure("LLWorldMap::setTracking() failed", mWorld->isTracking() && !mWorld->isTrackingValidLocation());

		// Test 22 : set click and commit flags
		mWorld->setTrackingDoubleClick();
		ensure("LLWorldMap::setTrackingDoubleClick() failed", mWorld->isTrackingDoubleClick());
		mWorld->setTrackingCommit();
		ensure("LLWorldMap::setTrackingCommit() failed", mWorld->isTrackingCommit());

		// Test 23 : in rectangle test
		bool inRect = mWorld->isTrackingInRectangle(	X_WORLD_TEST, Y_WORLD_TEST,  
														X_WORLD_TEST + REGION_WIDTH_METERS, 
														Y_WORLD_TEST + REGION_WIDTH_METERS);
		ensure("LLWorldMap::isTrackingInRectangle() in rectangle failed", inRect);
		inRect = mWorld->isTrackingInRectangle(			X_WORLD_TEST + REGION_WIDTH_METERS, 
														Y_WORLD_TEST + REGION_WIDTH_METERS,  
														X_WORLD_TEST + 2 * REGION_WIDTH_METERS, 
														Y_WORLD_TEST + 2 * REGION_WIDTH_METERS);
		ensure("LLWorldMap::isTrackingInRectangle() outside rectangle failed", !inRect);

		// Test 24 : set tracking to valid and invalid
		mWorld->setTrackingValid();
		ensure("LLWorldMap::setTrackingValid() failed", mWorld->isTrackingValidLocation() && !mWorld->isTrackingInvalidLocation());
		mWorld->setTrackingInvalid();
		ensure("LLWorldMap::setTrackingInvalid() failed", !mWorld->isTrackingValidLocation() && mWorld->isTrackingInvalidLocation());

		// Test 25 : getTrackedPositionGlobal()
		LLVector3d res = mWorld->getTrackedPositionGlobal();
		ensure("LLWorldMap::getTrackedPositionGlobal() failed", res == pos);

		// Test 26 : reset tracking
		mWorld->cancelTracking();
		ensure("LLWorldMap::cancelTracking() at end test failed", mWorld->isTracking() == false);
	}
}
