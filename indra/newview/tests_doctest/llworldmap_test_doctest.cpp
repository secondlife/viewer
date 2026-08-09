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

#include "doctest.h"
#include "indra/test/tut_compat_doctest.h"
#include "linden_common.h"
#include "llapr.h"
#include "llsingleton.h"
#include "lltrans.h"
#include "lluistring.h"
#include "../llviewertexture.h"
#include "../llviewercontrol.h"
#include "../llworldmapmessage.h"
#include "../llworldmap.h"

void LLGLTexture::setBoostLevel(S32) { }
void LLGLTexture::setAddressMode(LLTexUnit::eTextureAddressMode) { }
LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTexture(const LLUUID&, FTType, bool, LLGLTexture::EBoostLevel, S8,
                                                                  LLGLint, LLGLenum, LLHost)
{
    return NULL;
}

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

std::string LLTrans::getString(std::string_view, const LLStringUtil::format_map_t&, bool def_string) { return std::string("test_trans"); }
void LLUIString::updateResult() const { }
void LLUIString::setArg(const std::string&, const std::string&) { }
void LLUIString::assign(const std::string&) { }

LLControlGroup::LLControlGroup(const std::string& name) : LLInstanceTracker<LLControlGroup, std::string>(name) {}
LLControlGroup::~LLControlGroup() {}
bool LLControlGroup::getBOOL(std::string_view) { return true; }
LLControlGroup gSavedSettings("test_settings");

const F32 X_WORLD_TEST = 1000.0f * REGION_WIDTH_METERS;
const F32 Y_WORLD_TEST = 2000.0f * REGION_WIDTH_METERS;
const F32 Z_WORLD_TEST = 240.0f;
const std::string ITEM_NAME_TEST = "Item Foo";
const std::string TOOLTIP_TEST = "Tooltip Foo";
const std::string SIM_NAME_TEST = "Sim Foo";

namespace tut
{
    using tut_compat::ensure;
    using tut_compat::fail;

    struct iteminfo_test
    {
        iteminfo_test()
        {
            LLUUID id;
            mItem = new LLItemInfo(X_WORLD_TEST, Y_WORLD_TEST, ITEM_NAME_TEST, id);
        }

        ~iteminfo_test()
        {
            delete mItem;
        }

        LLItemInfo* mItem;
    };

    struct siminfo_test
    {
        siminfo_test()
        {
            U64 handle = to_region_handle_global(X_WORLD_TEST, Y_WORLD_TEST);
            mSim = new LLSimInfo(handle);
        }

        ~siminfo_test()
        {
            delete mSim;
        }

        LLSimInfo* mSim;
    };

    struct worldmap_test
    {
        worldmap_test()
        {
            mWorld = LLWorldMap::getInstance();
        }

        ~worldmap_test()
        {
            mWorld = NULL;
        }

        LLWorldMap* mWorld;
    };
}

TUT_SUITE("LLItemInfo")
{
    TUT_CASE("LLItemInfo::iteminfo_object_t_test_1")
    {
        using namespace tut;
        iteminfo_test data;
        data.mItem->setCount(10);
        ensure("LLItemInfo::setCount() test failed", data.mItem->getCount() == 10);
        std::string tooltip = TOOLTIP_TEST;
        data.mItem->setTooltip(tooltip);
        ensure("LLItemInfo::setTooltip() test failed", data.mItem->getToolTip() == TOOLTIP_TEST);
        data.mItem->setElevation(Z_WORLD_TEST);
        LLVector3d pos = data.mItem->getGlobalPosition();
        LLVector3d ref(X_WORLD_TEST, Y_WORLD_TEST, Z_WORLD_TEST);
        ensure("LLItemInfo::getGlobalPosition() test failed", pos == ref);
        std::string name = data.mItem->getName();
        ensure("LLItemInfo::getName() test failed", name == ITEM_NAME_TEST);
        ensure("LLItemInfo::isName() test failed", data.mItem->isName(name));
        LLUUID id;
        ensure("LLItemInfo::getUUID() test failed", data.mItem->getUUID() == id);
        U64 handle = to_region_handle_global(X_WORLD_TEST, Y_WORLD_TEST);
        ensure("LLItemInfo::getRegionHandle() test failed", data.mItem->getRegionHandle() == handle);
    }
}

TUT_SUITE("LLSimInfo")
{
    TUT_CASE("LLSimInfo::siminfo_object_t_test_1")
    {
        using namespace tut;
        siminfo_test data;
        std::string name = SIM_NAME_TEST;
        data.mSim->setName(name);
        ensure("LLSimInfo::setName() test failed", data.mSim->getName() == SIM_NAME_TEST);
        ensure("LLSimInfo::isName() test failed", data.mSim->isName(name));
        LLVector3 local;
        LLVector3d ref(X_WORLD_TEST, Y_WORLD_TEST, 0.0f);
        LLVector3d pos = data.mSim->getGlobalPos(local);
        ensure("LLSimInfo::getGlobalPos() test failed", pos == ref);
        pos = data.mSim->getGlobalOrigin();
        ensure("LLSimInfo::getGlobalOrigin() test failed", pos == ref);
        try
        {
            data.mSim->clearImage();
        }
        catch (...)
        {
            fail("LLSimInfo::clearImage() test failed");
        }
        try
        {
            data.mSim->dropImagePriority();
        }
        catch (...)
        {
            fail("LLSimInfo::dropImagePriority() test failed");
        }
        try
        {
            data.mSim->updateAgentCount(0.0f);
        }
        catch (...)
        {
            fail("LLSimInfo::updateAgentCount() test failed");
        }
        S32 agents = data.mSim->getAgentCount();
        ensure("LLSimInfo::getAgentCount() test failed", agents == 0);
        LLUUID id;
        data.mSim->setLandForSaleImage(id);
        LLPointer<LLViewerFetchedTexture> image = data.mSim->getLandForSaleImage();
        ensure("LLSimInfo::getLandForSaleImage() test failed", image.isNull());
        data.mSim->setAccess(SIM_ACCESS_PG);
        ensure("LLSimInfo::isPG() test failed", data.mSim->isPG());
        data.mSim->setAccess(SIM_ACCESS_DOWN);
        ensure("LLSimInfo::isDown() test failed", data.mSim->isDown());
    }

    TUT_CASE("LLSimInfo::siminfo_object_t_test_2")
    {
        using namespace tut;
        siminfo_test data;
        try
        {
            data.mSim->clearItems();
        }
        catch (...)
        {
            fail("LLSimInfo::clearItems() at init test failed");
        }

        LLSimInfo::item_info_list_t list;
        list = data.mSim->getTeleHub();
        ensure("LLSimInfo::getTeleHub() empty at init test failed", list.empty());
        list = data.mSim->getInfoHub();
        ensure("LLSimInfo::getInfoHub() empty at init test failed", list.empty());
        list = data.mSim->getPGEvent();
        ensure("LLSimInfo::getPGEvent() empty at init test failed", list.empty());
        list = data.mSim->getMatureEvent();
        ensure("LLSimInfo::getMatureEvent() empty at init test failed", list.empty());
        list = data.mSim->getLandForSale();
        ensure("LLSimInfo::getLandForSale() empty at init test failed", list.empty());
        list = data.mSim->getAgentLocation();
        ensure("LLSimInfo::getAgentLocation() empty at init test failed", list.empty());

        LLUUID id;
        LLItemInfo item(X_WORLD_TEST, Y_WORLD_TEST, ITEM_NAME_TEST, id);

        data.mSim->insertTeleHub(item);
        data.mSim->insertInfoHub(item);
        data.mSim->insertPGEvent(item);
        data.mSim->insertMatureEvent(item);
        data.mSim->insertLandForSale(item);
        data.mSim->insertAgentLocation(item);

        list = data.mSim->getTeleHub();
        ensure("LLSimInfo::insertTeleHub() test failed", list.size() == 1);
        list = data.mSim->getInfoHub();
        ensure("LLSimInfo::insertInfoHub() test failed", list.size() == 1);
        list = data.mSim->getPGEvent();
        ensure("LLSimInfo::insertPGEvent() test failed", list.size() == 1);
        list = data.mSim->getMatureEvent();
        ensure("LLSimInfo::insertMatureEvent() test failed", list.size() == 1);
        list = data.mSim->getLandForSale();
        ensure("LLSimInfo::insertLandForSale() test failed", list.size() == 1);
        list = data.mSim->getAgentLocation();
        ensure("LLSimInfo::insertAgentLocation() test failed", list.size() == 1);

        try
        {
            data.mSim->clearItems();
        }
        catch (...)
        {
            fail("LLSimInfo::clearItems() at end test failed");
        }

        list = data.mSim->getTeleHub();
        ensure("LLSimInfo::getTeleHub() empty after clear test failed", list.empty());
        list = data.mSim->getInfoHub();
        ensure("LLSimInfo::getInfoHub() empty after clear test failed", list.empty());
        list = data.mSim->getPGEvent();
        ensure("LLSimInfo::getPGEvent() empty after clear test failed", list.empty());
        list = data.mSim->getMatureEvent();
        ensure("LLSimInfo::getMatureEvent() empty after clear test failed", list.empty());
        list = data.mSim->getLandForSale();
        ensure("LLSimInfo::getLandForSale() empty after clear test failed", list.empty());
        list = data.mSim->getAgentLocation();
        ensure("LLSimInfo::getAgentLocation() empty after clear test failed", list.size() == 1);
    }
}

TUT_SUITE("LLWorldMap")
{
    TUT_CASE("LLWorldMap::worldmap_object_t_test_1")
    {
        using namespace tut;
        worldmap_test data;
        try
        {
            data.mWorld->reset();
        }
        catch (...)
        {
            fail("LLWorldMap::reset() at init test failed");
        }
        try
        {
            data.mWorld->clearImageRefs();
        }
        catch (...)
        {
            fail("LLWorldMap::clearImageRefs() test failed");
        }
        try
        {
            data.mWorld->dropImagePriorities();
        }
        catch (...)
        {
            fail("LLWorldMap::dropImagePriorities() test failed");
        }
        try
        {
            data.mWorld->reloadItems(true);
        }
        catch (...)
        {
            fail("LLWorldMap::reloadItems() test failed");
        }
        try
        {
            data.mWorld->updateRegions(1000, 1000, 1004, 1004);
        }
        catch (...)
        {
            fail("LLWorldMap::updateRegions() test failed");
        }
        try
        {
            data.mWorld->equalizeBoostLevels();
        }
        catch (...)
        {
            fail("LLWorldMap::equalizeBoostLevels() test failed");
        }
        try
        {
            LLPointer<LLViewerFetchedTexture> image = data.mWorld->getObjectsTile((U32)(X_WORLD_TEST / REGION_WIDTH_METERS), (U32)(Y_WORLD_TEST / REGION_WIDTH_METERS), 1);
            ensure("LLWorldMap::getObjectsTile() failed", image.isNull());
        }
        catch (...)
        {
            fail("LLWorldMap::getObjectsTile() test failed with exception");
        }
    }

    TUT_CASE("LLWorldMap::worldmap_object_t_test_2")
    {
        using namespace tut;
        worldmap_test data;
        try
        {
            data.mWorld->reset();
        }
        catch (...)
        {
            fail("LLWorldMap::reset() at init test failed");
        }

        LLWorldMap::sim_info_map_t list;
        list = data.mWorld->getRegionMap();
        ensure("LLWorldMap::getRegionMap() empty at init test failed", list.empty());

        bool success;
        LLUUID id;
        std::string name_sim = SIM_NAME_TEST;
        success = data.mWorld->insertRegion(U32(X_WORLD_TEST), U32(Y_WORLD_TEST), name_sim, id, SIM_ACCESS_PG, REGION_FLAGS_SANDBOX);
        list = data.mWorld->getRegionMap();
        ensure("LLWorldMap::insertRegion() failed", success && (list.size() == 1));

        std::string name_item = ITEM_NAME_TEST;
        success = data.mWorld->insertItem(U32(X_WORLD_TEST + REGION_WIDTH_METERS / 2), U32(Y_WORLD_TEST + REGION_WIDTH_METERS / 2), name_item, id, MAP_ITEM_LAND_FOR_SALE, 0, 0);
        list = data.mWorld->getRegionMap();
        ensure("LLWorldMap::insertItem() in existing region failed", success && (list.size() == 1));

        success = data.mWorld->insertItem(U32(X_WORLD_TEST + REGION_WIDTH_METERS * 2), U32(Y_WORLD_TEST + REGION_WIDTH_METERS * 2), name_item, id, MAP_ITEM_LAND_FOR_SALE, 0, 0);
        list = data.mWorld->getRegionMap();
        ensure("LLWorldMap::insertItem() in unexisting region failed", success && (list.size() == 2));

        LLVector3d pos1(X_WORLD_TEST + REGION_WIDTH_METERS * 2 + REGION_WIDTH_METERS / 2, Y_WORLD_TEST + REGION_WIDTH_METERS * 2 + REGION_WIDTH_METERS / 2, 0.0f);
        LLSimInfo* sim;
        sim = data.mWorld->simInfoFromPosGlobal(pos1);
        ensure("LLWorldMap::simInfoFromPosGlobal() test on existing region failed", sim != NULL);

        LLVector3d pos2(X_WORLD_TEST + REGION_WIDTH_METERS * 4 + REGION_WIDTH_METERS / 2, Y_WORLD_TEST + REGION_WIDTH_METERS * 4 + REGION_WIDTH_METERS / 2, 0.0f);
        sim = data.mWorld->simInfoFromPosGlobal(pos2);
        ensure("LLWorldMap::simInfoFromPosGlobal() test outside region failed", sim == NULL);

        sim = data.mWorld->simInfoFromName(name_sim);
        ensure("LLWorldMap::simInfoFromName() test on existing region failed", sim != NULL);

        U64 handle = to_region_handle_global(X_WORLD_TEST, Y_WORLD_TEST);
        sim = data.mWorld->simInfoFromHandle(handle);
        ensure("LLWorldMap::simInfoFromHandle() test on existing region failed", sim != NULL);

        LLVector3d pos3(X_WORLD_TEST + REGION_WIDTH_METERS / 2, Y_WORLD_TEST + REGION_WIDTH_METERS / 2, 0.0f);
        success = data.mWorld->simNameFromPosGlobal(pos3, name_sim);
        ensure("LLWorldMap::simNameFromPosGlobal() test on existing region failed", success && (name_sim == SIM_NAME_TEST));

        try
        {
            data.mWorld->reset();
        }
        catch (...)
        {
            fail("LLWorldMap::reset() at end test failed");
        }

        list = data.mWorld->getRegionMap();
        ensure("LLWorldMap::getRegionMap() empty at end test failed", list.empty());
    }

    TUT_CASE("LLWorldMap::worldmap_object_t_test_3")
    {
        using namespace tut;
        worldmap_test data;
        LLVector3d pos(X_WORLD_TEST + REGION_WIDTH_METERS / 2, Y_WORLD_TEST + REGION_WIDTH_METERS / 2, Z_WORLD_TEST);

        data.mWorld->cancelTracking();
        ensure("LLWorldMap::cancelTracking() at begin test failed", data.mWorld->isTracking() == false);

        data.mWorld->setTracking(pos);
        ensure("LLWorldMap::setTracking() failed", data.mWorld->isTracking() && !data.mWorld->isTrackingValidLocation());

        data.mWorld->setTrackingDoubleClick();
        ensure("LLWorldMap::setTrackingDoubleClick() failed", data.mWorld->isTrackingDoubleClick());
        data.mWorld->setTrackingCommit();
        ensure("LLWorldMap::setTrackingCommit() failed", data.mWorld->isTrackingCommit());

        bool inRect = data.mWorld->isTrackingInRectangle(X_WORLD_TEST, Y_WORLD_TEST,
                                                         X_WORLD_TEST + REGION_WIDTH_METERS,
                                                         Y_WORLD_TEST + REGION_WIDTH_METERS);
        ensure("LLWorldMap::isTrackingInRectangle() in rectangle failed", inRect);
        inRect = data.mWorld->isTrackingInRectangle(X_WORLD_TEST + REGION_WIDTH_METERS,
                                                    Y_WORLD_TEST + REGION_WIDTH_METERS,
                                                    X_WORLD_TEST + 2 * REGION_WIDTH_METERS,
                                                    Y_WORLD_TEST + 2 * REGION_WIDTH_METERS);
        ensure("LLWorldMap::isTrackingInRectangle() outside rectangle failed", !inRect);

        data.mWorld->setTrackingValid();
        ensure("LLWorldMap::setTrackingValid() failed", data.mWorld->isTrackingValidLocation() && !data.mWorld->isTrackingInvalidLocation());
        data.mWorld->setTrackingInvalid();
        ensure("LLWorldMap::setTrackingInvalid() failed", !data.mWorld->isTrackingValidLocation() && data.mWorld->isTrackingInvalidLocation());

        LLVector3d res = data.mWorld->getTrackedPositionGlobal();
        ensure("LLWorldMap::getTrackedPositionGlobal() failed", res == pos);

        data.mWorld->cancelTracking();
        ensure("LLWorldMap::cancelTracking() at end test failed", data.mWorld->isTracking() == false);
    }
}
