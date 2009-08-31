/** 
 * @file llimageworker.h
 * @brief Object for managing images and their textures.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLIMAGEWORKER_H
#define LL_LLIMAGEWORKER_H

#include "llimage.h"
#include "llpointer.h"
#include "llworkerthread.h"

class LLImageWorker : public LLWorkerClass
{
public:
	static void initImageWorker(LLWorkerThread* workerthread);
	static void cleanupImageWorker();
	
public:
	static LLWorkerThread* getWorkerThread() { return sWorkerThread; }

	// LLWorkerThread
public:
	LLImageWorker(LLImageFormatted* image, U32 priority, S32 discard,
				  LLPointer<LLResponder> responder);
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
