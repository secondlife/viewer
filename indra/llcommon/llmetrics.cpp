/** 
 * @file llmetrics.cpp
 * @author Kelly
 * @date 2007-05-25
 * @brief Metrics accumulation and associated functions
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llmetrics.h"

#include "llsd.h"
#include "llsdserialize.h"
#include "llframetimer.h"

class LLMetricsImpl
{
public:
	LLMetricsImpl() { }
	~LLMetricsImpl();

	void recordEvent(const std::string& location, const std::string& mesg, bool success);
	void printTotals(LLSD metadata);
	void recordEventDetails(const std::string& location, 
									const std::string& mesg, 
									bool success, 
									LLSD stats);
private:
	LLFrameTimer mLastPrintTimer;
	LLSD mMetricsMap;
};

LLMetricsImpl::~LLMetricsImpl()
{
}

void LLMetricsImpl::recordEventDetails(const std::string& location, 
									const std::string& mesg, 
									bool success, 
									LLSD stats)
{
	recordEvent(location,mesg,success);

	LLSD metrics = LLSD::emptyMap();
	metrics["location"] = location;
	metrics["stats"]  = stats;
	
	llinfos << "LLMETRICS: " << LLSDNotationStreamer(metrics) << llendl; 
}

// Store this:
// [ {'location_1':{'mesg_1':{'success':i10, 'fail':i0},
//					'mesg_2':{'success':i10, 'fail':i0}},
//   {'location_2',{'mesg_3':{'success':i10, 'fail':i0}} ]
void LLMetricsImpl::recordEvent(const std::string& location, const std::string& mesg, bool success)
{
	LLSD& stats = mMetricsMap[location][mesg];
	if (success)
	{
		stats["success"] = stats["success"].asInteger() + 1;
	}
	else
	{
		stats["fail"] = stats["fail"].asInteger() + 1;
	}
}

// Print this:
// { 'meta':
//		{ 'elapsed_time':r3600.000 }
//   'stats':
//		[ {'location':'location_1', 'mesg':'mesg_1', 'success':i10, 'fail':i0},
//		  {'location':'location_1', 'mesg':'mesg_2', 'success':i10, 'fail':i0},
//		  {'location':'location_2', 'mesg':'mesg_3', 'success':i10, 'fail':i0} ] }
void LLMetricsImpl::printTotals(LLSD metadata)
{
	F32 elapsed_time = mLastPrintTimer.getElapsedTimeAndResetF32();
	metadata["elapsed_time"] = elapsed_time;

	LLSD out_sd = LLSD::emptyMap();
	out_sd["meta"] = metadata;
		
	LLSD stats = LLSD::emptyArray();

	LLSD::map_const_iterator loc_it = mMetricsMap.beginMap();
	LLSD::map_const_iterator loc_end = mMetricsMap.endMap();
	for ( ; loc_it != loc_end; ++loc_it)
	{
		const std::string& location = (*loc_it).first;
		
		const LLSD& loc_map = (*loc_it).second;
		LLSD::map_const_iterator mesg_it = loc_map.beginMap();
		LLSD::map_const_iterator mesg_end = loc_map.endMap();
		for ( ; mesg_it != mesg_end; ++mesg_it)
		{
			const std::string& mesg = (*mesg_it).first;
			const LLSD& mesg_map = (*mesg_it).second;

			LLSD entry = LLSD::emptyMap();
			entry["location"] = location;
			entry["mesg"] = mesg;
			entry["success"] = mesg_map["success"];
			entry["fail"] = mesg_map["fail"];
		
			stats.append(entry);
		}
	}

	out_sd["stats"] = stats;

	llinfos << "LLMETRICS: AGGREGATE: " << LLSDOStreamer<LLSDNotationFormatter>(out_sd) << llendl;
}

LLMetrics::LLMetrics()
{
	mImpl = new LLMetricsImpl();
}

LLMetrics::~LLMetrics()
{
	delete mImpl;
	mImpl = NULL;
}

void LLMetrics::recordEvent(const std::string& location, const std::string& mesg, bool success)
{
	if (mImpl) mImpl->recordEvent(location,mesg,success);
}

void LLMetrics::printTotals(LLSD meta)
{
	if (mImpl) mImpl->printTotals(meta);
}


void LLMetrics::recordEventDetails(const std::string& location, 
									const std::string& mesg, 
									bool success, 
									LLSD stats)
{
	if (mImpl) mImpl->recordEventDetails(location,mesg,success,stats);
}

