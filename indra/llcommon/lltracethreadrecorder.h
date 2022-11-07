/** 
 * @file lltrace.h
 * @brief Runtime statistics accumulation.
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

#ifndef LL_LLTRACETHREADRECORDER_H
#define LL_LLTRACETHREADRECORDER_H

#include "stdtypes.h"
#include "llpreprocessor.h"

#include "llmutex.h"
#include "lltraceaccumulators.h"
#include "llthreadlocalstorage.h"

namespace LLTrace
{
    class LL_COMMON_API ThreadRecorder
    {
    protected:
        struct ActiveRecording;
        typedef std::vector<ActiveRecording*> active_recording_list_t;
    public:
        ThreadRecorder();
        explicit ThreadRecorder(ThreadRecorder& parent);

        ~ThreadRecorder();

        AccumulatorBufferGroup* activate(AccumulatorBufferGroup* recording);
        void deactivate(AccumulatorBufferGroup* recording);
        active_recording_list_t::iterator bringUpToDate(AccumulatorBufferGroup* recording);

        void addChildRecorder(class ThreadRecorder* child);
        void removeChildRecorder(class ThreadRecorder* child);

        // call this periodically to gather stats data from child threads
        void pullFromChildren();
        void pushToParent();

        TimeBlockTreeNode* getTimeBlockTreeNode(S32 index);

    protected:
        void init();

    protected:
        struct ActiveRecording
        {
            ActiveRecording(AccumulatorBufferGroup* target);

            AccumulatorBufferGroup* mTargetRecording;
            AccumulatorBufferGroup  mPartialRecording;

            void movePartialToTarget();
        };

        AccumulatorBufferGroup          mThreadRecordingBuffers;

        BlockTimerStackRecord           mBlockTimerStackRecord;
        active_recording_list_t         mActiveRecordings;

        class BlockTimer*               mRootTimer;
        TimeBlockTreeNode*              mTimeBlockTreeNodes;
        size_t                          mNumTimeBlockTreeNodes;
        typedef std::list<class ThreadRecorder*> child_thread_recorder_list_t;

        child_thread_recorder_list_t    mChildThreadRecorders;  // list of child thread recorders associated with this master
        LLMutex                         mChildListMutex;        // protects access to child list
        LLMutex                         mSharedRecordingMutex;
        AccumulatorBufferGroup          mSharedRecordingBuffers;
        ThreadRecorder*                 mParentRecorder;

    };

    const LLThreadLocalPointer<ThreadRecorder>& get_thread_recorder();
    void set_thread_recorder(ThreadRecorder*);

    void set_master_thread_recorder(ThreadRecorder*);
    ThreadRecorder* get_master_thread_recorder();
}

#endif // LL_LLTRACETHREADRECORDER_H
