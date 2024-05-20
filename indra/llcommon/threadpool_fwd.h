/**
 * @file   threadpool_fwd.h
 * @author Nat Goodspeed
 * @date   2022-12-09
 * @brief  Forward declarations for ThreadPool et al.
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Copyright (c) 2022, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_THREADPOOL_FWD_H)
#define LL_THREADPOOL_FWD_H

#include "workqueue.h"

namespace LL
{
    template <class QUEUE>
    struct ThreadPoolUsing;

    using ThreadPool = ThreadPoolUsing<WorkQueue>;
} // namespace LL

#endif /* ! defined(LL_THREADPOOL_FWD_H) */
