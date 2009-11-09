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

class LLImageDecodeThread : public LLQueuedThread
{
public:
	class Responder : public LLThreadSafeRefCount
	{
	protected:
		virtual ~Responder();
	public:
		virtual void completed(bool success, LLImageRaw* raw, LLImageRaw* aux) = 0;
	};

	class ImageRequest : public LLQueuedThread::QueuedRequest
	{
	protected:
		virtual ~ImageRequest(); // use deleteRequest()
		
	public:
		ImageRequest(handle_t handle, LLImageFormatted* image,
					 U32 priority, S32 discard, BOOL needs_aux,
					 LLImageDecodeThread::Responder* responder);

		/*virtual*/ bool processRequest();
		/*virtual*/ void finishRequest(bool completed);

		// Used by unit tests to check the consitency of the request instance
		bool tut_isOK();
		
	private:
		// input
		LLPointer<LLImageFormatted> mFormattedImage;
		S32 mDiscardLevel;
		BOOL mNeedsAux;
		// output
		LLPointer<LLImageRaw> mDecodedImageRaw;
		LLPointer<LLImageRaw> mDecodedImageAux;
		BOOL mDecodedRaw;
		BOOL mDecodedAux;
		LLPointer<LLImageDecodeThread::Responder> mResponder;
	};
	
public:
	LLImageDecodeThread(bool threaded = true);
	handle_t decodeImage(LLImageFormatted* image,
						 U32 priority, S32 discard, BOOL needs_aux,
						 Responder* responder);
	S32 update(U32 max_time_ms);

	// Used by unit tests to check the consistency of the thread instance
	S32 tut_size();
	
private:
	struct creation_info
	{
		handle_t handle;
		LLPointer<LLImageFormatted> image;
		U32 priority;
		S32 discard;
		BOOL needs_aux;
		LLPointer<Responder> responder;
		creation_info(handle_t h, LLImageFormatted* i, U32 p, S32 d, BOOL aux, Responder* r)
			: handle(h), image(i), priority(p), discard(d), needs_aux(aux), responder(r)
		{}
	};
	typedef std::list<creation_info> creation_list_t;
	creation_list_t mCreationList;
	LLMutex* mCreationMutex;
};

#endif
