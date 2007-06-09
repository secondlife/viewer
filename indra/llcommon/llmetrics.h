/**
 * @file llmetrics.h
 * @author Kelly
 * @date 2007-05-25
 * @brief Declaration of metrics accumulation and associated functions
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLMETRICS_H
#define LL_LLMETRICS_H

class LLMetricsImpl;
class LLSD;

class LLMetrics
{
public:
	LLMetrics();
	virtual ~LLMetrics();

	// Adds this event to aggregate totals and records details to syslog (llinfos)
	virtual void recordEventDetails(const std::string& location, 
						const std::string& mesg, 
						bool success, 
						LLSD stats);

	// Adds this event to aggregate totals
	virtual void recordEvent(const std::string& location, const std::string& mesg, bool success);

	// Prints aggregate totals and resets the counts.
	virtual void printTotals(LLSD meta);


private:
	
	LLMetricsImpl* mImpl;
};

#endif

