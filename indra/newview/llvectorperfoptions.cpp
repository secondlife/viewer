/** 
 * @file llvectorperfoptions.cpp
 * @brief Control of vector perfomance options.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

