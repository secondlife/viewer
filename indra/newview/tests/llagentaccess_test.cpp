/** 
 * @file llagentaccess_test.cpp
 * @brief LLAgentAccess tests
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * $/LicenseInfo$
 */
#include "../test/lltut.h"

#include "../llagentaccess.h"

#include "llcontrolgroupreader.h"
#include "indra_constants.h"

#include <iostream>

class LLControlGroupReader_Test : public LLControlGroupReader
{
public:
	LLControlGroupReader_Test() : test_preferred_maturity(SIM_ACCESS_PG) {}
	
	virtual std::string getString(const std::string& name)
	{
		return "";
	}
	virtual std::string	getText(const std::string& name)
	{
		return "";
	}
	virtual BOOL getBOOL(const std::string& name)
	{
		return false;
	}
	virtual S32	getS32(const std::string& name)
	{
		return 0;
	}
	virtual F32 getF32(const std::string& name)
	{
		return 0;
	}
	virtual U32	getU32(const std::string& name)
	{
		return test_preferred_maturity;
	}
	
	//--------------------------------------
	// Everything from here down is test code and not part of the interface
	void setPreferredMaturity(U32 m)
	{
		test_preferred_maturity = m;
	}
private:
	U32 test_preferred_maturity;
	
};

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
		LLControlGroupReader_Test cgr;
		LLAgentAccess aa(cgr);
		
		cgr.setPreferredMaturity(SIM_ACCESS_PG);
		ensure("1 prefersPG",     aa.prefersPG());
		ensure("1 prefersMature", !aa.prefersMature());
		ensure("1 prefersAdult",  !aa.prefersAdult());
		
		cgr.setPreferredMaturity(SIM_ACCESS_MATURE);
		ensure("2 prefersPG",     !aa.prefersPG());
		ensure("2 prefersMature", aa.prefersMature());
		ensure("2 prefersAdult",  !aa.prefersAdult());
		
		cgr.setPreferredMaturity(SIM_ACCESS_ADULT);
		ensure("3 prefersPG",     !aa.prefersPG());
		ensure("3 prefersMature", aa.prefersMature());
		ensure("3 prefersAdult",  aa.prefersAdult());
    }
    
	template<> template<>
	void agentaccess_object_t::test<2>()
	{
		LLControlGroupReader_Test cgr;
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
		LLControlGroupReader_Test cgr;
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
		LLControlGroupReader_Test cgr;
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
		
		cgr.setPreferredMaturity(SIM_ACCESS_MATURE);
		ensure("1 mature", !aa.wantsPGOnly());
		ensure("2 mature", aa.canAccessMature());
		ensure("3 mature", !aa.canAccessAdult());
		
		cgr.setPreferredMaturity(SIM_ACCESS_PG);
		ensure("1 mature pref pg", aa.wantsPGOnly());
		ensure("2 mature pref pg", !aa.canAccessMature());
		ensure("3 mature pref pg", !aa.canAccessAdult());
		
		aa.setMaturity('A');
		ensure("1 adult pref pg", aa.wantsPGOnly());
		ensure("2 adult pref pg", !aa.canAccessMature());
		ensure("3 adult pref pg", !aa.canAccessAdult());

		cgr.setPreferredMaturity(SIM_ACCESS_ADULT);
		ensure("1 adult", !aa.wantsPGOnly());
		ensure("2 adult", aa.canAccessMature());
		ensure("3 adult", aa.canAccessAdult());

		// make sure that even if pref is high, if access is low we block access
		// this shouldn't occur in real life but we want to be safe
		cgr.setPreferredMaturity(SIM_ACCESS_ADULT);
		aa.setMaturity('P');
		ensure("1 pref adult, actual pg", aa.wantsPGOnly());
		ensure("2 pref adult, actual pg", !aa.canAccessMature());
		ensure("3 pref adult, actual pg", !aa.canAccessAdult());
		
	}

	template<> template<>
	void agentaccess_object_t::test<5>()
	{
		LLControlGroupReader_Test cgr;
		LLAgentAccess aa(cgr);
		
		ensure("1 transition starts false", !aa.isInTransition());
		aa.setTransition();
		ensure("2 transition now true", aa.isInTransition());
	}

}


