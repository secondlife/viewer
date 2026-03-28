/**
 * @file llworldmipmap_test.cpp
 * @author Merov Linden
 * @date 2009-02-03
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

#include "../llviewertexture.h"
#include "../llviewercontrol.h"
#include "../llworldmipmap.h"

void LLGLTexture::setBoostLevel(S32) { }
LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTextureFromUrl(const std::string&, FTType, bool, LLGLTexture::EBoostLevel, S8,
                                                                         LLGLint, LLGLenum, const LLUUID&)
{
    return NULL;
}

LLControlGroup::LLControlGroup(const std::string& name) : LLInstanceTracker<LLControlGroup, std::string>(name) { }
LLControlGroup::~LLControlGroup() { }
std::string LLControlGroup::getString(std::string_view) { return std::string("test_url"); }
LLControlGroup gSavedSettings("test_settings");

namespace tut
{
    using tut_compat::ensure;
    using tut_compat::fail;

    struct worldmipmap_test
    {
        class LLTestWorldMipmap : public LLWorldMipmap
        {
        };

        worldmipmap_test()
        {
            mMap = new LLTestWorldMipmap;
        }

        ~worldmipmap_test()
        {
            delete mMap;
        }

        LLTestWorldMipmap* mMap;
    };
}

TUT_SUITE("LLWorldMipmap")
{
    TUT_CASE("LLWorldMipmap::worldmipmap_object_t_test_1")
    {
        using namespace tut;
        worldmipmap_test data;
        S32 level = data.mMap->scaleToLevel(0.0);
        ensure("scaleToLevel() test 1 failed", level == LLWorldMipmap::MAP_LEVELS);
        level = data.mMap->scaleToLevel((F32)LLWorldMipmap::MAP_TILE_SIZE);
        ensure("scaleToLevel() test 2 failed", level == 1);
        level = data.mMap->scaleToLevel(10.f * LLWorldMipmap::MAP_TILE_SIZE);
        ensure("scaleToLevel() test 3 failed", level == 1);
    }

    TUT_CASE("LLWorldMipmap::worldmipmap_object_t_test_2")
    {
        using namespace tut;
        worldmipmap_test data;
        U32 grid_x, grid_y;
        data.mMap->globalToMipmap(1000.f * REGION_WIDTH_METERS, 1000.f * REGION_WIDTH_METERS, 1, &grid_x, &grid_y);
        ensure("globalToMipmap() test 1 failed", (grid_x == 1000) && (grid_y == 1000));
        data.mMap->globalToMipmap(0.0, 0.0, LLWorldMipmap::MAP_LEVELS, &grid_x, &grid_y);
        ensure("globalToMipmap() test 2 failed", (grid_x == 0) && (grid_y == 0));
    }

    TUT_CASE("LLWorldMipmap::worldmipmap_object_t_test_3")
    {
    }

    TUT_CASE("LLWorldMipmap::worldmipmap_object_t_test_4")
    {
        using namespace tut;
        worldmipmap_test data;
        try
        {
            data.mMap->equalizeBoostLevels();
        }
        catch (...)
        {
            fail("equalizeBoostLevels() test failed");
        }
    }

    TUT_CASE("LLWorldMipmap::worldmipmap_object_t_test_5")
    {
        using namespace tut;
        worldmipmap_test data;
        try
        {
            data.mMap->dropBoostLevels();
        }
        catch (...)
        {
            fail("dropBoostLevels() test failed");
        }
    }

    TUT_CASE("LLWorldMipmap::worldmipmap_object_t_test_6")
    {
        using namespace tut;
        worldmipmap_test data;
        try
        {
            data.mMap->reset();
        }
        catch (...)
        {
            fail("reset() test failed");
        }
    }
}
