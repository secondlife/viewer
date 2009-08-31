/** 
 * @file llagentaccess_test.cpp
 * @brief LLAgentAccess tests
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#include "../test/lltut.h"

#include "../llagentaccess.h"

#include "llcontrol.h"
#include "indra_constants.h"

#include <iostream>

//----------------------------------------------------------------------------
// Implementation of enough of LLControlGroup to support the tests:

static U32 test_preferred_maturity = SIM_ACCESS_PG;

LLControlGroup::LLControlGroup(const std::string& name)
	: LLInstanceTracker<LLControlGroup, std::string>(name)
{
}

LLControlGroup::~LLControlGroup()
{
}

// Implementation of just the LLControlGroup methods we requre
BOOL LLControlGroup::declareU32(const std::string& name, U32 initial_val, const std::string& comment, BOOL persist)
{
	test_preferred_maturity = initial_val;
	return true;
}

void LLControlGroup::setU32(const std::string& name, U32 val)
{
	test_preferred_maturity = val;
}

U32 LLControlGroup::getU32(const std::string& name)
{
	return test_preferred_maturity;
}
//----------------------------------------------------------------------------
	
namespace tut
{
    struct agentaccess
    {
    };
    
	typedef test_group<agentaccess> agentaccess_t;
	typedef agentaccess_t::object agentaccess_object_t;
	tut::agentaccess_t tut_agentaccess("agentaccess");

	template<> template<>
	void agentaccess_object_t::test<1>()
	{
		LLControlGroup cgr("test");
		cgr.declareU32("PreferredMaturity", SIM_ACCESS_PG, "declared_for_test", FALSE);
		LLAgentAccess aa(cgr);
		
		cgr.setU32("PreferredMaturity", SIM_ACCESS_PG);
		ensure("1 prefersPG",     aa.prefersPG());
		ensure("1 prefersMature", !aa.prefersMature());
		ensure("1 prefersAdult",  !aa.prefersAdult());
		
		cgr.setU32("PreferredMaturity", SIM_ACCESS_MATURE);
		ensure("2 prefersPG",     !aa.prefersPG());
		ensure("2 prefersMature", aa.prefersMature());
		ensure("2 prefersAdult",  !aa.prefersAdult());
		
		cgr.setU32("PreferredMaturity", SIM_ACCESS_ADULT);
		ensure("3 prefersPG",     !aa.prefersPG());
		ensure("3 prefersMature", aa.prefersMature());
		ensure("3 prefersAdult",  aa.prefersAdult());
    }
    
	template<> template<>
	void agentaccess_object_t::test<2>()
	{
		LLControlGroup cgr("test");
		cgr.declareU32("PreferredMaturity", SIM_ACCESS_PG, "declared_for_test", FALSE);
		LLAgentAccess aa(cgr);
		
		// make sure default is PG
		ensure("1 isTeen",     aa.isTeen());
		ensure("1 isMature",   !aa.isMature());
		ensure("1 isAdult",    !aa.isAdult());
		
		// this is kinda bad -- setting this forces maturity to MATURE but !teen != Mature anymore
		aa.setTeen(false);
		ensure("2 isTeen",     !aa.isTeen());
		ensure("2 isMature",   aa.isMature());
		ensure("2 isAdult",    !aa.isAdult());

		// have to flip it back and make sure it still works
		aa.setTeen(true);
		ensure("3 isTeen",     aa.isTeen());
		ensure("3 isMature",   !aa.isMature());
		ensure("3 isAdult",    !aa.isAdult());		

		// check the conversion routine
		ensure_equals("1 conversion", SIM_ACCESS_PG, aa.convertTextToMaturity('P'));
		ensure_equals("2 conversion", SIM_ACCESS_MATURE, aa.convertTextToMaturity('M'));
		ensure_equals("3 conversion", SIM_ACCESS_ADULT, aa.convertTextToMaturity('A'));
		ensure_equals("4 conversion", SIM_ACCESS_MIN, aa.convertTextToMaturity('Q'));
		
		// now try the other method of setting it - PG
		aa.setMaturity('P');
		ensure("4 isTeen",     aa.isTeen());
		ensure("4 isMature",   !aa.isMature());
		ensure("4 isAdult",    !aa.isAdult());
		
		// Mature
		aa.setMaturity('M');
		ensure("5 isTeen",     !aa.isTeen());
		ensure("5 isMature",   aa.isMature());
		ensure("5 isAdult",    !aa.isAdult());
		
		// Adult
		aa.setMaturity('A');
		ensure("6 isTeen",     !aa.isTeen());
		ensure("6 isMature",   aa.isMature());
		ensure("6 isAdult",    aa.isAdult());
		
	}

	template<> template<>
	void agentaccess_object_t::test<3>()
	{
		LLControlGroup cgr("test");
		cgr.declareU32("PreferredMaturity", SIM_ACCESS_PG, "declared_for_test", FALSE);
		LLAgentAccess aa(cgr);
		
		ensure("starts normal", !aa.isGodlike());
		aa.setGodLevel(GOD_NOT);
		ensure("stays normal", !aa.isGodlike());
		aa.setGodLevel(GOD_FULL);
		ensure("sets full", aa.isGodlike());
		aa.setGodLevel(GOD_NOT);
		ensure("resets normal", !aa.isGodlike());
		aa.setAdminOverride(true);
		ensure("admin true", aa.getAdminOverride());
		ensure("overrides 1", aa.isGodlike());
		aa.setGodLevel(GOD_FULL);
		ensure("overrides 2", aa.isGodlike());
		aa.setAdminOverride(false);
		ensure("admin false", !aa.getAdminOverride());
		ensure("overrides 3", aa.isGodlike());
    }
    
	template<> template<>
	void agentaccess_object_t::test<4>()
	{
		LLControlGroup cgr("test");
		cgr.declareU32("PreferredMaturity", SIM_ACCESS_PG, "declared_for_test", FALSE);
		LLAgentAccess aa(cgr);
		
		ensure("1 pg to start", aa.wantsPGOnly());
		ensure("2 pg to start", !aa.canAccessMature());
		ensure("3 pg to start", !aa.canAccessAdult());
		
		aa.setGodLevel(GOD_FULL);
		ensure("1 full god", !aa.wantsPGOnly());
		ensure("2 full god", aa.canAccessMature());
		ensure("3 full god", aa.canAccessAdult());
				
		aa.setGodLevel(GOD_NOT);
		aa.setAdminOverride(true);
		ensure("1 admin mode", !aa.wantsPGOnly());
		ensure("2 admin mode", aa.canAccessMature());
		ensure("3 admin mode", aa.canAccessAdult());

		aa.setAdminOverride(false);
		aa.setMaturity('M');
		// preferred is still pg by default
		ensure("1 mature pref pg", aa.wantsPGOnly());
		ensure("2 mature pref pg", !aa.canAccessMature());
		ensure("3 mature pref pg", !aa.canAccessAdult());
		
		cgr.setU32("PreferredMaturity", SIM_ACCESS_MATURE);
		ensure("1 mature", !aa.wantsPGOnly());
		ensure("2 mature", aa.canAccessMature());
		ensure("3 mature", !aa.canAccessAdult());
		
		cgr.setU32("PreferredMaturity", SIM_ACCESS_PG);
		ensure("1 mature pref pg", aa.wantsPGOnly());
		ensure("2 mature pref pg", !aa.canAccessMature());
		ensure("3 mature pref pg", !aa.canAccessAdult());
		
		aa.setMaturity('A');
		ensure("1 adult pref pg", aa.wantsPGOnly());
		ensure("2 adult pref pg", !aa.canAccessMature());
		ensure("3 adult pref pg", !aa.canAccessAdult());

		cgr.setU32("PreferredMaturity", SIM_ACCESS_ADULT);
		ensure("1 adult", !aa.wantsPGOnly());
		ensure("2 adult", aa.canAccessMature());
		ensure("3 adult", aa.canAccessAdult());

		// make sure that even if pref is high, if access is low we block access
		// this shouldn't occur in real life but we want to be safe
		cgr.setU32("PreferredMaturity", SIM_ACCESS_ADULT);
		aa.setMaturity('P');
		ensure("1 pref adult, actual pg", aa.wantsPGOnly());
		ensure("2 pref adult, actual pg", !aa.canAccessMature());
		ensure("3 pref adult, actual pg", !aa.canAccessAdult());
		
	}

	template<> template<>
	void agentaccess_object_t::test<5>()
	{
		LLControlGroup cgr("test");
		cgr.declareU32("PreferredMaturity", SIM_ACCESS_PG, "declared_for_test", FALSE);
		LLAgentAccess aa(cgr);
		
		ensure("1 transition starts false", !aa.isInTransition());
		aa.setTransition();
		ensure("2 transition now true", aa.isInTransition());
	}

	template<> template<>
	void agentaccess_object_t::test<6>()
	{
		LLControlGroup cgr("test");
		cgr.declareU32("PreferredMaturity", SIM_ACCESS_PG, "declared_for_test", FALSE);
		LLAgentAccess aa(cgr);
		
		cgr.setU32("PreferredMaturity", SIM_ACCESS_ADULT);
		aa.setMaturity('M');
		ensure("1 preferred maturity pegged to M when maturity is M", cgr.getU32("PreferredMaturity") == SIM_ACCESS_MATURE);
		
		aa.setMaturity('P');
		ensure("1 preferred maturity pegged to P when maturity is P", cgr.getU32("PreferredMaturity") == SIM_ACCESS_PG);
	}
}


