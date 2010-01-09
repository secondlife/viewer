/** 
 * @file llmetricperformancetester.cpp
 * @brief LLMetricPerformanceTester class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "indra_constants.h"
#include "llerror.h"
#include "llmath.h"
#include "llfontgl.h"
#include "llsdserialize.h"
#include "llstat.h"
#include "lltreeiterators.h"
#include "llmetricperformancetester.h"

LLMetricPerformanceTester::name_tester_map_t LLMetricPerformanceTester::sTesterMap ;

//static 
void LLMetricPerformanceTester::initClass() 
{
}
//static 
void LLMetricPerformanceTester::cleanClass() 
{
	for(name_tester_map_t::iterator iter = sTesterMap.begin() ; iter != sTesterMap.end() ; ++iter)
	{
		delete iter->second ;
	}
	sTesterMap.clear() ;
}

//static 
void LLMetricPerformanceTester::addTester(LLMetricPerformanceTester* tester) 
{
	if(!tester)
	{
		llerrs << "invalid tester!" << llendl ;
		return ;
	}
	
	std::string name = tester->getName() ;
	if(getTester(name))
	{
		llerrs << "Tester name is used by some other tester: " << name << llendl ;
		return ;
	}

	sTesterMap.insert(std::make_pair(name, tester));

	return ;
}
	
//static 
LLMetricPerformanceTester* LLMetricPerformanceTester::getTester(std::string label) 
{
	name_tester_map_t::iterator found_it = sTesterMap.find(label) ;
	if(found_it != sTesterMap.end())
	{
		return found_it->second ;
	}

	return NULL ;
}
	
LLMetricPerformanceTester::LLMetricPerformanceTester(std::string name, BOOL use_default_performance_analysis)
	: mName(name),
	mBaseSessionp(NULL),
	mCurrentSessionp(NULL),
	mCount(0),
	mUseDefaultPerformanceAnalysis(use_default_performance_analysis)
{
	if(mName == std::string())
	{
		llerrs << "invalid name." << llendl ;
	}

	LLMetricPerformanceTester::addTester(this) ;
}

/*virtual*/ 
LLMetricPerformanceTester::~LLMetricPerformanceTester() 
{
	if(mBaseSessionp)
	{
		delete mBaseSessionp ;
		mBaseSessionp = NULL ;
	}
	if(mCurrentSessionp)
	{
		delete mCurrentSessionp ;
		mCurrentSessionp = NULL ;
	}
}

void LLMetricPerformanceTester::incLabel()
{
	mCurLabel = llformat("%s-%d", mName.c_str(), mCount++) ;
}
void LLMetricPerformanceTester::preOutputTestResults(LLSD* sd) 
{
	incLabel() ;
	(*sd)[mCurLabel]["Name"] = mName ;
}
void LLMetricPerformanceTester::postOutputTestResults(LLSD* sd)
{
	LLMutexLock lock(LLFastTimerUtil::sLogLock);
	LLFastTimerUtil::sLogQueue.push((*sd));
}

void LLMetricPerformanceTester::outputTestResults() 
{
	LLSD sd ;
	preOutputTestResults(&sd) ; 

	outputTestRecord(&sd) ;

	postOutputTestResults(&sd) ;
}

void LLMetricPerformanceTester::addMetricString(std::string str)
{
	mMetricStrings.push_back(str) ;
}

const std::string& LLMetricPerformanceTester::getMetricString(U32 index) const 
{
	return mMetricStrings[index] ;
}

void LLMetricPerformanceTester::prePerformanceAnalysis() 
{
	mCount = 0 ;
	incLabel() ;
}

//
//default analyzing the performance
//
/*virtual*/ 
void LLMetricPerformanceTester::analyzePerformance(std::ofstream* os, LLSD* base, LLSD* current) 
{
	if(mUseDefaultPerformanceAnalysis)//use default performance analysis
	{
		prePerformanceAnalysis() ;

		BOOL in_base = (*base).has(mCurLabel) ;
		BOOL in_current = (*current).has(mCurLabel) ;

		while(in_base || in_current)
		{
			LLSD::String label = mCurLabel ;		
			
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

			incLabel() ;
			in_base = (*base).has(mCurLabel) ;
			in_current = (*current).has(mCurLabel) ;
		}
	}//end of default
	else
	{
		//load the base session
		prePerformanceAnalysis() ;
		mBaseSessionp = loadTestSession(base) ;

		//load the current session
		prePerformanceAnalysis() ;
		mCurrentSessionp = loadTestSession(current) ;

		if(!mBaseSessionp || !mCurrentSessionp)
		{
			llerrs << "memory error during loading test sessions." << llendl ;
		}

		//compare
		compareTestSessions(os) ;

		//release memory
		if(mBaseSessionp)
		{
			delete mBaseSessionp ;
			mBaseSessionp = NULL ;
		}
		if(mCurrentSessionp)
		{
			delete mCurrentSessionp ;
			mCurrentSessionp = NULL ;
		}
	}
}

//virtual 
void LLMetricPerformanceTester::compareTestResults(std::ofstream* os, std::string metric_string, S32 v_base, S32 v_current) 
{
	*os << llformat(" ,%s, %d, %d, %d, %.4f\n", metric_string.c_str(), v_base, v_current, 
						v_current - v_base, (v_base != 0) ? 100.f * v_current / v_base : 0) ;
}

//virtual 
void LLMetricPerformanceTester::compareTestResults(std::ofstream* os, std::string metric_string, F32 v_base, F32 v_current) 
{
	*os << llformat(" ,%s, %.4f, %.4f, %.4f, %.4f\n", metric_string.c_str(), v_base, v_current,						
						v_current - v_base, (fabs(v_base) > 0.0001f) ? 100.f * v_current / v_base : 0.f ) ;
}

//virtual 
LLMetricPerformanceTester::LLTestSession::~LLTestSession() 
{
}

