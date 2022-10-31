/** 
 * @file lltracethreadrecorder.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "lltracethreadrecorder.h"
#include "llfasttimer.h"
#include "lltrace.h"
#include "llstl.h"

namespace LLTrace
{
extern MemStatHandle gTraceMemStat;

static ThreadRecorder* sMasterThreadRecorder = NULL;

///////////////////////////////////////////////////////////////////////
// ThreadRecorder
///////////////////////////////////////////////////////////////////////

ThreadRecorder::ThreadRecorder()
:   mParentRecorder(NULL)
{
    init();
}

void ThreadRecorder::init()
{
#if LL_TRACE_ENABLED
    LLThreadLocalSingletonPointer<BlockTimerStackRecord>::setInstance(&mBlockTimerStackRecord);
    //NB: the ordering of initialization in this function is very fragile due to a large number of implicit dependencies
    set_thread_recorder(this);
    BlockTimerStatHandle& root_time_block = BlockTimer::getRootTimeBlock();

    BlockTimerStackRecord* timer_stack = LLThreadLocalSingletonPointer<BlockTimerStackRecord>::getInstance();
    timer_stack->mTimeBlock = &root_time_block;
    timer_stack->mActiveTimer = NULL;

    mNumTimeBlockTreeNodes = AccumulatorBuffer<TimeBlockAccumulator>::getDefaultBuffer()->size();

    mTimeBlockTreeNodes = new TimeBlockTreeNode[mNumTimeBlockTreeNodes];

    activate(&mThreadRecordingBuffers);

    // initialize time block parent pointers
    for (auto& base : BlockTimerStatHandle::instance_snapshot())
    {
        // because of indirect derivation from LLInstanceTracker, have to downcast
        BlockTimerStatHandle& time_block = static_cast<BlockTimerStatHandle&>(base);
        TimeBlockTreeNode& tree_node = mTimeBlockTreeNodes[time_block.getIndex()];
        tree_node.mBlock = &time_block;
        tree_node.mParent = &root_time_block;

        time_block.getCurrentAccumulator().mParent = &root_time_block;
    }

    mRootTimer = new BlockTimer(root_time_block);
    timer_stack->mActiveTimer = mRootTimer;

    BlockTimer::getRootTimeBlock().getCurrentAccumulator().mActiveCount = 1;

    claim_alloc(gTraceMemStat, this);
    claim_alloc(gTraceMemStat, mRootTimer);
    claim_alloc(gTraceMemStat, sizeof(TimeBlockTreeNode) * mNumTimeBlockTreeNodes);
#endif
}


ThreadRecorder::ThreadRecorder( ThreadRecorder& parent )
:   mParentRecorder(&parent)
{
    init();
    mParentRecorder->addChildRecorder(this);
}


ThreadRecorder::~ThreadRecorder()
{
#if LL_TRACE_ENABLED
    LLThreadLocalSingletonPointer<BlockTimerStackRecord>::setInstance(NULL);

    disclaim_alloc(gTraceMemStat, this);
    disclaim_alloc(gTraceMemStat, sizeof(BlockTimer));
    disclaim_alloc(gTraceMemStat, sizeof(TimeBlockTreeNode) * mNumTimeBlockTreeNodes);

    deactivate(&mThreadRecordingBuffers);

    delete mRootTimer;

    if (!mActiveRecordings.empty())
    {
        std::for_each(mActiveRecordings.begin(), mActiveRecordings.end(), DeletePointer());
        mActiveRecordings.clear();
    }

    set_thread_recorder(NULL);
    delete[] mTimeBlockTreeNodes;

    if (mParentRecorder)
    {
        mParentRecorder->removeChildRecorder(this);
    }
#endif
}

TimeBlockTreeNode* ThreadRecorder::getTimeBlockTreeNode( S32 index )
{
#if LL_TRACE_ENABLED
    if (0 <= index && index < mNumTimeBlockTreeNodes)
    {
        return &mTimeBlockTreeNodes[index];
    }
#endif
    return NULL;
}

AccumulatorBufferGroup* ThreadRecorder::activate( AccumulatorBufferGroup* recording)
{
#if LL_TRACE_ENABLED
    ActiveRecording* active_recording = new ActiveRecording(recording);
    if (!mActiveRecordings.empty())
    {
        AccumulatorBufferGroup& prev_active_recording = mActiveRecordings.back()->mPartialRecording;
        prev_active_recording.sync();
        BlockTimer::updateTimes();
        prev_active_recording.handOffTo(active_recording->mPartialRecording);
    }
    mActiveRecordings.push_back(active_recording);

    mActiveRecordings.back()->mPartialRecording.makeCurrent();
    return &active_recording->mPartialRecording;
#else
    return NULL;
#endif
}

ThreadRecorder::active_recording_list_t::iterator ThreadRecorder::bringUpToDate( AccumulatorBufferGroup* recording )
{
#if LL_TRACE_ENABLED
    if (mActiveRecordings.empty()) return mActiveRecordings.end();

    mActiveRecordings.back()->mPartialRecording.sync();
    BlockTimer::updateTimes();

    active_recording_list_t::reverse_iterator it, end_it;
    for (it = mActiveRecordings.rbegin(), end_it = mActiveRecordings.rend();
        it != end_it;
        ++it)
    {
        ActiveRecording* cur_recording = *it;

        active_recording_list_t::reverse_iterator next_it = it;
        ++next_it;

        // if we have another recording further down in the stack...
        if (next_it != mActiveRecordings.rend())
        {
            // ...push our gathered data down to it
            (*next_it)->mPartialRecording.append(cur_recording->mPartialRecording);
        }

        // copy accumulated measurements into result buffer and clear accumulator (mPartialRecording)
        cur_recording->movePartialToTarget();

        if (cur_recording->mTargetRecording == recording)
        {
            // found the recording, so return it
            break;
        }
    }

    if (it == end_it)
    {
        LL_WARNS() << "Recording not active on this thread" << LL_ENDL;
    }

    return (++it).base();
#else
    return ThreadRecorder::active_recording_list_t::iterator();
#endif
}

void ThreadRecorder::deactivate( AccumulatorBufferGroup* recording )
{
#if LL_TRACE_ENABLED
    active_recording_list_t::iterator recording_it = bringUpToDate(recording);
    // this method should only be called on a thread where the recorder is active
    llassert_always(recording_it != mActiveRecordings.end());

    ActiveRecording* recording_to_remove = *recording_it;
    bool was_current = recording_to_remove->mPartialRecording.isCurrent();
    llassert(recording_to_remove->mTargetRecording == recording);
    mActiveRecordings.erase(recording_it);
    if (was_current)
    {
        if (mActiveRecordings.empty())
        {
            AccumulatorBufferGroup::clearCurrent();
        }
        else
        {
            mActiveRecordings.back()->mPartialRecording.makeCurrent();
        }
    }
    delete recording_to_remove;
#endif
}

ThreadRecorder::ActiveRecording::ActiveRecording( AccumulatorBufferGroup* target ) 
:   mTargetRecording(target)
{}

void ThreadRecorder::ActiveRecording::movePartialToTarget()
{
#if LL_TRACE_ENABLED
    mTargetRecording->append(mPartialRecording);
    // reset based on self to keep history
    mPartialRecording.reset(&mPartialRecording);
#endif
}


// called by child thread
void ThreadRecorder::addChildRecorder( class ThreadRecorder* child )
{
#if LL_TRACE_ENABLED
    { LLMutexLock lock(&mChildListMutex);
        mChildThreadRecorders.push_back(child);
    }
#endif
}

// called by child thread
void ThreadRecorder::removeChildRecorder( class ThreadRecorder* child )
{   
#if LL_TRACE_ENABLED
    { LLMutexLock lock(&mChildListMutex);
        mChildThreadRecorders.remove(child);
    }
#endif
}

void ThreadRecorder::pushToParent()
{
#if LL_TRACE_ENABLED
    { LLMutexLock lock(&mSharedRecordingMutex); 
        LLTrace::get_thread_recorder()->bringUpToDate(&mThreadRecordingBuffers);
        mSharedRecordingBuffers.append(mThreadRecordingBuffers);
        mThreadRecordingBuffers.reset();
    }
#endif
}


void ThreadRecorder::pullFromChildren()
{
#if LL_TRACE_ENABLED
    LL_PROFILE_ZONE_SCOPED_CATEGORY_STATS;
    if (mActiveRecordings.empty()) return;

    { LLMutexLock lock(&mChildListMutex);

        AccumulatorBufferGroup& target_recording_buffers = mActiveRecordings.back()->mPartialRecording;
        target_recording_buffers.sync();
        for (child_thread_recorder_list_t::iterator it = mChildThreadRecorders.begin(), end_it = mChildThreadRecorders.end();
            it != end_it;
            ++it)
        { LLMutexLock lock(&(*it)->mSharedRecordingMutex);

            target_recording_buffers.merge((*it)->mSharedRecordingBuffers);
            (*it)->mSharedRecordingBuffers.reset();
        }
    }
#endif
}


void set_master_thread_recorder( ThreadRecorder* recorder )
{
    sMasterThreadRecorder = recorder;
}


ThreadRecorder* get_master_thread_recorder()
{
    return sMasterThreadRecorder;
}

LLThreadLocalPointer<ThreadRecorder>& get_thread_recorder_ptr()
{
    static LLThreadLocalPointer<ThreadRecorder> s_thread_recorder;
    return s_thread_recorder;
}

const LLThreadLocalPointer<ThreadRecorder>& get_thread_recorder()
{
    return get_thread_recorder_ptr();
}

void set_thread_recorder( ThreadRecorder* recorder )
{
    get_thread_recorder_ptr() = recorder;
}

}
