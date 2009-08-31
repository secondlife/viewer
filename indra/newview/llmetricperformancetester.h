/** 
 * @file LLMetricPerformanceTester.h 
 * @brief LLMetricPerformanceTester class definition
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

#ifndef LL_METRICPERFORMANCETESTER_H 
#define LL_METRICPERFORMANCETESTER_H 

class LLMetricPerformanceTester 
{
public:
	//
    //name passed to the constructor is a unique string for each tester.
    //an error is reported if the name is already used by some other tester.
    //
	LLMetricPerformanceTester(std::string name, BOOL use_default_performance_analysis) ;
	virtual ~LLMetricPerformanceTester();

	//
    //return the name of the tester
    //
	std::string getName() const { return mName ;}
	//
    //return the number of the test metrics in this tester
    //
	S32 getNumOfMetricStrings() const { return mMetricStrings.size() ;}
	//
    //return the metric string at the index
    //
	const std::string& getMetricString(U32 index) const ;

	//
    //this function to compare the test results.
    //by default, it compares the test results against the baseline one by one, item by item, 
    //in the increasing order of the LLSD label counter, starting from the first one.
	//you can define your own way to analyze performance by passing FALSE to "use_default_performance_analysis",
    //and implement two abstract virtual functions below: loadTestSession(...) and compareTestSessions(...).
    //
	void analyzePerformance(std::ofstream* os, LLSD* base, LLSD* current) ;

protected:
	//
    //insert metric strings used in the tester.
    //
	void addMetricString(std::string str) ;

	//
    //increase LLSD label by 1
    //
	void incLabel() ;
	
	//
    //the function to write a set of test results to the log LLSD.
    //
	void outputTestResults() ;

	//
    //compare the test results.
    //you can write your own to overwrite the default one.
    //
	virtual void compareTestResults(std::ofstream* os, std::string metric_string, S32 v_base, S32 v_current) ;
	virtual void compareTestResults(std::ofstream* os, std::string metric_string, F32 v_base, F32 v_current) ;

	//
	//for performance analysis use 
	//it defines an interface for the two abstract virtual functions loadTestSession(...) and compareTestSessions(...).
    //please make your own test session class derived from it.
	//
	class LLTestSession
	{
	public:
		virtual ~LLTestSession() ;
	};

	//
    //load a test session for log LLSD
    //you need to implement it only when you define your own way to analyze performance.
    //otherwise leave it empty.
    //
	virtual LLMetricPerformanceTester::LLTestSession* loadTestSession(LLSD* log) = 0 ;
	//
    //compare the base session and the target session
    //you need to implement it only when you define your own way to analyze performance.
    //otherwise leave it empty.
    //
	virtual void compareTestSessions(std::ofstream* os) = 0 ;
	//
    //the function to write a set of test results to the log LLSD.
    //you have to write you own version of this function.	
	//
	virtual void outputTestRecord(LLSD* sd) = 0 ;

private:
	void preOutputTestResults(LLSD* sd) ;
	void postOutputTestResults(LLSD* sd) ;
	void prePerformanceAnalysis() ;

protected:
	//
    //the unique name string of the tester
    //
	std::string mName ;
	//
    //the current label counter for the log LLSD
    //
	std::string mCurLabel ;
	S32 mCount ;
	
	BOOL mUseDefaultPerformanceAnalysis ;
	LLTestSession* mBaseSessionp ;
	LLTestSession* mCurrentSessionp ;

	//metrics strings
	std::vector< std::string > mMetricStrings ;

//static members
private:
	static void addTester(LLMetricPerformanceTester* tester) ;

public:	
	typedef std::map< std::string, LLMetricPerformanceTester* > name_tester_map_t;	
	static name_tester_map_t sTesterMap ;

	static LLMetricPerformanceTester* getTester(std::string label) ;
	static BOOL hasMetricPerformanceTesters() {return !sTesterMap.empty() ;}

	static void initClass() ;
	static void cleanClass() ;
};

#endif

