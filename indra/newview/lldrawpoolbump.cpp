/** 
 * @file lldrawpoolbump.cpp
 * @brief LLDrawPoolBump class implementation
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolbump.h"

#include "llstl.h"
#include "llviewercontrol.h"
#include "lldir.h"
#include "llimagegl.h"
#include "m3math.h"
#include "m4math.h"

#include "llagent.h"
#include "llagparray.h"
#include "llcubemap.h"
#include "lldrawable.h"
#include "lldrawpoolsimple.h"
#include "llface.h"
#include "llgl.h"
#include "llsky.h"
#include "lltextureentry.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "pipeline.h"


//#include "llimagebmp.h"
//#include "../tools/imdebug/imdebug.h"

// static
LLStandardBumpmap gStandardBumpmapList[TEM_BUMPMAP_COUNT]; 

// static
U32 LLStandardBumpmap::sStandardBumpmapCount = 0;

// static
LLBumpImageList gBumpImageList;

const S32 STD_BUMP_LATEST_FILE_VERSION = 1;

S32 LLDrawPoolBump::sBumpTex = -1;
S32 LLDrawPoolBump::sDiffTex = -1;
S32 LLDrawPoolBump::sEnvTex = -1;

// static 
void LLStandardBumpmap::init()
{
	LLStandardBumpmap::restoreGL();
}

// static 
void LLStandardBumpmap::shutdown()
{
	LLStandardBumpmap::destroyGL();
}

// static 
void LLStandardBumpmap::restoreGL()
{
	llassert( LLStandardBumpmap::sStandardBumpmapCount == 0 );
	gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount++] = LLStandardBumpmap("None");		// BE_NO_BUMP
	gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount++] = LLStandardBumpmap("Brightness");	// BE_BRIGHTNESS
	gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount++] = LLStandardBumpmap("Darkness");	// BE_DARKNESS

	std::string file_name = gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS, "std_bump.ini" );
	FILE* file = LLFile::fopen( file_name.c_str(), "rt" );
	if( !file )
	{
		llwarns << "Could not open std_bump <" << file_name << ">" << llendl;
		return;
	}

	S32 file_version = 0;

	S32 fields_read = fscanf( file, "LLStandardBumpmap version %d", &file_version );
	if( fields_read != 1 )
	{
		llwarns << "Bad LLStandardBumpmap header" << llendl;
		return;
	}

	if( file_version > STD_BUMP_LATEST_FILE_VERSION )
	{
		llwarns << "LLStandardBumpmap has newer version (" << file_version << ") than viewer (" << STD_BUMP_LATEST_FILE_VERSION << ")" << llendl;
		return;
	}

	while( !feof(file) && (LLStandardBumpmap::sStandardBumpmapCount < (U32)TEM_BUMPMAP_COUNT) )
	{
		char label[2048] = "";
		char bump_file[2048] = "";
		fields_read = fscanf( file, "\n%s %s", label, bump_file);
		if( EOF == fields_read )
		{
			break;
		}
		if( fields_read != 2 )
		{
			llwarns << "Bad LLStandardBumpmap entry" << llendl;
			return;
		}

		llinfos << "Loading bumpmap: " << bump_file << " from viewerart" << llendl;
		gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mLabel = label;
		gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mImage = gImageList.getImage( LLUUID(gViewerArt.getString(bump_file)) );
		LLStandardBumpmap::sStandardBumpmapCount++;
	}

	fclose( file );
}

// static
void LLStandardBumpmap::destroyGL()
{
	for( U32 i = 0; i < LLStandardBumpmap::sStandardBumpmapCount; i++ )
	{
		gStandardBumpmapList[i].mLabel.assign("");
		gStandardBumpmapList[i].mImage = NULL;
	}
	sStandardBumpmapCount = 0;
}



////////////////////////////////////////////////////////////////

LLDrawPoolBump::LLDrawPoolBump(LLViewerImage *texturep) :
	LLDrawPool(POOL_BUMP, DATA_BUMP_IL_MASK | DATA_COLORS_MASK, DATA_SIMPLE_NIL_MASK),
	mTexturep(texturep)
{
}

LLDrawPool *LLDrawPoolBump::instancePool()
{
	return new LLDrawPoolBump(mTexturep);
}


void LLDrawPoolBump::prerender()
{
	mVertexShaderLevel = gPipeline.getVertexShaderLevel(LLPipeline::SHADER_OBJECT);
}

BOOL LLDrawPoolBump::match(LLFace* last_face, LLFace* facep)  
{
	if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_CHAIN_FACES) &&
		!last_face->isState(LLFace::LIGHT | LLFace::FULLBRIGHT) &&
		!facep->isState(LLFace::LIGHT | LLFace::FULLBRIGHT) &&
		facep->getIndicesStart() == last_face->getIndicesStart()+last_face->getIndicesCount() &&
		facep->getRenderColor() == last_face->getRenderColor() &&
		facep->getTextureEntry()->getShiny() == last_face->getTextureEntry()->getShiny() &&
		facep->getTextureEntry()->getBumpmap() == last_face->getTextureEntry()->getBumpmap())
	{
		if (facep->isState(LLFace::GLOBAL))
		{
			if (last_face->isState(LLFace::GLOBAL))
			{
				return TRUE;
			}
		}
		else
		{
			if (!last_face->isState(LLFace::GLOBAL))
			{
				if (last_face->getRenderMatrix() == facep->getRenderMatrix())
				{
					return TRUE;
				}
			}
		}
	}
	
	return FALSE;
}

// static
S32 LLDrawPoolBump::numBumpPasses()
{
	if (gPipeline.getVertexShaderLevel(LLPipeline::SHADER_OBJECT) > 0)
	{
		return 1; // single pass for shaders
	}
	else
	{
		if (gSavedSettings.getBOOL("RenderObjectBump"))
			return 3;
		else
			return 1;
	}
}

S32 LLDrawPoolBump::getNumPasses()
{
	return numBumpPasses();
}

void LLDrawPoolBump::beginRenderPass(S32 pass)
{
	switch( pass )
	{
		case 0:
			beginPass0(this);
			break;
		case 1:
			beginPass1();
			break;
		case 2:
			beginPass2();
			break;
		default:
			llassert(0);
			break;
	}
}

void LLDrawPoolBump::render(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_BUMP);
	if (!mTexturep)
	{
		return;
	}

	if (mDrawFace.empty())
	{
		return;
	}

	const U32* index_array = getRawIndices();
	
	S32 indices = 0;
	switch( pass )
	{
	  case 0:
	  {
		  stop_glerror();
		  
		  bindGLVertexPointer();
		  bindGLTexCoordPointer();
		  bindGLNormalPointer();
		  if (gPipeline.getLightingDetail() >= 2)
		  {
			  bindGLColorPointer();
		  }

		  stop_glerror();
		  
		  LLGLState alpha_test(GL_ALPHA_TEST, FALSE);
		  LLGLState blend(GL_BLEND, FALSE);
		  LLViewerImage* tex = getTexture();
		  if (tex && tex->getPrimaryFormat() == GL_ALPHA)
		  {
			  // Enable Invisibility Hack
			  alpha_test.enable();
			  blend.enable();
		  }
		  indices += renderPass0(this, mDrawFace, index_array, mTexturep);
		  break;
	  }
	  case 1:
	  {
		  bindGLVertexPointer();
		  bindGLNormalPointer();
		  indices += renderPass1(mDrawFace, index_array, mTexturep);
		  break;
	  }
	  case 2:
	  {
		  bindGLVertexPointer();
		  // Texture unit 0
		  glActiveTextureARB(GL_TEXTURE0_ARB);
		  glClientActiveTextureARB(GL_TEXTURE0_ARB);
		  bindGLTexCoordPointer();
		  // Texture unit 1
		  glActiveTextureARB(GL_TEXTURE1_ARB);
		  glClientActiveTextureARB(GL_TEXTURE1_ARB);
		  bindGLTexCoordPointer(1);
		  indices += renderPass2(mDrawFace, index_array, mTexturep);
		  break;
	  }
	  default:
	  {
		  llassert(0);
		  break;
	  }
	}
	mIndicesDrawn += indices;
}

void LLDrawPoolBump::endRenderPass(S32 pass)
{
	switch( pass )
	{
		case 0:
			endPass0(this);
			break;
		case 1:
			endPass1();
			break;
		case 2:
			endPass2();
			break;
		default:
			llassert(0);
			break;
	}
}

//static
void LLDrawPoolBump::beginPass0(LLDrawPool* pool)
{
	stop_glerror();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	if (gPipeline.getLightingDetail() >= 2)
	{
		glEnableClientState(GL_COLOR_ARRAY);
	}

	if (pool->getVertexShaderLevel() > 0)
	{
		enable_binormals(gPipeline.mObjectBumpProgram.mAttribute[LLPipeline::GLSL_BINORMAL]);

		sEnvTex = gPipeline.mObjectBumpProgram.enableTexture(LLPipeline::GLSL_ENVIRONMENT_MAP, GL_TEXTURE_CUBE_MAP_ARB);
		LLCubeMap* cube_map = gSky.mVOSkyp->getCubeMap();
		if (sEnvTex >= 0 && cube_map)
		{
			cube_map->bind();
			cube_map->setMatrix(1);
		}
		
		sBumpTex = gPipeline.mObjectBumpProgram.enableTexture(LLPipeline::GLSL_BUMP_MAP);
		sDiffTex = gPipeline.mObjectBumpProgram.enableTexture(LLPipeline::GLSL_DIFFUSE_MAP);
		S32 scatterTex = gPipeline.mObjectBumpProgram.enableTexture(LLPipeline::GLSL_SCATTER_MAP);
		LLViewerImage::bindTexture(gSky.mVOSkyp->getScatterMap(), scatterTex);
	}
	stop_glerror();
}

//static
S32 LLDrawPoolBump::renderPass0(LLDrawPool* pool, face_array_t& face_list, const U32* index_array, LLViewerImage* tex)
{
	if (!tex)
	{
		return 0;
	}
	
	if (face_list.empty())
	{
		return 0;
	}

	stop_glerror();

	S32 res = 0;
	if (pool->getVertexShaderLevel() > 0)
	{
		LLFastTimer t(LLFastTimer::FTM_RENDER_BUMP);
		pool->bindGLBinormalPointer(gPipeline.mObjectBumpProgram.mAttribute[LLPipeline::GLSL_BINORMAL]);
		
		LLViewerImage::bindTexture(tex, sDiffTex);

		//single pass shader driven shiny/bump
		LLGLDisable(GL_ALPHA_TEST);

		LLViewerImage::sWhiteImagep->bind(sBumpTex);
		
		GLfloat alpha[4] =
		{
			0.00f,
			0.25f,
			0.5f,
			0.75f
		};

		LLImageGL* last_bump = NULL;

		for (std::vector<LLFace*>::iterator iter = face_list.begin();
			 iter != face_list.end(); iter++)
		{
			LLFace *facep = *iter;
			if (facep->mSkipRender)
			{
				continue;
			}
			
			const LLTextureEntry* te = facep->getTextureEntry();
			if (te)
			{
				U8 index = te->getShiny();
				LLColor4 col = te->getColor();
				
				gPipeline.mObjectBumpProgram.vertexAttrib4f(LLPipeline::GLSL_MATERIAL_COLOR,
					col.mV[0], col.mV[1], col.mV[2], alpha[index]);
				gPipeline.mObjectBumpProgram.vertexAttrib4f(LLPipeline::GLSL_SPECULAR_COLOR, 
					alpha[index], alpha[index], alpha[index], alpha[index]);

				LLImageGL* bump = getBumpMap(te, tex);
				if (bump != last_bump)
				{
					if (bump)
					{
						bump->bind(sBumpTex);
					}
					else
					{
						LLViewerImage::sWhiteImagep->bind(sBumpTex);
					}
				}
				last_bump = bump;

				// Draw the geometry
				facep->enableLights();
				res += facep->renderIndexed(index_array);
				stop_glerror();
			}
			else
			{
				llwarns << "DrawPoolBump has face with invalid texture entry." << llendl;
			}
		}
	}
	else
	{
		LLFastTimer t(LLFastTimer::FTM_RENDER_SIMPLE);
		LLViewerImage::bindTexture(tex);
		res = LLDrawPool::drawLoop(face_list, index_array);
	}
	return res;
}

//static
void LLDrawPoolBump::endPass0(LLDrawPool* pool)
{
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	if (pool->getVertexShaderLevel() > 0)
	{
		gPipeline.mObjectBumpProgram.disableTexture(LLPipeline::GLSL_ENVIRONMENT_MAP, GL_TEXTURE_CUBE_MAP_ARB);
		LLCubeMap* cube_map = gSky.mVOSkyp->getCubeMap();
		if (sEnvTex >= 0 && cube_map)
		{
			cube_map->restoreMatrix();
		}
		
		gPipeline.mObjectBumpProgram.disableTexture(LLPipeline::GLSL_SCATTER_MAP);
		gPipeline.mObjectBumpProgram.disableTexture(LLPipeline::GLSL_BUMP_MAP);
		gPipeline.mObjectBumpProgram.disableTexture(LLPipeline::GLSL_DIFFUSE_MAP);

		disable_binormals(gPipeline.mObjectBumpProgram.mAttribute[LLPipeline::GLSL_BINORMAL]);
		
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glEnable(GL_TEXTURE_2D);
	}
}


//static
void LLDrawPoolBump::beginPass1()
{
	// Second pass: environment map
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	LLCubeMap* cube_map = gSky.mVOSkyp->getCubeMap();
	if( cube_map )
	{
		cube_map->enable(0);
		cube_map->setMatrix(0);
		cube_map->bind();
	}
}

//static
S32 LLDrawPoolBump::renderPass1(face_array_t& face_list, const U32* index_array, LLViewerImage* tex)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_SHINY);
	if (gPipeline.getVertexShaderLevel(LLPipeline::SHADER_OBJECT) > 0) //everything happens in pass0
	{
		return 0;
	}

	S32 res = 0;
	if( gSky.mVOSkyp->getCubeMap() )
	{
		//LLGLSPipelineAlpha gls;
		//LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_EQUAL);
		LLGLEnable blend_enable(GL_BLEND);
		
		GLfloat alpha[4] =
		{
			0.00f,
			0.25f,
			0.5f,
			0.75f
		};

		for (std::vector<LLFace*>::iterator iter = face_list.begin();
			 iter != face_list.end(); iter++)
		{
			LLFace *facep = *iter;
			if (facep->mSkipRender)
			{
				continue;
			}
			
			const LLTextureEntry* te = facep->getTextureEntry();
			if (te)
			{
				U8 index = te->getShiny();
				if( index > 0 )
				{
					LLOverrideFaceColor override_color(facep->getPool(), 1, 1, 1, alpha[index]);
				
					// Draw the geometry
					facep->enableLights();
					res += facep->renderIndexed(index_array);
					stop_glerror();
				}
			}
			else
			{
				llwarns << "DrawPoolBump has face with invalid texture entry." << llendl;
			}
		}
	}
	return res;
}

//static
void LLDrawPoolBump::endPass1()
{
	LLCubeMap* cube_map = gSky.mVOSkyp->getCubeMap();
	if( cube_map )
	{
		cube_map->disable();
		cube_map->restoreMatrix();
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_MODULATE);
	}
	
	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
	
	glDisableClientState(GL_NORMAL_ARRAY);
}


// static
LLImageGL* LLDrawPoolBump::getBumpMap(const LLTextureEntry* te, LLViewerImage* tex)
{
	U32 bump_code = te->getBumpmap();
	LLImageGL* bump = NULL;

	switch( bump_code )
	{
	case BE_NO_BUMP:
		bump = NULL;
		break;
	case BE_BRIGHTNESS: 
	case BE_DARKNESS:
		if( tex )
		{
			bump = gBumpImageList.getBrightnessDarknessImage( tex, bump_code );
		}
		break;

	default:
		if( bump_code < LLStandardBumpmap::sStandardBumpmapCount )
		{
			bump = gStandardBumpmapList[bump_code].mImage;
		}
		break;
	}

	return bump;
}

//static
void LLDrawPoolBump::beginPass2()
{	
	LLFastTimer t(LLFastTimer::FTM_RENDER_BUMP);
	// Optional third pass: emboss bump map
	stop_glerror();

	// TEXTURE UNIT 0
	// Output.rgb = texture at texture coord 0
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,	GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,	GL_REPLACE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,	GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,	GL_SRC_ALPHA);

	// Don't care about alpha output
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);


	// TEXTURE UNIT 1
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glEnable(GL_TEXTURE_2D); // Texture unit 1

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,	GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,	GL_ADD_SIGNED_ARB);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,	GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,	GL_SRC_COLOR);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,	GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,	GL_ONE_MINUS_SRC_ALPHA);

	// Don't care about alpha output
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

	// src	= tex0 + (1 - tex1) - 0.5
	//		= (bump0/2 + 0.5) + (1 - (bump1/2 + 0.5)) - 0.5
	//		= (1 + bump0 - bump1) / 2


	// Blend: src * dst + dst * src
	//		= 2 * src * dst
	//		= 2 * ((1 + bump0 - bump1) / 2) * dst   [0 - 2 * dst]
	//		= (1 + bump0 - bump1) * dst.rgb
	//		= dst.rgb + dst.rgb * (bump0 - bump1)
	glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
//	glBlendFunc(GL_ONE, GL_ZERO);  // temp

	stop_glerror();
}

//static
S32 LLDrawPoolBump::renderPass2(face_array_t& face_list, const U32* index_array, LLViewerImage* tex)
{
	if (gPipeline.getVertexShaderLevel(LLPipeline::SHADER_OBJECT) > 0) //everything happens in pass0
	{
		return 0;
	}

	LLGLDisable fog(GL_FOG);
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_EQUAL);
	LLGLEnable tex2d(GL_TEXTURE_2D);
	LLGLEnable blend(GL_BLEND);
	S32 res = 0;

	LLImageGL* last_bump = NULL;

	for (std::vector<LLFace*>::iterator iter = face_list.begin();
		 iter != face_list.end(); iter++)
	{
		LLFace *facep = *iter;
		if (facep->mSkipRender)
		{
			continue;
		}
		LLOverrideFaceColor override_color(facep->getPool(), 1,1,1,1);
	
		const LLTextureEntry* te = facep->getTextureEntry();
		LLImageGL* bump = getBumpMap(te, tex);
		
		if( bump )
		{
			if( bump != last_bump )
			{
				last_bump = bump;

				// Texture unit 0
				bump->bind(0);
				stop_glerror();

				// Texture unit 1
				bump->bind(1);
				stop_glerror();
			}

			// Draw the geometry
			res += facep->renderIndexed(index_array);
			stop_glerror();
		}
		else
		{
// 			llwarns << "Skipping invalid bump code " << (S32) te->getBumpmap() << llendl;
		}
	}
	return res;
}

//static
void LLDrawPoolBump::endPass2()
{
	// Disable texture unit 1
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D); // Texture unit 1
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// Disable texture unit 0
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}


void LLDrawPoolBump::renderForSelect()
{
	if (mDrawFace.empty() || !mMemory.count())
	{
		return;
	}

	glEnableClientState ( GL_VERTEX_ARRAY );

	bindGLVertexPointer();

	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
		 iter != mDrawFace.end(); iter++)
	{
		LLFace *facep = *iter;
		if (facep->getDrawable() && !facep->getDrawable()->isDead() && (facep->getViewerObject()->mGLName))
		{
			facep->renderForSelect();
		}
	}
}


void LLDrawPoolBump::renderFaceSelected(LLFace *facep, 
										LLImageGL *image, 
										const LLColor4 &color,
										const S32 index_offset, const S32 index_count)
{
	facep->renderSelected(image, color, index_offset, index_count);
}


void LLDrawPoolBump::dirtyTexture(const LLViewerImage *texturep)
{
	if (mTexturep == texturep)
	{
		for (std::vector<LLFace*>::iterator iter = mReferences.begin();
			 iter != mReferences.end(); iter++)
		{
			LLFace *facep = *iter;
			gPipeline.markTextured(facep->getDrawable());
		}
	}
}

LLViewerImage *LLDrawPoolBump::getTexture()
{
	return mTexturep;
}

LLViewerImage *LLDrawPoolBump::getDebugTexture()
{
	return mTexturep;
}

LLColor3 LLDrawPoolBump::getDebugColor() const
{
	return LLColor3(1.f, 1.f, 0.f);
}

////////////////////////////////////////////////////////////////
// List of one-component bump-maps created from other texures.


//const LLUUID TEST_BUMP_ID("3d33eaf2-459c-6f97-fd76-5fce3fc29447");

void LLBumpImageList::init()
{
	llassert( mBrightnessEntries.size() == 0 );
	llassert( mDarknessEntries.size() == 0 );

	LLStandardBumpmap::init();
}

void LLBumpImageList::shutdown()
{
	mBrightnessEntries.clear();
	mDarknessEntries.clear();
	LLStandardBumpmap::shutdown();
}

void LLBumpImageList::destroyGL()
{
	mBrightnessEntries.clear();
	mDarknessEntries.clear();
	LLStandardBumpmap::destroyGL();
}

void LLBumpImageList::restoreGL()
{
	// Images will be recreated as they are needed.
	LLStandardBumpmap::restoreGL();
}


LLBumpImageList::~LLBumpImageList()
{
	// Shutdown should have already been called.
	llassert( mBrightnessEntries.size() == 0 );
	llassert( mDarknessEntries.size() == 0 );
}


// Note: Does nothing for entries in gStandardBumpmapList that are not actually standard bump images (e.g. none, brightness, and darkness)
void LLBumpImageList::addTextureStats(U8 bump, const LLUUID& base_image_id,
									  F32 pixel_area, F32 texel_area_ratio, F32 cos_center_angle)
{
	bump &= TEM_BUMP_MASK;
	LLViewerImage* bump_image = gStandardBumpmapList[bump].mImage;
	if( bump_image )
	{
		bump_image->addTextureStats(pixel_area, texel_area_ratio, cos_center_angle);
	}
}


void LLBumpImageList::updateImages()
{	
	for (bump_image_map_t::iterator iter = mBrightnessEntries.begin(); iter != mBrightnessEntries.end(); )
	{
		bump_image_map_t::iterator curiter = iter++;
		LLImageGL* image = curiter->second;
		if( image )
		{
			BOOL destroy = TRUE;
			if( image->getHasGLTexture())
			{
				if( image->getBoundRecently() )
				{
					destroy = FALSE;
				}
				else
				{
					image->destroyGLTexture();
				}
			}

			if( destroy )
			{
				//llinfos << "*** Destroying bright " << (void*)image << llendl;
				mBrightnessEntries.erase(curiter);   // deletes the image thanks to reference counting
			}
		}
	}

	for (bump_image_map_t::iterator iter = mDarknessEntries.begin(); iter != mDarknessEntries.end(); )
	{
		bump_image_map_t::iterator curiter = iter++;
		LLImageGL* image = curiter->second;
		if( image )
		{
			BOOL destroy = TRUE;
			if( image->getHasGLTexture())
			{
				if( image->getBoundRecently() )
				{
					destroy = FALSE;
				}
				else
				{
					image->destroyGLTexture();
				}
			}

			if( destroy )
			{
				//llinfos << "*** Destroying dark " << (void*)image << llendl;;
				mDarknessEntries.erase(curiter);  // deletes the image thanks to reference counting
			}
		}
	}

}


// Note: the caller SHOULD NOT keep the pointer that this function returns.  It may be updated as more data arrives.
LLImageGL* LLBumpImageList::getBrightnessDarknessImage(LLViewerImage* src_image, U8 bump_code )
{
	llassert( (bump_code == BE_BRIGHTNESS) || (bump_code == BE_DARKNESS) );

	LLImageGL* bump = NULL;
	const F32 BRIGHTNESS_DARKNESS_PIXEL_AREA_THRESHOLD = 1000;
	if( src_image->mMaxVirtualSize > BRIGHTNESS_DARKNESS_PIXEL_AREA_THRESHOLD )
	{
		bump_image_map_t* entries_list = NULL;
		void (*callback_func)( BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata ) = NULL;

		switch( bump_code )
		{
		case BE_BRIGHTNESS:
			entries_list = &mBrightnessEntries;
			callback_func = LLBumpImageList::onSourceBrightnessLoaded;
			break;
		case BE_DARKNESS:
			entries_list = &mDarknessEntries;
			callback_func = LLBumpImageList::onSourceDarknessLoaded;
			break;
		default:
			llassert(0);
			return NULL;
		}

		bump_image_map_t::iterator iter = entries_list->find(src_image->getID());
		if (iter != entries_list->end())
		{
			bump = iter->second;
		}
		else
		{
			LLPointer<LLImageRaw> raw = new LLImageRaw(1,1,1);
			raw->clear(0x77, 0x77, 0x77, 0xFF);
			bump = new LLImageGL( raw, TRUE);
			bump->setExplicitFormat(GL_ALPHA8, GL_ALPHA);
			(*entries_list)[src_image->getID()] = bump;

			// Note: this may create an LLImageGL immediately
			src_image->setLoadedCallback( callback_func, 0, TRUE, new LLUUID(src_image->getID()) );
			bump = (*entries_list)[src_image->getID()]; // In case callback was called immediately and replaced the image

//			bump_total++;
//			llinfos << "*** Creating " << (void*)bump << " " << bump_total << llendl;
		}
	}

	return bump;
}


// static
void LLBumpImageList::onSourceBrightnessLoaded( BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata )
{
	LLUUID* source_asset_id = (LLUUID*)userdata;
	LLBumpImageList::onSourceLoaded( success, src_vi, src, *source_asset_id, BE_BRIGHTNESS );
	if( final )
	{
		delete source_asset_id;
	}
}

// static
void LLBumpImageList::onSourceDarknessLoaded( BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata )
{
	LLUUID* source_asset_id = (LLUUID*)userdata;
	LLBumpImageList::onSourceLoaded( success, src_vi, src, *source_asset_id, BE_DARKNESS );
	if( final )
	{
		delete source_asset_id;
	}
}


// static
void LLBumpImageList::onSourceLoaded( BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLUUID& source_asset_id, EBumpEffect bump_code )
{
	if( success )
	{
		bump_image_map_t& entries_list(bump_code == BE_BRIGHTNESS ? gBumpImageList.mBrightnessEntries : gBumpImageList.mDarknessEntries );
		bump_image_map_t::iterator iter = entries_list.find(source_asset_id);
		if (iter != entries_list.end())
		{
			LLPointer<LLImageRaw> dst_image = new LLImageRaw(src->getWidth(), src->getHeight(), 1);
			U8* dst_data = dst_image->getData();
			S32 dst_data_size = dst_image->getDataSize();

			U8* src_data = src->getData();
			S32 src_data_size = src->getDataSize();

			S32 src_components = src->getComponents();

			// Convert to luminance and then scale and bias that to get ready for
			// embossed bump mapping.  (0-255 maps to 127-255)

			// Convert to fixed point so we don't have to worry about precision/clamping.
			const S32 FIXED_PT = 8;
			const S32 R_WEIGHT = S32(0.2995f * (1<<FIXED_PT));
			const S32 G_WEIGHT = S32(0.5875f * (1<<FIXED_PT));
			const S32 B_WEIGHT = S32(0.1145f * (1<<FIXED_PT));

			S32 minimum = 255;
			S32 maximum = 0;

			switch( src_components )
			{
			case 1:
			case 2:
				if( src_data_size == dst_data_size * src_components )
				{
					for( S32 i = 0, j=0; i < dst_data_size; i++, j+= src_components )
					{
						dst_data[i] = src_data[j];
						if( dst_data[i] < minimum )
						{
							minimum = dst_data[i];
						}
						if( dst_data[i] > maximum )
						{
							maximum = dst_data[i];
						}
					}
				}
				else
				{
					llassert(0);
					dst_image->clear();
				}
				break;
			case 3:
			case 4:
				if( src_data_size == dst_data_size * src_components )
				{
					for( S32 i = 0, j=0; i < dst_data_size; i++, j+= src_components )
					{
						// RGB to luminance
						dst_data[i] = (R_WEIGHT * src_data[j] + G_WEIGHT * src_data[j+1] + B_WEIGHT * src_data[j+2]) >> FIXED_PT;
						//llassert( dst_data[i] <= 255 );true because it's 8bit
						if( dst_data[i] < minimum )
						{
							minimum = dst_data[i];
						}
						if( dst_data[i] > maximum )
						{
							maximum = dst_data[i];
						}
					}
				}
				else
				{
					llassert(0);
					dst_image->clear();
				}
				break;
			default:
				llassert(0);
				dst_image->clear();
				break;
			}

			if( maximum > minimum )
			{
				U8 bias_and_scale_lut[256];
				F32 twice_one_over_range = 2.f / (maximum - minimum);
				S32 i;

				const F32 ARTIFICIAL_SCALE = 2.f;  // Advantage: exagerates the effect in midrange.  Disadvantage: clamps at the extremes.
				if( BE_DARKNESS == bump_code )
				{
					for( i = minimum; i <= maximum; i++ )
					{
						F32 minus_one_to_one = F32(maximum - i) * twice_one_over_range - 1.f;
						bias_and_scale_lut[i] = llclampb(llround(127 * minus_one_to_one * ARTIFICIAL_SCALE + 128));
					}
				}
				else
				{
					// BE_LIGHTNESS
					for( i = minimum; i <= maximum; i++ )
					{
						F32 minus_one_to_one = F32(i - minimum) * twice_one_over_range - 1.f;
						bias_and_scale_lut[i] = llclampb(llround(127 * minus_one_to_one * ARTIFICIAL_SCALE + 128));
					}
				}

				for( i = 0; i < dst_data_size; i++ )
				{
					dst_data[i] = bias_and_scale_lut[dst_data[i]];
				}
			}

			LLImageGL* bump = new LLImageGL( TRUE);
			bump->setExplicitFormat(GL_ALPHA8, GL_ALPHA);
			bump->createGLTexture(0, dst_image);
			iter->second = bump; // derefs (and deletes) old image
		}
		else
		{
			// entry should have been added in LLBumpImageList::getImage().

			// Not a legit assertion - the bump texture could have been flushed by the bump image manager
			//llassert(0);
		}
	}
}

S32 LLDrawPoolBump::getMaterialAttribIndex()
{
	return gPipeline.mObjectBumpProgram.mAttribute[LLPipeline::GLSL_MATERIAL_COLOR];
}

// virtual
void LLDrawPoolBump::enableShade()
{
	glDisableClientState(GL_COLOR_ARRAY);
}

// virtual
void LLDrawPoolBump::disableShade()
{
	glEnableClientState(GL_COLOR_ARRAY);
}

// virtual
void LLDrawPoolBump::setShade(F32 shade)
{
	glColor4f(0,0,0,shade);
}
