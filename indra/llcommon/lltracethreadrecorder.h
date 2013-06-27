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

		virtual ~ThreadRecorder();

		void activate(AccumulatorBufferGroup* recording);
		void deactivate(AccumulatorBufferGroup* recording);
		active_recording_list_t::reverse_iterator bringUpToDate(AccumulatorBufferGroup* recording);

		virtual void pushToMaster() = 0;

		TimeBlockTreeNode* getTimeBlockTreeNode(S32 index);

	protected:
		struct ActiveRecording
		{
			ActiveRecording(AccumulatorBufferGroup* target);

			AccumulatorBufferGroup*	mTargetRecording;
			AccumulatorBufferGroup	mPartialRecording;

			void movePartialToTarget();
		};
		AccumulatorBufferGroup			mThreadRecordingBuffers;

		active_recording_list_t		mActiveRecordings;

		class BlockTimer*			mRootTimer;
		TimeBlockTreeNode*			mTimeBlockTreeNodes;
		size_t						mNumTimeBlockTreeNodes;
		BlockTimerStackRecord		mBlockTimerStackRecord;
	};

	class LL_COMMON_API MasterThreadRecorder : public ThreadRecorder
	{
	public:
		MasterThreadRecorder();

		void addSlaveThread(class SlaveThreadRecorder* child);
		void removeSlaveThread(class SlaveThreadRecorder* child);

		/*virtual */ void pushToMaster();

		// call this periodically to gather stats data from slave threads
		void pullFromSlaveThreads();

	private:

		typedef std::list<class SlaveThreadRecorder*> slave_thread_recorder_list_t;

		slave_thread_recorder_list_t	mSlaveThreadRecorders;	// list of slave thread recorders associated with this master
		LLMutex							mSlaveListMutex;		// protects access to slave list
	};

	class LL_COMMON_API SlaveThreadRecorder : public ThreadRecorder
	{
	public:
		SlaveThreadRecorder(MasterThreadRecorder& master);
		~SlaveThreadRecorder();

		// call this periodically to gather stats data for master thread to consume
		/*virtual*/ void pushToMaster();

	private:
		friend class MasterThreadRecorder;
		LLMutex					mSharedRecordingMutex;
		AccumulatorBufferGroup	mSharedRecordingBuffers;
		MasterThreadRecorder&	mMasterRecorder;
	};

	//FIXME: let user code set up thread recorder topology
	extern MasterThreadRecorder* gUIThreadRecorder ;

	const LLThreadLocalPointer<class ThreadRecorder>& get_thread_recorder();
	void set_thread_recorder(class ThreadRecorder*);
	class MasterThreadRecorder& getUIThreadRecorder();

}

#endif // LL_LLTRACETHREADRECORDER_H
