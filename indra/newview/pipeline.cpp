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
#include "llerror.h"
#include "llviewercontrol.h"
#include "llfasttimer.h"
#include "llfontgl.h"
#include "llmemory.h"
#include "llmemtype.h"
#include "llnamevalue.h"
#include "llprimitive.h"
#include "llvolume.h"
#include "material_codes.h"
#include "timing.h"
#include "v3color.h"
#include "llui.h"
#include "llglheaders.h"

// newview includes
#include "llagent.h"
#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolground.h"
#include "lldrawpoolsimple.h"
#include "lldrawpoolbump.h"
#include "lldrawpooltree.h"
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
#include "llvosurfacepatch.h"
#include "llvowater.h"
#include "llvotree.h"
#include "llvopartgroup.h"
#include "llworld.h"
#include "viewer.h"
#include "llcubemap.h"

#ifdef _DEBUG
// Debug indices is disabled for now for debug performance - djs 4/24/02
//#define DEBUG_INDICES
#else
//#define DEBUG_INDICES
#endif

#define AGGRESSIVE_OCCLUSION 0

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

S32		gTrivialAccepts = 0;

BOOL	gRenderForSelect = FALSE;

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

const char* LLPipeline::sShinyUniforms[] = 
{
	"origin"
};

U32 LLPipeline::sShinyUniformCount = sizeof(LLPipeline::sShinyUniforms)/sizeof(char*);

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

BOOL	LLPipeline::sShowHUDAttachments = TRUE;
BOOL	LLPipeline::sRenderPhysicalBeacons = FALSE;
BOOL	LLPipeline::sRenderScriptedBeacons = FALSE;
BOOL	LLPipeline::sRenderParticleBeacons = FALSE;
BOOL	LLPipeline::sRenderSoundBeacons = FALSE;
BOOL	LLPipeline::sUseOcclusion = FALSE;
BOOL	LLPipeline::sSkipUpdate = FALSE;
BOOL	LLPipeline::sDynamicReflections = FALSE;

LLPipeline::LLPipeline() :
	mCubeBuffer(NULL),
	mCubeList(0),
	mVertexShadersEnabled(FALSE),
	mVertexShadersLoaded(0),
	mLastRebuildPool(NULL),
	mAlphaPool(NULL),
	mAlphaPoolPostWater(NULL),
	mSkyPool(NULL),
	mStarsPool(NULL),
	mTerrainPool(NULL),
	mWaterPool(NULL),
	mGroundPool(NULL),
	mSimplePool(NULL),
	mBumpPool(NULL),
	mLightMask(0),
	mLightMovingMask(0)
{

}

void LLPipeline::init()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	
	stop_glerror();

	//create object partitions
	//MUST MATCH declaration of eObjectPartitions
	mObjectPartition.push_back(new LLVolumePartition());	//PARTITION_VOLUME
	mObjectPartition.push_back(new LLBridgePartition());	//PARTITION_BRIDGE
	mObjectPartition.push_back(new LLHUDPartition());		//PARTITION_HUD
	mObjectPartition.push_back(new LLTerrainPartition());	//PARTITION_TERRAIN
	mObjectPartition.push_back(new LLWaterPartition());		//PARTITION_WATER
	mObjectPartition.push_back(new LLTreePartition());		//PARTITION_TREE
	mObjectPartition.push_back(new LLParticlePartition());	//PARTITION_PARTICLE
	mObjectPartition.push_back(new LLCloudPartition());		//PARTITION_CLOUD
	mObjectPartition.push_back(new LLGrassPartition());		//PARTITION_GRASS
	mObjectPartition.push_back(NULL);						//PARTITION_NONE
	
	//create render pass pools
	getPool(LLDrawPool::POOL_ALPHA);
	getPool(LLDrawPool::POOL_ALPHA_POST_WATER);
	getPool(LLDrawPool::POOL_SIMPLE);
	getPool(LLDrawPool::POOL_BUMP);

	mTrianglesDrawnStat.reset();
	resetFrameStats();

	mRenderTypeMask = 0xffffffff;	// All render types start on
	mRenderDebugFeatureMask = 0xffffffff; // All debugging features on
	mRenderFeatureMask = 0;	// All features start off
	mRenderDebugMask = 0;	// All debug starts off

	mOldRenderDebugMask = mRenderDebugMask;
	
	mBackfaceCull = TRUE;

	stop_glerror();
	
	// Enable features
	stop_glerror();
		
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
		if (poolp->isFacePool())
		{
			LLFacePool* face_pool = (LLFacePool*) poolp;
			if (face_pool->mReferences.empty())
			{
				mPools.erase(curiter);
				removeFromQuickLookup( poolp );
				delete poolp;
			}
		}
		else
		{
			mPools.erase(curiter);
			removeFromQuickLookup( poolp );
			delete poolp;
		}
	}
	
	if (!mTerrainPools.empty())
	{
		llwarns << "Terrain Pools not cleaned up" << llendl;
	}
	if (!mTreePools.empty())
	{
		llwarns << "Tree Pools not cleaned up" << llendl;
	}
		
	delete mAlphaPool;
	mAlphaPool = NULL;
	delete mAlphaPoolPostWater;
	mAlphaPoolPostWater = NULL;
	delete mSkyPool;
	mSkyPool = NULL;
	delete mStarsPool;
	mStarsPool = NULL;
	delete mTerrainPool;
	mTerrainPool = NULL;
	delete mWaterPool;
	mWaterPool = NULL;
	delete mGroundPool;
	mGroundPool = NULL;
	delete mSimplePool;
	mSimplePool = NULL;
	delete mBumpPool;
	mBumpPool = NULL;

	if (mCubeBuffer)
	{
		delete mCubeBuffer;
		mCubeBuffer = NULL;
	}

	if (mCubeList)
	{
		glDeleteLists(mCubeList, 1);
		mCubeList = 0;
	}

	mBloomImagep = NULL;
	mBloomImage2p = NULL;
	mFaceSelectImagep = NULL;
	mAlphaSizzleImagep = NULL;

	for (S32 i = 0; i < NUM_PARTITIONS-1; i++)
	{
		delete mObjectPartition[i];
	}
	mObjectPartition.clear();

	mGroupQ.clear();
	mVisibleList.clear();
	mVisibleGroups.clear();
	mDrawableGroups.clear();
	mActiveGroups.clear();
	mVisibleBridge.clear();
	mMovedBridge.clear();
	mOccludedBridge.clear();
	mAlphaGroups.clear();
	clearRenderMap();
}

//============================================================================

void LLPipeline::destroyGL() 
{
	stop_glerror();
	unloadShaders();
	mHighlightFaces.clear();
	mGroupQ.clear();
	mVisibleList.clear();
	mVisibleGroups.clear();
	mDrawableGroups.clear();
	mActiveGroups.clear();
	mVisibleBridge.clear();
	mOccludedBridge.clear();
	mAlphaGroups.clear();
	clearRenderMap();
	resetVertexBuffers();

	if (mCubeBuffer)
	{
		delete mCubeBuffer;
		mCubeBuffer = NULL;
	}

	if (mCubeList)
	{
		glDeleteLists(mCubeList, 1);
		mCubeList = 0;
	}
}

void LLPipeline::restoreGL() 
{
	resetVertexBuffers();

	if (mVertexShadersEnabled)
	{
		setShaders();
	}
	
	for (U32 i = 0; i < mObjectPartition.size()-1; i++)
	{
		if (mObjectPartition[i])
		{
			mObjectPartition[i]->restoreGL();
		}
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
		
// 		llinfos << "Looking in " << fname.str().c_str() << llendl;
		file = fopen(fname.str().c_str(), "r");		/* Flawfinder: ignore */
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
	sDynamicReflections = gSavedSettings.getBOOL("RenderDynamicReflections");

	//hack to reset buffers that change behavior with shaders
	resetVertexBuffers();

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
		S32 obj_class = 0;

		if (getLightingDetail() == 0)
		{
			light_class = 1;
		}
		// Load lighting shaders
		mVertexShaderLevel[SHADER_LIGHTING] = light_class;
		mMaxVertexShaderLevel[SHADER_LIGHTING] = light_class;
		mVertexShaderLevel[SHADER_ENVIRONMENT] = env_class;
		mMaxVertexShaderLevel[SHADER_ENVIRONMENT] = env_class;
		mVertexShaderLevel[SHADER_OBJECT] = obj_class;
		mMaxVertexShaderLevel[SHADER_OBJECT] = obj_class;

		BOOL loaded = loadShadersLighting();

		if (loaded)
		{
			mVertexShadersEnabled = TRUE;
			mVertexShadersLoaded = 1;

			// Load all shaders to set max levels
			loadShadersEnvironment();
			loadShadersObject();
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
	mObjectShinyProgram.unload();
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
	//mVertexShaderLevel[SHADER_OBJECT] = llmin(mMaxVertexShaderLevel[SHADER_OBJECT], gSavedSettings.getS32("VertexShaderLevelObject"));
	mVertexShaderLevel[SHADER_OBJECT] = 0;
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
		mObjectShinyProgram.unload();
		mObjectSimpleProgram.unload();
		mObjectBumpProgram.unload();
		mObjectAlphaProgram.unload();
		return FALSE;
	}

#if 0
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
#endif

	if (success)
	{
		//load shiny vertex shader
		std::string shinyvertex = "objects/shinyV.glsl";
		std::string shinyfragment = "objects/shinyF.glsl";
		mObjectShinyProgram.mProgramObject = glCreateProgramObjectARB();
		mObjectShinyProgram.attachObjects(baseObjects, baseCount);
		mObjectShinyProgram.attachObject(loadShader(shinyvertex, SHADER_OBJECT, GL_VERTEX_SHADER_ARB));
		mObjectShinyProgram.attachObject(loadShader(shinyfragment, SHADER_OBJECT, GL_FRAGMENT_SHADER_ARB));

		success = mObjectShinyProgram.mapAttributes();
		if (success)
		{
			success = mObjectShinyProgram.mapUniforms(LLPipeline::sShinyUniforms, LLPipeline::sShinyUniformCount);
		}
		if( !success )
		{
			llwarns << "Failed to load " << shinyvertex << llendl;
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
	/*if (mVertexShaderLevel[SHADER_OBJECT] >= LLDrawPoolSimple::SHADER_LEVEL_LOCAL_LIGHTS)
	{
		return 3;
	}
	else*/
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

class LLOctreeDirtyTexture : public LLOctreeTraveler<LLDrawable>
{
public:
	const std::set<LLViewerImage*>& mTextures;

	LLOctreeDirtyTexture(const std::set<LLViewerImage*>& textures) : mTextures(textures) { }

	virtual void visit(const LLOctreeState<LLDrawable>* state)
	{
		LLSpatialGroup* group = (LLSpatialGroup*) state->getNode()->getListener(0);

		if (!group->isState(LLSpatialGroup::GEOM_DIRTY) && !group->getData().empty())
		{
			for (LLSpatialGroup::draw_map_t::iterator i = group->mDrawMap.begin(); i != group->mDrawMap.end(); ++i)
			{
				for (std::vector<LLDrawInfo*>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					LLDrawInfo* params = *j;
					if (mTextures.find(params->mTexture) != mTextures.end())
					{ 
						group->setState(LLSpatialGroup::GEOM_DIRTY);
						gPipeline.markRebuild(group);
					}
				}
			}
		}

		for (LLSpatialGroup::bridge_list_t::iterator i = group->mBridgeList.begin(); i != group->mBridgeList.end(); ++i)
		{
			LLSpatialBridge* bridge = *i;
			traverse(bridge->mOctree);
		}
	}
};

// Called when a texture changes # of channels (causes faces to move to alpha pool)
void LLPipeline::dirtyPoolObjectTextures(const std::set<LLViewerImage*>& textures)
{
	// *TODO: This is inefficient and causes frame spikes; need a better way to do this
	//        Most of the time is spent in dirty.traverse.

	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		if (poolp->isFacePool())
		{
			((LLFacePool*) poolp)->dirtyTextures(textures);
		}
	}
	
	LLOctreeDirtyTexture dirty(textures);
	for (U32 i = 0; i < mObjectPartition.size(); i++)
	{
		if (mObjectPartition[i])
		{
			dirty.traverse(mObjectPartition[i]->mOctree);
		}
	}
}

LLDrawPool *LLPipeline::findPool(const U32 type, LLViewerImage *tex0)
{
	LLDrawPool *poolp = NULL;
	switch( type )
	{
	case LLDrawPool::POOL_SIMPLE:
		poolp = mSimplePool;
		break;

	case LLDrawPool::POOL_TREE:
		poolp = get_if_there(mTreePools, (uintptr_t)tex0, (LLDrawPool*)0 );
		break;

	case LLDrawPool::POOL_TERRAIN:
		poolp = get_if_there(mTerrainPools, (uintptr_t)tex0, (LLDrawPool*)0 );
		break;

	case LLDrawPool::POOL_BUMP:
		poolp = mBumpPool;
		break;

	case LLDrawPool::POOL_ALPHA:
		poolp = mAlphaPool;
		break;

	case LLDrawPool::POOL_ALPHA_POST_WATER:
		poolp = mAlphaPoolPostWater;
		break;

	case LLDrawPool::POOL_AVATAR:
		break; // Do nothing

	case LLDrawPool::POOL_SKY:
		poolp = mSkyPool;
		break;

	case LLDrawPool::POOL_STARS:
		poolp = mStarsPool;
		break;

	case LLDrawPool::POOL_WATER:
		poolp = mWaterPool;
		break;

	case LLDrawPool::POOL_GROUND:
		poolp = mGroundPool;
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
	U32 type = getPoolTypeFromTE(te, imagep);
	return gPipeline.getPool(type, imagep);
}

//static 
U32 LLPipeline::getPoolTypeFromTE(const LLTextureEntry* te, LLViewerImage* imagep)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	
	if (!te || !imagep)
	{
		return 0;
	}
		
	bool alpha = te->getColor().mV[3] < 0.999f;
	if (imagep)
	{
		alpha = alpha || (imagep->getComponents() == 4) || (imagep->getComponents() == 2);
	}

	if (alpha)
	{
		return LLDrawPool::POOL_ALPHA;
	}
	else if ((te->getBumpmap() || te->getShiny()))
	{
		return LLDrawPool::POOL_BUMP;
	}
	else
	{
		return LLDrawPool::POOL_SIMPLE;
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


void LLPipeline::unlinkDrawable(LLDrawable *drawable)
{
	LLFastTimer t(LLFastTimer::FTM_PIPELINE);

	LLPointer<LLDrawable> drawablep = drawable; // make sure this doesn't get deleted before we are done
	
	// Based on flags, remove the drawable from the queues that it's on.
	if (drawablep->isState(LLDrawable::ON_MOVE_LIST))
	{
		LLDrawable::drawable_vector_t::iterator iter = std::find(mMovedList.begin(), mMovedList.end(), drawablep);
		if (iter != mMovedList.end())
		{
			mMovedList.erase(iter);
		}
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

	if (vobj->getParent())
	{
		vobj->setDrawableParent(((LLViewerObject*)vobj->getParent())->mDrawable); // LLPipeline::addObject 1
	}
	else
	{
		vobj->setDrawableParent(NULL); // LLPipeline::addObject 2
	}

	markRebuild(drawablep, LLDrawable::REBUILD_ALL, TRUE);

	return 1;
}


void LLPipeline::resetFrameStats()
{
	mCompilesStat.addValue(sCompiles);
	mLightingChangesStat.addValue(mLightingChanges);
	mGeometryChangesStat.addValue(mGeometryChanges);
	mTrianglesDrawnStat.addValue(mTrianglesDrawn/1000.f);
	mVerticesRelitStat.addValue(mVerticesRelit);
	mNumVisibleFacesStat.addValue(mNumVisibleFaces);
	mNumVisibleDrawablesStat.addValue((S32)mVisibleList.size());

	mTrianglesDrawn = 0;
	sCompiles        = 0;
	mVerticesRelit   = 0;
	mLightingChanges = 0;
	mGeometryChanges = 0;
	mNumVisibleFaces = 0;

	if (mOldRenderDebugMask != mRenderDebugMask)
	{
		gObjectList.clearDebugText();
		mOldRenderDebugMask = mRenderDebugMask;
	}
		
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
		mMovedList.push_back(drawablep);
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
		mMovedList.push_back(drawablep);
		drawablep->setState(LLDrawable::ON_MOVE_LIST);
	}
}

void LLPipeline::updateMovedList(LLDrawable::drawable_vector_t& moved_list)
{
	for (LLDrawable::drawable_vector_t::iterator iter = moved_list.begin();
		 iter != moved_list.end(); )
	{
		LLDrawable::drawable_vector_t::iterator curiter = iter++;
		LLDrawable *drawablep = *curiter;
		BOOL done = TRUE;
		if (!drawablep->isDead() && (!drawablep->isState(LLDrawable::EARLY_MOVE)))
		{
			done = drawablep->updateMove();
		}
		drawablep->clearState(LLDrawable::EARLY_MOVE | LLDrawable::MOVE_UNDAMPED);
		if (done)
		{
			drawablep->clearState(LLDrawable::ON_MOVE_LIST);
			iter = moved_list.erase(curiter);
		}
	}
}

void LLPipeline::updateMove()
{
	//LLFastTimer t(LLFastTimer::FTM_UPDATE_MOVE);
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	if (gSavedSettings.getBOOL("FreezeTime"))
	{
		return;
	}

	mMoveChangesStat.addValue((F32)mMovedList.size());

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

	updateMovedList(mMovedList);

	for (LLDrawable::drawable_set_t::iterator iter = mActiveQ.begin();
		 iter != mActiveQ.end(); )
	{
		LLDrawable::drawable_set_t::iterator curiter = iter++;
		LLDrawable* drawablep = *curiter;
		if (drawablep && !drawablep->isDead()) 
		{
			if (drawablep->isRoot() && 
				drawablep->mQuietCount++ > MAX_ACTIVE_OBJECT_QUIET_FRAMES && 
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

	//balance octrees
	{
 		LLFastTimer ot(LLFastTimer::FTM_OCTREE_BALANCE);
		for (U32 i = 0; i < mObjectPartition.size()-1; i++)
		{
			if (mObjectPartition[i])
			{
				mObjectPartition[i]->mOctree->balance();
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Culling and occlusion testing
/////////////////////////////////////////////////////////////////////////////

//static
F32 LLPipeline::calcPixelArea(LLVector3 center, LLVector3 size, LLCamera &camera)
{
	LLVector3 lookAt = center - camera.getOrigin();
	F32 dist = lookAt.magVec();

	//ramp down distance for nearby objects
	if (dist < 16.f)
	{
		dist /= 16.f;
		dist *= dist;
		dist *= 16.f;
	}

	//get area of circle around node
	F32 app_angle = atanf(size.magVec()/dist);
	F32 radius = app_angle*LLDrawable::sCurPixelAngle;
	return radius*radius * 3.14159f;
}

void LLPipeline::updateCull(LLCamera& camera)
{
	LLFastTimer t(LLFastTimer::FTM_CULL);
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	mVisibleList.clear();
	mVisibleGroups.clear();
	mDrawableGroups.clear();
	mActiveGroups.clear();
	gTrivialAccepts = 0;
	mVisibleBridge.clear();

	processOcclusion(camera);

	for (U32 i = 0; i < mObjectPartition.size(); i++)
	{	
		if (mObjectPartition[i] && hasRenderType(mObjectPartition[i]->mDrawableType))
		{
			mObjectPartition[i]->cull(camera);
		}
	}

	//do a terse update on some off-screen geometry
	processGeometry(camera);
	
	if (gSky.mVOSkyp.notNull() && gSky.mVOSkyp->mDrawable.notNull())
	{
		// Hack for sky - always visible.
		if (hasRenderType(LLPipeline::RENDER_TYPE_SKY)) 
		{
			gSky.mVOSkyp->mDrawable->setVisible(camera);
			mVisibleList.push_back(gSky.mVOSkyp->mDrawable);
			gSky.updateCull();
			stop_glerror();
		}
	}
	else
	{
		llinfos << "No sky drawable!" << llendl;
	}

	if (hasRenderType(LLPipeline::RENDER_TYPE_GROUND) && gSky.mVOGroundp.notNull() && gSky.mVOGroundp->mDrawable.notNull())
	{
		gSky.mVOGroundp->mDrawable->setVisible(camera);
		mVisibleList.push_back(gSky.mVOGroundp->mDrawable);
	}
}

void LLPipeline::markNotCulled(LLSpatialGroup* group, LLCamera& camera, BOOL active)
{
	if (group->getData().empty())
	{ 
		return;
	}
	
	if (!sSkipUpdate)
	{
		group->updateDistance(camera);
	}
	
	const F32 MINIMUM_PIXEL_AREA = 16.f;

	if (group->mPixelArea < MINIMUM_PIXEL_AREA)
	{
		return;
	}
	
	group->mLastRenderTime = gFrameTimeSeconds;
	if (!group->mSpatialPartition->mRenderByGroup)
	{ //render by drawable
		mDrawableGroups.push_back(group);
		for (LLSpatialGroup::element_iter i = group->getData().begin(); i != group->getData().end(); ++i)
		{
			markVisible(*i, camera);
		}
	}
	else
	{ //render by group
		if (active)
		{
			mActiveGroups.push_back(group);
		}
		else
		{
			mVisibleGroups.push_back(group);
			for (LLSpatialGroup::bridge_list_t::iterator i = group->mBridgeList.begin(); i != group->mBridgeList.end(); ++i)
			{
				LLSpatialBridge* bridge = *i;
				markVisible(bridge, camera);
			}
		}
	}
}

void LLPipeline::doOcclusion(LLCamera& camera)
{
	if (sUseOcclusion)
	{
		for (U32 i = 0; i < mObjectPartition.size(); i++)
		{
			if (mObjectPartition[i] && hasRenderType(mObjectPartition[i]->mDrawableType))
			{
				mObjectPartition[i]->doOcclusion(&camera);
			}
		}
		
#if AGGRESSIVE_OCCLUSION
		for (LLSpatialBridge::bridge_vector_t::iterator i = mVisibleBridge.begin(); i != mVisibleBridge.end(); ++i)
		{
			LLSpatialBridge* bridge = *i;
			if (!bridge->isDead() && hasRenderType(bridge->mDrawableType))
			{
				glPushMatrix();
				glMultMatrixf((F32*)bridge->mDrawable->getRenderMatrix().mMatrix);
				LLCamera trans = bridge->transformCamera(camera);
				bridge->doOcclusion(&trans);
				glPopMatrix();
				mOccludedBridge.push_back(bridge);
			}
		}
#endif 
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
	for (LLDrawable::drawable_list_t::iterator iter = mBuildQ1.begin();
		 iter != mBuildQ1.end();)
	{
		LLDrawable::drawable_list_t::iterator curiter = iter++;
		LLDrawable* drawablep = *curiter;
		if (drawablep && !drawablep->isDead())
		{
			if (drawablep->isState(LLDrawable::IN_REBUILD_Q2))
			{
				drawablep->clearState(LLDrawable::IN_REBUILD_Q2);
				LLDrawable::drawable_list_t::iterator find = std::find(mBuildQ2.begin(), mBuildQ2.end(), drawablep);
				if (find != mBuildQ2.end())
				{
					mBuildQ2.erase(find);
				}
			}

			if (updateDrawableGeom(drawablep, TRUE))
			{
				drawablep->clearState(LLDrawable::IN_REBUILD_Q1);
				mBuildQ1.erase(curiter);
			}
		}
		else
		{
			mBuildQ1.erase(curiter);
		}
	}
		
	// Iterate through some drawables on the non-priority build queue
	S32 min_count = 16;
	if (mBuildQ2.size() > 1000)
	{
		min_count = mBuildQ2.size();
	}
		
	S32 count = 0;
	
	max_dtime = llmax(update_timer.getElapsedTimeF32()+0.001f, max_dtime);
	LLSpatialGroup* last_group = NULL;
	LLSpatialBridge* last_bridge = NULL;

	for (LLDrawable::drawable_list_t::iterator iter = mBuildQ2.begin();
		 iter != mBuildQ2.end(); )
	{
		LLDrawable::drawable_list_t::iterator curiter = iter++;
		LLDrawable* drawablep = *curiter;

		LLSpatialBridge* bridge = drawablep->isRoot() ? drawablep->getSpatialBridge() :
									drawablep->getParent()->getSpatialBridge();

		if (drawablep->getSpatialGroup() != last_group && 
			(!last_bridge || bridge != last_bridge) &&
			(update_timer.getElapsedTimeF32() >= max_dtime) && count > min_count)
		{
			break;
		}

		//make sure updates don't stop in the middle of a spatial group
		//to avoid thrashing (objects are enqueued by group)
		last_group = drawablep->getSpatialGroup();
		last_bridge = bridge;

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
	}	

	updateMovedList(mMovedBridge);
}

void LLPipeline::markVisible(LLDrawable *drawablep, LLCamera& camera)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	if(!drawablep || drawablep->isDead())
	{
		llwarns << "LLPipeline::markVisible called with NULL drawablep" << llendl;
		return;
	}
	
	
#if LL_DEBUG
	if (drawablep->isSpatialBridge())
	{
		if (std::find(mVisibleBridge.begin(), mVisibleBridge.end(), (LLSpatialBridge*) drawablep) !=
			mVisibleBridge.end())
		{
			llerrs << "Spatial bridge marked visible redundantly." << llendl;
		}
	}
	else
	{
		if (std::find(mVisibleList.begin(), mVisibleList.end(), drawablep) !=
			mVisibleList.end())
		{
			llerrs << "Drawable marked visible redundantly." << llendl;
		}
	}
#endif

	if (drawablep->isSpatialBridge())
	{
		mVisibleBridge.push_back((LLSpatialBridge*) drawablep);
	}
	else
	{
		mVisibleList.push_back(drawablep);
	}
	drawablep->setVisible(camera);
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
		if (drawablep->isSpatialBridge())
		{
			mMovedBridge.push_back(drawablep);
		}
		else
		{
			mMovedList.push_back(drawablep);
		}
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

	for (U32 i = 0; i < mObjectPartition.size()-1; i++)
	{
		if (mObjectPartition[i])
		{
			mObjectPartition[i]->shift(offset);
		}
	}
}

void LLPipeline::markTextured(LLDrawable *drawablep)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	if (drawablep && !drawablep->isDead())
	{
		mRetexturedList.insert(drawablep);
	}
}

void LLPipeline::markRebuild(LLSpatialGroup* group)
{
	mGroupQ.insert(group);
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
			if (!drawablep->isState(LLDrawable::IN_REBUILD_Q1))
			{
				mBuildQ1.push_back(drawablep);
				drawablep->setState(LLDrawable::IN_REBUILD_Q1); // mark drawable as being in priority queue
			}
		}
		else if (!drawablep->isState(LLDrawable::IN_REBUILD_Q2))
		{
			mBuildQ2.push_back(drawablep);
			drawablep->setState(LLDrawable::IN_REBUILD_Q2); // need flag here because it is just a list
		}
		if (flag & (LLDrawable::REBUILD_VOLUME | LLDrawable::REBUILD_POSITION))
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

void LLPipeline::stateSort(LLCamera& camera)
{
	LLFastTimer ftm(LLFastTimer::FTM_STATESORT);
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	for (LLSpatialGroup::sg_vector_t::iterator iter = mVisibleGroups.begin(); iter != mVisibleGroups.end(); ++iter)
	{
		stateSort(*iter, camera);
	}
		
	for (LLSpatialBridge::bridge_vector_t::iterator i = mVisibleBridge.begin(); i != mVisibleBridge.end(); ++i)
	{
		LLSpatialBridge* bridge = *i;
		if (!bridge->isDead())
		{
			stateSort(bridge, camera);
		}
	}

	for (LLDrawable::drawable_vector_t::iterator iter = mVisibleList.begin();
		 iter != mVisibleList.end(); iter++)
	{
		LLDrawable *drawablep = *iter;
		if (!drawablep->isDead())
		{
			stateSort(drawablep, camera);
		}
	}

	for (LLSpatialGroup::sg_vector_t::iterator iter = mActiveGroups.begin(); iter != mActiveGroups.end(); ++iter)
	{
		stateSort(*iter, camera);
	}

	postSort(camera);
}

void LLPipeline::stateSort(LLSpatialGroup* group, LLCamera& camera)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	if (!sSkipUpdate && group->changeLOD())
	{
		for (LLSpatialGroup::element_iter i = group->getData().begin(); i != group->getData().end(); ++i)
		{
			LLDrawable* drawablep = *i;
			stateSort(drawablep, camera);
		}
	}
	
#if !LL_DARWIN
	if (gFrameTimeSeconds - group->mLastUpdateTime > 4.f)
	{
		group->makeStatic();
	}
#endif
}

void LLPipeline::stateSort(LLSpatialBridge* bridge, LLCamera& camera)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	if (!sSkipUpdate)
	{
		bridge->updateDistance(camera);
	}
}

void LLPipeline::stateSort(LLDrawable* drawablep, LLCamera& camera)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	LLFastTimer ftm(LLFastTimer::FTM_STATESORT_DRAWABLE);
	
	if (drawablep->isDead() || !hasRenderType(drawablep->getRenderType()))
	{
		return;
	}
	
	if (gHideSelectedObjects)
	{
		if (drawablep->getVObj() &&
			drawablep->getVObj()->isSelected())
		{
			return;
		}
	}

	if (drawablep && (hasRenderType(drawablep->mRenderType)))
	{
		if (!drawablep->isState(LLDrawable::INVISIBLE|LLDrawable::FORCE_INVISIBLE))
		{
			drawablep->setVisible(camera, NULL, FALSE);
		}
		else if (drawablep->isState(LLDrawable::CLEAR_INVISIBLE))
		{
			// clear invisible flag here to avoid single frame glitch
			drawablep->clearState(LLDrawable::FORCE_INVISIBLE|LLDrawable::CLEAR_INVISIBLE);
		}
	}

	if (!drawablep->isActive() && drawablep->isVisible())
	{
		if (!sSkipUpdate)
		{
			drawablep->updateDistance(camera);
		}
	}
	else if (drawablep->isAvatar() && drawablep->isVisible())
	{
		LLVOAvatar* vobj = (LLVOAvatar*) drawablep->getVObj();
		vobj->updateVisibility(FALSE);
	}

	for (LLDrawable::face_list_t::iterator iter = drawablep->mFaces.begin();
			iter != drawablep->mFaces.end(); iter++)
	{
		LLFace* facep = *iter;

		if (facep->hasGeometry())
		{
			if (facep->getPool())
			{
				facep->getPool()->enqueue(facep);
			}
			else
			{
				break;
			}
		}
	}
	
	
	mNumVisibleFaces += drawablep->getNumFaces();
}


void LLPipeline::forAllDrawables(LLSpatialGroup::sg_vector_t& groups, void (*func)(LLDrawable*))
{
	for (LLSpatialGroup::sg_vector_t::iterator i = groups.begin(); i != groups.end(); ++i)
	{
		for (LLSpatialGroup::element_iter j = (*i)->getData().begin(); j != (*i)->getData().end(); ++j)
		{
			func(*j);	
		}
	}
}

void LLPipeline::forAllVisibleDrawables(void (*func)(LLDrawable*))
{
	forAllDrawables(mDrawableGroups, func);
	forAllDrawables(mVisibleGroups, func);
	forAllDrawables(mActiveGroups, func);
}

//function for creating scripted beacons
void renderScriptedBeacons(LLDrawable* drawablep)
{
	LLViewerObject *vobj = drawablep->getVObj();
	if (vobj 
		&& !vobj->isAvatar() 
		&& !vobj->getParent()
		&& vobj->flagScripted())
	{
		gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(1.f, 0.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f));
	}
}

void renderPhysicalBeacons(LLDrawable* drawablep)
{
	LLViewerObject *vobj = drawablep->getVObj();
	if (vobj 
		&& !vobj->isAvatar() 
		&& !vobj->getParent()
		&& vobj->usePhysics())
	{
		gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(0.f, 1.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f));
	}
}

void renderParticleBeacons(LLDrawable* drawablep)
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

void LLPipeline::highlightPhysical(LLDrawable* drawablep)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	LLViewerObject *vobj;
	vobj = drawablep->getVObj();
	if (vobj && !vobj->isAvatar())
	{
		if (!vobj->isAvatar() && 
			(vobj->usePhysics() || vobj->flagHandleTouch()))
		{
			S32 face_id;
			for (face_id = 0; face_id < drawablep->getNumFaces(); face_id++)
			{
				gPipeline.mHighlightFaces.push_back(drawablep->getFace(face_id) );
			}
		}
	}
}

void LLPipeline::postSort(LLCamera& camera)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	LLFastTimer ftm(LLFastTimer::FTM_STATESORT_POSTSORT);
	//reset render data sets
	clearRenderMap();
	mAlphaGroups.clear();
	mAlphaGroupsPostWater.clear();
	
	if (!gSavedSettings.getBOOL("RenderRippleWater") && hasRenderType(LLDrawPool::POOL_ALPHA))
	{	//turn off clip plane for non-ripple water
		toggleRenderType(LLDrawPool::POOL_ALPHA);
	}

	F32 water_height = gAgent.getRegion()->getWaterHeight();
	BOOL above_water = gCamera->getOrigin().mV[2] > water_height ? TRUE : FALSE;

	//prepare occlusion geometry
	if (sUseOcclusion)
	{
		for (U32 i = 0; i < mObjectPartition.size(); i++)
		{
			if (mObjectPartition[i] && hasRenderType(mObjectPartition[i]->mDrawableType))
			{
				mObjectPartition[i]->buildOcclusion();
			}
		}
		
#if AGGRESSIVE_OCCLUSION
		for (LLSpatialBridge::bridge_vector_t::iterator i = mVisibleBridge.begin(); i != mVisibleBridge.end(); ++i)
		{
			LLSpatialBridge* bridge = *i;
			if (!bridge->isDead() && hasRenderType(bridge->mDrawableType))
			{	
				bridge->buildOcclusion();
			}
		}
#endif
	}


	//rebuild offscreen geometry
	if (!sSkipUpdate)
	{
		for (LLSpatialGroup::sg_set_t::iterator iter = mGroupQ.begin(); iter != mGroupQ.end(); ++iter)
		{
			LLSpatialGroup* group = *iter;
			group->rebuildGeom();
		}
		mGroupQ.clear();
	
		//rebuild drawable geometry
		for (LLSpatialGroup::sg_vector_t::iterator i = mDrawableGroups.begin(); i != mDrawableGroups.end(); ++i)
		{
			LLSpatialGroup* group = *i;
			group->rebuildGeom();
		}
	}

	//build render map
	for (LLSpatialGroup::sg_vector_t::iterator i = mVisibleGroups.begin(); i != mVisibleGroups.end(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (!sSkipUpdate)
		{
			group->rebuildGeom();
		}
		for (LLSpatialGroup::draw_map_t::iterator j = group->mDrawMap.begin(); j != group->mDrawMap.end(); ++j)
		{
			std::vector<LLDrawInfo*>& src_vec = j->second;
			std::vector<LLDrawInfo*>& dest_vec = mRenderMap[j->first];

			for (std::vector<LLDrawInfo*>::iterator k = src_vec.begin(); k != src_vec.end(); ++k)
			{
				dest_vec.push_back(*k);
			}
		}
		
		LLSpatialGroup::draw_map_t::iterator alpha = group->mDrawMap.find(LLRenderPass::PASS_ALPHA);
		
		if (alpha != group->mDrawMap.end())
		{ //store alpha groups for sorting
			if (!sSkipUpdate)
			{
				group->updateDistance(camera);
			}
			
			if (hasRenderType(LLDrawPool::POOL_ALPHA))
			{
				BOOL above = group->mObjectBounds[0].mV[2] + group->mObjectBounds[1].mV[2] > water_height ? TRUE : FALSE;
				BOOL below = group->mObjectBounds[0].mV[2] - group->mObjectBounds[1].mV[2] < water_height ? TRUE : FALSE;
				
				if (below == above_water || above == below)
				{
					mAlphaGroups.push_back(group);
				}

				if (above == above_water || below == above)
				{
					mAlphaGroupsPostWater.push_back(group);
				}
			}
			else
			{
				mAlphaGroupsPostWater.push_back(group);
			}
		}
	}
	
	//store active alpha groups
	for (LLSpatialGroup::sg_vector_t::iterator i = mActiveGroups.begin(); i != mActiveGroups.end(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (!sSkipUpdate)
		{
			group->rebuildGeom();
		}
		LLSpatialGroup::draw_map_t::iterator alpha = group->mDrawMap.find(LLRenderPass::PASS_ALPHA);
		
		if (alpha != group->mDrawMap.end())
		{
			LLSpatialBridge* bridge = group->mSpatialPartition->asBridge();
			LLCamera trans_camera = bridge->transformCamera(camera);
			if (!sSkipUpdate)
			{
				group->updateDistance(trans_camera);
			}
			
			if (hasRenderType(LLDrawPool::POOL_ALPHA))
			{
				LLSpatialGroup* bridge_group = bridge->getSpatialGroup();
				BOOL above = bridge_group->mObjectBounds[0].mV[2] + bridge_group->mObjectBounds[1].mV[2] > water_height ? TRUE : FALSE;
				BOOL below = bridge_group->mObjectBounds[0].mV[2] - bridge_group->mObjectBounds[1].mV[2] < water_height ? TRUE : FALSE;
					
				
				if (below == above_water || above == below)
				{
					mAlphaGroups.push_back(group);
				}
				
				if (above == above_water || below == above)
				{
					mAlphaGroupsPostWater.push_back(group);
				}
			}
			else
			{
				mAlphaGroupsPostWater.push_back(group);
			}
		}
	}

	//sort by texture or bump map
	for (U32 i = 0; i < LLRenderPass::NUM_RENDER_TYPES; ++i)
	{
		if (!mRenderMap[i].empty())
		{
			if (i == LLRenderPass::PASS_BUMP)
			{
				std::sort(mRenderMap[i].begin(), mRenderMap[i].end(), LLDrawInfo::CompareBump());
			}
			else 
			{
				std::sort(mRenderMap[i].begin(), mRenderMap[i].end(), LLDrawInfo::CompareTexturePtr());
			}
		}
	}

	std::sort(mAlphaGroups.begin(), mAlphaGroups.end(), LLSpatialGroup::CompareDepthGreater());
	std::sort(mAlphaGroupsPostWater.begin(), mAlphaGroupsPostWater.end(), LLSpatialGroup::CompareDepthGreater());

	if (sRenderScriptedBeacons)
	{
		// Only show the beacon on the root object.
		forAllVisibleDrawables(renderScriptedBeacons);
	}

	if (sRenderPhysicalBeacons)
	{
		// Only show the beacon on the root object.
		forAllVisibleDrawables(renderPhysicalBeacons);
	}

	if (sRenderParticleBeacons)
	{
		forAllVisibleDrawables(renderParticleBeacons);
	}

	// Draw physical objects in red.
	if (gHUDManager->getShowPhysical())
	{
		forAllVisibleDrawables(highlightPhysical);
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

	mSelectedFaces.clear();
	
	// Draw face highlights for selected faces.
	if (gSelectMgr->getTEMode())
	{
		LLViewerObject *vobjp;
		S32             te;
		gSelectMgr->getSelection()->getFirstTE(&vobjp,&te);

		while (vobjp)
		{
			mSelectedFaces.push_back(vobjp->mDrawable->getFace(te));
			gSelectMgr->getSelection()->getNextTE(&vobjp,&te);
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
		gViewerWindow->renderSelections(FALSE, FALSE, FALSE); // For HUD version in render_ui_3d()
	
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
	else if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
	{
		LLHUDText::renderAllHUD();
	}
}

void LLPipeline::renderHighlights()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
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

		for (U32 i = 0; i < mSelectedFaces.size(); i++)
		{
			LLFace *facep = mSelectedFaces[i];
			if (!facep || facep->getDrawable()->isDead())
			{
				llerrs << "Bad face on selection" << llendl;
			}
			
			facep->renderSelected(mFaceSelectImagep, color);
		}
	}

	if (hasRenderDebugFeatureMask(RENDER_DEBUG_FEATURE_SELECTED))
	{
		// Paint 'em red!
		color.setVec(1.f, 0.f, 0.f, 0.5f);
		for (U32 i = 0; i < mHighlightFaces.size(); i++)
		{
			LLFace* facep = mHighlightFaces[i];
			facep->renderSelected(LLViewerImage::sNullImagep, color);
		}
	}

	// Contains a list of the faces of objects that are physical or
	// have touch-handlers.
	mHighlightFaces.clear();

	if (mVertexShaderLevel[SHADER_INTERFACE] > 0)
	{
		mHighlightProgram.unbind();
	}
}

void LLPipeline::renderGeom(LLCamera& camera)
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

	glEnableClientState(GL_VERTEX_ARRAY);

	stop_glerror();
	gFrameStats.start(LLFrameStats::RENDER_SYNC);

	// Do verification of GL state
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();
#endif
	if (mRenderDebugMask & RENDER_DEBUG_VERIFY)
	{
		if (!verify())
		{
			llerrs << "Pipeline verification failed!" << llendl;
		}
	}

	{
		//LLFastTimer ftm(LLFastTimer::FTM_TEMP6);
		LLVertexBuffer::startRender();
	}

	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		if (hasRenderType(poolp->getType()))
		{
			poolp->prerender();
		}
	}

	gFrameStats.start(LLFrameStats::RENDER_GEOM);

	// Initialize lots of GL state to "safe" values
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	LLGLSPipeline gls_pipeline;
	
	LLGLState gls_color_material(GL_COLOR_MATERIAL, mLightingDetail < 2);
	// LLGLState normalize(GL_NORMALIZE, TRUE);
			
	// Toggle backface culling for debugging
	LLGLEnable cull_face(mBackfaceCull ? GL_CULL_FACE : 0);
	// Set fog
	LLGLEnable fog_enable(hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_FOG) ? GL_FOG : 0);
	gSky.updateFog(camera.getFar());

	LLViewerImage::sDefaultImagep->bind(0);
	LLViewerImage::sDefaultImagep->setClamp(FALSE, FALSE);
	
	//////////////////////////////////////////////
	//
	// Actually render all of the geometry
	//
	//	
	stop_glerror();
	BOOL did_hud_elements = FALSE;
	BOOL occlude = sUseOcclusion;

	U32 cur_type = 0;

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_PICKING))
	{
		gObjectList.renderObjectsForSelect(camera);
	}
	else
	{
		LLFastTimer t(LLFastTimer::FTM_POOLS);
		calcNearbyLights();

		pool_set_t::iterator iter1 = mPools.begin();
		while ( iter1 != mPools.end() )
		{
			LLDrawPool *poolp = *iter1;
			
			cur_type = poolp->getType();

			if (occlude && cur_type > LLDrawPool::POOL_AVATAR)
			{
				occlude = FALSE;
				doOcclusion(camera);
			}

			if (cur_type > LLDrawPool::POOL_ALPHA_POST_WATER && !did_hud_elements)
			{
				renderHighlights();
				// Draw 3D UI elements here (before we clear the Z buffer in POOL_HUD)
				render_hud_elements();
				did_hud_elements = TRUE;
			}

			pool_set_t::iterator iter2 = iter1;
			if (hasRenderType(poolp->getType()) && poolp->getNumPasses() > 0)
			{
				LLFastTimer t(LLFastTimer::FTM_POOLRENDER);

				setupHWLights(poolp);
			
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
						
						p->resetTrianglesDrawn();
						p->render(i);
						mTrianglesDrawn += p->getTrianglesDrawn();
					}
					poolp->endRenderPass(i);
#ifndef LL_RELEASE_FOR_DOWNLOAD
					GLint depth;
					glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &depth);
					if (depth > 3)
					{
						llerrs << "GL matrix stack corrupted!" << llendl;
					}
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
	}

#ifndef LL_RELEASE_FOR_DOWNLOAD
		LLGLState::checkStates();
		LLGLState::checkTextureChannels();
		LLGLState::checkClientArrays();
#endif

	if (occlude)
	{
		doOcclusion(camera);
		occlude = FALSE;
	}

	if (!did_hud_elements)
	{
		renderHighlights();
		render_hud_elements();
	}

	stop_glerror();
	
	{
		LLVertexBuffer::stopRender();
	}
	
#ifndef LL_RELEASE_FOR_DOWNLOAD
		LLGLState::checkStates();
		LLGLState::checkTextureChannels();
		LLGLState::checkClientArrays();
#endif
	
	// Contains a list of the faces of objects that are physical or
	// have touch-handlers.
	mHighlightFaces.clear();
}

void LLPipeline::processGeometry(LLCamera& camera)
{
	if (sSkipUpdate)
	{
		return;
	}

	for (U32 i = 0; i < mObjectPartition.size(); i++)
	{
		if (mObjectPartition[i] && hasRenderType(mObjectPartition[i]->mDrawableType))
		{
			mObjectPartition[i]->processGeometry(&camera);
		}
	}
}

void LLPipeline::processOcclusion(LLCamera& camera)
{
	//process occlusion (readback)
	if (sUseOcclusion)
	{
		for (U32 i = 0; i < mObjectPartition.size(); i++)
		{
			if (mObjectPartition[i] && hasRenderType(mObjectPartition[i]->mDrawableType))
			{
				mObjectPartition[i]->processOcclusion(&camera);
			}
		}
		
#if AGGRESSIVE_OCCLUSION
		for (LLSpatialBridge::bridge_vector_t::iterator i = mOccludedBridge.begin(); i != mOccludedBridge.end(); ++i)
		{
			LLSpatialBridge* bridge = *i;
			if (!bridge->isDead() && hasRenderType(bridge->mDrawableType))
			{
				LLCamera trans = bridge->transformCamera(camera);
				bridge->processOcclusion(&trans);
			}
		}
#endif
		mOccludedBridge.clear();
	}
}

void LLPipeline::renderDebug()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);

	// Disable all client state
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	// Debug stuff.
	for (U32 i = 0; i < mObjectPartition.size(); i++)
	{
		if (mObjectPartition[i] && hasRenderType(mObjectPartition[i]->mDrawableType))
		{
			mObjectPartition[i]->renderDebug();
		}
	}

	for (LLSpatialBridge::bridge_vector_t::iterator i = mVisibleBridge.begin(); i != mVisibleBridge.end(); ++i)
	{
		LLSpatialBridge* bridge = *i;
		if (!bridge->isDead() && hasRenderType(bridge->mDrawableType))
		{
			glPushMatrix();
			glMultMatrixf((F32*)bridge->mDrawable->getRenderMatrix().mMatrix);
			bridge->renderDebug();
			glPopMatrix();
		}
	}

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
}

void LLPipeline::renderForSelect(std::set<LLViewerObject*>& objects)
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	
	LLVertexBuffer::startRender();
	
	glMatrixMode(GL_MODELVIEW);

	LLGLSDefault gls_default;
	LLGLSObjectSelect gls_object_select;
	LLGLDepthTest gls_depth(GL_TRUE,GL_TRUE);
	disableLights();
	
    glEnableClientState ( GL_VERTEX_ARRAY );

	//for each drawpool
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkStates();
	LLGLState::checkTextureChannels();
	LLGLState::checkClientArrays();
	U32 last_type = 0;
#endif
	for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
	{
		LLDrawPool *poolp = *iter;
		if (poolp->isFacePool() && hasRenderType(poolp->getType()))
		{
			LLFacePool* face_pool = (LLFacePool*) poolp;
			face_pool->renderForSelect();
		
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
	}

	LLGLEnable tex(GL_TEXTURE_2D);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	LLGLEnable alpha_test(GL_ALPHA_TEST);
	if (gPickTransparent)
	{
		glAlphaFunc(GL_GEQUAL, 0.0f);
	}
	else
	{
		glAlphaFunc(GL_GREATER, 0.2f);
	}

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_MODULATE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB,		GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB,	GL_SRC_ALPHA);

	U32 prim_mask = LLVertexBuffer::MAP_VERTEX | 
					LLVertexBuffer::MAP_TEXCOORD;

	for (std::set<LLViewerObject*>::iterator i = objects.begin(); i != objects.end(); ++i)
	{
		LLViewerObject* vobj = *i;
		LLDrawable* drawable = vobj->mDrawable;
		if (vobj->isDead() || 
			vobj->isHUDAttachment() ||
			(gHideSelectedObjects && vobj->isSelected()) ||
			drawable->isDead() || 
			!hasRenderType(drawable->getRenderType()))
		{
			continue;
		}

		for (S32 j = 0; j < drawable->getNumFaces(); ++j)
		{
			LLFace* facep = drawable->getFace(j);
			if (!facep->getPool())
			{
				facep->renderForSelect(prim_mask);
			}
		}
	}

	// pick HUD objects
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (avatarp && sShowHUDAttachments)
	{
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		setup_hud_matrices(TRUE);
		LLViewerJointAttachment* attachmentp;
		for (attachmentp = avatarp->mAttachmentPoints.getFirstData();
			attachmentp;
			attachmentp = avatarp->mAttachmentPoints.getNextData())
		{
			if (attachmentp->getIsHUDAttachment())
			{
				LLViewerObject* objectp = attachmentp->getObject();
				if (objectp)
				{
					LLDrawable* drawable = objectp->mDrawable;
					if (drawable->isDead())
					{
						continue;
					}

					for (S32 j = 0; j < drawable->getNumFaces(); ++j)
					{
						LLFace* facep = drawable->getFace(j);
						if (!facep->getPool())
						{
							facep->renderForSelect(prim_mask);
						}
					}

					//render child faces
					for (U32 k = 0; k < drawable->getChildCount(); ++k)
					{
						LLDrawable* child = drawable->getChild(k);
						for (S32 l = 0; l < child->getNumFaces(); ++l)
						{
							LLFace* facep = child->getFace(l);
							if (!facep->getPool())
							{
								facep->renderForSelect(prim_mask);
							}
						}
					}
				}	
			}
		}

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	
	LLVertexBuffer::stopRender();
}

void LLPipeline::renderFaceForUVSelect(LLFace* facep)
{
	if (facep) facep->renderSelectedUV();
}

void LLPipeline::rebuildPools()
{
	LLMemType mt(LLMemType::MTYPE_PIPELINE);
	S32 max_count = mPools.size();
	pool_set_t::iterator iter1 = mPools.upper_bound(mLastRebuildPool);
	while(max_count > 0 && mPools.size() > 0) // && num_rebuilds < MAX_REBUILDS)
	{
		if (iter1 == mPools.end())
		{
			iter1 = mPools.begin();
		}
		LLDrawPool* poolp = *iter1;

		if (poolp->isDead())
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
		if (mSimplePool)
		{
			llassert(0);
			llwarns << "Ignoring duplicate simple pool." << llendl;
		}
		else
		{
			mSimplePool = (LLRenderPass*) new_poolp;
		}
		break;

	case LLDrawPool::POOL_TREE:
		mTreePools[ uintptr_t(new_poolp->getTexture()) ] = new_poolp ;
		break;
 
	case LLDrawPool::POOL_TERRAIN:
		mTerrainPools[ uintptr_t(new_poolp->getTexture()) ] = new_poolp ;
		break;

	case LLDrawPool::POOL_BUMP:
		if (mBumpPool)
		{
			llassert(0);
			llwarns << "Ignoring duplicate bump pool." << llendl;
		}
		else
		{
			mBumpPool = new_poolp;
		}
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

	case LLDrawPool::POOL_ALPHA_POST_WATER:
		if( mAlphaPoolPostWater )
		{
			llassert(0);
			llwarns << "LLPipeline::addPool(): Ignoring duplicate Alpha pool" << llendl;
		}
		else
		{
			mAlphaPoolPostWater = new_poolp;
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
		llassert(mSimplePool == poolp);
		mSimplePool = NULL;
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
		llassert( poolp == mBumpPool );
		mBumpPool = NULL;
		break;
	
	case LLDrawPool::POOL_ALPHA:
		llassert( poolp == mAlphaPool );
		mAlphaPool = NULL;
		break;

	case LLDrawPool::POOL_ALPHA_POST_WATER:
		llassert( poolp == mAlphaPoolPostWater );
		mAlphaPoolPostWater = NULL;
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

	case LLDrawPool::POOL_WATER:
		llassert( poolp == mWaterPool );
		mWaterPool = NULL;
		break;

	case LLDrawPool::POOL_GROUND:
		llassert( poolp == mGroundPool );
		mGroundPool = NULL;
		break;

	default:
		llassert(0);
		llwarns << "Invalid Pool Type in  LLPipeline::removeFromQuickLookup() type=" << poolp->getType() << llendl;
		break;
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
	if (std::find(mMovedList.begin(), mMovedList.end(), drawablep) != mMovedList.end())
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
	
	if (mActiveQ.find(drawablep) != mActiveQ.end())
	{
		llinfos << "In mActiveQ" << llendl;
	}
	if (std::find(mBuildQ1.begin(), mBuildQ1.end(), drawablep) != mBuildQ1.end())
	{
		llinfos << "In mBuildQ1" << llendl;
	}
	if (std::find(mBuildQ2.begin(), mBuildQ2.end(), drawablep) != mBuildQ2.end())
	{
		llinfos << "In mBuildQ2" << llendl;
	}

	S32 count;
	
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
			drawablep->clearState(LLDrawable::LIGHT);
			mLights.erase(drawablep);
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
void LLPipeline::toggleRenderType(U32 type)
{
	U32 bit = (1<<type);
	gPipeline.mRenderTypeMask ^= bit;
}

//static
void LLPipeline::toggleRenderTypeControl(void* data)
{
	U32 type = (U32)(intptr_t)data;
	U32 bit = (1<<type);
	if (gPipeline.hasRenderType(type))
	{
		llinfos << "Toggling render type mask " << std::hex << bit << " off" << std::dec << llendl;
	}
	else
	{
		llinfos << "Toggling render type mask " << std::hex << bit << " on" << std::dec << llendl;
	}
	gPipeline.toggleRenderType(type);
}

//static
BOOL LLPipeline::hasRenderTypeControl(void* data)
{
	U32 type = (U32)(intptr_t)data;
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
	char name[1024];		/* Flawfinder: ignore */
	name[0] = 0;

	glGetActiveUniformARB(mProgramObject, index, 1024, &length, &size, &type, name);
	
	//find the index of this uniform
	for (S32 i = 0; i < (S32) LLPipeline::sReservedUniformCount; i++)
	{
		if (mUniform[i] == -1 && !strncmp(LLPipeline::sReservedUniforms[i],name, strlen(LLPipeline::sReservedUniforms[i])))		/* Flawfinder: ignore */
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
			!strncmp(uniform_names[i],name, strlen(uniform_names[i])))		/* Flawfinder: ignore */
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
	LLDrawable* drawable = mObjectPartition[PARTITION_VOLUME]->pickDrawable(start, end, collision);
	return drawable ? drawable->getVObj() : NULL;
}

LLSpatialPartition* LLPipeline::getSpatialPartition(LLViewerObject* vobj)
{
	if (vobj)
	{
		return getSpatialPartition(vobj->getPartitionType());
	}
	return NULL;
}

LLSpatialPartition* LLPipeline::getSpatialPartition(U32 type)
{
	if (type < mObjectPartition.size())
	{
		return mObjectPartition[type];
	}
	return NULL;
}

void LLPipeline::clearRenderMap()
{
	for (U32 i = 0; i < LLRenderPass::NUM_RENDER_TYPES; i++)
	{
		mRenderMap[i].clear();
	}
}


void LLPipeline::resetVertexBuffers(LLDrawable* drawable)
{
	for (S32 i = 0; i < drawable->getNumFaces(); i++)
	{
		LLFace* facep = drawable->getFace(i);
		facep->mVertexBuffer = NULL;
		facep->mLastVertexBuffer = NULL;
	}
}

void LLPipeline::resetVertexBuffers()
{
	for (U32 i = 0; i < mObjectPartition.size(); ++i)
	{
		if (mObjectPartition[i])
		{
			mObjectPartition[i]->resetVertexBuffers();
		}
	}

	resetDrawOrders();

	if (gSky.mVOSkyp.notNull())
	{
		resetVertexBuffers(gSky.mVOSkyp->mDrawable);
		resetVertexBuffers(gSky.mVOGroundp->mDrawable);
		resetVertexBuffers(gSky.mVOStarsp->mDrawable);
		markRebuild(gSky.mVOSkyp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
		markRebuild(gSky.mVOGroundp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
		markRebuild(gSky.mVOStarsp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
	}

	if (LLVertexBuffer::sGLCount > 0)
	{
		LLVertexBuffer::cleanupClass();
	}
}

void LLPipeline::renderObjects(U32 type, U32 mask, BOOL texture)
{
	mSimplePool->renderStatic(type, mask, texture);
	mSimplePool->renderActive(type, mask, texture);
}

void LLPipeline::setUseVBO(BOOL use_vbo)
{
	if (use_vbo != LLVertexBuffer::sEnableVBOs)
	{
		if (use_vbo)
		{
			llinfos << "Enabling VBO." << llendl;
		}
		else
		{ 
			llinfos << "Disabling VBO." << llendl;
		}
		
		resetVertexBuffers();
		LLVertexBuffer::initClass(use_vbo);
	}
}

void apply_cube_face_rotation(U32 face)
{
	switch (face)
	{
		case 0: 
			glRotatef(90.f, 0, 1, 0);
			glRotatef(180.f, 1, 0, 0);
		break;
		case 2: 
			glRotatef(-90.f, 1, 0, 0);
		break;
		case 4:
			glRotatef(180.f, 0, 1, 0);
			glRotatef(180.f, 0, 0, 1);
		break;
		case 1: 
			glRotatef(-90.f, 0, 1, 0);
			glRotatef(180.f, 1, 0, 0);
		break;
		case 3:
			glRotatef(90, 1, 0, 0);
		break;
		case 5: 
			glRotatef(180, 0, 0, 1);
		break;
	}
}
void LLPipeline::generateReflectionMap(LLCubeMap* cube_map, LLCamera& cube_cam, GLsizei res)
{
	//render dynamic cube map
	U32 type_mask = gPipeline.getRenderTypeMask();
	BOOL use_occlusion = LLPipeline::sUseOcclusion;
	LLPipeline::sUseOcclusion = FALSE;
	LLPipeline::sSkipUpdate = TRUE;
	static GLuint blur_tex = 0;
	if (!blur_tex)
	{
		glGenTextures(1, &blur_tex);
	}

	BOOL toggle_ui = gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI);
	if (toggle_ui)
	{
		gPipeline.toggleRenderDebugFeature((void*) LLPipeline::RENDER_DEBUG_FEATURE_UI);
	}
	
	U32 cube_mask = (1 << LLPipeline::RENDER_TYPE_SIMPLE) |
					(1 << LLPipeline::RENDER_TYPE_WATER) |
					(1 << LLPipeline::RENDER_TYPE_BUMP) |
					(1 << LLPipeline::RENDER_TYPE_ALPHA) |
					(1 << LLPipeline::RENDER_TYPE_TREE) |
					(1 << LLDrawPool::POOL_ALPHA_POST_WATER) |
					//(1 << LLPipeline::RENDER_TYPE_PARTICLES) |
					(1 << LLPipeline::RENDER_TYPE_CLOUDS) |
					//(1 << LLPipeline::RENDER_TYPE_STARS) |
					//(1 << LLPipeline::RENDER_TYPE_AVATAR) |
					(1 << LLPipeline::RENDER_TYPE_GRASS) |
					(1 << LLPipeline::RENDER_TYPE_VOLUME) |
					(1 << LLPipeline::RENDER_TYPE_TERRAIN) |
					(1 << LLPipeline::RENDER_TYPE_SKY) |
					(1 << LLPipeline::RENDER_TYPE_GROUND);
	
	LLDrawPoolWater::sSkipScreenCopy = TRUE;
	cube_mask = cube_mask & type_mask;
	gPipeline.setRenderTypeMask(cube_mask);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glViewport(0,0,res,res);
	
	glClearColor(0,0,0,0);			
	
	U32 cube_face[] = 
	{
		GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB,
	};
	
	LLVector3 origin = cube_cam.getOrigin();

	gPipeline.calcNearbyLights();

	for (S32 i = 0; i < 6; i++)
	{
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(90.f, 1.f, 0.1f, 1024.f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		apply_cube_face_rotation(i);

		
		glTranslatef(-origin.mV[0], -origin.mV[1], -origin.mV[2]);
		cube_cam.setOrigin(origin);
		LLViewerCamera::updateFrustumPlanes(cube_cam);
		cube_cam.setOrigin(gCamera->getOrigin());
		gPipeline.updateCull(cube_cam);
		gPipeline.stateSort(cube_cam);
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		gPipeline.renderGeom(cube_cam);

		cube_map->enable(0);
		cube_map->bind();
		glCopyTexImage2D(cube_face[i], 0, GL_RGB, 0, 0, res, res, 0);
		cube_map->disable();
	}

	cube_cam.setOrigin(origin);
	gPipeline.resetDrawOrders();
	gPipeline.mShinyOrigin.setVec(cube_cam.getOrigin(), cube_cam.getFar()*2.f);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	gPipeline.setRenderTypeMask(type_mask);
	LLPipeline::sUseOcclusion = use_occlusion;
	LLPipeline::sSkipUpdate = FALSE;

	if (toggle_ui)
	{
		gPipeline.toggleRenderDebugFeature((void*)LLPipeline::RENDER_DEBUG_FEATURE_UI);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	LLDrawPoolWater::sSkipScreenCopy = FALSE;
}

//send cube map vertices and texture coordinates
void render_cube_map()
{
	if (gPipeline.mCubeList == 0)
	{
		gPipeline.mCubeList = glGenLists(1);
		glNewList(gPipeline.mCubeList, GL_COMPILE);

		U32 idx[36];

		idx[0] = 1; idx[1] = 0; idx[2] = 2; //front
		idx[3] = 3; idx[4] = 2; idx[5] = 0;

		idx[6] = 4; idx[7] = 5; idx[8] = 1; //top
		idx[9] = 0; idx[10] = 1; idx[11] = 5; 

		idx[12] = 5; idx[13] = 4; idx[14] = 6; //back
		idx[15] = 7; idx[16] = 6; idx[17] = 4;

		idx[18] = 6; idx[19] = 7; idx[20] = 3; //bottom
		idx[21] = 2; idx[22] = 3; idx[23] = 7;

		idx[24] = 0; idx[25] = 5; idx[26] = 3; //left
		idx[27] = 6; idx[28] = 3; idx[29] = 5;

		idx[30] = 4; idx[31] = 1; idx[32] = 7; //right
		idx[33] = 2; idx[34] = 7; idx[35] = 1;

		LLVector3 vert[8];
		LLVector3 r = LLVector3(1,1,1);

		vert[0] = r.scaledVec(LLVector3(-1,1,1)); //   0 - left top front
		vert[1] = r.scaledVec(LLVector3(1,1,1));  //   1 - right top front
		vert[2] = r.scaledVec(LLVector3(1,-1,1)); //   2 - right bottom front
		vert[3] = r.scaledVec(LLVector3(-1,-1,1)); //  3 - left bottom front

		vert[4] = r.scaledVec(LLVector3(1,1,-1)); //  4 - left top back
		vert[5] = r.scaledVec(LLVector3(-1,1,-1)); //   5 - right top back
		vert[6] = r.scaledVec(LLVector3(-1,-1,-1)); //  6 - right bottom back
		vert[7] = r.scaledVec(LLVector3(1,-1,-1)); // 7 -left bottom back

		glBegin(GL_TRIANGLES);
		for (U32 i = 0; i < 36; i++)
		{
			glTexCoord3fv(vert[idx[i]].mV);
			glVertex3fv(vert[idx[i]].mV);
		}
		glEnd();

		glEndList();
	}

	glCallList(gPipeline.mCubeList);

}

void LLPipeline::blurReflectionMap(LLCubeMap* cube_in, LLCubeMap* cube_out, U32 res)
{
	LLGLEnable cube(GL_TEXTURE_CUBE_MAP_ARB);
	LLGLDepthTest depth(GL_FALSE);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluPerspective(90.f+45.f/res, 1.f, 0.1f, 1024.f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glViewport(0, 0, res, res);
	LLGLEnable blend(GL_BLEND);
	
	S32 kernel = 2;
	F32 step = 90.f/res;
	F32 alpha = 1.f/((kernel*2+1));

	glColor4f(1,1,1,alpha);

	S32 x = 0;

	U32 cube_face[] = 
	{
		GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB,
	};

	LLVector3 axis[] = 
	{
		LLVector3(1,0,0),
		LLVector3(0,1,0),
		LLVector3(0,0,1)
	};

	
	glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);
	//3-axis blur
	for (U32 j = 0; j < 3; j++)
	{
		glViewport(0,0,res, res*6);
		glClear(GL_COLOR_BUFFER_BIT);
		if (j == 0)
		{
			cube_in->bind();
		}

		for (U32 i = 0; i < 6; i++)
		{
			glViewport(0,i*res, res, res);
			glLoadIdentity();
			apply_cube_face_rotation(i);
			for (x = -kernel; x <= kernel; ++x)
			{
				glPushMatrix();
				glRotatef(x*step, axis[j].mV[0], axis[j].mV[1], axis[j].mV[2]);
				render_cube_map();
				glPopMatrix();
			}
		}	

		//readback
		if (j == 0)
		{
			cube_out->bind();
		}
		for (U32 i = 0; i < 6; i++)
		{
			glCopyTexImage2D(cube_face[i], 0, GL_RGB, 0, i*res, res, res, 0);
		}
	}
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


