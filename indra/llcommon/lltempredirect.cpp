/**
 * @file   lltempredirect.cpp
 * @author Nat Goodspeed
 * @date   2019-10-31
 * @brief  Implementation for lltempredirect.
 * 
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Copyright (c) 2019, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "lltempredirect.h"
// STL headers
// std headers
#if !LL_WINDOWS
# include <unistd.h>
#else
# include <io.h>
#endif // !LL_WINDOWS
// external library headers
// other Linden headers

/*****************************************************************************
*   llfd
*****************************************************************************/
// We could restate the implementation of each of llfd::close(), etc., but
// this is way more succinct.
#if LL_WINDOWS
#define fhclose  _close
#define fhdup    _dup
#define fhdup2   _dup2
#define fhfdopen _fdopen
#define fhfileno _fileno
#else
#define fhclose  ::close
#define fhdup    ::dup
#define fhdup2   ::dup2
#define fhfdopen ::fdopen
#define fhfileno ::fileno
#endif

int llfd::close(int fd)
{
    return fhclose(fd);
}

int llfd::dup(int target)
{
    return fhdup(target);
}

int llfd::dup2(int target, int reference)
{
    return fhdup2(target, reference);
}

FILE* llfd::open(int fd, const char* mode)
{
    return fhfdopen(fd, mode);
}

int llfd::fileno(FILE* stream)
{
    return fhfileno(stream);
}

/*****************************************************************************
*   LLTempRedirect
*****************************************************************************/
LLTempRedirect::LLTempRedirect():
    mOrigTarget(-1),                // -1 is an invalid file descriptor
    mReference(-1)
{}

LLTempRedirect::LLTempRedirect(FILE* target, FILE* reference):
    LLTempRedirect((target?    fhfileno(target)    : -1),
                   (reference? fhfileno(reference) : -1))
{}

LLTempRedirect::LLTempRedirect(int target, int reference):
    // capture a duplicate file descriptor for the file originally targeted by
    // 'reference'
    mOrigTarget((reference >= 0)? fhdup(reference) : -1),
    mReference(reference)
{
    if (target >= 0 && reference >= 0)
    {
        // As promised, force 'reference' to refer to 'target'. This first
        // implicitly closes 'reference', which is why we first capture a
        // duplicate so the original target file stays open.
        fhdup2(target, reference);
    }
}

LLTempRedirect::LLTempRedirect(LLTempRedirect&& other)
{
    mOrigTarget = other.mOrigTarget;
    mReference  = other.mReference;
    // other LLTempRedirect must be in moved-from state so its destructor
    // won't repeat the same operations as ours!
    other.mOrigTarget = -1;
    other.mReference  = -1;
}

LLTempRedirect::~LLTempRedirect()
{
    reset();
}

void LLTempRedirect::reset()
{
    // If this instance was default-constructed (or constructed with an
    // invalid file descriptor), skip the following.
    if (mOrigTarget >= 0)
    {
        // Restore mReference to point to mOrigTarget. This implicitly closes
        // the duplicate created by our constructor of its 'target' file
        // descriptor.
        fhdup2(mOrigTarget, mReference);
        // mOrigTarget has served its purpose
        fhclose(mOrigTarget);
        // assign these because reset() is also responsible for a "moved from"
        // instance
        mOrigTarget = -1;
        mReference  = -1;
    }
}

LLTempRedirect& LLTempRedirect::operator=(LLTempRedirect&& other)
{
    reset();
    std::swap(mOrigTarget, other.mOrigTarget);
    std::swap(mReference,  other.mReference);
    return *this;
}
