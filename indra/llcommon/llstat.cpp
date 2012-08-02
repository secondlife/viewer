/** 
 * @file llstat.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llstat.h"
#include "lllivefile.h"
#include "llerrorcontrol.h"
#include "llframetimer.h"
#include "timing.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llstl.h"
#include "u64.h"


// statics
S32	            LLPerfBlock::sStatsFlags = LLPerfBlock::LLSTATS_NO_OPTIONAL_STATS;       // Control what is being recorded
LLPerfBlock::stat_map_t    LLPerfBlock::sStatMap;    // Map full path string to LLStatTime objects, tracks all active objects
std::string        LLPerfBlock::sCurrentStatPath = "";    // Something like "/total_time/physics/physics step"

//------------------------------------------------------------------------
// Live config file to trigger stats logging
static const char    STATS_CONFIG_FILE_NAME[]            = "/dev/shm/simperf/simperf_proc_config.llsd";
static const F32    STATS_CONFIG_REFRESH_RATE            = 5.0;        // seconds

class LLStatsConfigFile : public LLLiveFile
{
public:
    LLStatsConfigFile()
        : LLLiveFile(filename(), STATS_CONFIG_REFRESH_RATE),
        mChanged(false), mStatsp(NULL) { }

    static std::string filename();
    
protected:
    /* virtual */ bool loadFile();

public:
    void init(LLPerfStats* statsp);
    static LLStatsConfigFile& instance();
        // return the singleton stats config file

    bool mChanged;

protected:
    LLPerfStats*    mStatsp;
};

std::string LLStatsConfigFile::filename()
{
    return STATS_CONFIG_FILE_NAME;
}

void LLStatsConfigFile::init(LLPerfStats* statsp)
{
    mStatsp = statsp;
}

LLStatsConfigFile& LLStatsConfigFile::instance()
{
    static LLStatsConfigFile the_file;
    return the_file;
}


/* virtual */
// Load and parse the stats configuration file
bool LLStatsConfigFile::loadFile()
{
    if (!mStatsp)
    {
        llwarns << "Tries to load performance configure file without initializing LPerfStats" << llendl;
        return false;
    }
    mChanged = true;
    
    LLSD stats_config;
    {
        llifstream file(filename().c_str());
        if (file.is_open())
        {
            LLSDSerialize::fromXML(stats_config, file);
            if (stats_config.isUndefined())
            {
                llinfos << "Performance statistics configuration file ill-formed, not recording statistics" << llendl;
                mStatsp->setReportPerformanceDuration( 0.f );
                return false;
            }
        }
        else 
        {    // File went away, turn off stats if it was on
            if ( mStatsp->frameStatsIsRunning() )
            {
                llinfos << "Performance statistics configuration file deleted, not recording statistics" << llendl;
                mStatsp->setReportPerformanceDuration( 0.f );
            }
            return true;
        }
    }

    F32 duration = 0.f;
    F32 interval = 0.f;
	S32 flags = LLPerfBlock::LLSTATS_BASIC_STATS;

    const char * w = "duration";
    if (stats_config.has(w))
    {
        duration = (F32)stats_config[w].asReal();
    } 
    w = "interval";
    if (stats_config.has(w))
    {
        interval = (F32)stats_config[w].asReal();
    } 
    w = "flags";
    if (stats_config.has(w))
    {
		flags = (S32)stats_config[w].asInteger();
		if (flags == LLPerfBlock::LLSTATS_NO_OPTIONAL_STATS &&
			duration > 0)
		{   // No flags passed in, but have a duration, so reset to basic stats
			flags = LLPerfBlock::LLSTATS_BASIC_STATS;
		}
    } 

    mStatsp->setReportPerformanceDuration( duration, flags );
    mStatsp->setReportPerformanceInterval( interval );

    if ( duration > 0 )
    {
        if ( interval == 0.f )
        {
            llinfos << "Recording performance stats every frame for " << duration << " sec" << llendl;
        }
        else
        {
            llinfos << "Recording performance stats every " << interval << " seconds for " << duration << " seconds" << llendl;
        }
    }
    else
    {
        llinfos << "Performance stats recording turned off" << llendl;
    }
	return true;
}


//------------------------------------------------------------------------

LLPerfStats::LLPerfStats(const std::string& process_name, S32 process_pid) : 
    mFrameStatsFileFailure(FALSE),
    mSkipFirstFrameStats(FALSE),
    mProcessName(process_name),
    mProcessPID(process_pid),
    mReportPerformanceStatInterval(1.f),
    mReportPerformanceStatEnd(0.0) 
{ }

LLPerfStats::~LLPerfStats()
{
    LLPerfBlock::clearDynamicStats();
    mFrameStatsFile.close();
}

void LLPerfStats::init()
{
    // Initialize the stats config file instance.
    (void) LLStatsConfigFile::instance().init(this);
    (void) LLStatsConfigFile::instance().checkAndReload();
}

// Open file for statistics
void    LLPerfStats::openPerfStatsFile()
{
    if ( !mFrameStatsFile
        && !mFrameStatsFileFailure )
    {
        std::string stats_file = llformat("/dev/shm/simperf/%s_proc.%d.llsd", mProcessName.c_str(), mProcessPID);
        mFrameStatsFile.close();
        mFrameStatsFile.clear();
        mFrameStatsFile.open(stats_file, llofstream::out);
        if ( mFrameStatsFile.fail() )
        {
            llinfos << "Error opening statistics log file " << stats_file << llendl;
            mFrameStatsFileFailure = TRUE;
        }
        else
        {
            LLSD process_info = LLSD::emptyMap();
            process_info["name"] = mProcessName;
            process_info["pid"] = (LLSD::Integer) mProcessPID;
            process_info["stat_rate"] = (LLSD::Integer) mReportPerformanceStatInterval;
            // Add process-specific info.
            addProcessHeaderInfo(process_info);

            mFrameStatsFile << LLSDNotationStreamer(process_info) << std::endl; 
        }
    }
}

// Dump out performance metrics over some time interval
void LLPerfStats::dumpIntervalPerformanceStats()
{
    // Ensure output file is OK
    openPerfStatsFile();

    if ( mFrameStatsFile )
    {
        LLSD stats = LLSD::emptyMap();

        LLStatAccum::TimeScale scale;
        if ( getReportPerformanceInterval() == 0.f )
        {
            scale = LLStatAccum::SCALE_PER_FRAME;
        }
        else if ( getReportPerformanceInterval() < 0.5f )
        {
            scale = LLStatAccum::SCALE_100MS;
        }
        else
        {
            scale = LLStatAccum::SCALE_SECOND;
        }

        // Write LLSD into log
        stats["utc_time"] = (LLSD::String) LLError::utcTime();
        stats["timestamp"] = U64_to_str((totalTime() / 1000) + (gUTCOffset * 1000));    // milliseconds since epoch
        stats["frame_number"] = (LLSD::Integer) LLFrameTimer::getFrameCount();

        // Add process-specific frame info.
        addProcessFrameInfo(stats, scale);
        LLPerfBlock::addStatsToLLSDandReset( stats, scale );

        mFrameStatsFile << LLSDNotationStreamer(stats) << std::endl; 
    }
}

// Set length of performance stat recording.  
// If turning stats on, caller must provide flags
void    LLPerfStats::setReportPerformanceDuration( F32 seconds, S32 flags /* = LLSTATS_NO_OPTIONAL_STATS */ )
{ 
	if ( seconds <= 0.f )
	{
		mReportPerformanceStatEnd = 0.0;
		LLPerfBlock::setStatsFlags(LLPerfBlock::LLSTATS_NO_OPTIONAL_STATS);		// Make sure all recording is off
		mFrameStatsFile.close();
		LLPerfBlock::clearDynamicStats();
	}
	else
	{
		mReportPerformanceStatEnd = LLFrameTimer::getElapsedSeconds() + ((F64) seconds);
		// Clear failure flag to try and create the log file once
		mFrameStatsFileFailure = FALSE;
		mSkipFirstFrameStats = TRUE;		// Skip the first report (at the end of this frame)
		LLPerfBlock::setStatsFlags(flags);
	}
}

void LLPerfStats::updatePerFrameStats()
{
    (void) LLStatsConfigFile::instance().checkAndReload();
	static LLFrameTimer performance_stats_timer;
	if ( frameStatsIsRunning() )
	{
		if ( mReportPerformanceStatInterval == 0 )
		{	// Record info every frame
			if ( mSkipFirstFrameStats )
			{	// Skip the first time - was started this frame
				mSkipFirstFrameStats = FALSE;
			}
			else
			{
				dumpIntervalPerformanceStats();
			}
		}
		else
		{
			performance_stats_timer.setTimerExpirySec( getReportPerformanceInterval() );
			if (performance_stats_timer.checkExpirationAndReset( mReportPerformanceStatInterval ))
			{
				dumpIntervalPerformanceStats();
			}
		}
		
		if ( LLFrameTimer::getElapsedSeconds() > mReportPerformanceStatEnd )
		{	// Reached end of time, clear it to stop reporting
			setReportPerformanceDuration(0.f);			// Don't set mReportPerformanceStatEnd directly	
            llinfos << "Recording performance stats completed" << llendl;
		}
	}
}


//------------------------------------------------------------------------

U64 LLStatAccum::sScaleTimes[NUM_SCALES] =
{
	USEC_PER_SEC / 10,				// 100 millisec
	USEC_PER_SEC * 1,				// seconds
	USEC_PER_SEC * 60,				// minutes
#if ENABLE_LONG_TIME_STATS
	// enable these when more time scales are desired
	USEC_PER_SEC * 60*60,			// hours
	USEC_PER_SEC * 24*60*60,		// days
	USEC_PER_SEC * 7*24*60*60,		// weeks
#endif
};



LLStatAccum::LLStatAccum(bool useFrameTimer)
	: mUseFrameTimer(useFrameTimer),
	  mRunning(FALSE),
	  mLastTime(0),
	  mLastSampleValue(0.0),
	  mLastSampleValid(FALSE)
{
}

LLStatAccum::~LLStatAccum()
{
}



void LLStatAccum::reset(U64 when)
{
	mRunning = TRUE;
	mLastTime = when;

	for (int i = 0; i < NUM_SCALES; ++i)
	{
		mBuckets[i].accum = 0.0;
		mBuckets[i].endTime = when + sScaleTimes[i];
		mBuckets[i].lastValid = false;
	}
}

void LLStatAccum::sum(F64 value)
{
	sum(value, getCurrentUsecs());
}

void LLStatAccum::sum(F64 value, U64 when)
{
	if (!mRunning)
	{
		reset(when);
		return;
	}
	if (when < mLastTime)
	{
		// This happens a LOT on some dual core systems.
		lldebugs << "LLStatAccum::sum clock has gone backwards from "
			<< mLastTime << " to " << when << ", resetting" << llendl;

		reset(when);
		return;
	}

	// how long is this value for
	U64 timeSpan = when - mLastTime;

	for (int i = 0; i < NUM_SCALES; ++i)
	{
		Bucket& bucket = mBuckets[i];

		if (when < bucket.endTime)
		{
			bucket.accum += value;
		}
		else
		{
			U64 timeScale = sScaleTimes[i];

			U64 timeLeft = when - bucket.endTime;
				// how much time is left after filling this bucket
			
			if (timeLeft < timeScale)
			{
				F64 valueLeft = value * timeLeft / timeSpan;

				bucket.lastValid = true;
				bucket.lastAccum = bucket.accum + (value - valueLeft);
				bucket.accum = valueLeft;
				bucket.endTime += timeScale;
			}
			else
			{
				U64 timeTail = timeLeft % timeScale;

				bucket.lastValid = true;
				bucket.lastAccum = value * timeScale / timeSpan;
				bucket.accum = value * timeTail / timeSpan;
				bucket.endTime += (timeLeft - timeTail) + timeScale;
			}
		}
	}

	mLastTime = when;
}


F32 LLStatAccum::meanValue(TimeScale scale) const
{
	if (!mRunning)
	{
		return 0.0;
	}
	if ( scale == SCALE_PER_FRAME )
	{	// Per-frame not supported here
		scale = SCALE_100MS;
	}

	if (scale < 0 || scale >= NUM_SCALES)
	{
		llwarns << "llStatAccum::meanValue called for unsupported scale: "
			<< scale << llendl;
		return 0.0;
	}

	const Bucket& bucket = mBuckets[scale];

	F64 value = bucket.accum;
	U64 timeLeft = bucket.endTime - mLastTime;
	U64 scaleTime = sScaleTimes[scale];

	if (bucket.lastValid)
	{
		value += bucket.lastAccum * timeLeft / scaleTime;
	}
	else if (timeLeft < scaleTime)
	{
		value *= scaleTime / (scaleTime - timeLeft);
	}
	else
	{
		value = 0.0;
	}

	return (F32)(value / scaleTime);
}


U64 LLStatAccum::getCurrentUsecs() const
{
	if (mUseFrameTimer)
	{
		return LLFrameTimer::getTotalTime();
	}
	else
	{
		return totalTime();
	}
}


// ------------------------------------------------------------------------

LLStatRate::LLStatRate(bool use_frame_timer)
	: LLStatAccum(use_frame_timer)
{
}

void LLStatRate::count(U32 value)
{
	sum((F64)value * sScaleTimes[SCALE_SECOND]);
}


void LLStatRate::mark()
 { 
	// Effectively the same as count(1), but sets mLastSampleValue
	U64 when = getCurrentUsecs();

	if ( mRunning 
		 && (when > mLastTime) )
	{	// Set mLastSampleValue to the time from the last mark()
		F64 duration = ((F64)(when - mLastTime)) / sScaleTimes[SCALE_SECOND];
		if ( duration > 0.0 )
		{
			mLastSampleValue = 1.0 / duration;
		}
		else
		{
			mLastSampleValue = 0.0;
		}
	}

	sum( (F64) sScaleTimes[SCALE_SECOND], when);
 }


// ------------------------------------------------------------------------


LLStatMeasure::LLStatMeasure(bool use_frame_timer)
	: LLStatAccum(use_frame_timer)
{
}

void LLStatMeasure::sample(F64 value)
{
	U64 when = getCurrentUsecs();

	if (mLastSampleValid)
	{
		F64 avgValue = (value + mLastSampleValue) / 2.0;
		F64 interval = (F64)(when - mLastTime);

		sum(avgValue * interval, when);
	}
	else
	{
		reset(when);
	}

	mLastSampleValid = TRUE;
	mLastSampleValue = value;
}


// ------------------------------------------------------------------------

LLStatTime::LLStatTime(const std::string & key)
	: LLStatAccum(false),
	  mFrameNumber(LLFrameTimer::getFrameCount()),
	  mTotalTimeInFrame(0),
	  mKey(key)
#if LL_DEBUG
	  , mRunning(FALSE)
#endif
{
}

void LLStatTime::start()
{
	// Reset frame accumluation if the frame number has changed
	U32 frame_number = LLFrameTimer::getFrameCount();
	if ( frame_number != mFrameNumber )
	{
		mFrameNumber = frame_number;
		mTotalTimeInFrame = 0;
	}

	sum(0.0);

#if LL_DEBUG
	// Shouldn't be running already
	llassert( !mRunning );
	mRunning = TRUE;
#endif
}

void LLStatTime::stop()
{
	U64 end_time = getCurrentUsecs();
	U64 duration = end_time - mLastTime;
	sum(F64(duration), end_time);
	//llinfos << "mTotalTimeInFrame incremented from  " << mTotalTimeInFrame << " to " << (mTotalTimeInFrame + duration) << llendl; 
	mTotalTimeInFrame += duration;

#if LL_DEBUG
	mRunning = FALSE;
#endif
}

/* virtual */ F32 LLStatTime::meanValue(TimeScale scale) const
{
    if ( LLStatAccum::SCALE_PER_FRAME == scale )
    {
        return (F32)mTotalTimeInFrame;
    }
    else
    {
        return LLStatAccum::meanValue(scale);
    }
}


// ------------------------------------------------------------------------


// Use this constructor for pre-defined LLStatTime objects
LLPerfBlock::LLPerfBlock(LLStatTime* stat ) : mPredefinedStat(stat), mDynamicStat(NULL)
{
    if (mPredefinedStat)
    {
        // If dynamic stats are turned on, this will create a separate entry in the stat map.
        initDynamicStat(mPredefinedStat->mKey);

        // Start predefined stats.  These stats are not part of the stat map.
        mPredefinedStat->start();
    }
}

// Use this constructor for normal, optional LLPerfBlock time slices
LLPerfBlock::LLPerfBlock( const char* key ) : mPredefinedStat(NULL), mDynamicStat(NULL)
{
    if ((sStatsFlags & LLSTATS_BASIC_STATS) == 0)
	{	// These are off unless the base set is enabled
		return;
	}

	initDynamicStat(key);
}

	
// Use this constructor for dynamically created LLPerfBlock time slices
// that are only enabled by specific control flags
LLPerfBlock::LLPerfBlock( const char* key1, const char* key2, S32 flags ) : mPredefinedStat(NULL), mDynamicStat(NULL)
{
    if ((sStatsFlags & flags) == 0)
	{
		return;
	}

    if (NULL == key2 || strlen(key2) == 0)
    {
        initDynamicStat(key1);
    }
    else
    {
        std::ostringstream key;
        key << key1 << "_" << key2;
        initDynamicStat(key.str());
    }
}

// Set up the result data map if dynamic stats are enabled
void LLPerfBlock::initDynamicStat(const std::string& key)
{
    // Early exit if dynamic stats aren't enabled.
    if (sStatsFlags == LLSTATS_NO_OPTIONAL_STATS) 
		return;

    mLastPath = sCurrentStatPath;		// Save and restore current path
    sCurrentStatPath += "/" + key;		// Add key to current path

    // See if the LLStatTime object already exists
    stat_map_t::iterator iter = sStatMap.find(sCurrentStatPath);
    if ( iter == sStatMap.end() )
    {
        // StatEntry object doesn't exist, so create it
        mDynamicStat = new StatEntry( key );
        sStatMap[ sCurrentStatPath ] = mDynamicStat;	// Set the entry for this path
    }
    else
    {
        // Found this path in the map, use the object there
        mDynamicStat = (*iter).second;		// Get StatEntry for the current path
    }

    if (mDynamicStat)
    {
        mDynamicStat->mStat.start();
        mDynamicStat->mCount++;
    }
    else
    {
        llwarns << "Initialized NULL dynamic stat at '" << sCurrentStatPath << "'" << llendl;
       sCurrentStatPath = mLastPath;
    }
}


// Destructor does the time accounting
LLPerfBlock::~LLPerfBlock()
{
    if (mPredefinedStat) mPredefinedStat->stop();
    if (mDynamicStat)
    {
        mDynamicStat->mStat.stop();
        sCurrentStatPath = mLastPath;	// Restore the path in case sStatsEnabled changed during this block
    }
}


// Clear the map of any dynamic stats.  Static routine
void LLPerfBlock::clearDynamicStats()
{
    std::for_each(sStatMap.begin(), sStatMap.end(), DeletePairedPointer());
    sStatMap.clear();
}

// static - Extract the stat info into LLSD
void LLPerfBlock::addStatsToLLSDandReset( LLSD & stats,
										  LLStatAccum::TimeScale scale )
{
    // If we aren't in per-frame scale, we need to go from second to microsecond.
    U32 scale_adjustment = 1;
    if (LLStatAccum::SCALE_PER_FRAME != scale)
    {
        scale_adjustment = USEC_PER_SEC;
    }
	stat_map_t::iterator iter = sStatMap.begin();
	for ( ; iter != sStatMap.end(); ++iter )
	{	// Put the entry into LLSD "/full/path/to/stat/" = microsecond total time
		const std::string & stats_full_path = (*iter).first;

		StatEntry * stat = (*iter).second;
		if (stat)
		{
            if (stat->mCount > 0)
            {
                stats[stats_full_path] = LLSD::emptyMap();
                stats[stats_full_path]["us"] = (LLSD::Integer) (scale_adjustment * stat->mStat.meanValue(scale));
                if (stat->mCount > 1)
                {
                    stats[stats_full_path]["count"] = (LLSD::Integer) stat->mCount;
                }
                stat->mCount = 0;
            }
		}
		else
		{	// Shouldn't have a NULL pointer in the map.
            llwarns << "Unexpected NULL dynamic stat at '" << stats_full_path << "'" << llendl;
		}
	}	
}


// ------------------------------------------------------------------------

LLTimer LLStat::sTimer;
LLFrameTimer LLStat::sFrameTimer;

void LLStat::init()
{
	llassert(mNumBins > 0);
	mNumValues = 0;
	mLastValue = 0.f;
	mLastTime = 0.f;
	mCurBin = (mNumBins-1);
	mNextBin = 0;
	mBins      = new F32[mNumBins];
	mBeginTime = new F64[mNumBins];
	mTime      = new F64[mNumBins];
	mDT        = new F32[mNumBins];
	for (U32 i = 0; i < mNumBins; i++)
	{
		mBins[i]      = 0.f;
		mBeginTime[i] = 0.0;
		mTime[i]      = 0.0;
		mDT[i]        = 0.f;
	}

	if (!mName.empty())
	{
		stat_map_t::iterator iter = getStatList().find(mName);
		if (iter != getStatList().end())
			llwarns << "LLStat with duplicate name: " << mName << llendl;
		getStatList().insert(std::make_pair(mName, this));
	}
}

LLStat::stat_map_t& LLStat::getStatList()
{
	static LLStat::stat_map_t stat_list;
	return stat_list;
}

LLStat::LLStat(const U32 num_bins, const BOOL use_frame_timer)
	: mUseFrameTimer(use_frame_timer),
	  mNumBins(num_bins)
{
	init();
}

LLStat::LLStat(std::string name, U32 num_bins, BOOL use_frame_timer)
	: mUseFrameTimer(use_frame_timer),
	  mNumBins(num_bins),
	  mName(name)
{
	init();
}

LLStat::~LLStat()
{
	delete[] mBins;
	delete[] mBeginTime;
	delete[] mTime;
	delete[] mDT;

	if (!mName.empty())
	{
		// handle multiple entries with the same name
		stat_map_t::iterator iter = getStatList().find(mName);
		while (iter != getStatList().end() && iter->second != this)
			++iter;
		getStatList().erase(iter);
	}
}

void LLStat::reset()
{
	U32 i;

	mNumValues = 0;
	mLastValue = 0.f;
	mCurBin = (mNumBins-1);
	delete[] mBins;
	delete[] mBeginTime;
	delete[] mTime;
	delete[] mDT;
	mBins      = new F32[mNumBins];
	mBeginTime = new F64[mNumBins];
	mTime      = new F64[mNumBins];
	mDT        = new F32[mNumBins];
	for (i = 0; i < mNumBins; i++)
	{
		mBins[i]      = 0.f;
		mBeginTime[i] = 0.0;
		mTime[i]      = 0.0;
		mDT[i]        = 0.f;
	}
}

void LLStat::setBeginTime(const F64 time)
{
	mBeginTime[mNextBin] = time;
}

void LLStat::addValueTime(const F64 time, const F32 value)
{
	if (mNumValues < mNumBins)
	{
		mNumValues++;
	}

	// Increment the bin counters.
	mCurBin++;
	if ((U32)mCurBin == mNumBins)
	{
		mCurBin = 0;
	}
	mNextBin++;
	if ((U32)mNextBin == mNumBins)
	{
		mNextBin = 0;
	}

	mBins[mCurBin] = value;
	mTime[mCurBin] = time;
	mDT[mCurBin] = (F32)(mTime[mCurBin] - mBeginTime[mCurBin]);
	//this value is used to prime the min/max calls
	mLastTime = mTime[mCurBin];
	mLastValue = value;

	// Set the begin time for the next stat segment.
	mBeginTime[mNextBin] = mTime[mCurBin];
	mTime[mNextBin] = mTime[mCurBin];
	mDT[mNextBin] = 0.f;
}

void LLStat::start()
{
	if (mUseFrameTimer)
	{
		mBeginTime[mNextBin] = sFrameTimer.getElapsedSeconds();
	}
	else
	{
		mBeginTime[mNextBin] = sTimer.getElapsedTimeF64();
	}
}

void LLStat::addValue(const F32 value)
{
	if (mNumValues < mNumBins)
	{
		mNumValues++;
	}

	// Increment the bin counters.
	mCurBin++;
	if ((U32)mCurBin == mNumBins)
	{
		mCurBin = 0;
	}
	mNextBin++;
	if ((U32)mNextBin == mNumBins)
	{
		mNextBin = 0;
	}

	mBins[mCurBin] = value;
	if (mUseFrameTimer)
	{
		mTime[mCurBin] = sFrameTimer.getElapsedSeconds();
	}
	else
	{
		mTime[mCurBin] = sTimer.getElapsedTimeF64();
	}
	mDT[mCurBin] = (F32)(mTime[mCurBin] - mBeginTime[mCurBin]);

	//this value is used to prime the min/max calls
	mLastTime = mTime[mCurBin];
	mLastValue = value;

	// Set the begin time for the next stat segment.
	mBeginTime[mNextBin] = mTime[mCurBin];
	mTime[mNextBin] = mTime[mCurBin];
	mDT[mNextBin] = 0.f;
}


F32 LLStat::getMax() const
{
	U32 i;
	F32 current_max = mLastValue;
	if (mNumBins == 0)
	{
		current_max = 0.f;
	}
	else
	{
		for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
		{
			// Skip the bin we're currently filling.
			if (i == (U32)mNextBin)
			{
				continue;
			}
			if (mBins[i] > current_max)
			{
				current_max = mBins[i];
			}
		}
	}
	return current_max;
}

F32 LLStat::getMean() const
{
	U32 i;
	F32 current_mean = 0.f;
	U32 samples = 0;
	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		current_mean += mBins[i];
		samples++;
	}

	// There will be a wrap error at 2^32. :)
	if (samples != 0)
	{
		current_mean /= samples;
	}
	else
	{
		current_mean = 0.f;
	}
	return current_mean;
}

F32 LLStat::getMin() const
{
	U32 i;
	F32 current_min = mLastValue;

	if (mNumBins == 0)
	{
		current_min = 0.f;
	}
	else
	{
		for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
		{
			// Skip the bin we're currently filling.
			if (i == (U32)mNextBin)
			{
				continue;
			}
			if (mBins[i] < current_min)
			{
				current_min = mBins[i];
			}
		}
	}
	return current_min;
}

F32 LLStat::getSum() const
{
	U32 i;
	F32 sum = 0.f;
	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		sum += mBins[i];
	}

	return sum;
}

F32 LLStat::getSumDuration() const
{
	U32 i;
	F32 sum = 0.f;
	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		sum += mDT[i];
	}

	return sum;
}

F32 LLStat::getPrev(S32 age) const
{
	S32 bin;
	bin = mCurBin - age;

	while (bin < 0)
	{
		bin += mNumBins;
	}

	if (bin == mNextBin)
	{
		// Bogus for bin we're currently working on.
		return 0.f;
	}
	return mBins[bin];
}

F32 LLStat::getPrevPerSec(S32 age) const
{
	S32 bin;
	bin = mCurBin - age;

	while (bin < 0)
	{
		bin += mNumBins;
	}

	if (bin == mNextBin)
	{
		// Bogus for bin we're currently working on.
		return 0.f;
	}
	return mBins[bin] / mDT[bin];
}

F64 LLStat::getPrevBeginTime(S32 age) const
{
	S32 bin;
	bin = mCurBin - age;

	while (bin < 0)
	{
		bin += mNumBins;
	}

	if (bin == mNextBin)
	{
		// Bogus for bin we're currently working on.
		return 0.f;
	}

	return mBeginTime[bin];
}

F64 LLStat::getPrevTime(S32 age) const
{
	S32 bin;
	bin = mCurBin - age;

	while (bin < 0)
	{
		bin += mNumBins;
	}

	if (bin == mNextBin)
	{
		// Bogus for bin we're currently working on.
		return 0.f;
	}

	return mTime[bin];
}

F32 LLStat::getBin(S32 bin) const
{
	return mBins[bin];
}

F32 LLStat::getBinPerSec(S32 bin) const
{
	return mBins[bin] / mDT[bin];
}

F64 LLStat::getBinBeginTime(S32 bin) const
{
	return mBeginTime[bin];
}

F64 LLStat::getBinTime(S32 bin) const
{
	return mTime[bin];
}

F32 LLStat::getCurrent() const
{
	return mBins[mCurBin];
}

F32 LLStat::getCurrentPerSec() const
{
	return mBins[mCurBin] / mDT[mCurBin];
}

F64 LLStat::getCurrentBeginTime() const
{
	return mBeginTime[mCurBin];
}

F64 LLStat::getCurrentTime() const
{
	return mTime[mCurBin];
}

F32 LLStat::getCurrentDuration() const
{
	return mDT[mCurBin];
}

F32 LLStat::getMeanPerSec() const
{
	U32 i;
	F32 value = 0.f;
	F32 dt    = 0.f;

	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		value += mBins[i];
		dt    += mDT[i];
	}

	if (dt > 0.f)
	{
		return value/dt;
	}
	else
	{
		return 0.f;
	}
}

F32 LLStat::getMeanDuration() const
{
	F32 dur = 0.0f;
	U32 count = 0;
	for (U32 i=0; (i < mNumBins) && (i < mNumValues); i++)
	{
		if (i == (U32)mNextBin)
		{
			continue;
		}
		dur += mDT[i];
		count++;
	}

	if (count > 0)
	{
		dur /= F32(count);
		return dur;
	}
	else
	{
		return 0.f;
	}
}

F32 LLStat::getMaxPerSec() const
{
	U32 i;
	F32 value;

	if (mNextBin != 0)
	{
		value = mBins[0]/mDT[0];
	}
	else if (mNumValues > 0)
	{
		value = mBins[1]/mDT[1];
	}
	else
	{
		value = 0.f;
	}

	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		value = llmax(value, mBins[i]/mDT[i]);
	}
	return value;
}

F32 LLStat::getMinPerSec() const
{
	U32 i;
	F32 value;
	
	if (mNextBin != 0)
	{
		value = mBins[0]/mDT[0];
	}
	else if (mNumValues > 0)
	{
		value = mBins[1]/mDT[1];
	}
	else
	{
		value = 0.f;
	}

	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		value = llmin(value, mBins[i]/mDT[i]);
	}
	return value;
}

F32 LLStat::getMinDuration() const
{
	F32 dur = 0.0f;
	for (U32 i=0; (i < mNumBins) && (i < mNumValues); i++)
	{
		dur = llmin(dur, mDT[i]);
	}
	return dur;
}

U32 LLStat::getNumValues() const
{
	return mNumValues;
}

S32 LLStat::getNumBins() const
{
	return mNumBins;
}

S32 LLStat::getCurBin() const
{
	return mCurBin;
}

S32 LLStat::getNextBin() const
{
	return mNextBin;
}

F64 LLStat::getLastTime() const
{
	return mLastTime;
}
