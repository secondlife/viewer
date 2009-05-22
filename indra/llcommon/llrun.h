/** 
 * @file llrun.h
 * @author Phoenix
 * @date 2006-02-16
 * @brief Declaration of LLRunner and LLRunnable classes.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#ifndef LL_LLRUN_H
#define LL_LLRUN_H

#include <vector>
#include <boost/shared_ptr.hpp>

class LL_COMMON_API LLRunnable;

/** 
 * @class LLRunner
 * @brief This class manages a set of LLRunnable objects.
 *
 * An instance of this class has a collection of LLRunnable objects
 * which are scheduled to run on a repeating or one time basis.
 * @see LLRunnable
 */
class LL_COMMON_API LLRunner
{
public:
	/**
	 * @brief The pointer to a runnable.
	 */
	typedef boost::shared_ptr<LLRunnable> run_ptr_t;

	/**
	 * @brief The handle for use in the API.
	 */
	typedef S64 run_handle_t;

	/**
	 * @brief Constructor.
	 */
	LLRunner();

	/**
	 * @brief Destructor.
	 */
	~LLRunner();

	/** 
	 * @brief Enumeration which specifies when to run.
	 */
	enum ERunSchedule
	{
		// The runnable will run in N seconds
		RUN_IN,

		// The run every N seconds
		RUN_EVERY,

		// A count of the run types
		RUN_SCHEDULE_COUNT
	};

	/**
	 * @brief Run the runnables which are scheduled to run
	 *
	 * @return Returns the number of runnables run.
	 */
	S32 run();

	/** 
	 * @brief Add a runnable to the run list.
	 *
	 * The handle of the runnable is unique to each addition. If the
	 * same runnable is added a second time with the same or different
	 * schedule, this method will return a new handle.
	 * @param runnable The runnable to run() on schedule.
	 * @param schedule Specifies the run schedule.
	 * @param seconds When to run the runnable as interpreted by schedule.
	 * @return Returns the handle to the runnable. handle == 0 means failure.
	 */
	run_handle_t addRunnable(
		run_ptr_t runnable,
		ERunSchedule schedule,
		F64 seconds);
	
	/**
	 * @brief Remove the specified runnable.
	 *
	 * @param handle The handle of the runnable to remove.
	 * @return Returns the pointer to the runnable removed which may
	 * be empty.
	 */
	run_ptr_t removeRunnable(run_handle_t handle);

protected:
	struct LLRunInfo
	{
		run_handle_t mHandle;
		run_ptr_t mRunnable;
		ERunSchedule mSchedule;
		F64 mNextRunAt;
		F64 mIncrement;
		LLRunInfo(
			run_handle_t handle,
			run_ptr_t runnable,
			ERunSchedule schedule,
			F64 next_run_at,
			F64 increment);
	};
	typedef std::vector<LLRunInfo> run_list_t;
	run_list_t mRunOnce;
	run_list_t mRunEvery;
	run_handle_t mNextHandle;
};


/** 
 * @class LLRunnable
 * @brief Abstract base class for running some scheduled process.
 *
 * Users of the LLRunner class are expected to derive a concrete
 * implementation of this class which overrides the run() method to do
 * something useful.
 * @see LLRunner
 */
class LL_COMMON_API LLRunnable
{
public:
	LLRunnable();
	virtual ~LLRunnable();

	/** 
	 * @brief Do the process.
	 *
	 * This method will be called from the LLRunner according to 
	 * @param runner The Runner which call run().
	 * @param handle The handle this run instance is run under.
	 */
	virtual void run(LLRunner* runner, S64 handle) = 0;
};

#endif // LL_LLRUN_H
