/** 
 * @file llrun.h
 * @author Phoenix
 * @date 2006-02-16
 * @brief Declaration of LLRunner and LLRunnable classes.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLRUN_H
#define LL_LLRUN_H

#include <vector>
#include <boost/shared_ptr.hpp>

class LLRunnable;

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
