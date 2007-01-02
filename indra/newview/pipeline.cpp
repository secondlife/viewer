/** 
 * @file pipeline.cpp
 * @brief Rendering pipeline.
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "pipeline.h"

// library includes
#include "audioengine.h" // For MAX_BUFFERS for debugging.
#include "imageids.h"
#include "llagpmempool.h"
#include "llerror.h"
#include "llviewercontrol.h"
#include "llfasttimer.h"
#include "llfontgl.h"
#include "llmemory.h"
#include "llnamevalue.h"
#include "llprimitive.h"
#include "llvolume.h"
#include "material_codes.h"
#include "timing.h"
#include "v3color.h"
#include "llui.h"

// newview includes
#include "llagent.h"
#include "llagparray.h"
#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolground.h"
#include "lldrawpoolsimple.h"
#include "lldrawpooltree.h"
#include "lldrawpoolhud.h"
#include "lldrawpoolwater.h"
#include "llface.h"
#include "llfeaturemanager.h"
#include "llfloatertelehub.h"
#include "llframestats.h"
#include "llgldbg.h"
#include "llhudmanager.h"
#include "lllightconstants.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "lltracker.h"
#include "lltool.h"
#include "lltoolmgr.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h" // for audio debugging.
#include "llviewerwindow.h" // For getSpinAxis
#include "llvoavatar.h"
#include "llvoground.h"
#include "llvosky.h"
#include "llvotree.h"
#include "llvovolume.h"
#include "llworld.h"
#include "viewer.h"
#include "llagpmempoolarb.h"
#include "llagparray.inl"

#ifdef _DEBUG
// Debug indices is disabled for now for debug performance - djs 4/24/02
//#define DEBUG_INDICES
#else
//#define DEBUG_INDICES
#endif

const F32 BACKLIGHT_DAY_MAGNITUDE_AVATAR = 0.2f;
const F32 BACKLIGHT_NIGHT_MAGNITUDE_AVATAR = 0.1f;
const F32 BACKLIGHT_DAY_MAGNITUDE_OBJECT = 0.1f;
const F32 BACKLIGHT_NIGHT_MAGNITUDE_OBJECT = 0.08f;
const S32 MAX_ACTIVE_OBJECT_QUIET_FRAMES = 40;
const S32 MAX_OFFSCREEN_GEOMETRY_CHANGES_PER_FRAME = 10;

// Guess on the number of visible objects in the scene, used to
// pre-size std::vector and other arrays. JC
const S32 ESTIMATED_VISIBLE_OBJECT_COUNT = 8192;

// If the sum of the X + Y + Z scale of an object exceeds this number,
// it will be considered a potential occluder.  For instance,
// a box of size 6 x 6 x 1 has sum 13, which might be an occluder. JC
const F32 OCCLUDE_SCALE_SUM_THRESHOLD = 8.f;

// Max number of occluders to search for. JC
const S32 MAX_OCCLUDER_COUNT = 2;

extern S32 gBoxFrame;
extern BOOL gRenderLightGlows;
extern BOOL gHideSelectedObjects;


BOOL	gAvatarBacklight = FALSE;

F32		gMinObjectDistance = MIN_NEAR_PLANE;
S32		gTrivialAccepts = 0;

BOOL	gRenderForSelect = FALSE;

BOOL	gUsePickAlpha = TRUE;
F32		gPickAlphaThreshold = 0.f;
F32		gPickAlphaTargetThreshold = 0.f;

//glsl parameter tables
const char* LLPipeline::sReservedAttribs[] =
{
	"materialColor",
	"specularColor",
	"binormal"
};

U32 LLPipeline::sReservedAttribCount = LLPipeline::GLSL_END_RESERVED_ATTRIBS;

const char* LLPipeline::sAvatarAttribs[] = 
{
	"weight",
	"clothing",
	"gWindDir",
	"gSinWaveParams",
	"gGravity"
};

U32 LLPipeline::sAvatarAttribCount =  sizeof(LLPipeline::sAvatarAttribs)/sizeof(char*);

const char* LLPipeline::sAvatarUniforms[] = 
{
	"matrixPalette"
};

U32 LLPipeline::sAvatarUniformCount = 1;

const char* LLPipeline::sReservedUniforms[] =
{
	"diffuseMap",
	"specularMap",
	"bumpMap",
	"environmentMap",
	"scatterMap"
};

U32 LLPipeline::sReservedUniformCount = LLPipeline::GLSL_END_RESERVED_UNIFORMS;

const char* LLPipeline::sTerrainUniforms[] =
{
	"detail0",
	"detail1",
	"alphaRamp"
};

U32 LLPipeline::sTerrainUniformCount = sizeof(LLPipeline::sTerrainUniforms)/sizeof(char*);

const char* LLPipeline::sWaterUniforms[] =
{
	"screenTex",
	"eyeVec",
	"time",
	"d1",
	"d2",
	"lightDir",
	"specular",
	"lightExp",
	"fbScale",
	"refScale"
};

U32 LLPipeline::sWaterUniformCount =  sizeof(LLPipeline::sWaterUniforms)/sizeof(char*);

// the SSE variable is dependent on software blending being enabled. 

//----------------------------------------

void stamp(F32 x, F32 y, F32 xs, F32 ys)
{
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex3f(x,   y,   0.0f);
	glTexCoord2f(1,0);
	glVertex3f(x+xs,y,   0.0f);
	glTexCoord2f(1,1);
	glVertex3f(x+xs,y+ys,0.0f);
	glTexCoord2f(0,1);
	glVertex3f(x,   y+ys,0.0f);
	glEnd();
}



//----------------------------------------

S32		LLPipeline::sCompiles = 0;
S32		LLPipeline::sAGPMaxPoolSize = 1 << 25; // 32MB
BOOL	LLPipeline::sRenderPhysicalBeacons = FALSE;
BOOL	LLPipeline::sRenderScriptedBeacons = FALSE;
BOOL	LLPipeline::sRenderParticleBeacons = FALSE;
BOOL	LLPipeline::sRenderSoundBeacons = FALSE;

LLPipeline::LLPipeline() :
	mVertexShadersEnabled(FALSE),
	mVertexShadersLoaded(0),
	mLastRebuildPool(NULL),
	mAlphaPool(NULL),
	mSkyPool(NULL),
	mStarsPool(NULL),
	mCloudsPool(NULL),
	mTerrainPool(NULL),
	mWaterPool(NULL),
	mGroundPool(NULL),
	mHUDPool(NULL),
	mAGPMemPool(NULL),
	mGlobalFence(0),
	mBufferIndex(0),
	mBufferCount(kMaxBufferCount),
	mUseOcclusionCulling(FALSE),
	mLightMask(0),
	mLightMovingMask(0)
{
	for(S32 i = 0; i < kMaxBufferCount; i++)
	{
		mBufferMemory[i] = NULL;
	}
	for (S32 i = 0; i < kMaxBufferCount; i++)
	{
		mBufferFence[i] = 0;
	}
}

void LLPipeline::init()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	
	stop_glerror();

	mAGPBound = FALSE;
	mObjectPartition = new LLSpatialPartition;
	mTrianglesDrawnStat.reset();
	resetFrameStats();

	mRenderTypeMask = 0xffffffff;	// All render types start on
	mRenderDebugFeatureMask = 0xffffffff; // All debugging features on
	mRenderFeatureMask = 0;	// All features start off
	mRenderDebugMask = 0;	// All debug starts off

	mBackfaceCull = TRUE;

	// Disable AGP initially.
	mRenderFeatureMask &= ~RENDER_FEATURE_AGP;

	stop_glerror();
	
	// Enable features

	mUseVBO = gSavedSettings.getBOOL("RenderUseVBO");

	// Allocate the shared buffers for software skinning
	for(S32 i=0; i < mBufferCount; i++)
	{
		mBufferMemory[i] = new LLAGPArray<U8>;
		mBufferMemory[i]->reserve_block(AVATAR_VERTEX_BYTES*AVATAR_BUFFER_ELEMENTS);
	}

	if (gFeatureManagerp->isFeatureAvailable("RenderAGP"))
	{
		setUseAGP(gSavedSettings.getBOOL("RenderUseAGP") && gGLManager.mHasAnyAGP);
	}
	else
	{
		setUseAGP(FALSE);
	}

	stop_glerror();
	
	for(S32 i=0; i < mBufferCount; i++)
	{
		if (!mBufferMemory[i]->isAGP() && usingAGP())
		{
			llwarns << "pipeline buffer memory is non-AGP when AGP available!" << llendl;
		}
	}
	
	setShaders();
}

void LLPipeline::LLScatterShader::init(GLhandleARB shader, int map_stage)
{
	glUseProgramObjectARB(shader);
	glUniform1iARB(glGetUniformLocationARB(shader, "scatterMap"), map_stage);
	glUseProgramObjectARB(0);
}

LLPipeline::~LLPipeline()
{

}

void LLPipeline::cleanup()
{
	for(pool_set_t::iterator iter = mPools.begin();
		iter != mPools.end(); )
	{
		pool_set_t::iterator curiter = iter++;
		LLDrawPool* poolp = *curiter;
		if (poolp->mReferences.empty())
		{
			mPools.erase(curiter);
			removeFromQuickLookup( poolp );
			delete poolp;
		}
	}
	
	if (!mSimplePools.empty())
	{
		llwarns << "Simple Pools not cleaned up" << llendl;
	}
	if (!mTerrainPools.empty())
	{
		llwarns << "Terrain Pools not cleaned up" << llendl;
	}
	if (!mTreePools.empty())
	{
		llwarns << "Tree Pools not cleaned up" << llendl;
	}
	if (!mTreeNewPools.empty())
	{
		llwarns << "TreeNew Pools not cleaned up" << llendl;
	}
	if (!mBumpPools.empty())
	{
		llwarns << "Bump Pools not cleaned up" << llendl;
	}
	delete mAlphaPool;
	mAlphaPool = NULL;
	delete mSkyPool;
	mSkyPool = NULL;
	delete mStarsPool;
	mStarsPool = NULL;
	delete mCloudsPool;
	mCloudsPool = NULL;
	delete mTerrainPool;
	mTerrainPool = NULL;
	delete mWaterPool;
	mWaterPool = NULL;
	delete mGroundPool;
	mGroundPool = NULL;
	delete mHUDPool;
	mHUDPool = NULL;

	mBloomImagep = NULL;
	mBloomImage2p = NULL;
	mFaceSelectImagep = NULL;
	mAlphaSizzleImagep = NULL;

	for(S32 i=0; i < mBufferCount; i++)
	{
		delete mBufferMemory[i];
		mBufferMemory[i] = NULL;
	}

	delete mObjectPartition;
	mObjectPartition = NULL;

	if (mAGPMemPool && mGlobalFence)
	{
		mAGPMemPool->deleteFence(mGlobalFence);
		mGlobalFence = 0;
	}
	delete mAGPMemPool;
	mAGPMemPool = NULL;
}

//============================================================================

BOOL LLPipeline::initAGP()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	
	mAGPMemPool = LLAGPMemPool::createPool(sAGPMaxPoolSize, mUseVBO);

	if (!mAGPMemPool)
	{
		llinfos << "Warning! Couldn't allocate AGP memory!" << llendl;
		llinfos << "Disabling AGP!" << llendl;
		mAGPMemPool = NULL;
		mRenderFeatureMask &= ~RENDER_FEATURE_AGP; // Need to disable the using AGP flag
		return FALSE;
	}
	else if (!mAGPMemPool->getSize())
	{
		llinfos << "Warning!  Unable to allocate AGP memory! Disabling AGP" << llendl;
		delete mAGPMemPool;
		mAGPMemPool = NULL;
		mRenderFeatureMask &= ~RENDER_FEATURE_AGP; // Need to disable the using AGP flag
		return FALSE;
	}
	else
	{
		llinfos << "Allocated " << mAGPMemPool->getSize() << " bytes of AGP memory" << llendl;
		mAGPMemPool->bind();

		if (mAGPMemPool->getSize() < MIN_AGP_SIZE)
		{
			llwarns << "Not enough AGP memory!" << llendl;
			delete mAGPMemPool;
			mAGPMemPool = NULL;
			mRenderFeatureMask &= ~RENDER_FEATURE_AGP; // Need to disable the using AGP flag
			return FALSE;
		}


	    if (mAGPMemPool)
	    {
	        // Create the fence that we use for global synchronization.
	        mGlobalFence = mAGPMemPool->createFence();
	    }
		return TRUE;
	}

}

void LLPipeline::cleanupAGP()
{
	int i;
	for(i=0; i < mBufferCount; i++)
	{
		mBufferMemory[i]->deleteFence(mBufferFence[i]);
		mBufferMemory[i]->setUseAGP(FALSE);
	}
	
	flushAGPMemory();
	if (mAGPMemPool && mGlobalFence)
	{
	    mAGPMemPool->deleteFence(mGlobalFence);
	    mGlobalFence = 0;
	}
	delete mAGPMemPool;
	mAGPMemPool = NULL;
}

BOOL LLPipeline::usingAGP() const
{
	return (mRenderFeatureMask & RENDER_FEATURE_AGP) ? TRUE : FALSE; 
}

void LLPipeline::setUseAGP(const BOOL use_agp)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	
	if (use_agp == usingAGP())
	{
		return;
	}
	else if (use_agp)
	{
		mRenderFeatureMask |= RENDER_FEATURE_AGP;
		initAGP();

		// Forces us to allocate an AGP memory block immediately.
		int i;
		for(i=0; i < mBufferCount; i++)
		{
			mBufferMemory[i]->setUseAGP(use_agp);
			mBufferMemory[i]->realloc(mBufferMemory[i]->getMax());
			mBufferFence[i] = mBufferMemory[i]->createFence();
		}
		
		// Must be done AFTER you initialize AGP
		for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
		{
			LLDrawPool *poolp = *iter;
			poolp->setUseAGP(use_agp);
		}
	}
	else
	{
		unbindAGP();
		mRenderFeatureMask &= ~RENDER_FEATURE_AGP;

		// Must be done BEFORE you blow away AGP
		for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
		{
			LLDrawPool *poolp = *iter;
			poolp->setUseAGP(use_agp);
		}

		int i;
		for(i=0; i < mBufferCount; i++)
		{
			if (mBufferMemory[i])
			{
				mBufferMemory[i]->setUseAGP(use_agp);
				mBufferMemory[i]->deleteFence(mBufferFence[i]);
				mBufferFence[i] = 0;
			}
			else
			{
				llerrs << "setUseAGP without buffer memory" << llendl;
			}
		}
		
		cleanupAGP();
	}

}

//============================================================================

void LLPipeline::destroyGL() 
{
	setUseAGP(FALSE);
	stop_glerror();
	unloadShaders();
	mHighlightFaces.reset();
}

void LLPipeline::restoreGL() 
{
	if (mVertexShadersEnabled)
	{
		setShaders();
	}
	
	if (mObjectPartition)
	{
		mObjectPartition->restoreGL();
	}
}

//============================================================================
// Load Shader

static LLString get_object_log(GLhandleARB ret)
{
	LLString res;
	
	//get log length
	GLint length;
	glGetObjectParameterivARB(ret, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
	if (length > 0)
	{
		//the log could be any size, so allocate appropriately
		GLcharARB* log = new GLcharARB[length];
		glGetInfoLogARB(ret, length, &length, log);
		res = LLString(log);
		delete[] log;
	}
	return res;
}

void LLPipeline::dumpObjectLog(GLhandleARB ret, BOOL warns) 
{
	LLString log = get_object_log(ret);
	if (warns)
	{
		llwarns << log << llendl;
	}
	else
	{
		llinfos << log << llendl;
	}
}

GLhandleARB LLPipeline::loadShader(const LLString& filename, S32 cls, GLenum type)
{
	GLenum error;
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		llwarns << "GL ERROR entering loadShader(): " << error << llendl;
	}
	
	llinfos << "Loading shader file: " << filename << llendl;

	if (filename.empty()) 
	{
		return 0;
	}


	//read in from file
	FILE* file = NULL;

	S32 try_gpu_class = mVertexShaderLevel[cls];
	S32 gpu_class;

	//find the most relevant file
	for (gpu_class = try_gpu_class; gpu_class > 0; gpu_class--)
	{	//search from the current gpu class down to class 1 to find the most relevant shader
		std::stringstream fname;
		fname << gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "shaders/class");
		fname << gpu_class << "/" << filename;
		
		llinfos << "Looking in " << fname.str().c_str() << llendl;
		file = fopen(fname.str().c_str(), "r");
		if (file)
		{
			break; // done
		}
	}
	
	if (file == NULL)
	{
		llinfos << "GLSL Shader file not found: " << filename << llendl;
		return 0;
	}

	//we can't have any lines longer than 1024 characters 
	//or any shaders longer than 1024 lines... deal - DaveP
	GLcharARB buff[1024];
	GLcharARB* text[1024];
	GLuint count = 0;

	//copy file into memory
	while(fgets(buff, 1024, file) != NULL) 
	{
		text[count++] = strdup(buff);
    }
	fclose(file);

	//create shader object
	GLhandleARB ret = glCreateShaderObjectARB(type);
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		llwarns << "GL ERROR in glCreateShaderObjectARB: " << error << llendl;
	}
	else
	{
		//load source
		glShaderSourceARB(ret, count, (const GLcharARB**) text, NULL);
		error = glGetError();
		if (error != GL_NO_ERROR)
		{
			llwarns << "GL ERROR in glShaderSourceARB: " << error << llendl;
		}
		else
		{
			//compile source
			glCompileShaderARB(ret);
			error = glGetError();
			if (error != GL_NO_ERROR)
			{
				llwarns << "GL ERROR in glCompileShaderARB: " << error << llendl;
			}
		}
	}
	//free memory
	for (GLuint i = 0; i < count; i++)
	{
		free(text[i]);
	}
	if (error == GL_NO_ERROR)
	{
		//check for errors
		GLint success = GL_TRUE;
		glGetObjectParameterivARB(ret, GL_OBJECT_COMPILE_STATUS_ARB, &success);
		error = glGetError();
		if (error != GL_NO_ERROR || success == GL_FALSE) 
		{
			//an error occured, print log
			llwarns << "GLSL Compilation Error: (" << error << ") in " << filename << llendl;
			dumpObjectLog(ret);
			ret = 0;
		}
	}
	else
	{
		ret = 0;
	}
	stop_glerror();

	//successfully loaded, save results
#if 1 // 1.9.1
	if (ret)
	{
		mVertexShaderLevel[cls] = try_gpu_class;
	}
	else
	{
		if (mVertexShaderLevel[cls] > 1)
		{
			mVertexShaderLevel[cls] = mVertexShaderLevel[cls] - 1;
			ret = loadShader(filename,cls,type);
			if (ret && mMaxVertexShaderLevel[cls] > mVertexShaderLevel[cls])
			{
				mMaxVertexShaderLevel[cls] = mVertexShaderLevel[cls];
			}
		}
	}
#else
	if (ret)
	{
		S32 max = -1;
		/*if (try_gpu_class == mMaxVertexShaderLevel[cls])
		{
			max = gpu_class;
		}*/
		saveVertexShaderLevel(cls,try_gpu_class,max);
	}
	else
	{
		if (mVertexShaderLevel[cls] > 1)
		{
			mVertexShaderLevel[cls] = mVertexShaderLevel[cls] - 1;
			ret = loadShader(f,cls,type);
			if (ret && mMaxVertexShaderLevel[cls] > mVertexShaderLevel[cls])
			{
				saveVertexShaderLevel(cls, mVertexShaderLevel[cls], mVertexShaderLevel[cls]);
			}
		}
	}
#endif
	return ret;
}

BOOL LLPipeline::linkProgramObject(GLhandleARB obj, BOOL suppress_errors) 
{
	//check for errors
	glLinkProgramARB(obj);
	GLint success = GL_TRUE;
	glGetObjectParameterivARB(obj, GL_OBJECT_LINK_STATUS_ARB, &success);
	if (!suppress_errors && success == GL_FALSE) 
	{
		//an error occured, print log
		llwarns << "GLSL Linker Error:" << llendl;
	}

	LLString log = get_object_log(obj);
	LLString::toLower(log);
	if (log.find("software") != LLString::npos)
	{
		llwarns << "GLSL Linker: Running in Software:" << llendl;
		success = GL_FALSE;
		suppress_errors = FALSE;
	}
	if (!suppress_errors)
	{
        dumpObjectLog(obj, !success);
	}

	return success;
}

BOOL LLPipeline::validateProgramObject(GLhandleARB obj)
{
	//check program validity against current GL
	glValidateProgramARB(obj);
	GLint success = GL_TRUE;
	glGetObjectParameterivARB(obj, GL_OBJECT_VALIDATE_STATUS_ARB, &success);
	if (success == GL_FALSE)
	{
		llwarns << "GLSL program not valid: " << llendl;
		dumpObjectLog(obj);
	}
	else
	{
		dumpObjectLog(obj, FALSE);
	}

	return success;
}

//============================================================================
// Shader Management

void LLPipeline::setShaders()
{
	if (gViewerWindow)
	{
		gViewerWindow->setCursor(UI_CURSOR_WAIT);
	}

	// Lighting
	setLightingDetail(-1);

	// Shaders
	for (S32 i=0; i<SHADER_COUNT; i++)
	{
		mVertexShaderLevel[i] = 0;
		mMaxVertexShaderLevel[i] = 0;
	}
	if (canUseVertexShaders())
	{
		S32 light_class = 2;
		S32 env_class = 2;
		if (getLightingDetail() == 0)
		{
			light_class = 1;
		}
		// Load lighting shaders
		mVertexShaderLevel[SHADER_LIGHTING] = light_class;
		mMaxVertexShaderLevel[SHADER_LIGHTING] = light_class;
		mVertexShaderLevel[SHADER_ENVIRONMENT] = env_class;
		mMaxVertexShaderLevel[SHADER_ENVIRONMENT] = env_class;
		BOOL loaded = loadShadersLighting();
		if (loaded)
		{
			mVertexShadersEnabled = TRUE;
			mVertexShadersLoaded = 1;

			// Load all shaders to set max levels
			loadShadersEnvironment();
		
			// Load max avatar shaders to set the max level
			mVertexShaderLevel[SHADER_AVATAR] = 3;
			mMaxVertexShaderLevel[SHADER_AVATAR] = 3;
			loadShadersAvatar();

			// Load shaders to correct levels
			if (!gSavedSettings.getBOOL("RenderRippleWater"))
			{
				mVertexShaderLevel[SHADER_ENVIRONMENT] = 0;
				loadShadersEnvironment(); // unloads
			}

#if LL_DARWIN // force avatar shaders off for mac
			mVertexShaderLevel[SHADER_AVATAR] = 0;
			mMaxVertexShaderLevel[SHADER_AVATAR] = 0;
#else
			if (gSavedSettings.getBOOL("RenderAvatarVP"))
			{
				S32 avatar = gSavedSettings.getS32("RenderAvatarMode");
				S32 avatar_class = 1 + avatar;
				// Set the actual level
				mVertexShaderLevel[SHADER_AVATAR] = avatar_class;
				loadShadersAvatar();
				if (mVertexShaderLevel[SHADER_AVATAR] != avatar_class)
				{
					if (mVertexShaderLevel[SHADER_AVATAR] == 0)
					{
						gSavedSettings.setBOOL("RenderAvatarVP", FALSE);
					}
					avatar = llmax(mVertexShaderLevel[SHADER_AVATAR]-1,0);
					gSavedSettings.setS32("RenderAvatarMode", avatar);
				}
			}
			else
			{
				mVertexShaderLevel[SHADER_AVATAR] = 0;
				gSavedSettings.setS32("RenderAvatarMode", 0);
				loadShadersAvatar(); // unloads
			}
#endif
		}
		else
		{
			mVertexShadersEnabled = FALSE;
			mVertexShadersLoaded = 0;
		}
	}
	if (gViewerWindow)
	{
		gViewerWindow->setCursor(UI_CURSOR_ARROW);
	}
}

BOOL LLPipeline::canUseVertexShaders()
{
	if (!gGLManager.mHasVertexShader ||
		!gGLManager.mHasFragmentShader ||
		!gFeatureManagerp->isFeatureAvailable("VertexShaderEnable") ||
		mVertexShadersLoaded == -1)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

void LLPipeline::unloadShaders()
{
	mObjectSimpleProgram.unload();
	mObjectBumpProgram.unload();
	mObjectAlphaProgram.unload();
	mWaterProgram.unload();
	mTerrainProgram.unload();
	mGroundProgram.unload();
	mAvatarProgram.unload();
	mAvatarEyeballProgram.unload();
	mAvatarPickProgram.unload();
	mHighlightProgram.unload();

	mVertexShaderLevel[SHADER_LIGHTING] = 0;
	mVertexShaderLevel[SHADER_OBJECT] = 0;
	mVertexShaderLevel[SHADER_AVATAR] = 0;
	mVertexShaderLevel[SHADER_ENVIRONMENT] = 0;
	mVertexShaderLevel[SHADER_INTERFACE] = 0;

	mLightVertex = mLightFragment = mScatterVertex = mScatterFragment = 0;
	mVertexShadersLoaded = 0;
}

#if 0 // 1.9.2
// Any time shader options change
BOOL LLPipeline::loadShaders()
{
	unloadShaders();

	if (!canUseVertexShaders())
	{
		return FALSE;
	}

	S32 light_class = mMaxVertexShaderLevel[SHADER_LIGHTING];
	if (getLightingDetail() == 0)
	{
		light_class = 1; // Use minimum lighting shader
	}
	else if (getLightingDetail() == 1)
	{
		light_class = 2; // Use medium lighting shader
	}
	mVertexShaderLevel[SHADER_LIGHTING] = light_class;
	mVertexShaderLevel[SHADER_OBJECT] = llmin(mMaxVertexShaderLevel[SHADER_OBJECT], gSavedSettings.getS32("VertexShaderLevelObject"));
	mVertexShaderLevel[SHADER_AVATAR] = llmin(mMaxVertexShaderLevel[SHADER_AVATAR], gSavedSettings.getS32("VertexShaderLevelAvatar"));
	mVertexShaderLevel[SHADER_ENVIRONMENT] = llmin(mMaxVertexShaderLevel[SHADER_ENVIRONMENT], gSavedSettings.getS32("VertexShaderLevelEnvironment"));
	mVertexShaderLevel[SHADER_INTERFACE] = mMaxVertexShaderLevel[SHADER_INTERFACE];
	
	BOOL loaded = loadShadersLighting();
	if (loaded)
	{
		loadShadersEnvironment(); // Must load this before object/avatar for scatter
		loadShadersObject();
		loadShadersAvatar();
		loadShadersInterface();
		mVertexShadersLoaded = 1;
	}
	else
	{
		unloadShaders();
		mVertexShadersEnabled = FALSE;
		mVertexShadersLoaded = 0; //-1; // -1 = failed
		setLightingDetail(-1);
	}
	
	return loaded;
}
#endif

BOOL LLPipeline::loadShadersLighting()
{
	// Load light dependency shaders first
	// All of these have to load for any shaders to function
	
    std::string lightvertex = "lighting/lightV.glsl";
	//get default light function implementation
	mLightVertex = loadShader(lightvertex, SHADER_LIGHTING, GL_VERTEX_SHADER_ARB);
	if( !mLightVertex )
	{
		llwarns << "Failed to load " << lightvertex << llendl;
		return FALSE;
	}
	
	std::string lightfragment = "lighting/lightF.glsl";
	mLightFragment = loadShader(lightfragment, SHADER_LIGHTING, GL_FRAGMENT_SHADER_ARB);
	if ( !mLightFragment )
	{
		llwarns << "Failed to load " << lightfragment << llendl;
		return FALSE;
	}

	// NOTE: Scatter shaders use the ENVIRONMENT detail level
	
	std::string scattervertex = "environment/scatterV.glsl";
	mScatterVertex = loadShader(scattervertex, SHADER_ENVIRONMENT, GL_VERTEX_SHADER_ARB);
	if ( !mScatterVertex )
	{
		llwarns << "Failed to load " << scattervertex << llendl;
		return FALSE;
	}

	std::string scatterfragment = "environment/scatterF.glsl";
	mScatterFragment = loadShader(scatterfragment, SHADER_ENVIRONMENT, GL_FRAGMENT_SHADER_ARB);
	if ( !mScatterFragment )
	{
		llwarns << "Failed to load " << scatterfragment << llendl;
		return FALSE;
	}
	
	return TRUE;
}

BOOL LLPipeline::loadShadersEnvironment()
{
	GLhandleARB baseObjects[] = 
	{
		mLightFragment,
		mLightVertex,
		mScatterFragment,
		mScatterVertex
	};
	S32 baseCount = 4;

	BOOL success = TRUE;

	if (mVertexShaderLevel[SHADER_ENVIRONMENT] == 0)
	{
		mWaterProgram.unload();
		mGroundProgram.unload();
		mTerrainProgram.unload();
		return FALSE;
	}
	
	if (success)
	{
		//load water vertex shader
		std::string waterfragment = "environment/waterF.glsl";
		std::string watervertex = "environment/waterV.glsl";
		mWaterProgram.mProgramObject = glCreateProgramObjectARB();
		mWaterProgram.attachObjects(baseObjects, baseCount);
		mWaterProgram.attachObject(loadShader(watervertex, SHADER_ENVIRONMENT, GL_VERTEX_SHADER_ARB));
		mWaterProgram.attachObject(loadShader(waterfragment, SHADER_ENVIRONMENT, GL_FRAGMENT_SHADER_ARB));

		success = mWaterProgram.mapAttributes();	
		if (success)
		{
			success = mWaterProgram.mapUniforms(sWaterUniforms, sWaterUniformCount);
		}
		if (!success)
		{
			llwarns << "Failed to load " << watervertex << llendl;
		}
	}
	if (success)
	{
		//load ground vertex shader
		std::string groundvertex = "environment/groundV.glsl";
		std::string groundfragment = "environment/groundF.glsl";
		mGroundProgram.mProgramObject = glCreateProgramObjectARB();
		mGroundProgram.attachObjects(baseObjects, baseCount);
		mGroundProgram.attachObject(loadShader(groundvertex, SHADER_ENVIRONMENT, GL_VERTEX_SHADER_ARB));
		mGroundProgram.attachObject(loadShader(groundfragment, SHADER_ENVIRONMENT, GL_FRAGMENT_SHADER_ARB));
	
		success = mGroundProgram.mapAttributes();
		if (success)
		{
			success = mGroundProgram.mapUniforms();
		}
		if (!success)
		{
			llwarns << "Failed to load " << groundvertex << llendl;
		}
	}

	if (success)
	{
		//load terrain vertex shader
		std::string terrainvertex = "environment/terrainV.glsl";
		std::string terrainfragment = "environment/terrainF.glsl";
		mTerrainProgram.mProgramObject = glCreateProgramObjectARB();
		mTerrainProgram.attachObjects(baseObjects, baseCount);
		mTerrainProgram.attachObject(loadShader(terrainvertex, SHADER_ENVIRONMENT, GL_VERTEX_SHADER_ARB));
		mTerrainProgram.attachObject(loadShader(terrainfragment, SHADER_ENVIRONMENT, GL_FRAGMENT_SHADER_ARB));
		success = mTerrainProgram.mapAttributes();
		if (success)
		{
			success = mTerrainProgram.mapUniforms(sTerrainUniforms, sTerrainUniformCount);
		}
		if (!success)
		{
			llwarns << "Failed to load " << terrainvertex << llendl;
		}
	}

	if( !success )
	{
		mVertexShaderLevel[SHADER_ENVIRONMENT] = 0;
		mMaxVertexShaderLevel[SHADER_ENVIRONMENT] = 0;
		return FALSE;
	}
	
	if (gWorldPointer)
	{
		gWorldPointer->updateWaterObjects();
	}
	
	return TRUE;
}

BOOL LLPipeline::loadShadersObject()
{
	GLhandleARB baseObjects[] = 
	{
		mLightFragment,
		mLightVertex,
		mScatterFragment,
		mScatterVertex
	};
	S32 baseCount = 4;

	BOOL success = TRUE;

	if (mVertexShaderLevel[SHADER_OBJECT] == 0)
	{
		mObjectSimpleProgram.unload();
		mObjectBumpProgram.unload();
		mObjectAlphaProgram.unload();
		return FALSE;
	}
	
	if (success)
	{
		//load object (volume/tree) vertex shader
		std::string simplevertex = "objects/simpleV.glsl";
		std::string simplefragment = "objects/simpleF.glsl";
		mObjectSimpleProgram.mProgramObject = glCreateProgramObjectARB();
		mObjectSimpleProgram.attachObjects(baseObjects, baseCount);
		mObjectSimpleProgram.attachObject(loadShader(simplevertex, SHADER_OBJECT, GL_VERTEX_SHADER_ARB));
		mObjectSimpleProgram.attachObject(loadShader(simplefragment, SHADER_OBJECT, GL_FRAGMENT_SHADER_ARB));
		success = mObjectSimpleProgram.mapAttributes();
		if (success)
		{
			success = mObjectSimpleProgram.mapUniforms();
		}
		if( !success )
		{
			llwarns << "Failed to load " << simplevertex << llendl;
		}
	}
	
	if (success)
	{
		//load object bumpy vertex shader
		std::string bumpshinyvertex = "objects/bumpshinyV.glsl";
		std::string bumpshinyfragment = "objects/bumpshinyF.glsl";
		mObjectBumpProgram.mProgramObject = glCreateProgramObjectARB();
		mObjectBumpProgram.attachObjects(baseObjects, baseCount);
		mObjectBumpProgram.attachObject(loadShader(bumpshinyvertex, SHADER_OBJECT, GL_VERTEX_SHADER_ARB));
		mObjectBumpProgram.attachObject(loadShader(bumpshinyfragment, SHADER_OBJECT, GL_FRAGMENT_SHADER_ARB));
		success = mObjectBumpProgram.mapAttributes();
		if (success)
		{
			success = mObjectBumpProgram.mapUniforms();
		}
		if( !success )
		{
			llwarns << "Failed to load " << bumpshinyvertex << llendl;
		}
	}

	if (success)
	{
		//load object alpha vertex shader
		std::string alphavertex = "objects/alphaV.glsl";
		std::string alphafragment = "objects/alphaF.glsl";
		mObjectAlphaProgram.mProgramObject = glCreateProgramObjectARB();
		mObjectAlphaProgram.attachObjects(baseObjects, baseCount);
		mObjectAlphaProgram.attachObject(loadShader(alphavertex, SHADER_OBJECT, GL_VERTEX_SHADER_ARB));
		mObjectAlphaProgram.attachObject(loadShader(alphafragment, SHADER_OBJECT, GL_FRAGMENT_SHADER_ARB));

		success = mObjectAlphaProgram.mapAttributes();
		if (success)
		{
			success = mObjectAlphaProgram.mapUniforms();
		}
		if( !success )
		{
			llwarns << "Failed to load " << alphavertex << llendl;
		}
	}

	if( !success )
	{
		mVertexShaderLevel[SHADER_OBJECT] = 0;
		mMaxVertexShaderLevel[SHADER_OBJECT] = 0;
		return FALSE;
	}
	
	return TRUE;
}

BOOL LLPipeline::loadShadersAvatar()
{
	GLhandleARB baseObjects[] = 
	{
		mLightFragment,
		mLightVertex,
		mScatterFragment,
		mScatterVertex
	};
	S32 baseCount = 4;
	
	BOOL success = TRUE;

	if (mVertexShaderLevel[SHADER_AVATAR] == 0)
	{
		mAvatarProgram.unload();
		mAvatarEyeballProgram.unload();
		mAvatarPickProgram.unload();
		return FALSE;
	}
	
	if (success)
	{
		//load specular (eyeball) vertex program
		std::string eyeballvertex = "avatar/eyeballV.glsl";
		std::string eyeballfragment = "avatar/eyeballF.glsl";
		mAvatarEyeballProgram.mProgramObject = glCreateProgramObjectARB();
		mAvatarEyeballProgram.attachObjects(baseObjects, baseCount);
		mAvatarEyeballProgram.attachObject(loadShader(eyeballvertex, SHADER_AVATAR, GL_VERTEX_SHADER_ARB));
		mAvatarEyeballProgram.attachObject(loadShader(eyeballfragment, SHADER_AVATAR, GL_FRAGMENT_SHADER_ARB));
		success = mAvatarEyeballProgram.mapAttributes();
		if (success)
		{
			success = mAvatarEyeballProgram.mapUniforms();
		}
		if( !success )
		{
			llwarns << "Failed to load " << eyeballvertex << llendl;
		}
	}

	if (success)
	{
		mAvatarSkinVertex = loadShader("avatar/avatarSkinV.glsl", SHADER_AVATAR, GL_VERTEX_SHADER_ARB);
		//load avatar vertex shader
		std::string avatarvertex = "avatar/avatarV.glsl";
		std::string avatarfragment = "avatar/avatarF.glsl";
		
		mAvatarProgram.mProgramObject = glCreateProgramObjectARB();
		mAvatarProgram.attachObjects(baseObjects, baseCount);
		mAvatarProgram.attachObject(mAvatarSkinVertex);
		mAvatarProgram.attachObject(loadShader(avatarvertex, SHADER_AVATAR, GL_VERTEX_SHADER_ARB));
		mAvatarProgram.attachObject(loadShader(avatarfragment, SHADER_AVATAR, GL_FRAGMENT_SHADER_ARB));
		
		success = mAvatarProgram.mapAttributes(sAvatarAttribs, sAvatarAttribCount);
		if (success)
		{
			success = mAvatarProgram.mapUniforms(sAvatarUniforms, sAvatarUniformCount);
		}
		if( !success )
		{
			llwarns << "Failed to load " << avatarvertex << llendl;
		}
	}

	if (success)
	{
		//load avatar picking shader
		std::string pickvertex = "avatar/pickAvatarV.glsl";
		std::string pickfragment = "avatar/pickAvatarF.glsl";
		mAvatarPickProgram.mProgramObject = glCreateProgramObjectARB();
		mAvatarPickProgram.attachObject(loadShader(pickvertex, SHADER_AVATAR, GL_VERTEX_SHADER_ARB));
		mAvatarPickProgram.attachObject(loadShader(pickfragment, SHADER_AVATAR, GL_FRAGMENT_SHADER_ARB));
		mAvatarPickProgram.attachObject(mAvatarSkinVertex);

		success = mAvatarPickProgram.mapAttributes(sAvatarAttribs, sAvatarAttribCount);
		if (success)
		{
			success = mAvatarPickProgram.mapUniforms(sAvatarUniforms, sAvatarUniformCount);
		}
		if( !success )
		{
			llwarns << "Failed to load " << pickvertex << llendl;
		}
	}

	if( !success )
	{
		mVertexShaderLevel[SHADER_AVATAR] = 0;
		mMaxVertexShaderLevel[SHADER_AVATAR] = 0;
		return FALSE;
	}
	
	return TRUE;
}

BOOL LLPipeline::loadShadersInterface()
{
	BOOL success = TRUE;

	if (mVertexShaderLevel[SHADER_INTERFACE] == 0)
	{
		mHighlightProgram.unload();
		return FALSE;
	}
	
	if (success)
	{
		//load highlighting shader
		std::string highlightvertex = "interface/highlightV.glsl";
		std::string highlightfragment = "interface/highlightF.glsl";
		mHighlightProgram.mProgramObject = glCreateProgramObjectARB();
		mHighlightProgram.attachObject(loadShader(highlightvertex, SHADER_INTERFACE, GL_VERTEX_SHADER_ARB));
		mHighlightProgram.attachObject(loadShader(highlightfragment, SHADER_INTERFACE, GL_FRAGMENT_SHADER_ARB));
	
		success = mHighlightProgram.mapAttributes();
		if (success)
		{
			success = mHighlightProgram.mapUniforms();
		}
		if( !success )
		{
			llwarns << "Failed to load " << highlightvertex << llendl;
		}
	}

	if( !success )
	{
		mVertexShaderLevel[SHADER_INTERFACE] = 0;
		mMaxVertexShaderLevel[SHADER_INTERFACE] = 0;
		return FALSE;
	}
	
	return TRUE;
}

//============================================================================

void LLPipeline::enableShadows(const BOOL enable_shadows)
{
	//should probably do something here to wrangle shadows....	
}

S32 LLPipeline::getMaxLightingDetail() const
{
	if (mVertexShaderLevel[SHADER_OBJECT] >= LLDrawPoolSimple::SHADER_LEVEL_LOCAL_LIGHTS)
	{
		return 3;
	}
	else
	{
		return 1;
	}
}

S32 LLPipeline::setLightingDetail(S32 level)
{
	if (level < 0)
	{
		level = gSavedSettings.getS32("RenderLightingDetail");
	}
	level = llclamp(level, 0, getMaxLightingDetail());
	if (level != mLightingDetail)
	{
		gSavedSettings.setS32("RenderLightingDetail", level);
		if (level >= 2)
		{
			gObjectList.relightAllObjects();
		}
		mLightingDetail = level;

		if (mVertexShadersLoaded == 1)
		{
			gPipeline.setShaders();
		}
	}
	return mLightingDetail;
}

LLAGPMemBlock *LLPipeline::allocAGPFromPool(const S32 bytes, const U32 target)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	
	if (!mAGPMemPool)
	{
		llwarns << "Attempting to allocate AGP memory when AGP disabled!" << llendl;
		return NULL;
	}
	else
	{
		if (mUseVBO)
		{
			return ((LLAGPMemPoolARB*) mAGPMemPool)->allocBlock(bytes, target);
		}
		else
		{
			return mAGPMemPool->allocBlock(bytes);
		}
	}
}


void LLPipeline::unbindAGP()
{
	if (mAGPMemPool && mAGPBound)
	{
		mAGPMemPool->disable();
		mAGPBound = FALSE;
	}
}

void LLPipeline::bindAGP()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	
	if (mAGPMemPool && !mAGPBound && usingAGP())
	{
		mAGPMemPool->enable();
		mAGPBound = TRUE;
	}
}

U8* LLPipeline::bufferGetScratchMemory(void)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	return(mBufferMemory[mBufferIndex]->getScratchMemory());
}

void LLPipeline::bufferWaitFence(void)
{
	mBufferMemory[mBufferIndex]->waitFence(mBufferFence[mBufferIndex]);
}

void LLPipeline::bufferSendFence(void)
{
	mBufferMemory[mBufferIndex]->sendFence(mBufferFence[mBufferIndex]);
}

void LLPipeline::bufferRotate(void)
{
	mBufferIndex++;
	if(mBufferIndex >= mBufferCount)
		mBufferIndex = 0;
}

// Called when a texture changes # of channels (rare, may cause faces to move to alpha pool)
void LLPipeline::dirtyPoolObjectTextures(const LLViewerImage *texturep)
{
	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		poolp->dirtyTexture(texturep);
	}
}

LLDrawPool *LLPipeline::findPool(const U32 type, LLViewerImage *tex0)
{
	LLDrawPool *poolp = NULL;
	switch( type )
	{
	case LLDrawPool::POOL_SIMPLE:
		poolp = get_if_there(mSimplePools, (uintptr_t)tex0, (LLDrawPool*)0 );
		break;

	case LLDrawPool::POOL_TREE:
		poolp = get_if_there(mTreePools, (uintptr_t)tex0, (LLDrawPool*)0 );
		break;

	case LLDrawPool::POOL_TREE_NEW:
		poolp = get_if_there(mTreeNewPools, (uintptr_t)tex0, (LLDrawPool*)0 );
		break;

	case LLDrawPool::POOL_TERRAIN:
		poolp = get_if_there(mTerrainPools, (uintptr_t)tex0, (LLDrawPool*)0 );
		break;

	case LLDrawPool::POOL_BUMP:
		poolp = get_if_there(mBumpPools, (uintptr_t)tex0, (LLDrawPool*)0 );
		break;

	case LLDrawPool::POOL_MEDIA:
		poolp = get_if_there(mMediaPools, (uintptr_t)tex0, (LLDrawPool*)0 );
		break;

	case LLDrawPool::POOL_ALPHA:
		poolp = mAlphaPool;
		break;

	case LLDrawPool::POOL_AVATAR:
		break; // Do nothing

	case LLDrawPool::POOL_SKY:
		poolp = mSkyPool;
		break;

	case LLDrawPool::POOL_STARS:
		poolp = mStarsPool;
		break;

	case LLDrawPool::POOL_CLOUDS:
		poolp = mCloudsPool;
		break;

	case LLDrawPool::POOL_WATER:
		poolp = mWaterPool;
		break;

	case LLDrawPool::POOL_GROUND:
		poolp = mGroundPool;
		break;

	case LLDrawPool::POOL_HUD:
		poolp = mHUDPool;
		break;

	default:
		llassert(0);
		llerrs << "Invalid Pool Type in  LLPipeline::findPool() type=" << type << llendl;
		break;
	}

	return poolp;
}


LLDrawPool *LLPipeline::getPool(const U32 type,	LLViewerImage *tex0)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	LLDrawPool *poolp = findPool(type, tex0);
	if (poolp)
	{
		return poolp;
	}

	LLDrawPool *new_poolp = LLDrawPool::createPool(type, tex0);
	addPool( new_poolp );

	return new_poolp;
}


// static
LLDrawPool* LLPipeline::getPoolFromTE(const LLTextureEntry* te, LLViewerImage* imagep)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	bool alpha = te->getColor().mV[3] < 0.999f;
	if (imagep)
	{
		alpha = alpha || (imagep->getComponents() == 4) || (imagep->getComponents() == 2);
	}

	if (te->getMediaFlags() == LLTextureEntry::MF_WEB_PAGE)
	{
		return gPipeline.getPool(LLDrawPool::POOL_MEDIA, imagep);
	}
	else if (alpha)
	{
		return gPipeline.getPool(LLDrawPool::POOL_ALPHA);
	}
	else if ((te->getBumpmap() || te->getShiny()))
	{
		return gPipeline.getPool(LLDrawPool::POOL_BUMP, imagep);
	}
	else
	{
		return gPipeline.getPool(LLDrawPool::POOL_SIMPLE, imagep);
	}
}


void LLPipeline::addPool(LLDrawPool *new_poolp)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	mPools.insert(new_poolp);
	addToQuickLookup( new_poolp );
}

void LLPipeline::allocDrawable(LLViewerObject *vobj)
{
	LLMemType mt(LLMemType::MTYPE_DRAWABLE);
	LLDrawable *drawable = new LLDrawable();
	vobj->mDrawable = drawable;
	
	drawable->mVObjp     = vobj;
	
	//encompass completely sheared objects by taking 
	//the most extreme point possible (<1,1,0.5>)
	drawable->setRadius(LLVector3(1,1,0.5f).scaleVec(vobj->getScale()).magVec());
	if (vobj->isOrphaned())
	{
		drawable->setState(LLDrawable::FORCE_INVISIBLE);
	}
	drawable->updateXform(TRUE);
}


void LLPipeline::unlinkDrawable(LLDrawable *drawablep)
{
	LLFastTimer t(LLFastTimer::FTM_PIPELINE);
	
	// Based on flags, remove the drawable from the queues that it's on.
	if (drawablep->isState(LLDrawable::ON_MOVE_LIST))
	{
		mMovedList.erase(drawablep);
	}

	if (drawablep->getSpatialGroup())
	{
		if (!drawablep->getSpatialGroup()->mSpatialPartition->remove(drawablep, drawablep->getSpatialGroup()))
		{
#ifdef LL_RELEASE_FOR_DOWNLOAD
			llwarns << "Couldn't remove object from spatial group!" << llendl;
#else
			llerrs << "Couldn't remove object from spatial group!" << llendl;
#endif
		}
	}

	mLights.erase(drawablep);
}

U32 LLPipeline::addObject(LLViewerObject *vobj)
{
	LLMemType mt(LLMemType::MTYPE_DRAWABLE);
	if (gNoRender)
	{
		return 0;
	}

	LLDrawable *drawablep = vobj->createDrawable(this);

	llassert(drawablep);

	//mCompleteSet.put(drawable);
	//gResyncObjects = TRUE;

	if (vobj->getParent())
	{
		vobj->setDrawableParent(((LLViewerObject*)vobj->getParent())->mDrawable); // LLPipeline::addObject 1
	}
	else
	{
		vobj->setDrawableParent(NULL); // LLPipeline::addObject 2
	}

	
	if ((!drawablep->getVOVolume()) &&
		(vobj->getPCode() != LLViewerObject::LL_VO_SKY) &&
		(vobj->getPCode() != LLViewerObject::LL_VO_STARS) &&
		(vobj->getPCode() != LLViewerObject::LL_VO_GROUND))
	{
		drawablep->getSpatialPartition()->put(drawablep);
		if (!drawablep->getSpatialGroup())
		{
#ifdef LL_RELEASE_FOR_DOWNLOAD
			llwarns << "Failure adding drawable to object partition!" << llendl;
#else
			llerrs << "Failure adding drawable to object partition!" << llendl;
#endif
		}
	}
	else
	{
		markMoved(drawablep);
	}
	
	markMaterialed(drawablep);
	markRebuild(drawablep, LLDrawable::REBUILD_ALL, TRUE);

	return 1;
}


void LLPipeline::resetFrameStats()
{
	sCompiles        = 0;
	mVerticesRelit   = 0;
	mLightingChanges = 0;
	mGeometryChanges = 0;
	mNumVisibleFaces = 0;
}

//external functions for asynchronous updating
void LLPipeline::updateMoveDampedAsync(LLDrawable* drawablep)
{
	if (gSavedSettings.getBOOL("FreezeTime"))
	{
		return;
	}
	if (!drawablep)
	{
		llerrs << "updateMove called with NULL drawablep" << llendl;
	}
	if (drawablep->isState(LLDrawable::EARLY_MOVE))
	{
		return;
	}
	// update drawable now
	drawablep->clearState(LLDrawable::MOVE_UNDAMPED); // force to DAMPED
	drawablep->updateMove(); // returns done
	drawablep->setState(LLDrawable::EARLY_MOVE); // flag says we already did an undamped move this frame
	// Put on move list so that EARLY_MOVE gets cleared
	if (!drawablep->isState(LLDrawable::ON_MOVE_LIST))
	{
		mMovedList.insert(drawablep);
		drawablep->setState(LLDrawable::ON_MOVE_LIST);
	}
}

void LLPipeline::updateMoveNormalAsync(LLDrawable* drawablep)
{
	if (gSavedSettings.getBOOL("FreezeTime"))
	{
		return;
	}
	if (!drawablep)
	{
		llerrs << "updateMove called with NULL drawablep" << llendl;
	}
	if (drawablep->isState(LLDrawable::EARLY_MOVE))
	{
		return;
	}
	// update drawable now
	drawablep->setState(LLDrawable::MOVE_UNDAMPED); // force to UNDAMPED
	drawablep->updateMove();
	drawablep->setState(LLDrawable::EARLY_MOVE); // flag says we already did an undamped move this frame
	// Put on move list so that EARLY_MOVE gets cleared
	if (!drawablep->isState(LLDrawable::ON_MOVE_LIST))
	{
		mMovedList.insert(drawablep);
		drawablep->setState(LLDrawable::ON_MOVE_LIST);
	}
}

void LLPipeline::updateMove()
{
	mObjectPartition->mOctree->validate();
	LLFastTimer t(LLFastTimer::FTM_UPDATE_MOVE);
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	if (gSavedSettings.getBOOL("FreezeTime"))
	{
		return;
	}

	mMoveChangesStat.addValue((F32)mMovedList.size());

	for (LLDrawable::drawable_set_t::iterator iter = mMovedList.begin();
		 iter != mMovedList.end(); )
	{
		LLDrawable::drawable_set_t::iterator curiter = iter++;
		LLDrawable *drawablep = *curiter;
		BOOL done = TRUE;
		if (!drawablep->isDead() && (!drawablep->isState(LLDrawable::EARLY_MOVE)))
		{
			done = drawablep->updateMove();
		}
		drawablep->clearState(LLDrawable::EARLY_MOVE | LLDrawable::MOVE_UNDAMPED);
		if (done)
		{
			mMovedList.erase(curiter);
			drawablep->clearState(LLDrawable::ON_MOVE_LIST);
		}
	}

	for (LLDrawable::drawable_set_t::iterator iter = mActiveQ.begin();
		 iter != mActiveQ.end(); )
	{
		LLDrawable::drawable_set_t::iterator curiter = iter++;
		LLDrawable* drawablep = *curiter;
		if (drawablep && !drawablep->isDead()) 
		{
			if (drawablep->mQuietCount++ > MAX_ACTIVE_OBJECT_QUIET_FRAMES && 
				(!drawablep->getParent() || !drawablep->getParent()->isActive()))
			{
				drawablep->makeStatic(); // removes drawable and its children from mActiveQ
				iter = mActiveQ.upper_bound(drawablep); // next valid entry
			}
		}
		else
		{
			mActiveQ.erase(curiter);
		}
	}

	for (LLDrawable::drawable_set_t::iterator iter = mRetexturedList.begin();
		 iter != mRetexturedList.end(); ++iter)
	{
		LLDrawable* drawablep = *iter;
		if (drawablep && !drawablep->isDead())
		{
			drawablep->updateTexture();
		}
	}
	mRetexturedList.clear();

	for (LLDrawable::drawable_set_t::iterator iter = mRematerialedList.begin();
		 iter != mRematerialedList.end(); ++iter)
	{
		LLDrawable* drawablep = *iter;
		if (drawablep && !drawablep->isDead())
		{
			drawablep->updateMaterial();
		}
	}
	mRematerialedList.clear();

	if (mObjectPartition->mOctree)
	{
		//balance octree
 		LLFastTimer ot(LLFastTimer::FTM_OCTREE_BALANCE);
		mObjectPartition->mOctree->validate();
		mObjectPartition->mOctree->balance();
		mObjectPartition->mOctree->validate();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Culling and occlusion testing
/////////////////////////////////////////////////////////////////////////////

void LLPipeline::updateCull()
{
	LLFastTimer t(LLFastTimer::FTM_CULL);
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	LLDrawable::incrementVisible();
	mVisibleList.resize(0);
	mVisibleList.reserve(ESTIMATED_VISIBLE_OBJECT_COUNT);
	
	gTrivialAccepts = 0;

	if (mObjectPartition) 
	{
		if (gSavedSettings.getBOOL("UseOcclusion") && gGLManager.mHasOcclusionQuery)
		{
			mObjectPartition->processOcclusion(gCamera);
			stop_glerror();
		}
		mObjectPartition->cull(*gCamera);
	}
	
	// Hack for avatars - warning - this is really FRAGILE! - djs 05/06/02
	LLVOAvatar::updateAllAvatarVisiblity();

	// If there are any other hacks here, make sure to add them to the
	// standard pick code.

	gMinObjectDistance = llclamp(gMinObjectDistance, MIN_NEAR_PLANE, MAX_NEAR_PLANE);

	F32 water_height = gAgent.getRegion()->getWaterHeight();
	F32 camera_height = gAgent.getCameraPositionAgent().mV[2];
	if (fabs(camera_height - water_height) < 2.f)
	{
		gMinObjectDistance = MIN_NEAR_PLANE;
	}

	gCamera->setNear(gMinObjectDistance);

	// Disable near clip stuff for now...

	// now push it back out to max value
	gMinObjectDistance = MIN_NEAR_PLANE;

	if (gSky.mVOSkyp.notNull() && gSky.mVOSkyp->mDrawable.notNull())
	{
		// Hack for sky - always visible.
		gSky.mVOSkyp->mDrawable->setVisible(*gCamera);
		mVisibleList.push_back(gSky.mVOSkyp->mDrawable);
		gSky.updateCull();
		stop_glerror();
	}
	else
	{
		llinfos << "No sky drawable!" << llendl;
	}

	if (gSky.mVOGroundp.notNull() && gSky.mVOGroundp->mDrawable.notNull())
	{
		gSky.mVOGroundp->mDrawable->setVisible(*gCamera);
		mVisibleList.push_back(gSky.mVOGroundp->mDrawable);
	}

	// add all HUD attachments
	LLVOAvatar* my_avatarp = gAgent.getAvatarObject();
	if (my_avatarp && my_avatarp->hasHUDAttachment())
	{
		for (LLViewerJointAttachment* attachmentp = my_avatarp->mAttachmentPoints.getFirstData();
			attachmentp;
			attachmentp = my_avatarp->mAttachmentPoints.getNextData())
		{
			if (attachmentp->getIsHUDAttachment() && attachmentp->getObject(0))
			{
				LLViewerObject* objectp = attachmentp->getObject(0);
				markVisible(objectp->mDrawable);
				objectp->mDrawable->updateDistance(*gCamera);
				for (S32 i = 0; i < (S32)objectp->mChildList.size(); i++)
				{
					LLViewerObject* childp = objectp->mChildList[i];
					if (childp->mDrawable.notNull())
					{
						markVisible(childp->mDrawable);
						childp->mDrawable->updateDistance(*gCamera);
					}
				}
			}
		}
	}
}

void LLPipeline::markNotCulled(LLDrawable* drawablep, LLCamera& camera)
{
	if (drawablep->isVisible())
	{
		return;
	}
	
	// Tricky render mode to hide selected objects, but we definitely
	// don't want to do any unnecessary pointer dereferences.  JC
	if (gHideSelectedObjects)
	{
		if (drawablep->getVObj() && drawablep->getVObj()->isSelected())
		{
			return;
		}
	}

	if (drawablep && (hasRenderType(drawablep->mRenderType)))
	{
		if (!drawablep->isState(LLDrawable::INVISIBLE|LLDrawable::FORCE_INVISIBLE))
		{
			mVisibleList.push_back(drawablep);
			drawablep->setVisible(camera, NULL, FALSE);
		}
		else if (drawablep->isState(LLDrawable::CLEAR_INVISIBLE))
		{
			// clear invisible flag here to avoid single frame glitch
			drawablep->clearState(LLDrawable::FORCE_INVISIBLE|LLDrawable::CLEAR_INVISIBLE);
		}
	}
}

void LLPipeline::doOcclusion()
{
	if (gSavedSettings.getBOOL("UseOcclusion") && gGLManager.mHasOcclusionQuery)
	{
		mObjectPartition->doOcclusion(gCamera);
	}
}
	

BOOL LLPipeline::updateDrawableGeom(LLDrawable* drawablep, BOOL priority)
{
	BOOL update_complete = drawablep->updateGeometry(priority);
	if (update_complete)
	{
		drawablep->setState(LLDrawable::BUILT);
		mGeometryChanges++;
	}
	return update_complete;
}

void LLPipeline::updateGeom(F32 max_dtime)
{
	LLTimer update_timer;
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	LLPointer<LLDrawable> drawablep;

	LLFastTimer t(LLFastTimer::FTM_GEO_UPDATE);

	// notify various object types to reset internal cost metrics, etc.
	// for now, only LLVOVolume does this to throttle LOD changes
	LLVOVolume::preUpdateGeom();

	// Iterate through all drawables on the priority build queue,
	for (LLDrawable::drawable_set_t::iterator iter = mBuildQ1.begin();
		 iter != mBuildQ1.end();)
	{
		LLDrawable::drawable_set_t::iterator curiter = iter++;
		LLDrawable* drawablep = *curiter;
		BOOL update_complete = TRUE;
		if (drawablep && !drawablep->isDead())
		{
			update_complete = updateDrawableGeom(drawablep, TRUE);
		}
		if (update_complete)
		{
			drawablep->clearState(LLDrawable::IN_REBUILD_Q1);
			mBuildQ1.erase(curiter);
		}
	}
		
	// Iterate through some drawables on the non-priority build queue
	S32 min_count = 16;
	if (mBuildQ2.size() > 1000)
	{
		min_count = mBuildQ2.size();
	}
	else
	{
		mBuildQ2.sort(LLDrawable::CompareDistanceGreaterVisibleFirst());
	}
	
	S32 count = 0;
	
	max_dtime = llmax(update_timer.getElapsedTimeF32()+0.001f, max_dtime);
		
	for (LLDrawable::drawable_list_t::iterator iter = mBuildQ2.begin();
		 iter != mBuildQ2.end(); )
	{
		LLDrawable::drawable_list_t::iterator curiter = iter++;
		LLDrawable* drawablep = *curiter;
		BOOL update_complete = TRUE;
		if (drawablep && !drawablep->isDead())
		{
			update_complete = updateDrawableGeom(drawablep, FALSE);
			count++;
		}
		if (update_complete)
		{
			drawablep->clearState(LLDrawable::IN_REBUILD_Q2);
			mBuildQ2.erase(curiter);
		}
		if ((update_timer.getElapsedTimeF32() >= max_dtime) && count > min_count)
		{
			break;
		}
	}
}

void LLPipeline::markVisible(LLDrawable *drawablep)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	if(!drawablep || drawablep->isDead())
	{
		llwarns << "LLPipeline::markVisible called with NULL drawablep" << llendl;
		return;
	}
	if (!drawablep->isVisible())
	{
		drawablep->setVisible(*gCamera);
		mVisibleList.push_back(drawablep);
	}
}

void LLPipeline::markMoved(LLDrawable *drawablep, BOOL damped_motion)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	if (!drawablep)
	{
		llerrs << "Sending null drawable to moved list!" << llendl;
		return;
	}
	
	if (drawablep->isDead())
	{
		llwarns << "Marking NULL or dead drawable moved!" << llendl;
		return;
	}
	
	if (drawablep->getParent()) 
	{
		//ensure that parent drawables are moved first
		markMoved(drawablep->getParent(), damped_motion);
	}

	
	if (!drawablep->isState(LLDrawable::ON_MOVE_LIST))
	{
		mMovedList.insert(drawablep);
		drawablep->setState(LLDrawable::ON_MOVE_LIST);
	}
	if (damped_motion == FALSE)
	{
		drawablep->setState(LLDrawable::MOVE_UNDAMPED); // UNDAMPED trumps DAMPED
	}
	else if (drawablep->isState(LLDrawable::MOVE_UNDAMPED))
	{
		drawablep->clearState(LLDrawable::MOVE_UNDAMPED);
	}
}

void LLPipeline::markShift(LLDrawable *drawablep)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	if (!drawablep || drawablep->isDead())
	{
		return;
	}

	if (!drawablep->isState(LLDrawable::ON_SHIFT_LIST))
	{
		drawablep->getVObj()->setChanged(LLXform::SHIFTED | LLXform::SILHOUETTE);
		if (drawablep->getParent()) 
		{
			markShift(drawablep->getParent());
		}
		mShiftList.push_back(drawablep);
		drawablep->setState(LLDrawable::ON_SHIFT_LIST);
	}
}

void LLPipeline::shiftObjects(const LLVector3 &offset)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	for (LLDrawable::drawable_vector_t::iterator iter = mShiftList.begin();
		 iter != mShiftList.end(); iter++)
	{
		LLDrawable *drawablep = *iter;
		if (drawablep->isDead())
		{
			continue;
		}	
		drawablep->shiftPos(offset);	
		drawablep->clearState(LLDrawable::ON_SHIFT_LIST);
	}
	mShiftList.resize(0);

	mObjectPartition->shift(offset);
}

void LLPipeline::markTextured(LLDrawable *drawablep)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	if (!drawablep->isDead())
	{
		mRetexturedList.insert(drawablep);
	}
}

void LLPipeline::markMaterialed(LLDrawable *drawablep)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	if (!drawablep->isDead())
	{
		mRematerialedList.insert(drawablep);
	}
}

void LLPipeline::markRebuild(LLDrawable *drawablep, LLDrawable::EDrawableFlags flag, BOOL priority)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	if (drawablep && !drawablep->isDead())
	{
		if (!drawablep->isState(LLDrawable::BUILT))
		{
			priority = TRUE;
		}
		if (priority)
		{
			mBuildQ1.insert(drawablep);
			drawablep->setState(LLDrawable::IN_REBUILD_Q1); // flag is not needed, just for debugging
		}
		else if (!drawablep->isState(LLDrawable::IN_REBUILD_Q2))
		{
			mBuildQ2.push_back(drawablep);
			drawablep->setState(LLDrawable::IN_REBUILD_Q2); // need flag here because it is just a list
		}
		if (flag & LLDrawable::REBUILD_VOLUME)
		{
			drawablep->getVObj()->setChanged(LLXform::SILHOUETTE);
		}
		drawablep->setState(flag);
		if ((flag & LLDrawable::REBUILD_LIGHTING) && drawablep->getLit())
		{
			if (drawablep->isLight())
			{
				drawablep->clearState(LLDrawable::LIGHTING_BUILT);
			}
			else
			{
				drawablep->clearState(LLDrawable::LIGHTING_BUILT);
			}
		}
	}
}

void LLPipeline::markRelight(LLDrawable *drawablep, const BOOL priority)
{
	if (getLightingDetail() >= 2)
	{
		markRebuild(drawablep, LLDrawable::REBUILD_LIGHTING, FALSE);
	}
}

void LLPipeline::stateSort()
{
	LLFastTimer ftm(LLFastTimer::FTM_STATESORT);
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	for (LLDrawable::drawable_vector_t::iterator iter = mVisibleList.begin();
		 iter != mVisibleList.end(); iter++)
	{
		LLDrawable *drawablep = *iter;
		if (drawablep->isDead())
		{
			continue;
		}

		if (!drawablep->isActive())
		{
			drawablep->updateDistance(*gCamera);
		}

		/*
		if (!drawablep->isState(LLDrawable::BUILT))
		{
			// This geometry hasn't been rebuilt but it's visible, make sure it gets put on the rebuild list.
			llerrs << "Visible object " << drawablep << ":" << drawablep->getVObj()->getPCodeString();
			llcont << " visible but not built, put on rebuild" << llendl;
			markRebuild(drawablep);
			continue;
		}
		*/

		for (LLDrawable::face_list_t::iterator iter = drawablep->mFaces.begin();
			 iter != drawablep->mFaces.end(); iter++)
		{
			LLFace* facep = *iter;
			if (facep->hasGeometry())
			{
				facep->getPool()->enqueue(facep);
			}
		}
		
		if (sRenderPhysicalBeacons)
		{
			// Only show the beacon on the root object.
			LLViewerObject *vobj = drawablep->getVObj();
			if (vobj 
				&& !vobj->isAvatar() 
				&& !vobj->getParent()
				&& vobj->usePhysics())
			{
				gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(0.f, 1.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f));
			}
		}

		if (sRenderScriptedBeacons)
		{
			// Only show the beacon on the root object.
			LLViewerObject *vobj = drawablep->getVObj();
			if (vobj 
				&& !vobj->isAvatar() 
				&& !vobj->getParent()
				&& vobj->flagScripted())
			{
				gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(1.f, 0.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f));
			}
		}

		if (sRenderParticleBeacons)
		{
			// Look for attachments, objects, etc.
			LLViewerObject *vobj = drawablep->getVObj();
			if (vobj 
				&& vobj->isParticleSource())
			{
				LLColor4 light_blue(0.5f, 0.5f, 1.f, 0.5f);
				gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", light_blue, LLColor4(1.f, 1.f, 1.f, 0.5f));
			}
		}

		// Draw physical objects in red.
		if (gHUDManager->getShowPhysical())
		{
			LLViewerObject *vobj;
			vobj = drawablep->getVObj();
			if (vobj && !vobj->isAvatar())
			{
				if (!vobj->isAvatar() && 
					(vobj->usePhysics() || vobj->flagHandleTouch()))
				{
					if (!drawablep->isVisible())
					{
						// Skip objects that aren't visible.
						continue;
					}
					S32 face_id;
					for (face_id = 0; face_id < drawablep->getNumFaces(); face_id++)
					{
						mHighlightFaces.put(drawablep->getFace(face_id) );
					}
				}
			}
		}

		mNumVisibleFaces += drawablep->getNumFaces();
	}
	
	// If god mode, also show audio cues
	if (sRenderSoundBeacons && gAudiop)
	{
		// Update all of our audio sources, clean up dead ones.
		LLAudioEngine::source_map::iterator iter;
		for (iter = gAudiop->mAllSources.begin(); iter != gAudiop->mAllSources.end(); ++iter)
		{
			LLAudioSource *sourcep = iter->second;

			LLVector3d pos_global = sourcep->getPositionGlobal();
			LLVector3 pos = gAgent.getPosAgentFromGlobal(pos_global);
			//pos += LLVector3(0.f, 0.f, 0.2f);
			gObjectList.addDebugBeacon(pos, "", LLColor4(1.f, 1.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f));
		}
	}

	// If managing your telehub, draw beacons at telehub and currently selected spawnpoint.
	if (LLFloaterTelehub::renderBeacons())
	{
		LLFloaterTelehub::addBeacons();
	}

	mSelectedFaces.reset();
	
	// Draw face highlights for selected faces.
	if (gSelectMgr->getTEMode())
	{
		LLViewerObject *vobjp;
		S32             te;
		gSelectMgr->getFirstTE(&vobjp,&te);

		while (vobjp)
		{
			LLDrawable *drawablep = vobjp->mDrawable;
			if (!drawablep || drawablep->isDead() || (!vobjp->isHUDAttachment() && !drawablep->isVisible()))
			{
				llwarns << "Dead drawable on selected face list!" << llendl;
			}
			else
			{
				LLVOVolume *volp = drawablep->getVOVolume();
				if (volp)
				{
					if (volp->getAllTEsSame())
					{
						SelectedFaceInfo* faceinfo = mSelectedFaces.reserve_block(1);
						faceinfo->mFacep = drawablep->getFace(vobjp->getFaceIndexOffset());
						faceinfo->mTE = te;
					}
					else
					{
						// This is somewhat inefficient, but works correctly.
						S32 face_id;
						for (face_id = 0; face_id < vobjp->getVolume()->getNumFaces(); face_id++)
						{
							LLFace *facep = drawablep->getFace(face_id + vobjp->getFaceIndexOffset());
							if (te == facep->getTEOffset())
							{
								SelectedFaceInfo* faceinfo = mSelectedFaces.reserve_block(1);
								faceinfo->mFacep = facep;
								faceinfo->mTE = -1;
							}
						}
					}
				}
				else
				{
					// This is somewhat inefficient, but works correctly.
					S32 face_id;
					for (face_id = 0; face_id <  drawablep->getNumFaces(); face_id++)
					{
						LLFace *facep = drawablep->getFace(face_id + vobjp->getFaceIndexOffset());
						if (te == facep->getTEOffset())
						{
							SelectedFaceInfo* faceinfo = mSelectedFaces.reserve_block(1);
							faceinfo->mFacep = facep;
							faceinfo->mTE = -1;
						}
					}
				}
			}
			gSelectMgr->getNextTE(&vobjp,&te);
		}
	}
}


static void render_hud_elements()
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_UI);
	gPipeline.disableLights();		
	
	gPipeline.renderDebug();
	
	LLGLDisable fog(GL_FOG);
	LLGLSUIDefault gls_ui;
	
	if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
	{
		gViewerWindow->renderSelections(FALSE, FALSE, FALSE); // For HUD bersion in render_ui_3d()
	
		// Draw the tracking overlays
		LLTracker::render3D();
		
		// Show the property lines
		if (gWorldp)
		{
			gWorldp->renderPropertyLines();
		}
		if (gParcelMgr)
		{
			gParcelMgr->render();
			gParcelMgr->renderParcelCollision();
		}
		
		// Render debugging beacons.
		gObjectList.renderObjectBeacons();
		LLHUDObject::renderAll();
		gObjectList.resetObjectBeacons();
	}
	else if (gForceRenderLandFence)
	{
		// This is only set when not rendering the UI, for parcel snapshots
		gParcelMgr->render();
	}

}

void LLPipeline::renderHighlights()
{
	// Draw 3D UI elements here (before we clear the Z buffer in POOL_HUD)
	// Render highlighted faces.
	LLColor4 color(1.f, 1.f, 1.f, 0.5f);
	LLGLEnable color_mat(GL_COLOR_MATERIAL);
	disableLights();

	if ((mVertexShaderLevel[SHADER_INTERFACE] > 0))
	{
		mHighlightProgram.bind();
		gPipeline.mHighlightProgram.vertexAttrib4f(LLPipeline::GLSL_MATERIAL_COLOR,1,0,0,0.5f);
	}
	
	if (hasRenderDebugFeatureMask(RENDER_DEBUG_FEATURE_SELECTED))
	{
		// Make sure the selection image gets downloaded and decoded
		if (!mFaceSelectImagep)
		{
			mFaceSelectImagep = gImageList.getImage(IMG_FACE_SELECT);
		}
		mFaceSelectImagep->addTextureStats((F32)MAX_IMAGE_AREA);

		for (S32 i = 0; i < mSelectedFaces.count(); i++)
		{
			LLFace *facep = mSelectedFaces[i].mFacep;
			if (!facep || facep->getDrawable()->isDead())
			{
				llerrs << "Bad face on selection" << llendl;
			}
			
			LLDrawPool* poolp = facep->getPool();

			if (!poolp->canUseAGP())
			{
				unbindAGP();
			}
			else if (usingAGP())
			{
				bindAGP();
			}

			if (mSelectedFaces[i].mTE == -1)
			{
				// Yes, I KNOW this is stupid...
				poolp->renderFaceSelected(facep, mFaceSelectImagep, color);
			}
			else
			{
				LLVOVolume *volp = (LLVOVolume *)facep->getViewerObject();
				// Do the special coalesced face mode.
				S32 j;
				S32 offset = 0;
				S32 count = volp->getVolume()->getVolumeFace(0).mIndices.size();
				for (j = 0; j <= mSelectedFaces[i].mTE; j++)
				{
					count = volp->getVolume()->getVolumeFace(j).mIndices.size();
					if (j < mSelectedFaces[i].mTE)
					{
						offset += count;
					}
				}

				poolp->renderFaceSelected(facep, mFaceSelectImagep, color, offset, count);
			}
		}
	}

	if (hasRenderDebugFeatureMask(RENDER_DEBUG_FEATURE_SELECTED))
	{
		// Paint 'em red!
		color.setVec(1.f, 0.f, 0.f, 0.5f);
		for (S32 i = 0; i < mHighlightFaces.count(); i++)
		{
			LLFace* facep = mHighlightFaces[i];
			LLDrawPool* poolp = facep->getPool();
			if (!poolp->canUseAGP())
			{
				unbindAGP();
			}
			else if (usingAGP())
			{
				bindAGP();
			}

			poolp->renderFaceSelected(facep, LLViewerImage::sNullImagep, color);
		}
	}

	// Contains a list of the faces of objects that are physical or
	// have touch-handlers.
	mHighlightFaces.reset();

	if (mVertexShaderLevel[SHADER_INTERFACE] > 0)
	{
		mHighlightProgram.unbind();
	}
}

void LLPipeline::renderGeom()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	LLFastTimer t(LLFastTimer::FTM_RENDER_GEOMETRY);

	if (!mAlphaSizzleImagep)
	{
		mAlphaSizzleImagep = gImageList.getImage(LLUUID(gViewerArt.getString("alpha_sizzle.tga")), MIPMAP_TRUE, TRUE);
	}

	///////////////////////////////////////////
	//
	// Sync and verify GL state
	//
	//

	stop_glerror();
	gFrameStats.start(LLFrameStats::RENDER_SYNC);

	// Do verification of GL state
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
#endif
	if (mRenderDebugMask & RENDER_DEBUG_VERIFY)
	{
		if (!verify())
		{
			llerrs << "Pipeline verification failed!" << llendl;
		}
	}

	if (mAGPMemPool)
	{
		mAGPMemPool->waitFence(mGlobalFence);
	}

	unbindAGP();
	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		if (hasRenderType(poolp->getType()))
		{
			poolp->prerender();
			poolp->syncAGP();
		}
	}

	gFrameStats.start(LLFrameStats::RENDER_GEOM);

	// Initialize lots of GL state to "safe" values
	mTrianglesDrawn = 0;
	
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	LLGLSPipeline gls_pipeline;
	
	LLGLState gls_color_material(GL_COLOR_MATERIAL, mLightingDetail < 2);
	LLGLState normalize(GL_NORMALIZE, TRUE);
			
	// Toggle backface culling for debugging
	LLGLEnable cull_face(mBackfaceCull ? GL_CULL_FACE : 0);
	// Set fog
	LLGLEnable fog_enable(hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_FOG) ? GL_FOG : 0);


	LLViewerImage::sDefaultImagep->bind(0);
	LLViewerImage::sDefaultImagep->setClamp(FALSE, FALSE);
	
	//////////////////////////////////////////////
	//
	// Actually render all of the geometry
	//
	//

	stop_glerror();
	BOOL non_agp = FALSE;
	BOOL did_hud_elements = FALSE;
	
	U32 cur_type = 0;

	S32 skipped_vertices = 0;
	{
		LLFastTimer t(LLFastTimer::FTM_POOLS);
		BOOL occlude = TRUE;

		calcNearbyLights();

		pool_set_t::iterator iter1 = mPools.begin();
		while ( iter1 != mPools.end() )
		{
			LLDrawPool *poolp = *iter1;
			
			cur_type = poolp->getType();

			if (cur_type >= LLDrawPool::POOL_TREE && occlude)
			{ //all the occluders have been drawn, do occlusion queries
				if (mVertexShadersEnabled)
				{
					glUseProgramObjectARB(0);
				}
				doOcclusion();
				occlude = FALSE;
			}
			
			if (cur_type >= LLDrawPool::POOL_HUD && !did_hud_elements)
			{
				renderHighlights();
				// Draw 3D UI elements here (before we clear the Z buffer in POOL_HUD)
				if (mVertexShadersEnabled)
				{
					glUseProgramObjectARB(0);
				}
				render_hud_elements();
				did_hud_elements = TRUE;
			}

			pool_set_t::iterator iter2 = iter1;
			if (hasRenderType(poolp->getType()))
			{
				LLFastTimer t(LLFastTimer::FTM_POOLRENDER);

				setupHWLights(poolp);
				
				if (mVertexShadersEnabled && poolp->getVertexShaderLevel() == 0)
				{
					glUseProgramObjectARB(0);
				}
				else if (mVertexShadersEnabled)
				{
					mMaterialIndex = mSpecularIndex = 0;
					switch(cur_type)
					{
					  case LLDrawPool::POOL_SKY:
					  case LLDrawPool::POOL_STARS:
					  case LLDrawPool::POOL_CLOUDS:
					  	glUseProgramObjectARB(0); 
						break;
					  case LLDrawPool::POOL_TERRAIN:
						mTerrainProgram.bind();
						break;
					  case LLDrawPool::POOL_GROUND:
						mGroundProgram.bind();
						break;
					  case LLDrawPool::POOL_TREE:
					  case LLDrawPool::POOL_TREE_NEW:
					  case LLDrawPool::POOL_SIMPLE:
					  case LLDrawPool::POOL_MEDIA:
						mObjectSimpleProgram.bind();
						break;
					  case LLDrawPool::POOL_BUMP:
						mObjectBumpProgram.bind();
						break;
					  case LLDrawPool::POOL_AVATAR:
						glUseProgramObjectARB(0);
                        break;
					  case LLDrawPool::POOL_WATER:
						glUseProgramObjectARB(0);
						break;
					  case LLDrawPool::POOL_ALPHA:
						mObjectAlphaProgram.bind();
						break;
					  case LLDrawPool::POOL_HUD:
					  default:
						glUseProgramObjectARB(0);
						break;
					}
				}

				for( S32 i = 0; i < poolp->getNumPasses(); i++ )
				{
					poolp->beginRenderPass(i);
					for (iter2 = iter1; iter2 != mPools.end(); iter2++)
					{
						LLDrawPool *p = *iter2;
						if (p->getType() != cur_type)
						{
							break;
						}
						if (p->getType() != LLDrawPool::POOL_AVATAR
							&& p->getType() != LLDrawPool::POOL_ALPHA
							&& p->getType() != LLDrawPool::POOL_HUD
							&& (!p->getIndexCount() || !p->getVertexCount()))
						{
							continue;
						}

						if (p->canUseAGP() && usingAGP())
						{
							bindAGP();
						}
						else
						{
							//llinfos << "Rendering pool type " << p->getType() << " without AGP!" << llendl;
							unbindAGP();
							non_agp = TRUE;
						}
					
						p->resetTrianglesDrawn();
						p->render(i);
						mTrianglesDrawn += p->getTrianglesDrawn();
						skipped_vertices += p->mSkippedVertices;
						p->mSkippedVertices = 0;
					}
					poolp->endRenderPass(i);
#ifndef LL_RELEASE_FOR_DOWNLOAD
					LLGLState::checkStates();
					LLGLState::checkTextureChannels();
					LLGLState::checkClientArrays();
#endif
				}
			}
			else
			{
				// Skip all pools of this type
				for (iter2 = iter1; iter2 != mPools.end(); iter2++)
				{
					LLDrawPool *p = *iter2;
					if (p->getType() != cur_type)
					{
						break;
					}
				}
			}
			iter1 = iter2;
			stop_glerror();
		}

		if (occlude)
		{
			if (mVertexShadersEnabled)
			{
				glUseProgramObjectARB(0);
			}
			doOcclusion();
		}
	}
	stop_glerror();
	
	if (mVertexShadersEnabled)
	{
		glUseProgramObjectARB(0);
	}

	if (!did_hud_elements)
	{
		renderHighlights();
		render_hud_elements();
	}
	
	static S32 agp_mix_count = 0;
	if (non_agp && usingAGP())
	{
		if (0 == agp_mix_count % 16)
		{
			lldebugs << "Mixing AGP and non-AGP pools, slow!" << llendl;
		}
		agp_mix_count++;
	}
	else
	{
		agp_mix_count = 0;
	}

	// Contains a list of the faces of objects that are physical or
	// have touch-handlers.
	mHighlightFaces.reset();

	// This wait is in case we try to do multiple renders of a frame,
	// I don't know what happens when we send a fence multiple times without
	// checking it.
	if (mAGPMemPool)
	{
		mAGPMemPool->waitFence(mGlobalFence);
		mAGPMemPool->sendFence(mGlobalFence);
	}
}

void LLPipeline::renderDebug()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	// Disable all client state
    glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

	// Debug stuff.
	mObjectPartition->renderDebug();

	if (mRenderDebugMask & LLPipeline::RENDER_DEBUG_LIGHT_TRACE)
	{
		LLGLSNoTexture no_texture;

		LLVector3 pos, pos1;

		for (LLDrawable::drawable_vector_t::iterator iter = mVisibleList.begin();
			 iter != mVisibleList.end(); iter++)
		{
			LLDrawable *drawablep = *iter;
			if (drawablep->isDead())
			{
				continue;
			}
			for (LLDrawable::drawable_set_t::iterator iter = drawablep->mLightSet.begin();
				 iter != drawablep->mLightSet.end(); iter++)
			{
				LLDrawable *targetp = *iter;
				if (targetp->isDead() || !targetp->getVObj()->getNumTEs())
				{
					continue;
				}
				else
				{
					if (targetp->getTextureEntry(0))
					{
						if (drawablep->getVObj()->getPCode() == LLViewerObject::LL_VO_SURFACE_PATCH)
						{
							glColor4f(0.f, 1.f, 0.f, 1.f);
							gObjectList.addDebugBeacon(drawablep->getPositionAgent(), "TC");
						}
						else
						{
							glColor4fv (targetp->getTextureEntry(0)->getColor().mV);
						}
						glBegin(GL_LINES);
						glVertex3fv(targetp->getPositionAgent().mV);
						glVertex3fv(drawablep->getPositionAgent().mV);
						glEnd();
					}
				}
			}
		}
	}

	mCompilesStat.addValue(sCompiles);
	mLightingChangesStat.addValue(mLightingChanges);
	mGeometryChangesStat.addValue(mGeometryChanges);
	mTrianglesDrawnStat.addValue(mTrianglesDrawn/1000.f);
	mVerticesRelitStat.addValue(mVerticesRelit);
	mNumVisibleFacesStat.addValue(mNumVisibleFaces);
	mNumVisibleDrawablesStat.addValue((S32)mVisibleList.size());

	if (gRenderLightGlows)
	{
		displaySSBB();
	}

	/*if (mRenderDebugMask & RENDER_DEBUG_BBOXES)
	{
		LLGLSPipelineAlpha gls_pipeline_alpha;
		LLGLSNoTexture no_texture;

		for (LLDrawable::drawable_vector_t::iterator iter = mVisibleList.begin(); iter != mVisibleList.end(); iter++)
		{
			LLDrawable *drawablep = *iter;
			if (drawablep->isDead())
			{
				continue;
			}
			LLVector3 min, max;
			
			if (drawablep->getVObj() && drawablep->getVObj()->getPCode() == LLViewerObject::LL_VO_SURFACE_PATCH)
			{
				// Render drawable bbox
				drawablep->getBounds(min, max);
				glColor4f(0.f, 1.f, 0.f, 0.25f);
				render_bbox(min, max);

				// Render object bbox
				LLVector3 scale = drawablep->getVObj()->getScale();
				LLVector3 pos = drawablep->getVObj()->getPositionAgent();
				min = pos - scale * 0.5f;
				max = pos + scale * 0.5f;
				glColor4f(1.f, 0.f, 0.f, 0.25f);
				render_bbox(min, max);
			}
		}
	}*/

	/*
	// Debugging code for parcel sound.
	F32 x, y;

	LLGLSNoTexture gls_no_texture;

	glBegin(GL_POINTS);
	if (gAgent.getRegion())
	{
		// Draw the composition layer for the region that I'm in.
		for (x = 0; x <= 260; x++)
		{
			for (y = 0; y <= 260; y++)
			{
				if (gParcelMgr->isSoundLocal(gAgent.getRegion()->getOriginGlobal() + LLVector3d(x, y, 0.f)))
				{
					glColor4f(1.f, 0.f, 0.f, 1.f);
				}
				else
				{
					glColor4f(0.f, 0.f, 1.f, 1.f);
				}

				glVertex3f(x, y, gAgent.getRegion()->getLandHeightRegion(LLVector3(x, y, 0.f)));
			}
		}
	}
	glEnd();
	*/

	if (mRenderDebugMask & RENDER_DEBUG_COMPOSITION)
	{
		// Debug composition layers
		F32 x, y;

		LLGLSNoTexture gls_no_texture;

		glBegin(GL_POINTS);
		if (gAgent.getRegion())
		{
			// Draw the composition layer for the region that I'm in.
			for (x = 0; x <= 260; x++)
			{
				for (y = 0; y <= 260; y++)
				{
					if ((x > 255) || (y > 255))
					{
						glColor4f(1.f, 0.f, 0.f, 1.f);
					}
					else
					{
						glColor4f(0.f, 0.f, 1.f, 1.f);
					}
					F32 z = gAgent.getRegion()->getCompositionXY((S32)x, (S32)y);
					z *= 5.f;
					z += 50.f;
					glVertex3f(x, y, z);
				}
			}
		}
		glEnd();
	}

	if (mRenderDebugMask & RENDER_DEBUG_AGP_MEM)
	{
		displayAGP();
	}

	if (mRenderDebugMask & RENDER_DEBUG_POOLS)
	{
		displayPools();
	}

// 	if (mRenderDebugMask & RENDER_DEBUG_QUEUES)
// 	{
// 		displayQueues();
// 	}

	if (mRenderDebugMask & RENDER_DEBUG_MAP)
	{
		displayMap();
	}
}




BOOL compute_min_max(LLMatrix4& box, LLVector2& min, LLVector2& max)
{
	min.setVec(1000,1000);
	max.setVec(-1000,-1000);

	if (box.mMatrix[3][3] <= 0.0f) return FALSE;

	const F32 vec[8][3] = { 
		{ -0.5f,-0.5f,-0.5f },
		{ -0.5f,-0.5f,+0.5f },
		{ -0.5f,+0.5f,-0.5f },
		{ -0.5f,+0.5f,+0.5f },
		{ +0.5f,-0.5f,-0.5f },
		{ +0.5f,-0.5f,+0.5f },
		{ +0.5f,+0.5f,-0.5f },
		{ +0.5f,+0.5f,+0.5f } };
   
	LLVector4 v;

	for (S32 i=0;i<8;i++)
	{
		v.setVec(vec[i][0],vec[i][1],vec[i][2],1);
		v = v * box;
		F32 iw = 1.0f / v.mV[3];
		v.mV[0] *= iw;
		v.mV[1] *= iw;

		min.mV[0] = llmin(min.mV[0],v.mV[0]);
		max.mV[0] = llmax(max.mV[0],v.mV[0]);

		min.mV[1] = llmin(min.mV[1],v.mV[1]);
		max.mV[1] = llmax(max.mV[1],v.mV[1]);
	}

	/*
	min.mV[0] = max.mV[0] = box.mMatrix[3][0];
	min.mV[1] = max.mV[1] = box.mMatrix[3][1];
	F32        iw  = 1.0f / box.mMatrix[3][3];

	F32 f0 = (fabs(box.mMatrix[0][0])+fabs(box.mMatrix[1][0])+fabs(box.mMatrix[2][0])) * 0.5f;
	F32 f1 = (fabs(box.mMatrix[0][1])+fabs(box.mMatrix[1][1])+fabs(box.mMatrix[2][1])) * 0.5f;
	F32 f2 = (fabs(box.mMatrix[0][2])+fabs(box.mMatrix[1][2])+fabs(box.mMatrix[2][2])) * 0.5f;

	min.mV[0] -= f0; 
	min.mV[1] -= f1; 

	max.mV[0] += f0; 
	max.mV[1] += f1; 

	min.mV[0] *= iw;
	min.mV[1] *= iw;

	max.mV[0] *= iw;
	max.mV[1] *= iw;
	*/
	return TRUE;
}

void LLPipeline::displaySSBB()
{
	LLMatrix4 proj;
	LLMatrix4 cfr(OGL_TO_CFR_ROTATION);
	LLMatrix4 camera;
	LLMatrix4 comb;

	gCamera->getMatrixToLocal(camera);
	
	if (!mBloomImagep)
	{
		mBloomImagep = gImageList.getImage(IMG_BLOOM1);
	}

	// don't write to depth buffer with light glows so that chat bubbles can pop through
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);
	LLViewerImage::bindTexture(mBloomImagep);

	glGetFloatv(GL_PROJECTION_MATRIX,(float*)proj.mMatrix);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	LLGLSPipelineAlpha gls_pipeline_alpha;

	//glScalef(0.25,0.25,0.25);

	S32 sizex = gViewerWindow->getWindowWidth() / 2;
	S32 sizey = gViewerWindow->getWindowHeight() / 2;

	F32 aspect = (float)sizey / (float)sizex;

	for (LLDrawable::drawable_set_t::iterator iter = mLights.begin();
		 iter != mLights.end(); iter++)
	{
		LLDrawable *lightp = *iter;
		if (lightp->isDead())
		{
			continue;
		}
		
		LLMatrix4 mat = lightp->mXform.getWorldMatrix();

		mat *= camera;
		mat *= cfr;
		mat *= proj;

		U8  color[64];

		LLVector2 min,max;
		if (mat.mMatrix[3][3] < 160 && compute_min_max(mat,min,max))
		{
			F32 cx = (max.mV[0] + min.mV[0]) * 0.5f;
			F32 cy = (max.mV[1] + min.mV[1]) * 0.5f;
			F32 sx = (max.mV[0] - min.mV[0]) * 2.0f;
			F32 sy = (max.mV[1] - min.mV[1]) * 2.0f;
			S32 x  = (S32)(cx * (F32)sizex) + sizex;
			S32 y  = (S32)(cy * (F32)sizey) + sizey;

			if (cx > -1 && cx < 1 && cy > -1 && cy < 1)
			{
				glReadPixels(x-2,y-2,4,4,GL_RGBA,GL_UNSIGNED_BYTE,&color[0]);

				S32 total = 0;
				for (S32 i=0;i<64;i++)
				{
					total += color[i];
				}
				total /= 64;

				sx = (sy = (sx + sy) * 0.5f * ((float)total/255.0f)) * aspect;


				if (total > 60)
				{
					color[3+32] = total >> 1;
					glBegin(GL_QUADS);
					glColor4ubv(&color[32]);
					glTexCoord2f(0,0);
					glVertex3f(cx-sx,cy-sy,0);
					glTexCoord2f(1,0);
					glVertex3f(cx+sx,cy-sy,0);
					glTexCoord2f(1,1);
					glVertex3f(cx+sx,cy+sy,0);
					glTexCoord2f(0,1);
					glVertex3f(cx-sx,cy+sy,0);
					glEnd();
				}
			}
		}

	}

	// sun
	{
		LLVector4 sdir(gSky.getSunDirection() * 10000.0f + gAgent.getPositionAgent());
		sdir.mV[3] = 1.0f;
		sdir = sdir * camera;
		sdir = sdir * cfr;
		sdir = sdir * proj; // todo: preconcat

		sdir.mV[0] /= sdir.mV[3];
		sdir.mV[1] /= sdir.mV[3];

		U8  color[64];

		if (sdir.mV[3] > 0)
		{
			F32 cx = sdir.mV[0];
			F32 cy = sdir.mV[1];
			F32 sx, sy;
			S32 x  = (S32)(cx * (F32)sizex) + sizex;
			S32 y  = (S32)(cy * (F32)sizey) + sizey;

			if (cx > -1 && cx < 1 && cy > -1 && cy < 1)
			{
				glReadPixels(x-2,y-2,4,4,GL_RGBA,GL_UNSIGNED_BYTE,&color[0]);

				S32 total = 0;
				for (S32 i=0;i<64;i++)
				{
					total += color[i];
				}
				total >>= 7;

				sx = (sy = ((float)total/255.0f)) * aspect;

				const F32 fix = -0.1f;

				color[32] = (U8)(color[32] * 0.5f + 255 * 0.5f);
				color[33] = (U8)(color[33] * 0.5f + 255 * 0.5f);
				color[34] = (U8)(color[34] * 0.5f + 255 * 0.5f);

				if (total > 80)
				{
					color[32+3]  = (U8)total;
					glBegin(GL_QUADS);
					glColor4ubv(&color[32]);
					glTexCoord2f(0,0);
					glVertex3f(cx-sx,cy-sy+fix,0);
					glTexCoord2f(1,0);
					glVertex3f(cx+sx,cy-sy+fix,0);
					glTexCoord2f(1,1);
					glVertex3f(cx+sx,cy+sy+fix,0);
					glTexCoord2f(0,1);
					glVertex3f(cx-sx,cy+sy+fix,0);
					glEnd();
				}
			}
		}
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void LLPipeline::displayMap()
{
	LLGLSPipelineAlpha gls_pipeline_alpha;
	LLGLSNoTexture no_texture;
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glTranslatef(-1,-1,0);
	glScalef(2,2,0);

	glColor4f(0,0,0,0.5);
	glBegin(GL_QUADS);
	glVertex3f(0,0,0);
	glVertex3f(1,0,0);
	glVertex3f(1,1,0);
	glVertex3f(0,1,0);
	glEnd();

	static F32 totalW = 1.5f;
	static F32 offset = 0.5f;
	static F32 scale  = 1.0f;

	S32 mousex = gViewerWindow->getCurrentMouseX();
	S32 mousey = gViewerWindow->getCurrentMouseY();
	S32 w      = gViewerWindow->getWindowWidth();
	S32 h      = gViewerWindow->getWindowHeight();

	if (mousex < 20 && offset > 0) offset -= (20 - mousex) * 0.02f;
	if (mousex > (w - 20))         offset += (20 - (w - mousex)) * 0.02f;
	if (offset > (totalW-1))       offset = (totalW-1);

	if (mousey < 20 && scale > 0.1)        scale -= (20 - mousey) * 0.001f;
	if (mousey > (h - 20) && scale < 1.0f) scale += (20 - (h - mousey)) * 0.001f;

	glScalef(scale*scale,scale,1);
	glTranslatef(-offset,0,0);
	//totalW = mStaticTree->render2D(0,0.8f/scale);
	//mDynamicTree->render2D(0,0.4f/scale);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void LLPipeline::renderForSelect()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	//for each drawpool

	glMatrixMode(GL_MODELVIEW);

	LLGLSDefault gls_default;
	LLGLSObjectSelect gls_object_select;
	LLGLDepthTest gls_depth(GL_TRUE);
	disableLights();
	
    glEnableClientState ( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
    glDisableClientState( GL_TEXTURE_COORD_ARRAY );

#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();
	U32 last_type = 0;
#endif
	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		if (poolp->canUseAGP() && usingAGP())
		{
			bindAGP();
		}
		else
		{
			//llinfos << "Rendering pool type " << p->getType() << " without AGP!" << llendl;
			unbindAGP();
		}
		poolp->renderForSelect();
#ifndef LL_RELEASE_FOR_DOWNLOAD
		if (poolp->getType() != last_type)
		{
			last_type = poolp->getType();
			LLGLState::checkStates();
			LLGLState::checkTextureChannels();
			LLGLState::checkClientArrays();
		}
#endif
	}

	// Disable all of the client state
	glDisableClientState( GL_VERTEX_ARRAY );

	if (mAGPMemPool)
	{
		mAGPMemPool->waitFence(mGlobalFence);
		mAGPMemPool->sendFence(mGlobalFence);
	}
}

void LLPipeline::renderFaceForUVSelect(LLFace* facep)
{
	if (facep) facep->renderSelectedUV();
}

void LLPipeline::rebuildPools()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	S32 max_count = mPools.size();
	S32 num_rebuilds = 0;
	pool_set_t::iterator iter1 = mPools.upper_bound(mLastRebuildPool);
	while(max_count > 0 && mPools.size() > 0) // && num_rebuilds < MAX_REBUILDS)
	{
		if (iter1 == mPools.end())
		{
			iter1 = mPools.begin();
		}
		LLDrawPool* poolp = *iter1;
		num_rebuilds += poolp->rebuild();
		if (poolp->mReferences.empty())
		{
			mPools.erase(iter1++);
			removeFromQuickLookup( poolp );
			if (poolp == mLastRebuildPool)
			{
				mLastRebuildPool = NULL;
			}
			delete poolp;
		}
		else
		{
			mLastRebuildPool = poolp;
			iter1++;
		}
		max_count--;
	}

	if (gAgent.getAvatarObject())
	{
		gAgent.getAvatarObject()->rebuildHUD();
	}
}

void LLPipeline::addToQuickLookup( LLDrawPool* new_poolp )
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	switch( new_poolp->getType() )
	{
	case LLDrawPool::POOL_SIMPLE:
		mSimplePools[ uintptr_t(new_poolp->getTexture()) ] = new_poolp ;
		break;

	case LLDrawPool::POOL_TREE:
		mTreePools[ uintptr_t(new_poolp->getTexture()) ] = new_poolp ;
		break;

	case LLDrawPool::POOL_TREE_NEW:
		mTreeNewPools[ uintptr_t(new_poolp->getTexture()) ] = new_poolp ;
		break;
 
	case LLDrawPool::POOL_TERRAIN:
		mTerrainPools[ uintptr_t(new_poolp->getTexture()) ] = new_poolp ;
		break;

	case LLDrawPool::POOL_BUMP:
		mBumpPools[ uintptr_t(new_poolp->getTexture()) ] = new_poolp ;
		break;

	case LLDrawPool::POOL_MEDIA:
		mMediaPools[ uintptr_t(new_poolp->getTexture()) ] = new_poolp ;
		break;

	case LLDrawPool::POOL_ALPHA:
		if( mAlphaPool )
		{
			llassert(0);
			llwarns << "LLPipeline::addPool(): Ignoring duplicate Alpha pool" << llendl;
		}
		else
		{
			mAlphaPool = new_poolp;
		}
		break;

	case LLDrawPool::POOL_AVATAR:
		break; // Do nothing

	case LLDrawPool::POOL_SKY:
		if( mSkyPool )
		{
			llassert(0);
			llwarns << "LLPipeline::addPool(): Ignoring duplicate Sky pool" << llendl;
		}
		else
		{
			mSkyPool = new_poolp;
		}
		break;

	case LLDrawPool::POOL_STARS:
		if( mStarsPool )
		{
			llassert(0);
			llwarns << "LLPipeline::addPool(): Ignoring duplicate Stars pool" << llendl;
		}
		else
		{
			mStarsPool = new_poolp;
		}
		break;

	case LLDrawPool::POOL_CLOUDS:
		if( mCloudsPool )
		{
			llassert(0);
			llwarns << "LLPipeline::addPool(): Ignoring duplicate Clouds pool" << llendl;
		}
		else
		{
			mCloudsPool = new_poolp;
		}
		break;

	case LLDrawPool::POOL_WATER:
		if( mWaterPool )
		{
			llassert(0);
			llwarns << "LLPipeline::addPool(): Ignoring duplicate Water pool" << llendl;
		}
		else
		{
			mWaterPool = new_poolp;
		}
		break;

	case LLDrawPool::POOL_GROUND:
		if( mGroundPool )
		{
			llassert(0);
			llwarns << "LLPipeline::addPool(): Ignoring duplicate Ground Pool" << llendl;
		}
		else
		{ 
			mGroundPool = new_poolp;
		}
		break;

	case LLDrawPool::POOL_HUD:
		if( mHUDPool )
		{
			llerrs << "LLPipeline::addPool(): Duplicate HUD Pool!" << llendl;
		}
		mHUDPool = new_poolp;
		break;

	default:
		llassert(0);
		llwarns << "Invalid Pool Type in  LLPipeline::addPool()" << llendl;
		break;
	}
}

void LLPipeline::removePool( LLDrawPool* poolp )
{
	removeFromQuickLookup(poolp);
	mPools.erase(poolp);
	delete poolp;
}

void LLPipeline::removeFromQuickLookup( LLDrawPool* poolp )
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	switch( poolp->getType() )
	{
	case LLDrawPool::POOL_SIMPLE:
		#ifdef _DEBUG
			{
				BOOL found = mSimplePools.erase( (uintptr_t)poolp->getTexture() );
				llassert( found );
			}
		#else
			mSimplePools.erase( (uintptr_t)poolp->getTexture() );
		#endif
		break;

	case LLDrawPool::POOL_TREE:
		#ifdef _DEBUG
			{
				BOOL found = mTreePools.erase( (uintptr_t)poolp->getTexture() );
				llassert( found );
			}
		#else
			mTreePools.erase( (uintptr_t)poolp->getTexture() );
		#endif
		break;

	case LLDrawPool::POOL_TREE_NEW:
		#ifdef _DEBUG
			{
				BOOL found = mTreeNewPools.erase( (uintptr_t)poolp->getTexture() );
				llassert( found );
			}
		#else
			mTreeNewPools.erase( (uintptr_t)poolp->getTexture() );
		#endif
		break;

	case LLDrawPool::POOL_TERRAIN:
		#ifdef _DEBUG
			{
				BOOL found = mTerrainPools.erase( (uintptr_t)poolp->getTexture() );
				llassert( found );
			}
		#else
			mTerrainPools.erase( (uintptr_t)poolp->getTexture() );
		#endif
		break;

	case LLDrawPool::POOL_BUMP:
		#ifdef _DEBUG
			{
				BOOL found = mBumpPools.erase( (uintptr_t)poolp->getTexture() );
				llassert( found );
			}
		#else
			mBumpPools.erase( (uintptr_t)poolp->getTexture() );
		#endif
		break;

	case LLDrawPool::POOL_MEDIA:
		#ifdef _DEBUG
			{
				BOOL found = mMediaPools.erase( (uintptr_t)poolp->getTexture() );
				llassert( found );
			}
		#else
			mMediaPools.erase( (uintptr_t)poolp->getTexture() );
		#endif
		break;

	case LLDrawPool::POOL_ALPHA:
		llassert( poolp == mAlphaPool );
		mAlphaPool = NULL;
		break;

	case LLDrawPool::POOL_AVATAR:
		break; // Do nothing

	case LLDrawPool::POOL_SKY:
		llassert( poolp == mSkyPool );
		mSkyPool = NULL;
		break;

	case LLDrawPool::POOL_STARS:
		llassert( poolp == mStarsPool );
		mStarsPool = NULL;
		break;

	case LLDrawPool::POOL_CLOUDS:
		llassert( poolp == mCloudsPool );
		mCloudsPool = NULL;
		break;

	case LLDrawPool::POOL_WATER:
		llassert( poolp == mWaterPool );
		mWaterPool = NULL;
		break;

	case LLDrawPool::POOL_GROUND:
		llassert( poolp == mGroundPool );
		mGroundPool = NULL;
		break;

	case LLDrawPool::POOL_HUD:
		llassert( poolp == mHUDPool );
		mHUDPool = NULL;
		break;

	default:
		llassert(0);
		llwarns << "Invalid Pool Type in  LLPipeline::removeFromQuickLookup() type=" << poolp->getType() << llendl;
		break;
	}
}

void LLPipeline::flushAGPMemory()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		poolp->flushAGP();
	}
}

void LLPipeline::resetDrawOrders()
{
	// Iterate through all of the draw pools and rebuild them.
	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		poolp->resetDrawOrders();
	}
}

//-------------------------------


LLViewerObject *LLPipeline::nearestObjectAt(F32 yaw, F32 pitch)
{
	// Stub to find nearest Object at given yaw and pitch

	/*
	LLEdge *vd = NULL;

	if (vd) 
	{
		return (LLViewerObject*)vd->mDrawablep->getVObj();
	}
	*/

	return NULL;
}

void LLPipeline::printPools()
{
	/*
	// Iterate through all of the draw pools and rebuild them.
	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		if (pool->mTexturep[0])
		{
			llinfos << "Op pool " << pool->mTexturep[0]->mID << llendflush;
		}
		else
		{
			llinfos << "Opaque pool NULL" << llendflush;
		}
		llinfos << " Vertices: \t" << pool->getVertexCount() << llendl;
	}


	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		llinfos << "Al pool " << pool;
		llcont << " Vertices: \t" << pool->getVertexCount() << llendl;
	}

	pool = mHighlightPools.getFirst();
	llinfos << "Si pool " << pool;
	llcont << " Vertices: \t" << pool->getVertexCount() << llendl;
	*/
}


void LLPipeline::displayPools()
{
	// Needs to be fixed to handle chained pools - djs
	LLUI::setLineWidth(1.0);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	LLGLSPipelineAlpha gls_pipeline_alpha;
	LLGLSNoTexture no_texture;
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	glScalef(2,2,1);
	glTranslatef(-0.5f,-0.5f,0);

	F32 x   = 0.0f,    y   = 0.05f;
	F32 xs  = 0.01f,   ys  = 0.01f;
	F32 xs2 = xs*0.1f, ys2 = ys * 0.1f;

	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLGLSTexture gls_texture;
		LLDrawPool *poolp = *iter;
		if (poolp->getDebugTexture())
		{
			poolp->getDebugTexture()->bind();
			glColor4f(1,1,1,1);
			stamp(x,y,xs*4,ys*4);
		}

		LLGLSNoTexture no_texture;

		F32 a = 1.f - (F32)poolp->mRebuildTime / (F32)poolp->mRebuildFreq;
		glColor4f(a,a,a,1);
		stamp(x,y,xs,ys);

		x =  (xs + xs2) * 4.0f;

		F32 h = ys * 0.5f;

		S32 total = 0;
		
		for (std::vector<LLFace*>::iterator iter = poolp->mReferences.begin();
			 iter != poolp->mReferences.end(); iter++)
		{
			LLFace *face = *iter;
			F32        w = xs;

			if (!face || !face->getDrawable())
			{
				w = 16 / 3000.0f;

				stamp(x,y,w,h);

				if (x+w > 0.95f) 
				{
					x  = (xs + xs2) * 4.0f;
					y +=  h + ys2;
				}
				else
				{
					if (w) x += w + xs2;
				}

				continue;
			}

			if (face->getDrawable()->isVisible())
			{
				if (face->isState(LLFace::BACKLIST))
				{
					glColor4f(1,0,1,1);
				}
				else if (total > poolp->getMaxVertices())
				{
					glColor4f(1,0,0,1);
				}
				else
				{
					glColor4f(0,1,0,1);
					total += face->getGeomCount();
				}
			}
			else
			{
				if (face->isState(LLFace::BACKLIST))
				{
					glColor4f(0,0,1,1);
				}
				else
				{
					glColor4f(1,1,0,1);
				}
			}

			w = face->getGeomCount() / 3000.0f;

			stamp(x,y,w,h);

			if (x+w > 0.95f) 
			{
				x  = (xs + xs2) * 4.0f;
				y +=  h + ys2;
			}
			else
			{
				if (w) x += w + xs2;
			}
		}

		y   += ys + ys2;
		x    = 0;
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

static F32 xs  = 0.01f,   ys  = 0.01f;
static F32 xs2 = xs*0.1f, ys2 = ys * 0.1f;
static F32 winx, winy;

void displayDrawable(F32 &x, F32 &y, LLDrawable *drawable, F32 alpha = 0.5f)
{
	F32 w   = 0;
	F32 h   = ys * 0.5f;

	if (drawable && !drawable->isDead())
	{
		for (S32 f=0;f < drawable->getNumFaces(); f++)
		{
			w += drawable->getFace(f)->getGeomCount() / 30000.0f;
		}
		w+=xs;
		glColor4f(1,1,0, alpha);
	}
	else
	{
		w = 0.01f;
		glColor4f(0,0,0,alpha);
	}

//	const LLFontGL* font = gResMgr->getRes( LLFONT_SANSSERIF_SMALL );

//	U8 pcode = drawable->getVObj()->getPCode();

	//char *string = (char*)LLPrimitive::pCodeToString(pcode);
	//if (pcode == 0x3e)      string = "terrain";
	//else if (pcode == 0x2e) string = "cloud";
	//string[3] = 0;

	stamp(x * winx,y * winy,w * winx,h * winy);

	/*
	glColor4f(0,0,0,1);
	font->render(string,x*winx+1,y*winy+1);
	LLGLSNoTexture no_texture;
	*/

	if (x+w > 0.95f) 
	{
		x  = (xs + xs2) * 4.0f;
		y +=  h + ys2;
	}
	else
	{
		if (w) x += w + xs2;
	}

}

#if 0 // No longer up date

void displayQueue(F32 &x, F32 &y, LLDynamicArray<LLDrawable*>& processed, LLDynamicQueuePtr<LLPointer<LLDrawable> >& remaining)
{
	S32 i;
	for (i=0;i<processed.count();i++)
	{
		displayDrawable(x,y,processed[i],1);
	}

	x += xs * 2;

	S32 count = remaining.count();
	for (i=0;i<count;i++)
	{
		LLDrawable* drawablep = remaining[(i + remaining.getFirst()) % remaining.getMax()];
		if (drawablep && !drawablep->isDead())
		{
			displayDrawable(x,y,drawable,0.5);
		}
	}

	y  += ys * 4;
	x   = (xs + xs2) * 4.0f;

}

void LLPipeline::displayQueues()
{
	LLUI::setLineWidth(1.0);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	LLGLSPipelineAlpha gls_pipeline_alpha;
	LLGLSNoTexture no_texture;
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	glScalef(2,2,1);
	glTranslatef(-0.5f,-0.5f,0);

	winx = (F32)gViewerWindow->getWindowWidth();
	winy = (F32)gViewerWindow->getWindowHeight();

	glScalef(1.0f/winx,1.0f/winy,1);

	F32 x  = (xs + xs2) * 4.0f;
	F32 y = 0.1f;

	const LLFontGL* font = gResMgr->getRes( LLFONT_SANSSERIF );

	font->renderUTF8("Build1", 0,0,(S32)(y*winy),LLColor4(1,1,1,1));
	displayQueue(x,y, gBuildProcessed, mBuildQ1);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

}

#endif

// static
void render_bbox(const LLVector3 &min, const LLVector3 &max)
{
	S32 i;
	LLVector3 verticesp[16];

	verticesp[0].setVec(min.mV[0],min.mV[1],max.mV[2]);
	verticesp[1].setVec(min.mV[0],min.mV[1],min.mV[2]);
	verticesp[2].setVec(min.mV[0],max.mV[1],min.mV[2]);
	verticesp[3].setVec(min.mV[0],max.mV[1],max.mV[2]);
	verticesp[4].setVec(max.mV[0],max.mV[1],max.mV[2]);
	verticesp[5].setVec(max.mV[0],max.mV[1],min.mV[2]);
	verticesp[6].setVec(max.mV[0],min.mV[1],min.mV[2]);
	verticesp[7].setVec(max.mV[0],min.mV[1],max.mV[2]);
	verticesp[8 ] = verticesp[0];
	verticesp[9 ] = verticesp[1];
	verticesp[10] = verticesp[6];
	verticesp[11] = verticesp[7];
	verticesp[12] = verticesp[4];
	verticesp[13] = verticesp[5];
	verticesp[14] = verticesp[2];
	verticesp[15] = verticesp[3];

	LLGLSNoTexture gls_no_texture;
	{
		LLUI::setLineWidth(1.f);
		glBegin(GL_LINE_LOOP);
		for (i = 0; i < 16; i++)
		{
			glVertex3fv(verticesp[i].mV);
		}
		glEnd();
	}
	{
		LLGLDepthTest gls_depth(GL_TRUE);
		LLUI::setLineWidth(3.0f);
		glBegin(GL_LINE_LOOP);
		for (i = 0; i < 16; i++)
		{
			glVertex3fv(verticesp[i].mV);
		}
		glEnd();
	}
	LLUI::setLineWidth(1.0f);
}

//============================================================================
// Once-per-frame setup of hardware lights,
// including sun/moon, avatar backlight, and up to 6 local lights

void LLPipeline::setupAvatarLights(BOOL for_edit)
{
	const LLColor4 black(0,0,0,1);

	if (for_edit)
	{
		LLColor4 diffuse(0.8f, 0.8f, 0.8f, 0.f);
		LLVector4 light_pos_cam(-8.f, 0.25f, 10.f, 0.f);  // w==0 => directional light
		LLMatrix4 camera_mat = gCamera->getModelview();
		LLMatrix4 camera_rot(camera_mat.getMat3());
		camera_rot.invert();
		LLVector4 light_pos = light_pos_cam * camera_rot;
		
		light_pos.normVec();

		mHWLightColors[1] = diffuse;
		glLightfv(GL_LIGHT1, GL_DIFFUSE,  diffuse.mV);
		glLightfv(GL_LIGHT1, GL_AMBIENT,  black.mV);
		glLightfv(GL_LIGHT1, GL_SPECULAR, black.mV);
		glLightfv(GL_LIGHT1, GL_POSITION, light_pos.mV); 
		glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION,  1.0f);
		glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION, 	 0.0f);
		glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.0f);
		glLightf (GL_LIGHT1, GL_SPOT_EXPONENT, 		 0.0f);
		glLightf (GL_LIGHT1, GL_SPOT_CUTOFF, 			 180.0f);
	}
	else if (gAvatarBacklight) // Always true (unless overridden in a devs .ini)
	{
		LLVector3 opposite_pos = -1.f * mSunDir;
		LLVector3 orthog_light_pos = mSunDir % LLVector3::z_axis;
		LLVector4 backlight_pos = LLVector4(lerp(opposite_pos, orthog_light_pos, 0.3f), 0.0f);
		backlight_pos.normVec();
			
		LLColor4 light_diffuse = mSunDiffuse * mSunShadowFactor;
		LLColor4 backlight_diffuse(1.f - light_diffuse.mV[VRED], 1.f - light_diffuse.mV[VGREEN], 1.f - light_diffuse.mV[VBLUE], 1.f);
		F32 max_component = 0.001f;
		for (S32 i = 0; i < 3; i++)
		{
			if (backlight_diffuse.mV[i] > max_component)
			{
				max_component = backlight_diffuse.mV[i];
			}
		}
		F32 backlight_mag;
		if (gSky.getSunDirection().mV[2] >= NIGHTTIME_ELEVATION_COS)
		{
			backlight_mag = BACKLIGHT_DAY_MAGNITUDE_OBJECT;
		}
		else
		{
			backlight_mag = BACKLIGHT_NIGHT_MAGNITUDE_OBJECT;
		}
		backlight_diffuse *= backlight_mag / max_component;

		mHWLightColors[1] = backlight_diffuse;
		glLightfv(GL_LIGHT1, GL_POSITION, backlight_pos.mV); // this is just sun/moon direction
		glLightfv(GL_LIGHT1, GL_DIFFUSE,  backlight_diffuse.mV);
		glLightfv(GL_LIGHT1, GL_AMBIENT,  black.mV);
		glLightfv(GL_LIGHT1, GL_SPECULAR, black.mV);
		glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION,  1.0f);
		glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION,    0.0f);
		glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.0f);
		glLightf (GL_LIGHT1, GL_SPOT_EXPONENT,         0.0f);
		glLightf (GL_LIGHT1, GL_SPOT_CUTOFF,           180.0f);
	}
	else
	{
		mHWLightColors[1] = black;
		glLightfv(GL_LIGHT1, GL_DIFFUSE,  black.mV);
		glLightfv(GL_LIGHT1, GL_AMBIENT,  black.mV);
		glLightfv(GL_LIGHT1, GL_SPECULAR, black.mV);
	}
}

static F32 calc_light_dist(LLVOVolume* light, const LLVector3& cam_pos, F32 max_dist)
{
	F32 inten = light->getLightIntensity();
	if (inten < .001f)
	{
		return max_dist;
	}
	F32 radius = light->getLightRadius();
	BOOL selected = light->isSelected();
	LLVector3 dpos = light->getRenderPosition() - cam_pos;
	F32 dist2 = dpos.magVecSquared();
	if (!selected && dist2 > (max_dist + radius)*(max_dist + radius))
	{
		return max_dist;
	}
	F32 dist = fsqrtf(dist2);
	dist *= 1.f / inten;
	dist -= radius;
	if (selected)
	{
		dist -= 10000.f; // selected lights get highest priority
	}
	if (light->mDrawable.notNull() && light->mDrawable->isState(LLDrawable::ACTIVE))
	{
		// moving lights get a little higher priority (too much causes artifacts)
		dist -= light->getLightRadius()*0.25f;
	}
	return dist;
}

void LLPipeline::calcNearbyLights()
{
	if (mLightingDetail >= 1)
	{
		// mNearbyLight (and all light_set_t's) are sorted such that
		// begin() == the closest light and rbegin() == the farthest light
		const S32 MAX_LOCAL_LIGHTS = 6;
// 		LLVector3 cam_pos = gAgent.getCameraPositionAgent();
		LLVector3 cam_pos = gAgent.getPositionAgent();

		F32 max_dist = LIGHT_MAX_RADIUS * 4.f; // ignore enitrely lights > 4 * max light rad
		
		// UPDATE THE EXISTING NEARBY LIGHTS
		light_set_t cur_nearby_lights;
		for (light_set_t::iterator iter = mNearbyLights.begin();
			 iter != mNearbyLights.end(); iter++)
		{
			const Light* light = &(*iter);
			LLDrawable* drawable = light->drawable;
			LLVOVolume* volight = drawable->getVOVolume();
			if (!volight || !drawable->isState(LLDrawable::LIGHT))
			{
				drawable->clearState(LLDrawable::NEARBY_LIGHT);
				continue;
			}
			if (light->fade <= -LIGHT_FADE_TIME)
			{
				drawable->clearState(LLDrawable::NEARBY_LIGHT);
			}
			else
			{
				F32 dist = calc_light_dist(volight, cam_pos, max_dist);
				cur_nearby_lights.insert(Light(drawable, dist, light->fade));
			}
		}
		mNearbyLights = cur_nearby_lights;
		
		// FIND NEW LIGHTS THAT ARE IN RANGE
		light_set_t new_nearby_lights;
		for (LLDrawable::drawable_set_t::iterator iter = mLights.begin();
			 iter != mLights.end(); ++iter)
		{
			LLDrawable* drawable = *iter;
			LLVOVolume* light = drawable->getVOVolume();
			if (!light || drawable->isState(LLDrawable::NEARBY_LIGHT))
			{
				continue;
			}
			if (light->isHUDAttachment())
			{
				continue; // no lighting from HUD objects
			}
			F32 dist = calc_light_dist(light, cam_pos, max_dist);
			if (dist >= max_dist)
			{
				continue;
			}
			new_nearby_lights.insert(Light(drawable, dist, 0.f));
			if (new_nearby_lights.size() > (U32)MAX_LOCAL_LIGHTS)
			{
				new_nearby_lights.erase(--new_nearby_lights.end());
				const Light& last = *new_nearby_lights.rbegin();
				max_dist = last.dist;
			}
		}

		// INSERT ANY NEW LIGHTS
		for (light_set_t::iterator iter = new_nearby_lights.begin();
			 iter != new_nearby_lights.end(); iter++)
		{
			const Light* light = &(*iter);
			if (mNearbyLights.size() < (U32)MAX_LOCAL_LIGHTS)
			{
				mNearbyLights.insert(*light);
				((LLDrawable*) light->drawable)->setState(LLDrawable::NEARBY_LIGHT);
			}
			else
			{
				// crazy cast so that we can overwrite the fade value
				// even though gcc enforces sets as const
				// (fade value doesn't affect sort so this is safe)
				Light* farthest_light = ((Light*) (&(*(mNearbyLights.rbegin()))));
				if (light->dist < farthest_light->dist)
				{
					if (farthest_light->fade >= 0.f)
					{
						farthest_light->fade = -gFrameIntervalSeconds;
					}
				}
				else
				{
					break; // none of the other lights are closer
				}
			}
		}
		
	}
}

void LLPipeline::setupHWLights(LLDrawPool* pool)
{
	const LLColor4 black(0,0,0,1);

	setLightingDetail(-1); // update
	
	// Ambient
	LLColor4 ambient = gSky.getTotalAmbientColor();
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient.mV);

	// Light 0 = Sun or Moon (All objects)
	{
		mSunShadowFactor = 1.f; // no shadowing by defailt
		if (gSky.getSunDirection().mV[2] >= NIGHTTIME_ELEVATION_COS)
		{
			mSunDir.setVec(gSky.getSunDirection());
			mSunDiffuse.setVec(gSky.getSunDiffuseColor());
		}
		else
		{
			mSunDir.setVec(gSky.getMoonDirection());
			mSunDiffuse.setVec(gSky.getMoonDiffuseColor() * 1.5f);
		}

		F32 max_color = llmax(mSunDiffuse.mV[0], mSunDiffuse.mV[1], mSunDiffuse.mV[2]);
		if (max_color > 1.f)
		{
			mSunDiffuse *= 1.f/max_color;
		}
		mSunDiffuse.clamp();

		LLVector4 light_pos(mSunDir, 0.0f);
		LLColor4 light_diffuse = mSunDiffuse * mSunShadowFactor;
		mHWLightColors[0] = light_diffuse;
		glLightfv(GL_LIGHT0, GL_POSITION, light_pos.mV); // this is just sun/moon direction
		glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse.mV);
		glLightfv(GL_LIGHT0, GL_AMBIENT,  black.mV);
		glLightfv(GL_LIGHT0, GL_SPECULAR, black.mV);
		glLightf (GL_LIGHT0, GL_CONSTANT_ATTENUATION,  1.0f);
		glLightf (GL_LIGHT0, GL_LINEAR_ATTENUATION,    0.0f);
		glLightf (GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f);
		glLightf (GL_LIGHT0, GL_SPOT_EXPONENT,         0.0f);
		glLightf (GL_LIGHT0, GL_SPOT_CUTOFF,           180.0f);
	}
	
	// Light 1 = Backlight (for avatars)
	// (set by enableLightsAvatar)
	
	S32 cur_light = 2;
	
	// Nearby lights = LIGHT 2-7

	mLightMovingMask = 0;
	
	if (mLightingDetail >= 1)
	{
		for (light_set_t::iterator iter = mNearbyLights.begin();
			 iter != mNearbyLights.end(); ++iter)
		{
			LLDrawable* drawable = iter->drawable;
			LLVOVolume* light = drawable->getVOVolume();
			if (!light)
			{
				continue;
			}
			if (drawable->isState(LLDrawable::ACTIVE))
			{
				mLightMovingMask |= (1<<cur_light);
			}
			
			LLColor4  light_color = light->getLightColor();
			light_color.mV[3] = 0.0f;

			F32 fade = iter->fade;
			if (fade < LIGHT_FADE_TIME)
			{
				// fade in/out light
				if (fade >= 0.f)
				{
					fade = fade / LIGHT_FADE_TIME;
					((Light*) (&(*iter)))->fade += gFrameIntervalSeconds;
				}
				else
				{
					fade = 1.f + fade / LIGHT_FADE_TIME;
					((Light*) (&(*iter)))->fade -= gFrameIntervalSeconds;
				}
				fade = llclamp(fade,0.f,1.f);
				light_color *= fade;
			}

			LLVector3 light_pos(light->getRenderPosition());
			LLVector4 light_pos_gl(light_pos, 1.0f);
	
			F32 light_radius = llmax(light->getLightRadius(), 0.001f);
			F32 atten, quad;

#if 0 //1.9.1
			if (pool->getVertexShaderLevel() > 0)
			{
				atten = light_radius;
				quad = llmax(light->getLightFalloff(), 0.0001f);
			}
			else
#endif
			{
				F32 x = (3.f * (1.f + light->getLightFalloff()));
				atten = x / (light_radius); // % of brightness at radius
				quad = 0.0f;
			}
			mHWLightColors[cur_light] = light_color;
			S32 gllight = GL_LIGHT0+cur_light;
			glLightfv(gllight, GL_POSITION, light_pos_gl.mV);
			glLightfv(gllight, GL_DIFFUSE,  light_color.mV);
			glLightfv(gllight, GL_AMBIENT,  black.mV);
			glLightfv(gllight, GL_SPECULAR, black.mV);
			glLightf (gllight, GL_CONSTANT_ATTENUATION,   0.0f);
			glLightf (gllight, GL_LINEAR_ATTENUATION,     atten);
			glLightf (gllight, GL_QUADRATIC_ATTENUATION,  quad);
			glLightf (gllight, GL_SPOT_EXPONENT,          0.0f);
			glLightf (gllight, GL_SPOT_CUTOFF,            180.0f);
			cur_light++;
			if (cur_light >= 8)
			{
				break; // safety
			}
		}
	}
	for ( ; cur_light < 8 ; cur_light++)
	{
		mHWLightColors[cur_light] = black;
		S32 gllight = GL_LIGHT0+cur_light;
		glLightfv(gllight, GL_DIFFUSE,  black.mV);
		glLightfv(gllight, GL_AMBIENT,  black.mV);
		glLightfv(gllight, GL_SPECULAR, black.mV);
	}

	// Init GL state
	glDisable(GL_LIGHTING);
	for (S32 gllight=GL_LIGHT0; gllight<=GL_LIGHT7; gllight++)
	{
		glDisable(gllight);
	}
	mLightMask = 0;
}

void LLPipeline::enableLights(U32 mask, F32 shadow_factor)
{
	if (mLightingDetail == 0)
	{
		mask &= 0xf003; // sun and backlight only (and fullbright bit)
	}
	if (mLightMask != mask)
	{
		if (!mLightMask)
		{
			glEnable(GL_LIGHTING);
		}
		if (mask)
		{
			for (S32 i=0; i<8; i++)
			{
				if (mask & (1<<i))
				{
					glEnable(GL_LIGHT0 + i);
					glLightfv(GL_LIGHT0 + i, GL_DIFFUSE,  mHWLightColors[i].mV);
				}
				else
				{
					glDisable(GL_LIGHT0 + i);
					glLightfv(GL_LIGHT0 + i, GL_DIFFUSE,  LLColor4::black.mV);
				}
			}
		}
		else
		{
			glDisable(GL_LIGHTING);
		}
		mLightMask = mask;
		LLColor4 ambient = gSky.getTotalAmbientColor();
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient.mV);
	}
}

void LLPipeline::enableLightsStatic(F32 shadow_factor)
{
	U32 mask = 0x01; // Sun
	if (mLightingDetail >= 2)
	{
		mask |= mLightMovingMask; // Hardware moving lights
		glColor4f(0.f, 0.f, 0.f, 1.0f); // no local lighting by default
	}
	else
	{
		mask |= 0xff & (~2); // Hardware local lights
	}
	enableLights(mask, shadow_factor);
}

void LLPipeline::enableLightsDynamic(F32 shadow_factor)
{
	U32 mask = 0xff & (~2); // Local lights
	enableLights(mask, shadow_factor);
	if (mLightingDetail >= 2)
	{
		glColor4f(0.f, 0.f, 0.f, 1.f); // no local lighting by default
	}
}

void LLPipeline::enableLightsAvatar(F32 shadow_factor)
{
	U32 mask = 0xff; // All lights
	setupAvatarLights(FALSE);
	enableLights(mask, shadow_factor);
}

void LLPipeline::enableLightsAvatarEdit(const LLColor4& color)
{
	U32 mask = 0x2002; // Avatar backlight only, set ambient
	setupAvatarLights(TRUE);
	enableLights(mask, 1.0f);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,color.mV);
}

void LLPipeline::enableLightsFullbright(const LLColor4& color)
{
	U32 mask = 0x1000; // Non-0 mask, set ambient
	enableLights(mask, 1.f);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,color.mV);
	if (mLightingDetail >= 2)
	{
		glColor4f(0.f, 0.f, 0.f, 1.f); // no local lighting by default
	}
}

void LLPipeline::disableLights()
{
	enableLights(0, 0.f); // no lighting (full bright)
	glColor4f(1.f, 1.f, 1.f, 1.f); // lighting color = white by default
}

// Call *after*s etting up lights
void LLPipeline::setAmbient(const LLColor4& ambient)
{
	mLightMask |= 0x4000; // tweak mask so that ambient will get reset
	LLColor4 amb = ambient + gSky.getTotalAmbientColor();
	amb.clamp();
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,amb.mV);
}

//============================================================================

class LLMenuItemGL;
class LLInvFVBridge;
struct cat_folder_pair;
class LLVOBranch;
class LLVOLeaf;
class Foo;

template<> char* LLAGPArray<U8>::sTypeName               = "U8        [AGP]";
template<> char* LLAGPArray<U32>::sTypeName              = "U32       [AGP]";
template<> char* LLAGPArray<F32>::sTypeName              = "F32       [AGP]";
template<> char* LLAGPArray<LLColor4>::sTypeName         = "LLColor4  [AGP]";
template<> char* LLAGPArray<LLColor4U>::sTypeName        = "LLColor4U [AGP]";
template<> char* LLAGPArray<LLVector4>::sTypeName        = "LLVector4 [AGP]";
template<> char* LLAGPArray<LLVector3>::sTypeName        = "LLVector3 [AGP]";
template<> char* LLAGPArray<LLVector2>::sTypeName        = "LLVector2 [AGP]";
template<> char* LLAGPArray<LLFace*>::sTypeName          = "LLFace*   [AGP]";

void scale_stamp(const F32 x, const F32 y, const F32 xs, const F32 ys)
{
	stamp(0.25f + 0.5f*x,
		  0.5f + 0.45f*y,
		  0.5f*xs,
		  0.45f*ys);
}

void drawBars(const F32 begin, const F32 end, const F32 height = 1.f)
{
	if (begin >= 0 && end <=1)
	{
		F32 lines  = 40.0f;
		S32 ibegin = (S32)(begin * lines);
		S32 iend   = (S32)(end   * lines);
		F32 fbegin = begin * lines - ibegin;
		F32 fend   = end   * lines - iend;

		F32 line_height = height/lines;

		if (iend == ibegin)
		{
			scale_stamp(fbegin, (F32)ibegin/lines,fend-fbegin, line_height);
		}
		else
		{
			// Beginning row
			scale_stamp(fbegin, (F32)ibegin/lines,  1.0f-fbegin, line_height);

			// End row
			scale_stamp(0.0,    (F32)iend/lines,  fend, line_height);

			// Middle rows
			for (S32 l = (ibegin+1); l < iend; l++)
			{
				scale_stamp(0.0f, (F32)l/lines, 1.0f, line_height);
			}
		}
	}
}

void LLPipeline::displayAGP()
{
	LLUI::setLineWidth(1.0);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	LLGLSPipelineAlpha gls_alpha;
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
	glScalef(2,2,1);
	glTranslatef(-0.5f,-0.5f,0);

	glColor4f(0,0,0,0.5f);
	scale_stamp(0,0,1,1);

	F32 x   = 0.0f,    y   = 0.05f;
	F32 xs  = 0.015f,   ys  = 0.015f;
	F32 xs2 = xs*0.1f, ys2 = ys * 0.1f;
	F32 w   = xs+xs2, h    = ys + ys2;
	S32 i=0;

	F32 agp_size = 1.f;
	if (mAGPMemPool)
	{
		agp_size = (F32)mAGPMemPool->getSize();
	}

	x  = (xs + xs2) * 4.0f;
	
	static float c = 0.0f;
	c += 0.0001f;

	LLAGPMemBlock *blockp = mBufferMemory[mBufferIndex]->getAGPMemBlock();

	pool_set_t::iterator iter = mPools.begin();
	LLDrawPool *poolp = *iter;
	
	// Dump the shared AGP buffer
	if (blockp)
	{
		F32 begin = blockp->getOffset()/agp_size;
		F32 end   = begin + (blockp->getSize()/agp_size);
		F32 used = begin + (poolp->mMemory.count()/agp_size);

		LLViewerImage::bindTexture(NULL);
		glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		drawBars(begin,end);

		LLViewerImage::bindTexture(NULL);
		glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		drawBars(begin,end);

		LLViewerImage::bindTexture(NULL);
		glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		drawBars(begin,used, 0.5f);

		glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
		drawBars(begin,end, 0.25f);
	}

	S32 used_bytes = 0;
	S32 total_bytes = 0;
	while (iter != mPools.end())
	{
		poolp = *iter++;

		BOOL synced = FALSE;
		i++;
		total_bytes += poolp->mMemory.getMax();
		used_bytes += poolp->mMemory.count();
		LLViewerImage *texturep = poolp->getDebugTexture();


		if (poolp->mMemory.mSynced)
		{
			poolp->mMemory.mSynced = FALSE;
			synced = TRUE;
		}

		LLAGPMemBlock *blockp = poolp->mMemory.getAGPMemBlock();
		if (blockp)
		{
			F32 begin = blockp->getOffset()/agp_size;
			F32 end   = begin + (blockp->getSize()/agp_size);
			F32 used = begin + (poolp->mMemory.count()/agp_size);

			LLViewerImage::bindTexture(NULL);
			glColor4f(1.f, 0.5f, 0.5f, 0.5f);
			drawBars(begin,end);

			LLViewerImage::bindTexture(texturep);
			glColor4f(1.f, 0.75f, 0.75f, 0.5f);
			drawBars(begin,end);

			LLViewerImage::bindTexture(NULL);
			glColor4f(1.f, 0.75f, 0.75f, 1.f);
			drawBars(begin,used, 0.5f);

			glColor3fv(poolp->getDebugColor().mV);
			drawBars(begin,end, 0.25f);

			if (synced)
			{
				LLViewerImage::bindTexture(NULL);
				glColor4f(1.f, 1.f, 1.f, 0.4f);
				drawBars(begin,end);
			}
		}

		synced = FALSE;
		if (poolp->mWeights.mSynced)
		{
			poolp->mWeights.mSynced = FALSE;
			synced = TRUE;
		}
		blockp = poolp->mWeights.getAGPMemBlock();
		if (blockp)
		{
			F32 begin = blockp->getOffset()/agp_size;
			F32 end   = begin + (blockp->getSize()/agp_size);
			F32 used = begin + (poolp->mWeights.count()*sizeof(float)/agp_size);

			LLViewerImage::bindTexture(NULL);
			glColor4f(0.0f, 0.f, 0.75f, 0.5f);
			drawBars(begin,end);

			LLViewerImage::bindTexture(texturep);
			glColor4f(0.0, 0.f, 0.75f, 0.5f);
			drawBars(begin,end);

			LLViewerImage::bindTexture(NULL);
			glColor4f(0.0, 0.f, 0.75f, 1.f);
			drawBars(begin,used, 0.5f);

			LLViewerImage::bindTexture(NULL);
			glColor3fv(poolp->getDebugColor().mV);
			drawBars(begin,end, 0.25f);

			if (synced)
			{
				LLViewerImage::bindTexture(NULL);
				glColor4f(1.f, 1.f, 1.f, 0.4f);
				drawBars(begin,end);
			}
		}

		synced = FALSE;
		if (poolp->mClothingWeights.mSynced)
		{
			poolp->mClothingWeights.mSynced = FALSE;
			synced = TRUE;
		}
		blockp = poolp->mClothingWeights.getAGPMemBlock();
		if (blockp)
		{
			F32 begin = blockp->getOffset()/agp_size;
			F32 end   = begin + (blockp->getSize()/agp_size);
			F32 used = begin + (poolp->mClothingWeights.count()*sizeof(LLVector4)/agp_size);

			LLViewerImage::bindTexture(NULL);
			glColor4f(0.75f, 0.f, 0.75f, 0.5f);
			drawBars(begin,end);

			LLViewerImage::bindTexture(texturep);
			glColor4f(0.75f, 0.f, 0.75f, 0.5f);
			drawBars(begin,end);

			LLViewerImage::bindTexture(NULL);
			glColor4f(0.75f, 0.f, 0.75f, 0.5f);
			drawBars(begin,used, 0.5f);

			LLViewerImage::bindTexture(NULL);
			glColor3fv(poolp->getDebugColor().mV);
			drawBars(begin,end, 0.25f);

			if (synced)
			{
				LLViewerImage::bindTexture(NULL);
				glColor4f(1.f, 1.f, 1.f, 0.5f);
				drawBars(begin,end);
			}
		}

		//
		// Stamps on bottom of screen
		//
		LLViewerImage::bindTexture(texturep);
		glColor4f(1.f, 1.f, 1.f, 1.f);
		stamp(x,y,xs,ys);

		LLViewerImage::bindTexture(NULL);
		glColor3fv(poolp->getDebugColor().mV);
		stamp(x,y,xs, ys*0.25f);
		if (x+w > 0.95f) 
		{
			x  = (xs + xs2) * 4.0f;
			y +=  h;
		}
		else
		{
			x += w + xs2;
		}
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void LLPipeline::findReferences(LLDrawable *drawablep)
{
	if (std::find(mVisibleList.begin(), mVisibleList.end(), drawablep) != mVisibleList.end())
	{
		llinfos << "In mVisibleList" << llendl;
	}
	if (mLights.find(drawablep) != mLights.end())
	{
		llinfos << "In mLights" << llendl;
	}
	if (mMovedList.find(drawablep) != mMovedList.end())
	{
		llinfos << "In mMovedList" << llendl;
	}
	if (std::find(mShiftList.begin(), mShiftList.end(), drawablep) != mShiftList.end())
	{
		llinfos << "In mShiftList" << llendl;
	}
	if (mRetexturedList.find(drawablep) != mRetexturedList.end())
	{
		llinfos << "In mRetexturedList" << llendl;
	}
	if (mRematerialedList.find(drawablep) != mRematerialedList.end())
	{
		llinfos << "In mRematerialedList" << llendl;
	}

	if (mActiveQ.find(drawablep) != mActiveQ.end())
	{
		llinfos << "In mActiveQ" << llendl;
	}
	if (mBuildQ1.find(drawablep) != mBuildQ1.end())
	{
		llinfos << "In mBuildQ2" << llendl;
	}
	if (std::find(mBuildQ2.begin(), mBuildQ2.end(), drawablep) != mBuildQ2.end())
	{
		llinfos << "In mBuildQ2" << llendl;
	}

	S32 count;
	/*
	count = mStaticTree->count(drawablep);
	if (count)
	{
		llinfos << "In mStaticTree: " << count << " references" << llendl;
	}

	count = mDynamicTree->count(drawablep);
	if (count)
	{
		llinfos << "In mStaticTree: " << count << " references" << llendl;
	}
	*/
	count = gObjectList.findReferences(drawablep);
	if (count)
	{
		llinfos << "In other drawables: " << count << " references" << llendl;
	}
}

BOOL LLPipeline::verify()
{
	BOOL ok = TRUE;
	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		if (!poolp->verify())
		{
			ok = FALSE;
		}
	}

	if (!ok)
	{
		llwarns << "Pipeline verify failed!" << llendl;
	}
	return ok;
}

S32 LLPipeline::getAGPMemUsage()
{
	if (mAGPMemPool)
	{
		return mAGPMemPool->getSize();
	}
	else
	{
		return 0;
	}
}

S32 LLPipeline::getMemUsage(const BOOL print)
{
	S32 mem_usage = 0;

	if (mAGPMemPool)
	{
		S32 agp_usage = 0;
		agp_usage = mAGPMemPool->getSize();
		if (print)
		{
			llinfos << "AGP Mem: " << agp_usage << llendl;
			llinfos << "AGP Mem used: " << mAGPMemPool->getTotalAllocated() << llendl;
		}
		mem_usage += agp_usage;
	}


	S32 pool_usage = 0;
	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		pool_usage += poolp->getMemUsage(print);
	}

	if (print)
	{
		llinfos << "Pool Mem: " << pool_usage << llendl;
	}

	mem_usage += pool_usage;

	return mem_usage;

}

//////////////////////////////
//
// Collision detection
//
//

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	A method to compute a ray-AABB intersection.
 *	Original code by Andrew Woo, from "Graphics Gems", Academic Press, 1990
 *	Optimized code by Pierre Terdiman, 2000 (~20-30% faster on my Celeron 500)
 *	Epsilon value added by Klaus Hartmann. (discarding it saves a few cycles only)
 *
 *	Hence this version is faster as well as more robust than the original one.
 *
 *	Should work provided:
 *	1) the integer representation of 0.0f is 0x00000000
 *	2) the sign bit of the float is the most significant one
 *
 *	Report bugs: p.terdiman@codercorner.com
 *
 *	\param		aabb		[in] the axis-aligned bounding box
 *	\param		origin		[in] ray origin
 *	\param		dir			[in] ray direction
 *	\param		coord		[out] impact coordinates
 *	\return		true if ray intersects AABB
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define RAYAABB_EPSILON 0.00001f
#define IR(x)	((U32&)x)

bool LLRayAABB(const LLVector3 &center, const LLVector3 &size, const LLVector3& origin, const LLVector3& dir, LLVector3 &coord, F32 epsilon)
{
	BOOL Inside = TRUE;
	LLVector3 MinB = center - size;
	LLVector3 MaxB = center + size;
	LLVector3 MaxT;
	MaxT.mV[VX]=MaxT.mV[VY]=MaxT.mV[VZ]=-1.0f;

	// Find candidate planes.
	for(U32 i=0;i<3;i++)
	{
		if(origin.mV[i] < MinB.mV[i])
		{
			coord.mV[i]	= MinB.mV[i];
			Inside		= FALSE;

			// Calculate T distances to candidate planes
			if(IR(dir.mV[i]))	MaxT.mV[i] = (MinB.mV[i] - origin.mV[i]) / dir.mV[i];
		}
		else if(origin.mV[i] > MaxB.mV[i])
		{
			coord.mV[i]	= MaxB.mV[i];
			Inside		= FALSE;

			// Calculate T distances to candidate planes
			if(IR(dir.mV[i]))	MaxT.mV[i] = (MaxB.mV[i] - origin.mV[i]) / dir.mV[i];
		}
	}

	// Ray origin inside bounding box
	if(Inside)
	{
		coord = origin;
		return true;
	}

	// Get largest of the maxT's for final choice of intersection
	U32 WhichPlane = 0;
	if(MaxT.mV[1] > MaxT.mV[WhichPlane])	WhichPlane = 1;
	if(MaxT.mV[2] > MaxT.mV[WhichPlane])	WhichPlane = 2;

	// Check final candidate actually inside box
	if(IR(MaxT.mV[WhichPlane])&0x80000000) return false;

	for(U32 i=0;i<3;i++)
	{
		if(i!=WhichPlane)
		{
			coord.mV[i] = origin.mV[i] + MaxT.mV[WhichPlane] * dir.mV[i];
			if (epsilon > 0)
			{
				if(coord.mV[i] < MinB.mV[i] - epsilon || coord.mV[i] > MaxB.mV[i] + epsilon)	return false;
			}
			else
			{
				if(coord.mV[i] < MinB.mV[i] || coord.mV[i] > MaxB.mV[i])	return false;
			}
		}
	}
	return true;	// ray hits box
}

//////////////////////////////
//
// Macros, functions, and inline methods from other classes
//
//

void LLPipeline::setLight(LLDrawable *drawablep, BOOL is_light)
{
	if (drawablep)
	{
		if (is_light)
		{
			mLights.insert(drawablep);
			drawablep->setState(LLDrawable::LIGHT);
		}
		else
		{
			mLights.erase(drawablep);
			drawablep->clearState(LLDrawable::LIGHT);
		}
		markRelight(drawablep);
	}
}

void LLPipeline::setActive(LLDrawable *drawablep, BOOL active)
{
	if (active)
	{
		mActiveQ.insert(drawablep);
	}
	else
	{
		mActiveQ.erase(drawablep);
	}
}

//static
void LLPipeline::toggleRenderType(void* data)
{
	S32 type = (S32)(intptr_t)data;
	U32 bit = (1<<type);
	if (gPipeline.hasRenderType(type))
	{
		llinfos << "Toggling render type mask " << std::hex << bit << " off" << std::dec << llendl;
	}
	else
	{
		llinfos << "Toggling render type mask " << std::hex << bit << " on" << std::dec << llendl;
	}
	gPipeline.mRenderTypeMask ^= bit;
}

//static
BOOL LLPipeline::toggleRenderTypeControl(void* data)
{
	S32 type = (S32)(intptr_t)data;
	return gPipeline.hasRenderType(type);
}

// Allows UI items labeled "Hide foo" instead of "Show foo"
//static
BOOL LLPipeline::toggleRenderTypeControlNegated(void* data)
{
	S32 type = (S32)(intptr_t)data;
	return !gPipeline.hasRenderType(type);
}

//static
void LLPipeline::toggleRenderDebug(void* data)
{
	U32 bit = (U32)(intptr_t)data;
	if (gPipeline.hasRenderDebugMask(bit))
	{
		llinfos << "Toggling render debug mask " << std::hex << bit << " off" << std::dec << llendl;
	}
	else
	{
		llinfos << "Toggling render debug mask " << std::hex << bit << " on" << std::dec << llendl;
	}
	gPipeline.mRenderDebugMask ^= bit;
}


//static
BOOL LLPipeline::toggleRenderDebugControl(void* data)
{
	U32 bit = (U32)(intptr_t)data;
	return gPipeline.hasRenderDebugMask(bit);
}

//static
void LLPipeline::toggleRenderDebugFeature(void* data)
{
	U32 bit = (U32)(intptr_t)data;
	if (gPipeline.hasRenderDebugFeatureMask(bit))
	{
		llinfos << "Toggling render debug feature mask " << std::hex << bit << " off" << std::dec << llendl;
	}
	else
	{
		llinfos << "Toggling render debug feature mask " << std::hex << bit << " on" << std::dec << llendl;
	}
	gPipeline.mRenderDebugFeatureMask ^= bit;
}


//static
BOOL LLPipeline::toggleRenderDebugFeatureControl(void* data)
{
	U32 bit = (U32)(intptr_t)data;
	return gPipeline.hasRenderDebugFeatureMask(bit);
}

// static
void LLPipeline::toggleRenderScriptedBeacons(void*)
{
	sRenderScriptedBeacons = !sRenderScriptedBeacons;
}

// static
BOOL LLPipeline::getRenderScriptedBeacons(void*)
{
	return sRenderScriptedBeacons;
}

// static
void LLPipeline::toggleRenderPhysicalBeacons(void*)
{
	sRenderPhysicalBeacons = !sRenderPhysicalBeacons;
}

// static
BOOL LLPipeline::getRenderPhysicalBeacons(void*)
{
	return sRenderPhysicalBeacons;
}

// static
void LLPipeline::toggleRenderParticleBeacons(void*)
{
	sRenderParticleBeacons = !sRenderParticleBeacons;
}

// static
BOOL LLPipeline::getRenderParticleBeacons(void*)
{
	return sRenderParticleBeacons;
}

// static
void LLPipeline::toggleRenderSoundBeacons(void*)
{
	sRenderSoundBeacons = !sRenderSoundBeacons;
}

// static
BOOL LLPipeline::getRenderSoundBeacons(void*)
{
	return sRenderSoundBeacons;
}

//===============================
// LLGLSL Shader implementation
//===============================
LLGLSLShader::LLGLSLShader()
: mProgramObject(0)
{ }

void LLGLSLShader::unload()
{
	stop_glerror();
	mAttribute.clear();
	mTexture.clear();
	mUniform.clear();

	if (mProgramObject)
	{
		GLhandleARB obj[1024];
		GLsizei count;

		glGetAttachedObjectsARB(mProgramObject, 1024, &count, obj);
		for (GLsizei i = 0; i < count; i++)
		{
			glDeleteObjectARB(obj[i]);
		}

		glDeleteObjectARB(mProgramObject);

		mProgramObject = 0;
	}
	
	//hack to make apple not complain
	glGetError();
	
	stop_glerror();
}

void LLGLSLShader::attachObject(GLhandleARB object)
{
	if (object != 0)
	{
		stop_glerror();
		glAttachObjectARB(mProgramObject, object);
		stop_glerror();
	}
	else
	{
		llwarns << "Attempting to attach non existing shader object. " << llendl;
	}
}

void LLGLSLShader::attachObjects(GLhandleARB* objects, S32 count)
{
	for (S32 i = 0; i < count; i++)
	{
		attachObject(objects[i]);
	}
}

BOOL LLGLSLShader::mapAttributes(const char** attrib_names, S32 count)
{
	//link the program
	BOOL res = link();

	mAttribute.clear();
	mAttribute.resize(LLPipeline::sReservedAttribCount + count, -1);
	
	if (res)
	{ //read back channel locations

		//read back reserved channels first
		for (S32 i = 0; i < (S32) LLPipeline::sReservedAttribCount; i++)
		{
			const char* name = LLPipeline::sReservedAttribs[i];
			S32 index = glGetAttribLocationARB(mProgramObject, name);
			if (index != -1)
			{
				mAttribute[i] = index;
				llinfos << "Attribute " << name << " assigned to channel " << index << llendl;
			}
		}

		for (S32 i = 0; i < count; i++)
		{
			const char* name = attrib_names[i];
			S32 index = glGetAttribLocationARB(mProgramObject, name);
			if (index != -1)
			{
				mAttribute[LLPipeline::sReservedAttribCount + i] = index;
				llinfos << "Attribute " << name << " assigned to channel " << index << llendl;
			}
		}

		return TRUE;
	}
	
	return FALSE;
}

void LLGLSLShader::mapUniform(GLint index, const char** uniform_names, S32 count)
{
	if (index == -1)
	{
		return;
	}

	GLenum type;
	GLsizei length;
	GLint size;
	char name[1024];
	name[0] = 0;

	glGetActiveUniformARB(mProgramObject, index, 1024, &length, &size, &type, name);
	
	//find the index of this uniform
	for (S32 i = 0; i < (S32) LLPipeline::sReservedUniformCount; i++)
	{
		if (mUniform[i] == -1 && !strncmp(LLPipeline::sReservedUniforms[i],name, strlen(LLPipeline::sReservedUniforms[i])))
		{
			//found it
			S32 location = glGetUniformLocationARB(mProgramObject, name);
			mUniform[i] = location;
			llinfos << "Uniform " << name << " is at location " << location << llendl;
			mTexture[i] = mapUniformTextureChannel(location, type);
			return;
		}
	}

	for (S32 i = 0; i < count; i++)
	{
		if (mUniform[i+LLPipeline::sReservedUniformCount] == -1 && 
			!strncmp(uniform_names[i],name, strlen(uniform_names[i])))
		{
			//found it
			S32 location = glGetUniformLocationARB(mProgramObject, name);
			mUniform[i+LLPipeline::sReservedUniformCount] = location;
			llinfos << "Uniform " << name << " is at location " << location << " stored in index " << 
				(i+LLPipeline::sReservedUniformCount) << llendl;
			mTexture[i+LLPipeline::sReservedUniformCount] = mapUniformTextureChannel(location, type);
			return;
		}
	}

	//llinfos << "Unknown uniform: " << name << llendl;
 }

GLint LLGLSLShader::mapUniformTextureChannel(GLint location, GLenum type)
{
	if (type >= GL_SAMPLER_1D_ARB && type <= GL_SAMPLER_2D_RECT_SHADOW_ARB)
	{	//this here is a texture
		glUniform1iARB(location, mActiveTextureChannels);
		llinfos << "Assigned to texture channel " << mActiveTextureChannels << llendl;
		return mActiveTextureChannels++;
	}
	return -1;
}

BOOL LLGLSLShader::mapUniforms(const char** uniform_names,  S32 count)
{
	BOOL res = TRUE;
	
	mActiveTextureChannels = 0;
	mUniform.clear();
	mTexture.clear();

	//initialize arrays
	mUniform.resize(count + LLPipeline::sReservedUniformCount, -1);
	mTexture.resize(count + LLPipeline::sReservedUniformCount, -1);
	


	bind();

	//get the number of active uniforms
	GLint activeCount;
	glGetObjectParameterivARB(mProgramObject, GL_OBJECT_ACTIVE_UNIFORMS_ARB, &activeCount);

	for (S32 i = 0; i < activeCount; i++)
	{
		mapUniform(i, uniform_names, count);
	}
	
	unbind();

	return res;
}

BOOL LLGLSLShader::link(BOOL suppress_errors)
{
	return gPipeline.linkProgramObject(mProgramObject, suppress_errors);
}

void LLGLSLShader::bind()
{
	glUseProgramObjectARB(mProgramObject);
	if (mAttribute.size() > 0)
	{
		gPipeline.mMaterialIndex = mAttribute[0];
	}
}

void LLGLSLShader::unbind()
{
	for (U32 i = 0; i < mAttribute.size(); ++i)
	{
		vertexAttrib4f(i, 0,0,0,1);
	}
	glUseProgramObjectARB(0);
}

S32 LLGLSLShader::enableTexture(S32 uniform, S32 mode)
{
	if (uniform < 0 || uniform >= (S32)mTexture.size())
	{
		llerrs << "LLGLSLShader::enableTexture: uniform out of range: " << uniform << llendl;
	}
	S32 index = mTexture[uniform];
	if (index != -1)
	{
		glActiveTextureARB(GL_TEXTURE0_ARB+index);
		glEnable(mode);
	}
	return index;
}

S32 LLGLSLShader::disableTexture(S32 uniform, S32 mode)
{
	S32 index = mTexture[uniform];
	if (index != -1)
	{
		glActiveTextureARB(GL_TEXTURE0_ARB+index);
		glDisable(mode);
	}
	return index;
}

void LLGLSLShader::vertexAttrib4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	if (mAttribute[index] > 0)
	{
		glVertexAttrib4fARB(mAttribute[index], x, y, z, w);
	}
}

void LLGLSLShader::vertexAttrib4fv(U32 index, GLfloat* v)
{
	if (mAttribute[index] > 0)
	{
		glVertexAttrib4fvARB(mAttribute[index], v);
	}
}

LLViewerObject* LLPipeline::pickObject(const LLVector3 &start, const LLVector3 &end, LLVector3 &collision)
{
	LLDrawable* drawable = mObjectPartition->pickDrawable(start, end, collision);
	return drawable ? drawable->getVObj() : NULL;
}

	
