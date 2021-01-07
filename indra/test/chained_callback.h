/**
 * @file   chained_callback.h
 * @author Nat Goodspeed
 * @date   2020-01-03
 * @brief  Subclass of tut::callback used for chaining callbacks.
 * 
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Copyright (c) 2020, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_CHAINED_CALLBACK_H)
#define LL_CHAINED_CALLBACK_H

#include "lltut.h"

/**
 * Derive your TUT callback from chained_callback instead of tut::callback to
 * ensure that multiple such callbacks can coexist in a given test executable.
 * The relevant callback method will be called for each callback instance in
 * reverse order of the instance's link() methods being called: the most
 * recently link()ed callback will be called first, then the previous, and so
 * forth.
 *
 * Obviously, for this to work, all relevant callbacks must be derived from
 * chained_callback instead of tut::callback. Given that, control should reach
 * each of them regardless of their construction order. The chain is
 * guaranteed to stop because the first link() call will link to test_runner's
 * default_callback, which is simply an instance of the callback() base class.
 *
 * The rule for deriving from chained_callback is that you may override any of
 * its virtual methods, but your override must at some point call the
 * corresponding chained_callback method.
 */
class chained_callback: public tut::callback
{
public:
    /**
     * Instead of calling tut::test_runner::set_callback(&your_callback), call
     * your_callback.link();
     * This uses the canonical instance of tut::test_runner.
     */
    void link()
    {
        link(tut::runner.get());
    }

    /**
     * If for some reason you have a different instance of test_runner...
     */
    void link(tut::test_runner& runner)
    {
        // Since test_runner's constructor sets a default callback,
        // get_callback() will always return a reference to a valid callback
        // instance.
        mPrev = &runner.get_callback();
        runner.set_callback(this);
    }

    /**
     * Called when new test run started.
     */
    virtual void run_started()
    {
        mPrev->run_started();
    }

    /**
     * Called when a group started
     * @param name Name of the group
     */
    virtual void group_started(const std::string& name)
    {
        mPrev->group_started(name);
    }

    /**
     * Called when a test finished.
     * @param tr Test results.
     */
    virtual void test_completed(const tut::test_result& tr)
    {
        mPrev->test_completed(tr);
    }

    /**
     * Called when a group is completed
     * @param name Name of the group
     */
    virtual void group_completed(const std::string& name)
    {
        mPrev->group_completed(name);
    }

    /**
     * Called when all tests in run completed.
     */
    virtual void run_completed()
    {
        mPrev->run_completed();
    }

private:
    tut::callback* mPrev;
};

#endif /* ! defined(LL_CHAINED_CALLBACK_H) */
