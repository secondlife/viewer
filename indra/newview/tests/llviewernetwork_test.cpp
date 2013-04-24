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

const char *gSampleGridFile =
	"<?xml version=\"1.0\"?>"
	"<llsd>"
	"  <map>"
	"    <key>altgrid.long.name</key>"
	"    <map>"
	"      <key>helper_uri</key><string>https://helper1/helpers/</string>"
	"      <key>label</key><string>Alternative Grid</string>"
	"      <key>login_page</key><string>altgrid/loginpage</string>"
	"      <key>login_uri</key>"
	"      <array>"
	"        <string>altgrid/myloginuri1</string>"
	"        <string>altgrid/myloginuri2</string>"
	"      </array>"
	"      <key>keyname</key><string>altgrid.long.name</string>"
	"      <key>credential_type</key><string>agent</string>"
	"      <key>grid_login_id</key><string>AltGrid</string>"
	"    </map>"
	"    <key>minimal.long.name</key>"
	"    <map>"
	"      <key>keyname</key><string>minimal.long.name</string>"
	"    </map>"
	"    <!-- Note that the values for agni and aditi below are deliberately"
	"         incorrect to test that they are not overwritten -->"
	"    <key>util.agni.lindenlab.com</key> <!-- conflict -->"
	"    <map>"
	"      <key>helper_uri</key><string>https://helper1/helpers/</string>"
	"      <key>grid_login_id</key><string>mylabel</string>"
	"      <key>label</key><string>mylabel</string>"
	"      <key>login_page</key><string>loginpage</string>"
	"      <key>login_uri</key>"
	"      <array>"
	"        <string>myloginuri</string>"
	"      </array>"
	"      <key>keyname</key><string>util.agni.lindenlab.com</string> <!-- conflict -->"
	"    </map>"
	"    <key>util.foobar.lindenlab.com</key>"
	"    <map>"
	"      <key>helper_uri</key><string>https://helper1/helpers/</string>"
	"      <key>grid_login_id</key><string>Aditi</string> <!-- conflict -->"
	"      <key>label</key><string>mylabel</string>"
	"      <key>login_page</key><string>loginpage</string>"
	"      <key>login_uri</key>"
	"      <array>"
	"        <string>myloginuri</string>"
	"      </array>"
	"      <key>keyname</key><string>util.foobar.lindenlab.com</string>"
	"    </map>"
	"  </map>"
	"</llsd>"
	;
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
		ensure_equals("Known grids is a string-string map of size 2", known_grids.size(), 2);
		ensure_equals("Agni has the right name and label",
					  known_grids[std::string("util.agni.lindenlab.com")],
					  std::string("Second Life Main Grid (Agni)"));
		ensure_equals("Aditi has the right name and label",
					  known_grids[std::string("util.aditi.lindenlab.com")],
					  std::string("Second Life Beta Test Grid (Aditi)"));
		ensure_equals("name for agni",
					  LLGridManager::getInstance()->getGrid("util.agni.lindenlab.com"),
					  std::string("util.agni.lindenlab.com"));
		ensure_equals("id for agni",
					  std::string("Agni"),
					  LLGridManager::getInstance()->getGridId("util.agni.lindenlab.com"));
		ensure_equals("label for agni",
					  LLGridManager::getInstance()->getGridLabel("util.agni.lindenlab.com"),
					  std::string("Second Life Main Grid (Agni)"));

		std::vector<std::string> login_uris;
		LLGridManager::getInstance()->getLoginURIs(std::string("util.agni.lindenlab.com"), login_uris);
		ensure_equals("Number of login uris for agni", 1, login_uris.size());
		ensure_equals("Agni login uri",
					  login_uris[0],
					  std::string("https://login.agni.lindenlab.com/cgi-bin/login.cgi"));
		ensure_equals("Agni helper uri",
					  LLGridManager::getInstance()->getHelperURI("util.agni.lindenlab.com"),
					  std::string("https://secondlife.com/helpers/"));
		ensure_equals("Agni login page",
					  LLGridManager::getInstance()->getLoginPage("util.agni.lindenlab.com"),
					  std::string("http://viewer-login.agni.lindenlab.com/"));
		ensure("Agni is a system grid",
			   LLGridManager::getInstance()->isSystemGrid("util.agni.lindenlab.com"));

		ensure_equals("name for aditi",
					  LLGridManager::getInstance()->getGrid("util.aditi.lindenlab.com"),
					  std::string("util.aditi.lindenlab.com"));
		ensure_equals("id for aditi",
					  LLGridManager::getInstance()->getGridId("util.aditi.lindenlab.com"),
					  std::string("Aditi"));
		ensure_equals("label for aditi",
					  LLGridManager::getInstance()->getGridLabel("util.aditi.lindenlab.com"),
					  std::string("Second Life Beta Test Grid (Aditi)"));

		LLGridManager::getInstance()->getLoginURIs(std::string("util.aditi.lindenlab.com"), login_uris);

		ensure_equals("Number of login uris for aditi", 1, login_uris.size());
		ensure_equals("Aditi login uri",
					  login_uris[0],
					  std::string("https://login.aditi.lindenlab.com/cgi-bin/login.cgi"));
		ensure_equals("Aditi helper uri",
					  LLGridManager::getInstance()->getHelperURI("util.aditi.lindenlab.com"),
					  std::string("http://aditi-secondlife.webdev.lindenlab.com/helpers/"));
		ensure_equals("Aditi login page",
					  LLGridManager::getInstance()->getLoginPage("util.aditi.lindenlab.com"),
					  std::string("http://viewer-login.agni.lindenlab.com/"));
		ensure("Aditi is a system grid",
			   LLGridManager::getInstance()->isSystemGrid("util.aditi.lindenlab.com"));
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
		ensure_equals("adding a grid via a grid file increases known grid size",4, 
					  known_grids.size());

		// Verify that Agni and Aditi were not overwritten
		ensure_equals("Agni has the right name and label",
					  known_grids[std::string("util.agni.lindenlab.com")], 
					  std::string("Second Life Main Grid (Agni)"));
		ensure_equals("Aditi has the right name and label",
					  known_grids[std::string("util.aditi.lindenlab.com")], 
					  std::string("Second Life Beta Test Grid (Aditi)"));
		ensure_equals("name for agni",
					  LLGridManager::getInstance()->getGrid("util.agni.lindenlab.com"),
					  std::string("util.agni.lindenlab.com"));
		ensure_equals("id for agni",
					  LLGridManager::getInstance()->getGridId("util.agni.lindenlab.com"),
					  std::string("Agni"));
		ensure_equals("label for agni",
					  LLGridManager::getInstance()->getGridLabel("util.agni.lindenlab.com"),
					  std::string("Second Life Main Grid (Agni)"));
		std::vector<std::string> login_uris;
		LLGridManager::getInstance()->getLoginURIs(std::string("util.agni.lindenlab.com"), login_uris);
		ensure_equals("Number of login uris for agni", 1, login_uris.size());
		ensure_equals("Agni login uri",
					  login_uris[0],
					  std::string("https://login.agni.lindenlab.com/cgi-bin/login.cgi"));
		ensure_equals("Agni helper uri",
					  LLGridManager::getInstance()->getHelperURI("util.agni.lindenlab.com"),
					  std::string("https://secondlife.com/helpers/"));
		ensure_equals("Agni login page",
					  LLGridManager::getInstance()->getLoginPage("util.agni.lindenlab.com"),
					  std::string("http://viewer-login.agni.lindenlab.com/"));
		ensure("Agni is a system grid",
			   LLGridManager::getInstance()->isSystemGrid("util.agni.lindenlab.com"));

		ensure_equals("name for aditi",
					  LLGridManager::getInstance()->getGrid("util.aditi.lindenlab.com"),
					  std::string("util.aditi.lindenlab.com"));
		ensure_equals("id for aditi",
					  LLGridManager::getInstance()->getGridId("util.aditi.lindenlab.com"),
					  std::string("Aditi"));
		ensure_equals("label for aditi",
					  LLGridManager::getInstance()->getGridLabel("util.aditi.lindenlab.com"),
					  std::string("Second Life Beta Test Grid (Aditi)"));

		LLGridManager::getInstance()->getLoginURIs(std::string("util.aditi.lindenlab.com"), login_uris);
		ensure_equals("Number of login uris for aditi", 1, login_uris.size());
		ensure_equals("Aditi login uri",
					  login_uris[0],
					  std::string("https://login.aditi.lindenlab.com/cgi-bin/login.cgi"));
		ensure_equals("Aditi helper uri",
					  LLGridManager::getInstance()->getHelperURI("util.aditi.lindenlab.com"),
					  std::string("http://aditi-secondlife.webdev.lindenlab.com/helpers/"));
		ensure_equals("Aditi login page",
					  LLGridManager::getInstance()->getLoginPage("util.aditi.lindenlab.com"),
					  std::string("http://viewer-login.agni.lindenlab.com/"));
		ensure("Aditi is a system grid",
			   LLGridManager::getInstance()->isSystemGrid("util.aditi.lindenlab.com"));

		// Check the additional grid from the file
		ensure_equals("alternative grid is in name<->label map",
					  known_grids["altgrid.long.name"], 
					  std::string("Alternative Grid"));
		ensure_equals("alternative grid name is set",
					  LLGridManager::getInstance()->getGrid("altgrid.long.name"),
					  std::string("altgrid.long.name"));
		ensure_equals("alternative grid id",
					  LLGridManager::getInstance()->getGridId("altgrid.long.name"),
					  std::string("AltGrid"));
		ensure_equals("alternative grid label",
					  LLGridManager::getInstance()->getGridLabel("altgrid.long.name"),
					  std::string("Alternative Grid"));
		std::vector<std::string> alt_login_uris;
		LLGridManager::getInstance()->getLoginURIs(std::string("altgrid.long.name"), alt_login_uris);
		ensure_equals("Number of login uris for altgrid", 2, alt_login_uris.size());
		ensure_equals("alternative grid first login uri",
					  alt_login_uris[0],
					  std::string("altgrid/myloginuri1"));
		ensure_equals("alternative grid second login uri",
					  alt_login_uris[1],
					  std::string("altgrid/myloginuri2"));
		ensure_equals("alternative grid helper uri",
					  LLGridManager::getInstance()->getHelperURI("altgrid.long.name"),
					  std::string("https://helper1/helpers/"));
		ensure_equals("alternative grid login page",
					  LLGridManager::getInstance()->getLoginPage("altgrid.long.name"),
					  std::string("altgrid/loginpage"));
		ensure("alternative grid is NOT a system grid",
			   ! LLGridManager::getInstance()->isSystemGrid("altgrid.long.name"));

		ensure_equals("minimal grid is in name<->label map",
					  known_grids["minimal.long.name"], 
					  std::string("minimal.long.name"));
		ensure_equals("minimal grid name is set",
					  LLGridManager::getInstance()->getGrid("minimal.long.name"),
					  std::string("minimal.long.name"));
		ensure_equals("minimal grid id",
					  LLGridManager::getInstance()->getGridId("minimal.long.name"),
					  std::string("minimal.long.name"));
		ensure_equals("minimal grid label",
					  LLGridManager::getInstance()->getGridLabel("minimal.long.name"),
					  std::string("minimal.long.name"));

		LLGridManager::getInstance()->getLoginURIs(std::string("minimal.long.name"), alt_login_uris);
		ensure_equals("Number of login uris for altgrid", 1, alt_login_uris.size());
		ensure_equals("minimal grid login uri",
					  alt_login_uris[0],
					  std::string("https://minimal.long.name/cgi-bin/login.cgi"));
		ensure_equals("minimal grid helper uri",
					  LLGridManager::getInstance()->getHelperURI("minimal.long.name"),
					  std::string("https://minimal.long.name/helpers/"));
		ensure_equals("minimal grid login page",
					  LLGridManager::getInstance()->getLoginPage("minimal.long.name"),
					  std::string("http://minimal.long.name/app/login/"));

	}


	// validate grid selection
	template<> template<>
	void viewerNetworkTestObject::test<7>()
	{
		// adding a grid with simply a name will populate the values.
		llofstream gridfile("grid_test.xml");
		gridfile << gSampleGridFile;
		gridfile.close();

		LLGridManager::getInstance()->initialize("grid_test.xml");

		LLGridManager::getInstance()->setGridChoice("util.agni.lindenlab.com");
		ensure_equals("getGridLabel",
					  LLGridManager::getInstance()->getGridLabel(),
					  std::string("Second Life Main Grid (Agni)"));
		ensure_equals("getGridId",
					  LLGridManager::getInstance()->getGridId(),
					  std::string("Agni"));
		ensure_equals("getGrid",
					  LLGridManager::getInstance()->getGrid(),
					  std::string("util.agni.lindenlab.com"));
		ensure_equals("getHelperURI",
					  LLGridManager::getInstance()->getHelperURI(),
					  std::string("https://secondlife.com/helpers/"));
		ensure_equals("getLoginPage",
					  LLGridManager::getInstance()->getLoginPage(),
					  std::string("http://viewer-login.agni.lindenlab.com/"));
		ensure("Is Agni a production grid", LLGridManager::getInstance()->isInProductionGrid());
		std::vector<std::string> uris;
		LLGridManager::getInstance()->getLoginURIs(uris);
		ensure_equals("getLoginURIs size", 1, uris.size());
		ensure_equals("getLoginURIs",
					  uris[0],
					  std::string("https://login.agni.lindenlab.com/cgi-bin/login.cgi"));

		LLGridManager::getInstance()->setGridChoice("altgrid.long.name");
		ensure_equals("getGridLabel",
					  LLGridManager::getInstance()->getGridLabel(),
					  std::string("Alternative Grid"));
		ensure_equals("getGridId",
					  LLGridManager::getInstance()->getGridId(),
					  std::string("AltGrid"));
		ensure("alternative grid is not a system grid",
			   !LLGridManager::getInstance()->isSystemGrid());
		ensure("alternative grid is not a production grid",
			   !LLGridManager::getInstance()->isInProductionGrid());
	}

}
