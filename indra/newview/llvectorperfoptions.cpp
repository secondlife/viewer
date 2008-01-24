/** 
 * @file llvectorperfoptions.h
 * @brief SSE/SSE2 vector math performance options.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2008, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llviewerprecompiledheaders.h"

#include "llvectorperfoptions.h"
#include "llviewerjointmesh.h"
#include "llviewercontrol.h"

// Initially, we test the performance of the vectorization code, then
// turn it off if it ends up being slower. JC
BOOL	gVectorizePerfTest	= TRUE;
BOOL	gVectorizeEnable	= FALSE;
U32		gVectorizeProcessor	= 0;
BOOL	gVectorizeSkin		= FALSE;

void update_vector_performances(void)
{
	char *vp;
	
	switch(gVectorizeProcessor)
	{
		case 2: vp = "SSE2"; break;					// *TODO: replace the magic #s
		case 1: vp = "SSE"; break;
		default: vp = "COMPILER DEFAULT"; break;
	}
	llinfos << "Vectorization         : " << ( gVectorizeEnable ? "ENABLED" : "DISABLED" ) << llendl ;
	llinfos << "Vector Processor      : " << vp << llendl ;
	llinfos << "Vectorized Skinning   : " << ( gVectorizeSkin ? "ENABLED" : "DISABLED" ) << llendl ;
	
	if(gVectorizeEnable && gVectorizeSkin)
	{
		switch(gVectorizeProcessor)
		{
			case 2:
				LLViewerJointMesh::sUpdateGeometryFunc = &LLViewerJointMesh::updateGeometrySSE2;
				break;
			case 1:
				LLViewerJointMesh::sUpdateGeometryFunc = &LLViewerJointMesh::updateGeometrySSE;
				break;
			default:
				LLViewerJointMesh::sUpdateGeometryFunc = &LLViewerJointMesh::updateGeometryVectorized;
				break;
		}
	}
	else
	{
		LLViewerJointMesh::sUpdateGeometryFunc = &LLViewerJointMesh::updateGeometryOriginal;
	}
}


class LLVectorizationEnableListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gVectorizeEnable = event->getValue().asBoolean();
		update_vector_performances();
		return true;
	}
};
static LLVectorizationEnableListener vectorization_enable_listener;

class LLVectorizeSkinListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gVectorizeSkin = event->getValue().asBoolean();
		update_vector_performances();
		return true;
	}
};
static LLVectorizeSkinListener vectorize_skin_listener;

class LLVectorProcessorListener: public LLSimpleListener
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gVectorizeProcessor = event->getValue().asInteger();
		update_vector_performances();
		return true;
	}
};
static LLVectorProcessorListener vector_processor_listener;

void LLVectorPerformanceOptions::initClass()
{
	gVectorizePerfTest = gSavedSettings.getBOOL("VectorizePerfTest");
	gVectorizeEnable = gSavedSettings.getBOOL("VectorizeEnable");
	gVectorizeProcessor = gSavedSettings.getU32("VectorizeProcessor");
	gVectorizeSkin = gSavedSettings.getBOOL("VectorizeSkin");
	update_vector_performances();

	// these are currently static in this file, so they can't move to settings_setup_listeners
	gSavedSettings.getControl("VectorizeEnable")->addListener(&vectorization_enable_listener);
	gSavedSettings.getControl("VectorizeProcessor")->addListener(&vector_processor_listener);
	gSavedSettings.getControl("VectorizeSkin")->addListener(&vectorize_skin_listener);
}

void LLVectorPerformanceOptions::cleanupClass()
{
}

