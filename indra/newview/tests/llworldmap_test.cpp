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
#include "../test/lldoctest.h"

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
LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTexture(const LLUUID&, FTType, bool, LLGLTexture::EBoostLevel, S8,
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
std::string LLTrans::getString(std::string_view, const LLStringUtil::format_map_t&, bool def_string) { return std::string("test_trans"); }
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

TEST_SUITE("LLItemInfo") {

struct iteminfo_test
{

        // Instance to be tested
        LLItemInfo* mItem;

        // Constructor and destructor of the test wrapper
        iteminfo_test()
        {
            LLUUID id;
            mItem = new LLItemInfo(X_WORLD_TEST, Y_WORLD_TEST, ITEM_NAME_TEST, id);
        
};

TEST_CASE_FIXTURE(iteminfo_test, "test_1")
{

        // Test 1 : setCount() / getCount()
        mItem->setCount(10);
        CHECK_MESSAGE(mItem->getCount(, "LLItemInfo::setCount() test failed") == 10);
        // Test 2 : setTooltip() / getToolTip()
        std::string tooltip = TOOLTIP_TEST;
        mItem->setTooltip(tooltip);
        CHECK_MESSAGE(mItem->getToolTip(, "LLItemInfo::setTooltip() test failed") == TOOLTIP_TEST);
        // Test 3 : setElevation() / getGlobalPosition()
        mItem->setElevation(Z_WORLD_TEST);
        LLVector3d pos = mItem->getGlobalPosition();
        LLVector3d ref(X_WORLD_TEST, Y_WORLD_TEST, Z_WORLD_TEST);
        CHECK_MESSAGE(pos == ref, "LLItemInfo::getGlobalPosition() test failed");
        // Test 4 : getName()
        std::string name = mItem->getName();
        CHECK_MESSAGE(name == ITEM_NAME_TEST, "LLItemInfo::getName() test failed");
        // Test 5 : isName()
        CHECK_MESSAGE(mItem->isName(name, "LLItemInfo::isName() test failed"));
        // Test 6 : getUUID()
        LLUUID id;
        CHECK_MESSAGE(mItem->getUUID(, "LLItemInfo::getUUID() test failed") == id);
        // Test 7 : getRegionHandle()
        U64 handle = to_region_handle_global(X_WORLD_TEST, Y_WORLD_TEST);
        CHECK_MESSAGE(mItem->getRegionHandle(, "LLItemInfo::getRegionHandle() test failed") == handle);
    
}

TEST_CASE_FIXTURE(iteminfo_test, "test_1")
{

        // Test 1 : setName() / getName()
        std::string name = SIM_NAME_TEST;
        mSim->setName(name);
        CHECK_MESSAGE(mSim->getName(, "LLSimInfo::setName() test failed") == SIM_NAME_TEST);
        // Test 2 : isName()
        CHECK_MESSAGE(mSim->isName(name, "LLSimInfo::isName() test failed"));
        // Test 3 : getGlobalPos()
        LLVector3 local;
        LLVector3d ref(X_WORLD_TEST, Y_WORLD_TEST, 0.0f);
        LLVector3d pos = mSim->getGlobalPos(local);
        CHECK_MESSAGE(pos == ref, "LLSimInfo::getGlobalPos() test failed");
        // Test 4 : getGlobalOrigin()
        pos = mSim->getGlobalOrigin();
        CHECK_MESSAGE(pos == ref, "LLSimInfo::getGlobalOrigin() test failed");
        // Test 5 : clearImage()
        try {
            mSim->clearImage();
        
}

TEST_CASE_FIXTURE(iteminfo_test, "test_2")
{

        // Test 14 : clearItems()
        try {
            mSim->clearItems();
        
}

TEST_CASE_FIXTURE(iteminfo_test, "test_1")
{

        // Test 1 : reset()
        try {
            mWorld->reset();
        
}

TEST_CASE_FIXTURE(iteminfo_test, "test_2")
{

        // Test 8 : reset()
        try {
            mWorld->reset();
        
}

TEST_CASE_FIXTURE(iteminfo_test, "test_3")
{

        // Point to track
        LLVector3d pos( X_WORLD_TEST + REGION_WIDTH_METERS/2, Y_WORLD_TEST + REGION_WIDTH_METERS/2, Z_WORLD_TEST);

        // Test 20 : no tracking
        mWorld->cancelTracking();
        CHECK_MESSAGE(mWorld->isTracking(, "LLWorldMap::cancelTracking() at begin test failed") == false);

        // Test 21 : set tracking
        mWorld->setTracking(pos);
        CHECK_MESSAGE(mWorld->isTracking(, "LLWorldMap::setTracking() failed") && !mWorld->isTrackingValidLocation());

        // Test 22 : set click and commit flags
        mWorld->setTrackingDoubleClick();
        CHECK_MESSAGE(mWorld->isTrackingDoubleClick(, "LLWorldMap::setTrackingDoubleClick() failed"));
        mWorld->setTrackingCommit();
        CHECK_MESSAGE(mWorld->isTrackingCommit(, "LLWorldMap::setTrackingCommit() failed"));

        // Test 23 : in rectangle test
        bool inRect = mWorld->isTrackingInRectangle(    X_WORLD_TEST, Y_WORLD_TEST,
                                                        X_WORLD_TEST + REGION_WIDTH_METERS,
                                                        Y_WORLD_TEST + REGION_WIDTH_METERS);
        CHECK_MESSAGE(inRect, "LLWorldMap::isTrackingInRectangle() in rectangle failed");
        inRect = mWorld->isTrackingInRectangle(         X_WORLD_TEST + REGION_WIDTH_METERS,
                                                        Y_WORLD_TEST + REGION_WIDTH_METERS,
                                                        X_WORLD_TEST + 2 * REGION_WIDTH_METERS,
                                                        Y_WORLD_TEST + 2 * REGION_WIDTH_METERS);
        CHECK_MESSAGE(!inRect, "LLWorldMap::isTrackingInRectangle() outside rectangle failed");

        // Test 24 : set tracking to valid and invalid
        mWorld->setTrackingValid();
        CHECK_MESSAGE(mWorld->isTrackingValidLocation(, "LLWorldMap::setTrackingValid() failed") && !mWorld->isTrackingInvalidLocation());
        mWorld->setTrackingInvalid();
        CHECK_MESSAGE(!mWorld->isTrackingValidLocation(, "LLWorldMap::setTrackingInvalid() failed") && mWorld->isTrackingInvalidLocation());

        // Test 25 : getTrackedPositionGlobal()
        LLVector3d res = mWorld->getTrackedPositionGlobal();
        CHECK_MESSAGE(res == pos, "LLWorldMap::getTrackedPositionGlobal() failed");

        // Test 26 : reset tracking
        mWorld->cancelTracking();
        CHECK_MESSAGE(mWorld->isTracking(, "LLWorldMap::cancelTracking() at end test failed") == false);
    
}

} // TEST_SUITE

