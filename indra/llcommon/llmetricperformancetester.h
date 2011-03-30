/** 
 * @file llmetricperformancetester.h 
 * @brief LLMetricPerformanceTesterBasic and LLMetricPerformanceTesterWithSession classes definition
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

#ifndef LL_METRICPERFORMANCETESTER_H 
#define LL_METRICPERFORMANCETESTER_H 

char const* const DEFAULT_METRIC_NAME = "metric";

/**
 * @class LLMetricPerformanceTesterBasic
 * @brief Performance Metric Base Class
 */
class LL_COMMON_API LLMetricPerformanceTesterBasic
{
public:
	/**
	 * @brief Creates a basic tester instance.
	 * @param[in] name - Unique string identifying this tester instance.
	 */
	LLMetricPerformanceTesterBasic(std::string name);
	virtual ~LLMetricPerformanceTesterBasic();

	/**
	 * @return Returns true if the instance has been added to the tester map.
	 * Need to be tested after creation of a tester instance so to know if the tester is correctly handled.
	 * A tester might not be added to the map if another tester with the same name already exists.
	 */
	BOOL isValid() const { return mValidInstance; }

	/**
	 * @brief Write a set of test results to the log LLSD.
	 */
	void outputTestResults() ;

	/**
	 * @brief Compare the test results.
	 * By default, compares the test results against the baseline one by one, item by item, 
	 * in the increasing order of the LLSD record counter, starting from the first one.
	 */
	virtual void analyzePerformance(std::ofstream* os, LLSD* base, LLSD* current) ;

	static void doAnalysisMetrics(std::string baseline, std::string target, std::string output) ;

	/**
	 * @return Returns the number of the test metrics in this tester instance.
	 */
	S32 getNumberOfMetrics() const { return mMetricStrings.size() ;}
	/**
	 * @return Returns the metric name at index
	 * @param[in] index - Index on the list of metrics managed by this tester instance.
	 */
	std::string getMetricName(S32 index) const { return mMetricStrings[index] ;}

protected:
	/**
	 * @return Returns the name of this tester instance.
	 */
	std::string getTesterName() const { return mName ;}

	/**
	 * @brief Insert a new metric to be managed by this tester instance.
	 * @param[in] str - Unique string identifying the new metric.
	 */
	void addMetric(std::string str) ;

	/**
	 * @brief Compare test results, provided in 2 flavors: compare integers and compare floats.
	 * @param[out] os - Formatted output string holding the compared values.
	 * @param[in] metric_string - Name of the metric.
	 * @param[in] v_base - Base value of the metric.
	 * @param[in] v_current - Current value of the metric.
	 */
	virtual void compareTestResults(std::ofstream* os, std::string metric_string, S32 v_base, S32 v_current) ;
	virtual void compareTestResults(std::ofstream* os, std::string metric_string, F32 v_base, F32 v_current) ;

	/**
	 * @brief Reset internal record count. Count starts with 1.
	 */
	void resetCurrentCount() { mCount = 1; }
	/**
	 * @brief Increment internal record count.
	 */
	void incrementCurrentCount() { mCount++; }
	/**
	 * @return Returns the label to be used for the current count. It's "TesterName"-"Count".
	 */
	std::string getCurrentLabelName() const { return llformat("%s-%d", mName.c_str(), mCount) ;}

	/**
	 * @brief Write a test record to the LLSD. Implementers need to overload this method.
	 * @param[out] sd - The LLSD record to store metric data into.
	 */
	virtual void outputTestRecord(LLSD* sd) = 0 ;

private:
	void preOutputTestResults(LLSD* sd) ;
	void postOutputTestResults(LLSD* sd) ;
	static LLSD analyzeMetricPerformanceLog(std::istream& is) ;

	std::string mName ;							// Name of this tester instance
	S32 mCount ;								// Current record count
	BOOL mValidInstance;						// TRUE if the instance is managed by the map
	std::vector< std::string > mMetricStrings ; // Metrics strings

// Static members managing the collection of testers
public:	
	// Map of all the tester instances in use
	typedef std::map< std::string, LLMetricPerformanceTesterBasic* > name_tester_map_t;	
	static name_tester_map_t sTesterMap ;

	/**
	 * @return Returns a pointer to the tester
	 * @param[in] name - Name of the tester instance queried.
	 */
	static LLMetricPerformanceTesterBasic* getTester(std::string name) ;
	
	/**
	 * @return Delete the named tester from the list 
	 * @param[in] name - Name of the tester instance to delete.
	 */
	static void deleteTester(std::string name);

	/**
	 * @return Returns TRUE if that metric *or* the default catch all metric has been requested to be logged
	 * @param[in] name - Name of the tester queried.
	 */
	static BOOL isMetricLogRequested(std::string name);
	
	/**
	 * @return Returns TRUE if there's a tester defined, FALSE otherwise.
	 */
	static BOOL hasMetricPerformanceTesters() { return !sTesterMap.empty() ;}
	/**
	 * @brief Delete all testers and reset the tester map
	 */
	static void cleanClass() ;

private:
	// Add a tester to the map. Returns false if adding fails.
	static BOOL addTester(LLMetricPerformanceTesterBasic* tester) ;
};

/**
 * @class LLMetricPerformanceTesterWithSession
 * @brief Performance Metric Class with custom session 
 */
class LL_COMMON_API LLMetricPerformanceTesterWithSession : public LLMetricPerformanceTesterBasic
{
public:
	/**
	 * @param[in] name - Unique string identifying this tester instance.
	 */
	LLMetricPerformanceTesterWithSession(std::string name);
	virtual ~LLMetricPerformanceTesterWithSession();

	/**
	 * @brief Compare the test results.
	 * This will be loading the base and current sessions and compare them using the virtual 
	 * abstract methods loadTestSession() and compareTestSessions()
	 */
	virtual void analyzePerformance(std::ofstream* os, LLSD* base, LLSD* current) ;

protected:
	/**
	 * @class LLMetricPerformanceTesterWithSession::LLTestSession
	 * @brief Defines an interface for the two abstract virtual functions loadTestSession() and compareTestSessions()
	 */
	class LL_COMMON_API LLTestSession
		{
		public:
			virtual ~LLTestSession() ;
		};

	/**
	 * @brief Convert an LLSD log into a test session.
	 * @param[in] log - The LLSD record
	 * @return Returns the record as a test session
	 */
	virtual LLMetricPerformanceTesterWithSession::LLTestSession* loadTestSession(LLSD* log) = 0;

	/**
	 * @brief Compare the base session and the target session. Assumes base and current sessions have been loaded.
	 * @param[out] os - The comparison result as a standard stream
	 */
	virtual void compareTestSessions(std::ofstream* os) = 0;

	LLTestSession* mBaseSessionp;
	LLTestSession* mCurrentSessionp;
};

#endif

