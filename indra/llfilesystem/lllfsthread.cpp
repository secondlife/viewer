/**
 * @file lllfsthread.cpp
 * @brief LLLFSThread base class
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
#include "lllfsthread.h"
#include "llstl.h"
#include "llapr.h"

//============================================================================

/*static*/ LLLFSThread* LLLFSThread::sLocal = NULL;

//============================================================================
// Run on MAIN thread
//static
void LLLFSThread::initClass(bool local_is_threaded)
{
    llassert(sLocal == NULL);
    sLocal = new LLLFSThread(local_is_threaded);
}

//static
S32 LLLFSThread::updateClass(U32 ms_elapsed)
{
    return sLocal->update((F32)ms_elapsed);
}

//static
void LLLFSThread::cleanupClass()
{
    llassert(sLocal != NULL);
    sLocal->setQuitting();
    while (sLocal->getPending())
    {
        sLocal->update(0);
    }
    sLocal->shutdown();
    delete sLocal;
    sLocal = NULL;
}

//----------------------------------------------------------------------------

LLLFSThread::LLLFSThread(bool threaded) :
    LLQueuedThread("LFS", threaded)
{
    if(!mLocalAPRFilePoolp)
    {
        mLocalAPRFilePoolp = new LLVolatileAPRPool() ;
    }
}

LLLFSThread::~LLLFSThread()
{
    // mLocalAPRFilePoolp cleanup in LLThread
    // ~LLQueuedThread() will be called here
}

//----------------------------------------------------------------------------

LLLFSThread::handle_t LLLFSThread::read(const std::string& filename,    /* Flawfinder: ignore */
                                        U8* buffer, S32 offset, S32 numbytes,
                                        Responder* responder)
{
    LL_PROFILE_ZONE_SCOPED;
    handle_t handle = generateHandle();

    Request* req = new Request(this, handle,
                               FILE_READ, filename,
                               buffer, offset, numbytes,
                               responder);

    bool res = addRequest(req);
    if (!res)
    {
        LL_ERRS() << "LLLFSThread::read called after LLLFSThread::cleanupClass()" << LL_ENDL;
    }

    return handle;
}

LLLFSThread::handle_t LLLFSThread::write(const std::string& filename,
                                         U8* buffer, S32 offset, S32 numbytes,
                                         Responder* responder)
{
    LL_PROFILE_ZONE_SCOPED;
    handle_t handle = generateHandle();

    Request* req = new Request(this, handle,
                               FILE_WRITE, filename,
                               buffer, offset, numbytes,
                               responder);

    bool res = addRequest(req);
    if (!res)
    {
        LL_ERRS() << "LLLFSThread::read called after LLLFSThread::cleanupClass()" << LL_ENDL;
    }

    return handle;
}

//============================================================================

LLLFSThread::Request::Request(LLLFSThread* thread,
                              handle_t handle,
                              operation_t op, const std::string& filename,
                              U8* buffer, S32 offset, S32 numbytes,
                              Responder* responder) :
    QueuedRequest(handle, FLAG_AUTO_COMPLETE),
    mThread(thread),
    mOperation(op),
    mFileName(filename),
    mBuffer(buffer),
    mOffset(offset),
    mBytes(numbytes),
    mBytesRead(0),
    mResponder(responder)
{
    if (numbytes <= 0)
    {
        LL_WARNS() << "LLLFSThread: Request with numbytes = " << numbytes << LL_ENDL;
    }
}

LLLFSThread::Request::~Request()
{
}

// virtual, called from own thread
void LLLFSThread::Request::finishRequest(bool completed)
{
    LL_PROFILE_ZONE_SCOPED;
    if (mResponder.notNull())
    {
        mResponder->completed(completed ? mBytesRead : 0);
        mResponder = NULL;
    }
}

void LLLFSThread::Request::deleteRequest()
{
    LL_PROFILE_ZONE_SCOPED;
    if (getStatus() == STATUS_QUEUED)
    {
        LL_ERRS() << "Attempt to delete a queued LLLFSThread::Request!" << LL_ENDL;
    }
    if (mResponder.notNull())
    {
        mResponder->completed(0);
        mResponder = NULL;
    }
    LLQueuedThread::QueuedRequest::deleteRequest();
}

bool LLLFSThread::Request::processRequest()
{
    LL_PROFILE_ZONE_SCOPED;
    bool complete = false;
    if (mOperation ==  FILE_READ)
    {
        llassert(mOffset >= 0);
        LLAPRFile infile ; // auto-closes
        infile.open(mFileName, LL_APR_RB, mThread->getLocalAPRFilePool());
        if (!infile.getFileHandle())
        {
            LL_WARNS() << "LLLFS: Unable to read file: " << mFileName << LL_ENDL;
            mBytesRead = 0; // fail
            return true;
        }
        S32 off;
        if (mOffset < 0)
            off = infile.seek(APR_END, 0);
        else
            off = infile.seek(APR_SET, mOffset);
        llassert_always(off >= 0);
        mBytesRead = infile.read(mBuffer, mBytes );
        complete = true;
//      LL_INFOS() << "LLLFSThread::READ:" << mFileName << " Bytes: " << mBytesRead << LL_ENDL;
    }
    else if (mOperation ==  FILE_WRITE)
    {
        apr_int32_t flags = APR_CREATE|APR_WRITE|APR_BINARY;
        if (mOffset < 0)
            flags |= APR_APPEND;
        LLAPRFile outfile ; // auto-closes
        outfile.open(mFileName, flags, mThread->getLocalAPRFilePool());
        if (!outfile.getFileHandle())
        {
            LL_WARNS() << "LLLFS: Unable to write file: " << mFileName << LL_ENDL;
            mBytesRead = 0; // fail
            return true;
        }
        if (mOffset >= 0)
        {
            S32 seek = outfile.seek(APR_SET, mOffset);
            if (seek < 0)
            {
                LL_WARNS() << "LLLFS: Unable to write file (seek failed): " << mFileName << LL_ENDL;
                mBytesRead = 0; // fail
                return true;
            }
        }
        mBytesRead = outfile.write(mBuffer, mBytes );
        complete = true;
//      LL_INFOS() << "LLLFSThread::WRITE:" << mFileName << " Bytes: " << mBytesRead << "/" << mBytes << " Offset:" << mOffset << LL_ENDL;
    }
    else
    {
        LL_ERRS() << "LLLFSThread::unknown operation: " << (S32)mOperation << LL_ENDL;
    }
    return complete;
}

//============================================================================

LLLFSThread::Responder::~Responder()
{
}

//============================================================================
