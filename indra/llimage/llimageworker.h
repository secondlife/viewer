/** 
 * @file llimageworker.h
 * @brief Object for managing images and their textures.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLIMAGEWORKER_H
#define LL_LLIMAGEWORKER_H

#include "llimage.h"
#include "llworkerthread.h"

class LLImageWorker : public LLWorkerClass
{
public:
	static void initClass(LLWorkerThread* workerthread);
	static void cleanupClass();
	static LLWorkerThread* getWorkerThread() { return sWorkerThread; }

	// LLWorkerThread
public:
	LLImageWorker(LLImageFormatted* image, U32 priority, S32 discard, LLResponder* responder = NULL);
	~LLImageWorker();

	// called from WORKER THREAD, returns TRUE if done
	/*virtual*/ bool doWork(S32 param);
	
	BOOL requestDecodedData(LLPointer<LLImageRaw>& raw, S32 discard = -1);
	BOOL requestDecodedAuxData(LLPointer<LLImageRaw>& raw, S32 channel, S32 discard = -1);
	void releaseDecodedData();
	void cancelDecode();

private:
	// called from MAIN THREAD
	/*virtual*/ void startWork(S32 param); // called from addWork()
	/*virtual*/ void endWork(S32 param, bool aborted); // called from doWork()

protected:
	LLPointer<LLImageFormatted> mFormattedImage;
	LLPointer<LLImageRaw> mDecodedImage;
	S32 mDecodedType;
	S32 mDiscardLevel;

private:
	U32 mPriority;
	LLPointer<LLResponder> mResponder;
	
protected:
	static LLWorkerThread* sWorkerThread;

public:
	static S32 sCount;
};

#endif
