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

// Dependencies
#include "linden_common.h"
#include "../llviewertexture.h"
#include "../llviewercontrol.h"
// Class to test
#include "../llworldmipmap.h"
// Tut header
#include "../test/lltut.h"

// -------------------------------------------------------------------------------------------
// Stubbing: Declarations required to link and run the class being tested
// Notes: 
// * Add here stubbed implementation of the few classes and methods used in the class to be tested
// * Add as little as possible (let the link errors guide you)
// * Do not make any assumption as to how those classes or methods work (i.e. don't copy/paste code)
// * A simulator for a class can be implemented here. Please comment and document thoroughly.

void LLGLTexture::setBoostLevel(S32 ) { }
LLViewerFetchedTexture* LLViewerTextureManager::getFetchedTextureFromUrl(const std::string&, BOOL, LLGLTexture::EBoostLevel, S8, 
																		 LLGLint, LLGLenum, const LLUUID& ) { return NULL; }

LLControlGroup::LLControlGroup(const std::string& name) : LLInstanceTracker<LLControlGroup, std::string>(name) { }
LLControlGroup::~LLControlGroup() { }
std::string LLControlGroup::getString(const std::string& ) { return std::string("test_url"); }
LLControlGroup gSavedSettings("test_settings");

// End Stubbing
// -------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------
namespace tut
{
	// Test wrapper declaration
	struct worldmipmap_test
	{
		// Derived test class
		class LLTestWorldMipmap : public LLWorldMipmap
		{
			// Put here stubbs of virtual methods we shouldn't call all the way down
		};
		// Instance to be tested
		LLTestWorldMipmap* mMap;

		// Constructor and destructor of the test wrapper
		worldmipmap_test()
		{
			mMap = new LLTestWorldMipmap;
		}
		~worldmipmap_test()
		{
			delete mMap;
		}
	};

	// Tut templating thingamagic: test group, object and test instance
	typedef test_group<worldmipmap_test> worldmipmap_t;
	typedef worldmipmap_t::object worldmipmap_object_t;
	tut::worldmipmap_t tut_worldmipmap("LLWorldMipmap");

	// ---------------------------------------------------------------------------------------
	// Test functions
	// Notes:
	// * Test as many as you possibly can without requiring a full blown simulation of everything
	// * The tests are executed in sequence so the test instance state may change between calls
	// * Remember that you cannot test private methods with tut
	// ---------------------------------------------------------------------------------------
	// Test static methods
	// Test 1 : scaleToLevel()
	template<> template<>
	void worldmipmap_object_t::test<1>()
	{
		S32 level = mMap->scaleToLevel(0.0);
		ensure("scaleToLevel() test 1 failed", level == LLWorldMipmap::MAP_LEVELS);
		level = mMap->scaleToLevel((F32)LLWorldMipmap::MAP_TILE_SIZE);
		ensure("scaleToLevel() test 2 failed", level == 1);
		level = mMap->scaleToLevel(10.f * LLWorldMipmap::MAP_TILE_SIZE);
		ensure("scaleToLevel() test 3 failed", level == 1);
	}
	// Test 2 : globalToMipmap()
	template<> template<>
	void worldmipmap_object_t::test<2>()
	{
		U32 grid_x, grid_y;
		mMap->globalToMipmap(1000.f*REGION_WIDTH_METERS, 1000.f*REGION_WIDTH_METERS, 1, &grid_x, &grid_y);
		ensure("globalToMipmap() test 1 failed", (grid_x == 1000) && (grid_y == 1000));
		mMap->globalToMipmap(0.0, 0.0, LLWorldMipmap::MAP_LEVELS, &grid_x, &grid_y);
		ensure("globalToMipmap() test 2 failed", (grid_x == 0) && (grid_y == 0));
	}
	// Test 3 : getObjectsTile()
	template<> template<>
	void worldmipmap_object_t::test<3>()
	{
		// Depends on some inline methods in LLViewerImage... Thinking about how to make this work
		// LLPointer<LLViewerImage> img = mMap->getObjectsTile(0, 0, 1);
		// ensure("getObjectsTile() test failed", img.isNull());
	}
	// Test 4 : equalizeBoostLevels()
	template<> template<>
	void worldmipmap_object_t::test<4>()
	{
		try
		{
			mMap->equalizeBoostLevels();
		} 
		catch (...)
		{
			fail("equalizeBoostLevels() test failed");
		}
	}
	// Test 5 : dropBoostLevels()
	template<> template<>
	void worldmipmap_object_t::test<5>()
	{
		try
		{
			mMap->dropBoostLevels();
		} 
		catch (...)
		{
			fail("dropBoostLevels() test failed");
		}
	}
	// Test 6 : reset()
	template<> template<>
	void worldmipmap_object_t::test<6>()
	{
		try
		{
			mMap->reset();
		} 
		catch (...)
		{
			fail("reset() test failed");
		}
	}
}
