/** 
 * @file llviewernetwork_test.cpp
 * @author Roxie
 * @date 2009-03-9
 * @brief Test the viewernetwork functionality
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden LregisterSecAPIab
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
#include "../llviewerprecompiledheaders.h"
#include "../llviewernetwork.h"
#include "../test/lltut.h"
#include "../../llxml/llcontrol.h"
#include "llfile.h"

//----------------------------------------------------------------------------               
// Mock objects for the dependencies of the code we're testing                               

LLControlGroup::LLControlGroup(const std::string& name)
: LLInstanceTracker<LLControlGroup, std::string>(name) {}
LLControlGroup::~LLControlGroup() {}
BOOL LLControlGroup::declareString(const std::string& name,
                                   const std::string& initial_val,
                                   const std::string& comment,
                                   BOOL persist) {return TRUE;}
void LLControlGroup::setString(const std::string& name, const std::string& val){}

std::string gCmdLineLoginURI;
std::string gCmdLineGridChoice;
std::string gCmdLineHelperURI;
std::string gLoginPage;
std::string gCurrentGrid;
std::string LLControlGroup::getString(const std::string& name)
{
	if (name == "CmdLineGridChoice")
		return gCmdLineGridChoice;
	else if (name == "CmdLineHelperURI")
		return gCmdLineHelperURI;
	else if (name == "LoginPage")
		return gLoginPage;
	else if (name == "CurrentGrid")
		return gCurrentGrid;
	return "";
}

LLSD LLControlGroup::getLLSD(const std::string& name)
{
	if (name == "CmdLineLoginURI")
	{
		if(!gCmdLineLoginURI.empty())
		{
			return LLSD(gCmdLineLoginURI);
		}
	}
	return LLSD();
}


LLControlGroup gSavedSettings("test");

const char *gSampleGridFile = "<llsd><map>"
"<key>grid1</key><map>"
"  <key>favorite</key><integer>1</integer>"
"  <key>helper_uri</key><string>https://helper1/helpers/</string>"
"  <key>label</key><string>mylabel</string>"
"  <key>login_page</key><string>loginpage</string>"
"  <key>login_uri</key><array><string>myloginuri</string></array>"
"  <key>name</key><string>grid1</string>"
"  <key>visible</key><integer>1</integer>"
"  <key>credential_type</key><string>agent</string>"
"  <key>grid_login_id</key><string>MyGrid</string>"
"</map>"
"<key>util.agni.lindenlab.com</key><map>"
"  <key>favorite</key><integer>1</integer>"
"  <key>helper_uri</key><string>https://helper1/helpers/</string>"
"  <key>label</key><string>mylabel</string>"
"  <key>login_page</key><string>loginpage</string>"
"  <key>login_uri</key><array><string>myloginuri</string></array>"
"  <key>name</key><string>util.agni.lindenlab.com</string>"
"</map></map></llsd>";
// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------
namespace tut
{
  // Test wrapper declaration : wrapping nothing for the moment
  struct viewerNetworkTest
	{
		viewerNetworkTest()
		{
			LLFile::remove("grid_test.xml");
			gCmdLineLoginURI.clear();
			gCmdLineGridChoice.clear();
			gCmdLineHelperURI.clear();
			gLoginPage.clear();
			gCurrentGrid.clear();			
		}
		~viewerNetworkTest()
		{
			LLFile::remove("grid_test.xml");
		}
	};
	
	// Tut templating thingamagic: test group, object and test instance
	typedef test_group<viewerNetworkTest> viewerNetworkTestFactory;
	typedef viewerNetworkTestFactory::object viewerNetworkTestObject;
	tut::viewerNetworkTestFactory tut_test("llviewernetwork");
	
	// ---------------------------------------------------------------------------------------
	// Test functions 
	// ---------------------------------------------------------------------------------------
	// initialization without a grid file
	template<> template<>
	void viewerNetworkTestObject::test<1>()
	{

		LLGridManager *manager = LLGridManager::getInstance();
		// grid file doesn't exist
		manager->initialize("grid_test.xml");
		// validate that some of the defaults are available.
		std::map<std::string, std::string> known_grids = manager->getKnownGrids();
#ifndef LL_RELEASE_FOR_DOWNLOAD
		ensure_equals("Known grids is a string-string map of size 18", known_grids.size(), 18);
		ensure_equals("Agni has the right name and label", 
					  known_grids[std::string("util.agni.lindenlab.com")], std::string("Agni"));
#else // LL_RELEASE_FOR_DOWNLOAD
		ensure_equals("Known grids is a string-string map of size 2", known_grids.size(), 2);
		ensure_equals("Agni has the right name and label", 
					  known_grids[std::string("util.agni.lindenlab.com")], std::string("Secondlife.com"));		
#endif // LL_RELEASE_FOR_DOWNLOAD


		ensure_equals("None exists", known_grids[""], "None");
		
		LLSD grid = LLGridManager::getInstance()->getGridInfo("util.agni.lindenlab.com");
		ensure("Grid info for agni is a map", grid.isMap());
		ensure_equals("name is correct for agni", 
					  grid[GRID_VALUE].asString(), std::string("util.agni.lindenlab.com"));
#ifndef LL_RELEASE_FOR_DOWNLOAD		
		ensure_equals("label is correct for agni", 
					  grid[GRID_LABEL_VALUE].asString(), std::string("Agni"));
#else // LL_RELEASE_FOR_DOWNLOAD
		ensure_equals("label is correct for agni", 
					  grid[GRID_LABEL_VALUE].asString(), std::string("Secondlife.com"));		
#endif // LL_RELEASE_FOR_DOWNLOAD
		ensure("Login URI is an array", 
			   grid[GRID_LOGIN_URI_VALUE].isArray());
		ensure_equals("Agni login uri is correct", 
					  grid[GRID_LOGIN_URI_VALUE][0].asString(), 
					  std::string("https://login.agni.lindenlab.com/cgi-bin/login.cgi"));
		ensure_equals("Agni helper uri is correct",
					  grid[GRID_HELPER_URI_VALUE].asString(), 
					  std::string("https://secondlife.com/helpers/"));
		ensure_equals("Agni login page is correct",
					  grid[GRID_LOGIN_PAGE_VALUE].asString(), 
					  std::string("http://secondlife.com/app/login/"));
		ensure("Agni is a favorite",
			   grid.has(GRID_IS_FAVORITE_VALUE));
		ensure("Agni is a system grid", 
			   grid.has(GRID_IS_SYSTEM_GRID_VALUE));
		ensure("Grid file wasn't greated as it wasn't saved", 
			   !LLFile::isfile("grid_test.xml"));
	}
	
	// initialization with a grid file
	template<> template<>
	void viewerNetworkTestObject::test<2>()
	{
		llofstream gridfile("grid_test.xml");
		gridfile << gSampleGridFile;
		gridfile.close();
		
		LLGridManager::getInstance()->initialize("grid_test.xml");
		std::map<std::string, std::string> known_grids = LLGridManager::getInstance()->getKnownGrids();
#ifndef LL_RELEASE_FOR_DOWNLOAD
		ensure_equals("adding a grid via a grid file increases known grid size", 
					  known_grids.size(), 19);
		ensure_equals("Agni is still there after we've added a grid via a grid file", 
					  known_grids["util.agni.lindenlab.com"], std::string("Agni"));
	
#else
		ensure_equals("adding a grid via a grid file increases known grid size", 
					  known_grids.size(), 3);
		ensure_equals("Agni is still there after we've added a grid via a grid file", 
					  known_grids["util.agni.lindenlab.com"], std::string("Secondlife.com"));

#endif
	
		
		// assure Agni doesn't get overwritten
		LLSD grid = LLGridManager::getInstance()->getGridInfo("util.agni.lindenlab.com");
#ifndef LL_RELEASE_FOR_DOWNLOAD
		ensure_equals("Agni grid label was not modified by grid file", 
					  grid[GRID_LABEL_VALUE].asString(), std::string("Agni"));
#else \\ LL_RELEASE_FOR_DOWNLOAD
		ensure_equals("Agni grid label was not modified by grid file", 
					  grid[GRID_LABEL_VALUE].asString(), std::string("Secondlife.com"));
#endif \\ LL_RELEASE_FOR_DOWNLOAD
		
		ensure_equals("Agni name wasn't modified by grid file",
					  grid[GRID_VALUE].asString(), std::string("util.agni.lindenlab.com"));
		ensure("Agni grid URI is still an array after grid file", 
			   grid[GRID_LOGIN_URI_VALUE].isArray());
		ensure_equals("Agni login uri still the same after grid file", 
					  grid[GRID_LOGIN_URI_VALUE][0].asString(),  
					  std::string("https://login.agni.lindenlab.com/cgi-bin/login.cgi"));
		ensure_equals("Agni helper uri still the same after grid file", 
					  grid[GRID_HELPER_URI_VALUE].asString(), 
					  std::string("https://secondlife.com/helpers/"));
		ensure_equals("Agni login page the same after grid file", 
					  grid[GRID_LOGIN_PAGE_VALUE].asString(), 
					  std::string("http://secondlife.com/app/login/"));
		ensure("Agni still a favorite after grid file", 
			   grid.has(GRID_IS_FAVORITE_VALUE));
		ensure("Agni system grid still set after grid file", 
			   grid.has(GRID_IS_SYSTEM_GRID_VALUE));
		
		ensure_equals("Grid file adds to name<->label map", 
					  known_grids["grid1"], std::string("mylabel"));
		grid = LLGridManager::getInstance()->getGridInfo("grid1");
		ensure_equals("grid file grid name is set",
					  grid[GRID_VALUE].asString(), std::string("grid1"));
		ensure_equals("grid file label is set", 
					  grid[GRID_LABEL_VALUE].asString(), std::string("mylabel"));
		ensure("grid file login uri is an array",
			   grid[GRID_LOGIN_URI_VALUE].isArray());
		ensure_equals("grid file login uri is set",
					  grid[GRID_LOGIN_URI_VALUE][0].asString(), 
					  std::string("myloginuri"));
		ensure_equals("grid file helper uri is set",
					  grid[GRID_HELPER_URI_VALUE].asString(), 
					  std::string("https://helper1/helpers/"));
		ensure_equals("grid file login page is set",
					  grid[GRID_LOGIN_PAGE_VALUE].asString(), 
					  std::string("loginpage"));
		ensure("grid file favorite is set",
			   grid.has(GRID_IS_FAVORITE_VALUE));
		ensure("grid file isn't a system grid",
			   !grid.has(GRID_IS_SYSTEM_GRID_VALUE));		
		ensure("Grid file still exists after loading", 
			   LLFile::isfile("grid_test.xml"));
	}
	
	// Initialize via command line
	
	template<> template<>
	void viewerNetworkTestObject::test<3>()
	{	
		gCmdLineLoginURI = "https://my.login.uri/cgi-bin/login.cgi";
		
		LLGridManager::getInstance()->initialize("grid_test.xml");
		// with single login uri specified.
		std::map<std::string, std::string> known_grids = LLGridManager::getInstance()->getKnownGrids();
		ensure_equals("adding a command line grid increases known grid size", 
					  known_grids.size(), 19);
		ensure_equals("Command line grid is added to the list of grids", 
					  known_grids["my.login.uri"], std::string("my.login.uri"));
		LLSD grid = LLGridManager::getInstance()->getGridInfo("my.login.uri");
		ensure_equals("Command line grid name is set",
					  grid[GRID_VALUE].asString(), std::string("my.login.uri"));
		ensure_equals("Command line grid label is set", 
					  grid[GRID_LABEL_VALUE].asString(), std::string("my.login.uri"));
		ensure("Command line grid login uri is an array",
			   grid[GRID_LOGIN_URI_VALUE].isArray());
		ensure_equals("Command line grid login uri is set",
					  grid[GRID_LOGIN_URI_VALUE][0].asString(), 
					  std::string("https://my.login.uri/cgi-bin/login.cgi"));
		ensure_equals("Command line grid helper uri is set",
					  grid[GRID_HELPER_URI_VALUE].asString(), 
					  std::string("https://my.login.uri/helpers/"));
		ensure_equals("Command line grid login page is set",
					  grid[GRID_LOGIN_PAGE_VALUE].asString(), 
					  std::string("http://my.login.uri/app/login/"));
		ensure("Command line grid favorite is set",
			   !grid.has(GRID_IS_FAVORITE_VALUE));
		ensure("Command line grid isn't a system grid",
			   !grid.has(GRID_IS_SYSTEM_GRID_VALUE));		
		
		// now try a command line with a custom grid identifier
		gCmdLineGridChoice = "mycustomgridchoice";		
		LLGridManager::getInstance()->initialize("grid_test.xml");
		known_grids = LLGridManager::getInstance()->getKnownGrids();
		ensure_equals("adding a command line grid with custom name increases known grid size", 
					  known_grids.size(), 19);
		ensure_equals("Custom Command line grid is added to the list of grids", 
					  known_grids["mycustomgridchoice"], std::string("mycustomgridchoice"));
		grid = LLGridManager::getInstance()->getGridInfo("mycustomgridchoice");
		ensure_equals("Custom Command line grid name is set",
					  grid[GRID_VALUE].asString(), std::string("mycustomgridchoice"));
		ensure_equals("Custom Command line grid label is set", 
					  grid[GRID_LABEL_VALUE].asString(), std::string("mycustomgridchoice"));		
		ensure("Custom Command line grid login uri is an array",
			   grid[GRID_LOGIN_URI_VALUE].isArray());
		ensure_equals("Custom Command line grid login uri is set",
					  grid[GRID_LOGIN_URI_VALUE][0].asString(), 
					  std::string("https://my.login.uri/cgi-bin/login.cgi"));
		
		// add a helperuri
		gCmdLineHelperURI = "myhelperuri";
		LLGridManager::getInstance()->initialize("grid_test.xml");
		grid = LLGridManager::getInstance()->getGridInfo("mycustomgridchoice");		
		ensure_equals("Validate command line helper uri", 
					  grid[GRID_HELPER_URI_VALUE].asString(), std::string("myhelperuri"));		
		
		// add a login page
		gLoginPage = "myloginpage";
		LLGridManager::getInstance()->initialize("grid_test.xml");
		grid = LLGridManager::getInstance()->getGridInfo("mycustomgridchoice");		
		ensure_equals("Validate command line helper uri", 
					  grid[GRID_LOGIN_PAGE_VALUE].asString(), std::string("myloginpage"));			
	}
	
	// validate grid selection
	template<> template<>
	void viewerNetworkTestObject::test<4>()
	{	
		LLSD loginURI = LLSD::emptyArray();
		LLSD grid = LLSD::emptyMap();
		// adding a grid with simply a name will populate the values.
		grid[GRID_VALUE] = "myaddedgrid";

		LLGridManager::getInstance()->initialize("grid_test.xml");
		LLGridManager::getInstance()->addGrid(grid);
		LLGridManager::getInstance()->setGridChoice("util.agni.lindenlab.com");
#ifndef LL_RELEASE_FOR_DOWNLOAD		
		ensure_equals("getGridLabel", LLGridManager::getInstance()->getGridLabel(), std::string("Agni"));
#else // LL_RELEASE_FOR_DOWNLOAD
		ensure_equals("getGridLabel", LLGridManager::getInstance()->getGridLabel(), std::string("Secondlife.com"));		
#endif // LL_RELEASE_FOR_DOWNLOAD
		ensure_equals("getGrid", LLGridManager::getInstance()->getGrid(), 
					  std::string("util.agni.lindenlab.com"));
		ensure_equals("getHelperURI", LLGridManager::getInstance()->getHelperURI(), 
					  std::string("https://secondlife.com/helpers/"));
		ensure_equals("getLoginPage", LLGridManager::getInstance()->getLoginPage(), 
					  std::string("http://secondlife.com/app/login/"));
		ensure_equals("getLoginPage2", LLGridManager::getInstance()->getLoginPage("util.agni.lindenlab.com"), 
					  std::string("http://secondlife.com/app/login/"));
		ensure("Is Agni a production grid", LLGridManager::getInstance()->isInProductionGrid());		
		std::vector<std::string> uris;
		LLGridManager::getInstance()->getLoginURIs(uris);
		ensure_equals("getLoginURIs size", uris.size(), 1);
		ensure_equals("getLoginURIs", uris[0], 
					  std::string("https://login.agni.lindenlab.com/cgi-bin/login.cgi"));
		LLGridManager::getInstance()->setGridChoice("myaddedgrid");
		ensure_equals("getGridLabel", LLGridManager::getInstance()->getGridLabel(), std::string("myaddedgrid"));		
		ensure("Is myaddedgrid a production grid", !LLGridManager::getInstance()->isInProductionGrid());
		
		LLGridManager::getInstance()->setFavorite();
		grid = LLGridManager::getInstance()->getGridInfo("myaddedgrid");
		ensure("setting favorite", grid.has(GRID_IS_FAVORITE_VALUE));
	}
	
	// name based grid population
	template<> template<>
	void viewerNetworkTestObject::test<5>()
	{
		LLGridManager::getInstance()->initialize("grid_test.xml");
		LLSD grid = LLSD::emptyMap();
		// adding a grid with simply a name will populate the values.
		grid[GRID_VALUE] = "myaddedgrid";
		LLGridManager::getInstance()->addGrid(grid);
		grid = LLGridManager::getInstance()->getGridInfo("myaddedgrid");
		
		ensure_equals("name based grid has name value", 
					  grid[GRID_VALUE].asString(),
					  std::string("myaddedgrid"));
		ensure_equals("name based grid has label value", 
					  grid[GRID_LABEL_VALUE].asString(),
					  std::string("myaddedgrid"));
		ensure_equals("name based grid has name value", 
					  grid[GRID_HELPER_URI_VALUE].asString(),
					  std::string("https://myaddedgrid/helpers/"));
		ensure_equals("name based grid has name value", 
					  grid[GRID_LOGIN_PAGE_VALUE].asString(),
					  std::string("http://myaddedgrid/app/login/"));
		ensure("name based grid has array loginuri", 
			   grid[GRID_LOGIN_URI_VALUE].isArray());
		ensure_equals("name based grid has single login uri value",
			   grid[GRID_LOGIN_URI_VALUE].size(), 1);
		ensure_equals("Name based grid login uri is correct",
					  grid[GRID_LOGIN_URI_VALUE][0].asString(),
					  std::string("https://myaddedgrid/cgi-bin/login.cgi"));
		ensure("name based grid is not a favorite yet", 
			   !grid.has(GRID_IS_FAVORITE_VALUE));
		ensure("name based grid does not have system setting",
			   !grid.has(GRID_IS_SYSTEM_GRID_VALUE));
		
		llofstream gridfile("grid_test.xml");
		gridfile << gSampleGridFile;
		gridfile.close();
	}
	
	// persistence of the grid list with an empty gridfile.
	template<> template<>
	void viewerNetworkTestObject::test<6>()
	{
		// try with initial grid list without a grid file,
		// without setting the grid to a saveable favorite.
		LLGridManager::getInstance()->initialize("grid_test.xml");
		LLSD grid = LLSD::emptyMap();
		grid[GRID_VALUE] = std::string("mynewgridname");
		LLGridManager::getInstance()->addGrid(grid);
		LLGridManager::getInstance()->saveFavorites();
		ensure("Grid file exists after saving", 
			   LLFile::isfile("grid_test.xml"));
		LLGridManager::getInstance()->initialize("grid_test.xml");
		// should not be there
		std::map<std::string, std::string> known_grids = LLGridManager::getInstance()->getKnownGrids();
		ensure("New grid wasn't added to persisted list without being marked a favorite",
					  known_grids.find(std::string("mynewgridname")) == known_grids.end());
		
		// mark a grid a favorite to make sure it's persisted
		LLGridManager::getInstance()->addGrid(grid);
		LLGridManager::getInstance()->setGridChoice("mynewgridname");
		LLGridManager::getInstance()->setFavorite();
		LLGridManager::getInstance()->saveFavorites();
		ensure("Grid file exists after saving", 
			   LLFile::isfile("grid_test.xml"));
		LLGridManager::getInstance()->initialize("grid_test.xml");
		// should not be there
		known_grids = LLGridManager::getInstance()->getKnownGrids();
		ensure("New grid wasn't added to persisted list after being marked a favorite",
					  known_grids.find(std::string("mynewgridname")) !=
					  known_grids.end());
	}
	
	// persistence of the grid file with existing gridfile
	template<> template<>
	void viewerNetworkTestObject::test<7>()
	{
		
		llofstream gridfile("grid_test.xml");
		gridfile << gSampleGridFile;
		gridfile.close();
		
		LLGridManager::getInstance()->initialize("grid_test.xml");
		LLSD grid = LLSD::emptyMap();
		grid[GRID_VALUE] = std::string("mynewgridname");
		LLGridManager::getInstance()->addGrid(grid);
		LLGridManager::getInstance()->saveFavorites();
		// validate we didn't lose existing favorites
		LLGridManager::getInstance()->initialize("grid_test.xml");
		std::map<std::string, std::string> known_grids = LLGridManager::getInstance()->getKnownGrids();
		ensure("New grid wasn't added to persisted list after being marked a favorite",
			   known_grids.find(std::string("grid1")) !=
			   known_grids.end());
		
		// add a grid
		LLGridManager::getInstance()->addGrid(grid);
		LLGridManager::getInstance()->setGridChoice("mynewgridname");
		LLGridManager::getInstance()->setFavorite();
		LLGridManager::getInstance()->saveFavorites();
		known_grids = LLGridManager::getInstance()->getKnownGrids();
		ensure("New grid wasn't added to persisted list after being marked a favorite",
			   known_grids.find(std::string("grid1")) !=
			   known_grids.end());
		known_grids = LLGridManager::getInstance()->getKnownGrids();
		ensure("New grid wasn't added to persisted list after being marked a favorite",
			   known_grids.find(std::string("mynewgridname")) !=
			   known_grids.end());
	}
}
