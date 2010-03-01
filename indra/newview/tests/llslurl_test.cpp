/** 
 * @file llsecapi_test.cpp
 * @author Roxie
 * @date 2009-02-10
 * @brief Test the sec api functionality
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version maps.secondlife.com2.0
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
#include "../llslurl.h"
#include "../../llxml/llcontrol.h"
#include "llsdserialize.h"
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

// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------
namespace tut
{
	// Test wrapper declaration : wrapping nothing for the moment
	struct slurlTest
	{
		slurlTest()
		{	
			LLGridManager::getInstance()->initialize(std::string(""));
		}
		~slurlTest()
		{
		}
	};
	
	// Tut templating thingamagic: test group, object and test instance
	typedef test_group<slurlTest> slurlTestFactory;
	typedef slurlTestFactory::object slurlTestObject;
	tut::slurlTestFactory tut_test("llslurl");
	
	// ---------------------------------------------------------------------------------------
	// Test functions 
	// ---------------------------------------------------------------------------------------
	// construction from slurl string
	template<> template<>
	void slurlTestObject::test<1>()
	{
		LLGridManager::getInstance()->setGridChoice("util.agni.lindenlab.com");
		
		LLSLURL slurl = LLSLURL("");
		ensure_equals("null slurl", (int)slurl.getType(), LLSLURL::LAST_LOCATION);
		
		slurl = LLSLURL("http://slurl.com/secondlife/myregion");
		ensure_equals("slurl.com slurl, region only - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("slurl.com slurl, region only", slurl.getSLURLString(), 
					  "http://maps.secondlife.com/secondlife/myregion/128/128/0");
		
		slurl = LLSLURL("http://maps.secondlife.com/secondlife/myregion/1/2/3");
		ensure_equals("maps.secondlife.com slurl, region + coords - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("maps.secondlife.com slurl, region + coords", slurl.getSLURLString(), 
					  "http://maps.secondlife.com/secondlife/myregion/1/2/3");

		slurl = LLSLURL("secondlife://myregion");
		ensure_equals("secondlife: slurl, region only - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("secondlife: slurl, region only", slurl.getSLURLString(), 
					  "http://maps.secondlife.com/secondlife/myregion/128/128/0");
		
		slurl = LLSLURL("secondlife://myregion/1/2/3");
		ensure_equals("secondlife: slurl, region + coords - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("secondlife slurl, region + coords", slurl.getSLURLString(), 
					  "http://maps.secondlife.com/secondlife/myregion/1/2/3");
		
		slurl = LLSLURL("/myregion");
		ensure_equals("/region slurl, region- type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("/region slurl, region ", slurl.getSLURLString(), 
					  "http://maps.secondlife.com/secondlife/myregion/128/128/0");
		
		slurl = LLSLURL("/myregion/1/2/3");
		ensure_equals("/: slurl, region + coords - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("/ slurl, region + coords", slurl.getSLURLString(), 
					  "http://maps.secondlife.com/secondlife/myregion/1/2/3");	
		
		slurl = LLSLURL("my region/1/2/3");
		ensure_equals(" slurl, region + coords - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals(" slurl, region + coords", slurl.getSLURLString(), 
					  "http://maps.secondlife.com/secondlife/my%20region/1/2/3");	
		
		slurl = LLSLURL("https://my.grid.com/region/my%20region/1/2/3");
		ensure_equals("grid slurl, region + coords - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("grid slurl, region + coords", slurl.getSLURLString(), 
					  "https://my.grid.com/region/my%20region/1/2/3");	
		
		slurl = LLSLURL("https://my.grid.com/region/my region");
		ensure_equals("grid slurl, region + coords - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("grid slurl, region + coords", slurl.getSLURLString(), 
					  "https://my.grid.com/region/my%20region/128/128/0");
		
		LLGridManager::getInstance()->setGridChoice("foo.bar.com");		
		slurl = LLSLURL("/myregion/1/2/3");
		ensure_equals("/: slurl, region + coords - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("/ slurl, region + coords", slurl.getSLURLString(), 
					  "https://foo.bar.com/region/myregion/1/2/3");		
		
		slurl = LLSLURL("myregion/1/2/3");
		ensure_equals(": slurl, region + coords - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals(" slurl, region + coords", slurl.getSLURLString(), 
					  "https://foo.bar.com/region/myregion/1/2/3");		
		
		slurl = LLSLURL(LLSLURL::SIM_LOCATION_HOME);
		ensure_equals("home", slurl.getType(), LLSLURL::HOME_LOCATION);

		slurl = LLSLURL(LLSLURL::SIM_LOCATION_LAST);
		ensure_equals("last", slurl.getType(), LLSLURL::LAST_LOCATION);
		
		slurl = LLSLURL("secondlife:///app/foo/bar?12345");
		ensure_equals("app", slurl.getType(), LLSLURL::APP);		
		ensure_equals("appcmd", slurl.getAppCmd(), "foo");
		ensure_equals("apppath", slurl.getAppPath().size(), 1);
		ensure_equals("apppath2", slurl.getAppPath()[0].asString(), "bar");
		ensure_equals("appquery", slurl.getAppQuery(), "12345");
		ensure_equals("grid1", "foo.bar.com", slurl.getGrid());
	
		slurl = LLSLURL("secondlife://Aditi/app/foo/bar?12345");
		ensure_equals("app", slurl.getType(), LLSLURL::APP);		
		ensure_equals("appcmd", slurl.getAppCmd(), "foo");
		ensure_equals("apppath", slurl.getAppPath().size(), 1);
		ensure_equals("apppath2", slurl.getAppPath()[0].asString(), "bar");
		ensure_equals("appquery", slurl.getAppQuery(), "12345");
		ensure_equals("grid2", "util.aditi.lindenlab.com", slurl.getGrid());		

		LLGridManager::getInstance()->setGridChoice("foo.bar.com");			
		slurl = LLSLURL("secondlife:///secondlife/myregion/1/2/3");
		ensure_equals("/: slurl, region + coords - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("location", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("region" , "myregion", slurl.getRegion());
		ensure_equals("grid3", "util.agni.lindenlab.com", slurl.getGrid());
				
		slurl = LLSLURL("secondlife://Aditi/secondlife/myregion/1/2/3");
		ensure_equals("/: slurl, region + coords - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("location", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("region" , "myregion", slurl.getRegion());
		ensure_equals("grid4", "util.aditi.lindenlab.com", slurl.getGrid());		
		
		slurl = LLSLURL("https://my.grid.com/app/foo/bar?12345");
		ensure_equals("app", slurl.getType(), LLSLURL::APP);		
		ensure_equals("appcmd", slurl.getAppCmd(), "foo");
		ensure_equals("apppath", slurl.getAppPath().size(), 1);
		ensure_equals("apppath2", slurl.getAppPath()[0].asString(), "bar");
		ensure_equals("appquery", slurl.getAppQuery(), "12345");	
		
	}
	
	// construction from grid/region/vector combos
	template<> template<>
	void slurlTestObject::test<2>()
	{
		LLSLURL slurl = LLSLURL("mygrid.com", "my region");
		ensure_equals("grid/region - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals("grid/region", slurl.getSLURLString(), 
					  "https://mygrid.com/region/my%20region/128/128/0");	
		
		slurl = LLSLURL("mygrid.com", "my region", LLVector3(1,2,3));
		ensure_equals("grid/region/vector - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals(" grid/region/vector", slurl.getSLURLString(), 
					  "https://mygrid.com/region/my%20region/1/2/3");			

		LLGridManager::getInstance()->setGridChoice("foo.bar.com.bar");			
		slurl = LLSLURL("my region", LLVector3(1,2,3));
		ensure_equals("grid/region/vector - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals(" grid/region/vector", slurl.getSLURLString(), 
					  "https://foo.bar.com.bar/region/my%20region/1/2/3");	
		
		LLGridManager::getInstance()->setGridChoice("util.agni.lindenlab.com");	
		slurl = LLSLURL("my region", LLVector3(1,2,3));
		ensure_equals("default grid/region/vector - type", slurl.getType(), LLSLURL::LOCATION);
		ensure_equals(" default grid/region/vector", slurl.getSLURLString(), 
					  "http://maps.secondlife.com/secondlife/my%20region/1/2/3");	
		
	}
	// Accessors
	template<> template<>
	void slurlTestObject::test<3>()
	{
		LLSLURL slurl = LLSLURL("https://my.grid.com/region/my%20region/1/2/3");
		ensure_equals("login string", slurl.getLoginString(), "uri:my region&amp;1&amp;2&amp;3");
		ensure_equals("location string", slurl.getLocationString(), "my region/1/2/3");
		ensure_equals("grid", slurl.getGrid(), "my.grid.com");
		ensure_equals("region", slurl.getRegion(), "my region");
		ensure_equals("position", slurl.getPosition(), LLVector3(1, 2, 3));
		
	}
}
