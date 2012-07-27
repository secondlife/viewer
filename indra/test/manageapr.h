/**
 * @file   manageapr.h
 * @author Nat Goodspeed
 * @date   2012-01-13
 * @brief  ManageAPR class for simple test programs
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_MANAGEAPR_H)
#define LL_MANAGEAPR_H

#include "llapr.h"
#include <boost/noncopyable.hpp>

/**
 * Declare a static instance of this class for dead-simple ll_init_apr() at
 * program startup, ll_cleanup_apr() at termination. This is recommended for
 * use only with simple test programs. Once you start introducing static
 * instances of other classes that depend on APR already being initialized,
 * the indeterminate static-constructor-order problem rears its ugly head.
 */
class ManageAPR: public boost::noncopyable
{
public:
    ManageAPR()
    {
        ll_init_apr();
    }

    ~ManageAPR()
    {
        ll_cleanup_apr();
    }

    static std::string strerror(apr_status_t rv)
    {
        char errbuf[256];
        apr_strerror(rv, errbuf, sizeof(errbuf));
        return errbuf;
    }
};

#endif /* ! defined(LL_MANAGEAPR_H) */
