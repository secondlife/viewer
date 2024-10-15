/**
 * @file   tempset.h
 * @author Nat Goodspeed
 * @date   2024-06-12
 * @brief  Temporarily override a variable for scope duration, then restore
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_TEMPSET_H)
#define LL_TEMPSET_H

// RAII class to set specified variable to specified value
// only for the duration of containing scope
template <typename VAR, typename VALUE>
class TempSet
{
public:
    TempSet(VAR& var, const VALUE& value):
        mVar(var),
        mOldValue(mVar)
    {
        mVar = value;
    }

    TempSet(const TempSet&) = delete;
    TempSet& operator=(const TempSet&) = delete;

    ~TempSet()
    {
        mVar = mOldValue;
    }

private:
    VAR& mVar;
    VAR mOldValue;
};

#endif /* ! defined(LL_TEMPSET_H) */
