/** 
 * @file llworldmipmap_test.cpp
 * @author Merov Linden
 * @date 2009-02-03
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

// Precompiled header: almost always required for newview cpp files
#include "../llviewerprecompiledheaders.h"
// Class to test
#include "../llworldmipmap.h"
// Dependencies
#include "../llviewerimagelist.h"
// Tut header
#include "../test/lltut.h"

// -------------------------------------------------------------------------------------------
// Stubbing: Declarations required to link and run the class being tested
// Notes: 
// * Add here stubbed implementation of the few classes and methods used in the class to be tested
// * Add as little as possible (let the link errors guide you)
// * Do not make any assumption as to how those classes or methods work (i.e. don't copy/paste code)
// * A simulator for a class can be implemented here. Please comment and document thoroughly.

LLViewerImageList::LLViewerImageList() { }
LLViewerImageList::~LLViewerImageList() { }

LLViewerImageList gImageList;

LLViewerImage* LLViewerImageList::getImageFromUrl(const std::string& url,
												   BOOL usemipmaps,
												   BOOL level_immediate,
												   LLGLint internal_format,
												   LLGLenum primary_format, 
												   const LLUUID& force_id)
{ return NULL; }
void LLViewerImage::setBoostLevel(S32 level) { }

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
	tut::worldmipmap_t tut_worldmipmap("worldmipmap");

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
		level = mMap->scaleToLevel(LLWorldMipmap::MAP_TILE_SIZE);
		ensure("scaleToLevel() test 2 failed", level == 1);
		level = mMap->scaleToLevel(10 * LLWorldMipmap::MAP_TILE_SIZE);
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
