/** 
 * @file llmetricperformancetester.cpp
 * @brief LLMetricPerformanceTesterBasic and LLMetricPerformanceTesterWithSession classes implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "linden_common.h"

#include "indra_constants.h"
#include "llerror.h"
#include "llsdserialize.h"
#include "llstat.h"
#include "lltreeiterators.h"
#include "llmetricperformancetester.h"

//----------------------------------------------------------------------------------------------
// LLMetricPerformanceTesterBasic : static methods and testers management
//----------------------------------------------------------------------------------------------

LLMetricPerformanceTesterBasic::name_tester_map_t LLMetricPerformanceTesterBasic::sTesterMap ;

/*static*/ 
void LLMetricPerformanceTesterBasic::cleanClass() 
{
	for (name_tester_map_t::iterator iter = sTesterMap.begin() ; iter != sTesterMap.end() ; ++iter)
	{
		delete iter->second ;
	}
	sTesterMap.clear() ;
}

/*static*/ 
BOOL LLMetricPerformanceTesterBasic::addTester(LLMetricPerformanceTesterBasic* tester) 
{
	llassert_always(tester != NULL);	
	std::string name = tester->getTesterName() ;
	if (getTester(name))
	{
		llerrs << "Tester name is already used by some other tester : " << name << llendl ;
		return FALSE;
	}

	sTesterMap.insert(std::make_pair(name, tester));
	return TRUE;
}
	
/*static*/ 
LLMetricPerformanceTesterBasic* LLMetricPerformanceTesterBasic::getTester(std::string name) 
{
	// Check for the requested metric name
	name_tester_map_t::iterator found_it = sTesterMap.find(name) ;
	if (found_it != sTesterMap.end())
	{
		return found_it->second ;
	}
	return NULL ;
}

/*static*/ 
// Return TRUE if this metric is requested or if the general default "catch all" metric is requested
BOOL LLMetricPerformanceTesterBasic::isMetricLogRequested(std::string name)
{
	return (LLFastTimer::sMetricLog && ((LLFastTimer::sLogName == name) || (LLFastTimer::sLogName == DEFAULT_METRIC_NAME)));
}

	
//----------------------------------------------------------------------------------------------
// LLMetricPerformanceTesterBasic : Tester instance methods
//----------------------------------------------------------------------------------------------

LLMetricPerformanceTesterBasic::LLMetricPerformanceTesterBasic(std::string name) : 
	mName(name),
	mCount(0)
{
	if (mName == std::string())
	{
		llerrs << "LLMetricPerformanceTesterBasic construction invalid : Empty name passed to constructor" << llendl ;
	}

	mValidInstance = LLMetricPerformanceTesterBasic::addTester(this) ;
}

LLMetricPerformanceTesterBasic::~LLMetricPerformanceTesterBasic() 
{
}

void LLMetricPerformanceTesterBasic::preOutputTestResults(LLSD* sd) 
{
	incrementCurrentCount() ;
	(*sd)[getCurrentLabelName()]["Name"] = mName ;
}

void LLMetricPerformanceTesterBasic::postOutputTestResults(LLSD* sd)
{
	LLMutexLock lock(LLFastTimer::sLogLock);
	LLFastTimer::sLogQueue.push((*sd));
}

void LLMetricPerformanceTesterBasic::outputTestResults() 
{
	LLSD sd;

	preOutputTestResults(&sd) ; 
	outputTestRecord(&sd) ;
	postOutputTestResults(&sd) ;
}

void LLMetricPerformanceTesterBasic::addMetric(std::string str)
{
	mMetricStrings.push_back(str) ;
}

/*virtual*/ 
void LLMetricPerformanceTesterBasic::analyzePerformance(std::ofstream* os, LLSD* base, LLSD* current) 
{
	resetCurrentCount() ;

	std::string current_label = getCurrentLabelName();
	BOOL in_base = (*base).has(current_label) ;
	BOOL in_current = (*current).has(current_label) ;

	while(in_base || in_current)
	{
		LLSD::String label = current_label ;		

		if(in_base && in_current)
		{				
			*os << llformat("%s\n", label.c_str()) ;

			for(U32 index = 0 ; index < mMetricStrings.size() ; index++)
			{
				switch((*current)[label][ mMetricStrings[index] ].type())
				{
				case LLSD::TypeInteger:
					compareTestResults(os, mMetricStrings[index], 
						(S32)((*base)[label][ mMetricStrings[index] ].asInteger()), (S32)((*current)[label][ mMetricStrings[index] ].asInteger())) ;
					break ;
				case LLSD::TypeReal:
					compareTestResults(os, mMetricStrings[index], 
						(F32)((*base)[label][ mMetricStrings[index] ].asReal()), (F32)((*current)[label][ mMetricStrings[index] ].asReal())) ;
					break;
				default:
					llerrs << "unsupported metric " << mMetricStrings[index] << " LLSD type: " << (S32)(*current)[label][ mMetricStrings[index] ].type() << llendl ;
				}
			}	
		}

		incrementCurrentCount();
		current_label = getCurrentLabelName();
		in_base = (*base).has(current_label) ;
		in_current = (*current).has(current_label) ;
	}
}

/*virtual*/ 
void LLMetricPerformanceTesterBasic::compareTestResults(std::ofstream* os, std::string metric_string, S32 v_base, S32 v_current) 
{
	*os << llformat(" ,%s, %d, %d, %d, %.4f\n", metric_string.c_str(), v_base, v_current, 
						v_current - v_base, (v_base != 0) ? 100.f * v_current / v_base : 0) ;
}

/*virtual*/ 
void LLMetricPerformanceTesterBasic::compareTestResults(std::ofstream* os, std::string metric_string, F32 v_base, F32 v_current) 
{
	*os << llformat(" ,%s, %.4f, %.4f, %.4f, %.4f\n", metric_string.c_str(), v_base, v_current,						
						v_current - v_base, (fabs(v_base) > 0.0001f) ? 100.f * v_current / v_base : 0.f ) ;
}

//----------------------------------------------------------------------------------------------
// LLMetricPerformanceTesterWithSession
//----------------------------------------------------------------------------------------------

LLMetricPerformanceTesterWithSession::LLMetricPerformanceTesterWithSession(std::string name) : 
	LLMetricPerformanceTesterBasic(name),
	mBaseSessionp(NULL),
	mCurrentSessionp(NULL)
{
}

LLMetricPerformanceTesterWithSession::~LLMetricPerformanceTesterWithSession()
{
	if (mBaseSessionp)
	{
		delete mBaseSessionp ;
		mBaseSessionp = NULL ;
	}
	if (mCurrentSessionp)
	{
		delete mCurrentSessionp ;
		mCurrentSessionp = NULL ;
	}
}

/*virtual*/ 
void LLMetricPerformanceTesterWithSession::analyzePerformance(std::ofstream* os, LLSD* base, LLSD* current) 
{
	// Load the base session
	resetCurrentCount() ;
	mBaseSessionp = loadTestSession(base) ;

	// Load the current session
	resetCurrentCount() ;
	mCurrentSessionp = loadTestSession(current) ;

	if (!mBaseSessionp || !mCurrentSessionp)
	{
		llerrs << "Error loading test sessions." << llendl ;
	}

	// Compare
	compareTestSessions(os) ;

	// Release memory
	if (mBaseSessionp)
	{
		delete mBaseSessionp ;
		mBaseSessionp = NULL ;
	}
	if (mCurrentSessionp)
	{
		delete mCurrentSessionp ;
		mCurrentSessionp = NULL ;
	}
}


//----------------------------------------------------------------------------------------------
// LLTestSession
//----------------------------------------------------------------------------------------------

LLMetricPerformanceTesterWithSession::LLTestSession::~LLTestSession() 
{
}

