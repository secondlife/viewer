/** 
 * @file llviewernetwork_test.cpp
 * @author Roxie
 * @date 2009-03-9
 * @brief Test the viewernetwork functionality
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

LLPointer<LLControlVariable> LLControlGroup::getControl(const std::string& name)
{
	ctrl_name_table_t::iterator iter = mNameTable.find(name);
	return iter == mNameTable.end() ? LLPointer<LLControlVariable>() : iter->second;
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
	tut::viewerNetworkTestFactory tut_test("LLViewerNetwork");
	
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
		ensure_equals("Known grids is a string-string map of size 23", known_grids.size(), 23);
		ensure_equals("Agni has the right name and label", 
					  known_grids[std::string("util.agni.lindenlab.com")], std::string("Agni"));
		ensure_equals("None exists", known_grids[""], "None");
		
		LLSD grid;
		LLGridManager::getInstance()->getGridInfo("util.agni.lindenlab.com", grid);
		ensure("Grid info for agni is a map", grid.isMap());
		ensure_equals("name is correct for agni", 
					  grid[GRID_VALUE].asString(), std::string("util.agni.lindenlab.com"));
		ensure_equals("label is correct for agni", 
					  grid[GRID_LABEL_VALUE].asString(), std::string("Agni"));
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
					  std::string("http://viewer-login.agni.lindenlab.com/"));
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
		ensure_equals("adding a grid via a grid file increases known grid size", 
					  known_grids.size(), 24);
		ensure_equals("Agni is still there after we've added a grid via a grid file", 
					  known_grids["util.agni.lindenlab.com"], std::string("Agni"));
	
		
		// assure Agni doesn't get overwritten
		LLSD grid;
		LLGridManager::getInstance()->getGridInfo("util.agni.lindenlab.com", grid);

		ensure_equals("Agni grid label was not modified by grid file", 
					  grid[GRID_LABEL_VALUE].asString(), std::string("Agni"));
		
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
					  std::string("http://viewer-login.agni.lindenlab.com/"));
		ensure("Agni still a favorite after grid file", 
			   grid.has(GRID_IS_FAVORITE_VALUE));
		ensure("Agni system grid still set after grid file", 
			   grid.has(GRID_IS_SYSTEM_GRID_VALUE));
		
		ensure_equals("Grid file adds to name<->label map", 
					  known_grids["grid1"], std::string("mylabel"));
		LLGridManager::getInstance()->getGridInfo("grid1", grid);
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
		// USE --grid command line
		// initialize with a known grid
		LLSD grid;
		gCmdLineGridChoice = "Aditi";
		LLGridManager::getInstance()->initialize("grid_test.xml");
		// with single login uri specified.
		std::map<std::string, std::string> known_grids = LLGridManager::getInstance()->getKnownGrids();
		ensure_equals("Using a known grid via command line doesn't increase number of known grids", 
					  known_grids.size(), 23);
		ensure_equals("getGridLabel", LLGridManager::getInstance()->getGridLabel(), std::string("Aditi"));
		// initialize with a known grid in lowercase
		gCmdLineGridChoice = "agni";
		LLGridManager::getInstance()->initialize("grid_test.xml");
		ensure_equals("getGridLabel", LLGridManager::getInstance()->getGridLabel(), std::string("Agni"));		
		
		// now try a command line with a custom grid identifier
		gCmdLineGridChoice = "mycustomgridchoice";		
		LLGridManager::getInstance()->initialize("grid_test.xml");
		known_grids = LLGridManager::getInstance()->getKnownGrids();
		ensure_equals("adding a command line grid with custom name increases known grid size", 
					  known_grids.size(), 24);
		ensure_equals("Custom Command line grid is added to the list of grids", 
					  known_grids["mycustomgridchoice"], std::string("mycustomgridchoice"));
		LLGridManager::getInstance()->getGridInfo("mycustomgridchoice", grid);
		ensure_equals("Custom Command line grid name is set",
					  grid[GRID_VALUE].asString(), std::string("mycustomgridchoice"));
		ensure_equals("Custom Command line grid label is set", 
					  grid[GRID_LABEL_VALUE].asString(), std::string("mycustomgridchoice"));		
		ensure("Custom Command line grid login uri is an array",
			   grid[GRID_LOGIN_URI_VALUE].isArray());
		ensure_equals("Custom Command line grid login uri is set",
					  grid[GRID_LOGIN_URI_VALUE][0].asString(), 
					  std::string("https://mycustomgridchoice/cgi-bin/login.cgi"));
		ensure_equals("Custom Command line grid helper uri is set",
					  grid[GRID_HELPER_URI_VALUE].asString(), 
					  std::string("https://mycustomgridchoice/helpers/"));
		ensure_equals("Custom Command line grid login page is set",
					  grid[GRID_LOGIN_PAGE_VALUE].asString(), 
					  std::string("http://mycustomgridchoice/app/login/"));
	}
	
	// validate override of login uri with cmd line
	template<> template<>
	void viewerNetworkTestObject::test<4>()
	{			
		// Override with loginuri
		// override known grid
		LLSD grid;
		gCmdLineGridChoice = "Aditi";
		gCmdLineLoginURI = "https://my.login.uri/cgi-bin/login.cgi";		
		LLGridManager::getInstance()->initialize("grid_test.xml");		
		std::map<std::string, std::string> known_grids = LLGridManager::getInstance()->getKnownGrids();		
		ensure_equals("Override known grid login uri: No grids are added", 
					  known_grids.size(), 23);
		LLGridManager::getInstance()->getGridInfo(grid);
		ensure("Override known grid login uri: login uri is an array",
			   grid[GRID_LOGIN_URI_VALUE].isArray());
		ensure_equals("Override known grid login uri: Command line grid login uri is set",
					  grid[GRID_LOGIN_URI_VALUE][0].asString(), 
					  std::string("https://my.login.uri/cgi-bin/login.cgi"));
		ensure_equals("Override known grid login uri: helper uri is not changed",
					  grid[GRID_HELPER_URI_VALUE].asString(), 
					  std::string("http://aditi-secondlife.webdev.lindenlab.com/helpers/"));
		ensure_equals("Override known grid login uri: login page is not set",
					  grid[GRID_LOGIN_PAGE_VALUE].asString(), 
					  std::string("http://viewer-login.agni.lindenlab.com/"));		
		
		// Override with loginuri
		// override custom grid
		gCmdLineGridChoice = "mycustomgridchoice";
		gCmdLineLoginURI = "https://my.login.uri/cgi-bin/login.cgi";		
		LLGridManager::getInstance()->initialize("grid_test.xml");		
		known_grids = LLGridManager::getInstance()->getKnownGrids();
		LLGridManager::getInstance()->getGridInfo(grid);
		ensure_equals("Override custom grid login uri: Grid is added", 
					  known_grids.size(), 24);		
		ensure("Override custom grid login uri: login uri is an array",
			   grid[GRID_LOGIN_URI_VALUE].isArray());
		ensure_equals("Override custom grid login uri: login uri is set",
					  grid[GRID_LOGIN_URI_VALUE][0].asString(), 
					  std::string("https://my.login.uri/cgi-bin/login.cgi"));
		ensure_equals("Override custom grid login uri: Helper uri is not set",
					  grid[GRID_HELPER_URI_VALUE].asString(), 
					  std::string("https://mycustomgridchoice/helpers/"));
		ensure_equals("Override custom grid login uri: Login page is not set",
					  grid[GRID_LOGIN_PAGE_VALUE].asString(), 
					  std::string("http://mycustomgridchoice/app/login/"));
	}
	
	// validate override of helper uri with cmd line
	template<> template<>
	void viewerNetworkTestObject::test<5>()
	{	
		// Override with helperuri
		// override known grid
		LLSD grid;
		gCmdLineGridChoice = "Aditi";
		gCmdLineLoginURI = "";
		gCmdLineHelperURI = "https://my.helper.uri/mycustomhelpers";		
		LLGridManager::getInstance()->initialize("grid_test.xml");		
		std::map<std::string, std::string> known_grids = LLGridManager::getInstance()->getKnownGrids();		
		ensure_equals("Override known grid helper uri: No grids are added", 
					  known_grids.size(), 23);
		LLGridManager::getInstance()->getGridInfo(grid);
		ensure("Override known known helper uri: login uri is an array",
			   grid[GRID_LOGIN_URI_VALUE].isArray());
		ensure_equals("Override known grid helper uri: login uri is not changed",
					  grid[GRID_LOGIN_URI_VALUE][0].asString(), 
					  std::string("https://login.aditi.lindenlab.com/cgi-bin/login.cgi"));
		ensure_equals("Override known grid helper uri: helper uri is changed",
					  grid[GRID_HELPER_URI_VALUE].asString(), 
					  std::string("https://my.helper.uri/mycustomhelpers"));
		ensure_equals("Override known grid helper uri: login page is not changed",
					  grid[GRID_LOGIN_PAGE_VALUE].asString(), 
					  std::string("http://viewer-login.agni.lindenlab.com/"));		
		
		// Override with helperuri
		// override custom grid
		gCmdLineGridChoice = "mycustomgridchoice";
		gCmdLineHelperURI = "https://my.helper.uri/mycustomhelpers";		
		LLGridManager::getInstance()->initialize("grid_test.xml");	
		known_grids = LLGridManager::getInstance()->getKnownGrids();
		ensure_equals("Override custom grid helper uri: grids is added", 
					  known_grids.size(), 24);
		LLGridManager::getInstance()->getGridInfo(grid);
		ensure("Override custom helper uri: login uri is an array",
			   grid[GRID_LOGIN_URI_VALUE].isArray());
		ensure_equals("Override custom grid helper uri: login uri is not changed",
					  grid[GRID_LOGIN_URI_VALUE][0].asString(), 
					  std::string("https://mycustomgridchoice/cgi-bin/login.cgi"));
		ensure_equals("Override custom grid helper uri: helper uri is changed",
					  grid[GRID_HELPER_URI_VALUE].asString(), 
					  std::string("https://my.helper.uri/mycustomhelpers"));
		ensure_equals("Override custom grid helper uri: login page is not changed",
					  grid[GRID_LOGIN_PAGE_VALUE].asString(), 
					  std::string("http://mycustomgridchoice/app/login/"));
	}
	
	// validate overriding of login page via cmd line
	template<> template<>
	void viewerNetworkTestObject::test<6>()
	{	
		// Override with login page
		// override known grid
		LLSD grid;
		gCmdLineGridChoice = "Aditi";
		gCmdLineHelperURI = "";
		gLoginPage = "myloginpage";		
		LLGridManager::getInstance()->initialize("grid_test.xml");		
		std::map<std::string, std::string> known_grids = LLGridManager::getInstance()->getKnownGrids();				
		ensure_equals("Override known grid login page: No grids are added", 
					  known_grids.size(), 23);
		LLGridManager::getInstance()->getGridInfo(grid);
		ensure("Override known grid login page: Command line grid login uri is an array",
			   grid[GRID_LOGIN_URI_VALUE].isArray());
		ensure_equals("Override known grid login page: login uri is not changed",
					  grid[GRID_LOGIN_URI_VALUE][0].asString(), 
					  std::string("https://login.aditi.lindenlab.com/cgi-bin/login.cgi"));
		ensure_equals("Override known grid login page: helper uri is not changed",
					  grid[GRID_HELPER_URI_VALUE].asString(), 
					  std::string("http://aditi-secondlife.webdev.lindenlab.com/helpers/"));
		ensure_equals("Override known grid login page: login page is changed",
					  grid[GRID_LOGIN_PAGE_VALUE].asString(), 
					  std::string("myloginpage"));		
		
		// Override with login page
		// override custom grid
		gCmdLineGridChoice = "mycustomgridchoice";
		gLoginPage = "myloginpage";
		LLGridManager::getInstance()->initialize("grid_test.xml");		
		known_grids = LLGridManager::getInstance()->getKnownGrids();
		ensure_equals("Override custom grid login page: grids are added", 
					  known_grids.size(), 24);
		LLGridManager::getInstance()->getGridInfo(grid);
		ensure("Override custom grid login page: Command line grid login uri is an array",
			   grid[GRID_LOGIN_URI_VALUE].isArray());
		ensure_equals("Override custom grid login page: login uri is not changed",
					  grid[GRID_LOGIN_URI_VALUE][0].asString(), 
					  std::string("https://mycustomgridchoice/cgi-bin/login.cgi"));
		ensure_equals("Override custom grid login page: helper uri is not changed",
					  grid[GRID_HELPER_URI_VALUE].asString(), 
					  std::string("https://mycustomgridchoice/helpers/"));
		ensure_equals("Override custom grid login page: login page is changed",
					  grid[GRID_LOGIN_PAGE_VALUE].asString(), 
					  std::string("myloginpage"));	
		
	}
	
	// validate grid selection
	template<> template<>
	void viewerNetworkTestObject::test<7>()
	{	
		LLSD loginURI = LLSD::emptyArray();
		LLSD grid = LLSD::emptyMap();
		// adding a grid with simply a name will populate the values.
		grid[GRID_VALUE] = "myaddedgrid";

		LLGridManager::getInstance()->initialize("grid_test.xml");
		LLGridManager::getInstance()->addGrid(grid);
		LLGridManager::getInstance()->setGridChoice("util.agni.lindenlab.com");	
		ensure_equals("getGridLabel", LLGridManager::getInstance()->getGridLabel(), std::string("Agni"));
		ensure_equals("getGrid", LLGridManager::getInstance()->getGrid(), 
					  std::string("util.agni.lindenlab.com"));
		ensure_equals("getHelperURI", LLGridManager::getInstance()->getHelperURI(), 
					  std::string("https://secondlife.com/helpers/"));
		ensure_equals("getLoginPage", LLGridManager::getInstance()->getLoginPage(), 
					  std::string("http://viewer-login.agni.lindenlab.com/"));
		ensure_equals("getLoginPage2", LLGridManager::getInstance()->getLoginPage("util.agni.lindenlab.com"), 
					  std::string("http://viewer-login.agni.lindenlab.com/"));
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
		LLGridManager::getInstance()->getGridInfo("myaddedgrid", grid);
		ensure("setting favorite", grid.has(GRID_IS_FAVORITE_VALUE));
	}
	
	// name based grid population
	template<> template<>
	void viewerNetworkTestObject::test<8>()
	{
		LLGridManager::getInstance()->initialize("grid_test.xml");
		LLSD grid = LLSD::emptyMap();
		// adding a grid with simply a name will populate the values.
		grid[GRID_VALUE] = "myaddedgrid";
		LLGridManager::getInstance()->addGrid(grid);
		LLGridManager::getInstance()->getGridInfo("myaddedgrid", grid);
		
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
	void viewerNetworkTestObject::test<9>()
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
	void viewerNetworkTestObject::test<10>()
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
