/** 
 * @file lldrawpoolbump.cpp
 * @brief LLDrawPoolBump class implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

#include "lldrawpoolbump.h"

#include "llstl.h"
#include "llviewercontrol.h"
#include "lldir.h"
#include "llimagegl.h"
#include "m3math.h"
#include "m4math.h"
#include "v4math.h"
#include "llglheaders.h"
#include "llrender.h"

#include "llagent.h"
#include "llcubemap.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "lltextureentry.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llviewershadermgr.h"

//#include "llimagebmp.h"
//#include "../tools/imdebug/imdebug.h"

// static
LLStandardBumpmap gStandardBumpmapList[TEM_BUMPMAP_COUNT]; 

// static
U32 LLStandardBumpmap::sStandardBumpmapCount = 0;

// static
LLBumpImageList gBumpImageList;

const S32 STD_BUMP_LATEST_FILE_VERSION = 1;

const U32 VERTEX_MASK_SHINY = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_COLOR;
const U32 VERTEX_MASK_BUMP = LLVertexBuffer::MAP_VERTEX |LLVertexBuffer::MAP_TEXCOORD | LLVertexBuffer::MAP_TEXCOORD2;

U32 LLDrawPoolBump::sVertexMask = VERTEX_MASK_SHINY;
static LLPointer<LLCubeMap> sCubeMap;

static LLGLSLShader* shader = NULL;
static S32 cube_channel = -1;
static S32 diffuse_channel = -1;

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
	LLFILE* file = LLFile::fopen( file_name, "rt" );	 /*Flawfinder: ignore*/
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
		// *NOTE: This buffer size is hard coded into scanf() below.
		char label[2048] = "";	/* Flawfinder: ignore */
		char bump_file[2048] = "";	/* Flawfinder: ignore */
		fields_read = fscanf(	/* Flawfinder: ignore */
			file, "\n%2047s %2047s", label, bump_file);
		if( EOF == fields_read )
		{
			break;
		}
		if( fields_read != 2 )
		{
			llwarns << "Bad LLStandardBumpmap entry" << llendl;
			return;
		}

// 		llinfos << "Loading bumpmap: " << bump_file << " from viewerart" << llendl;
		gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mLabel = label;
		gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mImage = 
			gImageList.getImageFromFile(bump_file,
										TRUE, 
										FALSE, 
										0, 
										0);
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

LLDrawPoolBump::LLDrawPoolBump() 
:  LLRenderPass(LLDrawPool::POOL_BUMP)
{
	mShiny = FALSE;
}


void LLDrawPoolBump::prerender()
{
	mVertexShaderLevel = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}

// static
S32 LLDrawPoolBump::numBumpPasses()
{
	if (gSavedSettings.getBOOL("RenderObjectBump"))
	{
		if (mVertexShaderLevel > 1)
		{
			if (LLPipeline::sImpostorRender)
			{
				return 2;
			}
			else
			{
				return 3;
			}
		}
		else if (LLPipeline::sImpostorRender)
		{
			return 1;
		}
		else
		{
			return 2;
		}
	}
    else
	{
		return 0;
	}
}

S32 LLDrawPoolBump::getNumPasses()
{
	return numBumpPasses();
}

void LLDrawPoolBump::beginRenderPass(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_BUMP);
	switch( pass )
	{
		case 0:
			beginShiny();
			break;
		case 1:
			if (mVertexShaderLevel > 1)
			{
				beginFullbrightShiny();
			}
			else 
			{
				beginBump();
			}
			break;
		case 2:
			beginBump();
			break;
		default:
			llassert(0);
			break;
	}
}

void LLDrawPoolBump::render(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_BUMP);
	
	if (!gPipeline.hasRenderType(LLDrawPool::POOL_SIMPLE))
	{
		return;
	}
	
	switch( pass )
	{
		case 0:
			renderShiny();
			break;
		case 1:
			if (mVertexShaderLevel > 1)
			{
				renderFullbrightShiny();
			}
			else 
			{
				renderBump(); 
			}
			break;
		case 2:
			renderBump();
			break;
		default:
			llassert(0);
			break;
	}
}

void LLDrawPoolBump::endRenderPass(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_BUMP);
	switch( pass )
	{
		case 0:
			endShiny();
			break;
		case 1:
			if (mVertexShaderLevel > 1)
			{
				endFullbrightShiny();
			}
			else 
			{
				endBump();
			}
			break;
		case 2:
			endBump();
			break;
		default:
			llassert(0);
			break;
	}
}

//static
void LLDrawPoolBump::beginShiny(bool invisible)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_SHINY);
	if (!invisible && !gPipeline.hasRenderBatches(LLRenderPass::PASS_SHINY)|| 
		invisible && !gPipeline.hasRenderBatches(LLRenderPass::PASS_INVISI_SHINY))
	{
		return;
	}

	mShiny = TRUE;
	sVertexMask = VERTEX_MASK_SHINY;
	// Second pass: environment map
	if (!invisible && mVertexShaderLevel > 1)
	{
		sVertexMask = VERTEX_MASK_SHINY | LLVertexBuffer::MAP_TEXCOORD;
	}
	
	if (LLPipeline::sUnderWaterRender)
	{
		shader = &gObjectShinyWaterProgram;
	}
	else
	{
		shader = &gObjectShinyProgram;
	}

	LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
	if( cube_map )
	{
		if (!invisible && LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT) > 0 )
		{
			LLMatrix4 mat;
			mat.initRows(LLVector4(gGLModelView+0),
						 LLVector4(gGLModelView+4),
						 LLVector4(gGLModelView+8),
						 LLVector4(gGLModelView+12));
			shader->bind();
			LLVector3 vec = LLVector3(gShinyOrigin) * mat;
			LLVector4 vec4(vec, gShinyOrigin.mV[3]);
			shader->uniform4fv(LLViewerShaderMgr::SHINY_ORIGIN, 1, vec4.mV);			
			if (mVertexShaderLevel > 1)
			{
				cube_map->setMatrix(1);
				// Make sure that texture coord generation happens for tex unit 1, as that's the one we use for 
				// the cube map in the one pass shiny shaders
				cube_channel = shader->enableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, GL_TEXTURE_CUBE_MAP_ARB);
				cube_map->enableTexture(cube_channel);
				cube_map->enableTextureCoords(1);
				diffuse_channel = shader->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
			}
			else
			{
				cube_channel = 0;
				diffuse_channel = -1;
				cube_map->setMatrix(0);
				cube_map->enable(shader->enableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, GL_TEXTURE_CUBE_MAP_ARB));
			}			
			cube_map->bind();
		}
		else
		{
			cube_channel = 0;
			diffuse_channel = -1;
			cube_map->enable(0);
			cube_map->setMatrix(0);
			cube_map->bind();

			gGL.getTexUnit(0)->setTextureColorBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_TEX_COLOR);
			gGL.getTexUnit(0)->setTextureAlphaBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_VERT_ALPHA);
		}
	}
}

void LLDrawPoolBump::renderShiny(bool invisible)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_SHINY);
	if (!invisible && !gPipeline.hasRenderBatches(LLRenderPass::PASS_SHINY)|| 
		invisible && !gPipeline.hasRenderBatches(LLRenderPass::PASS_INVISI_SHINY))
	{
		return;
	}

	sCubeMap = NULL;

	if( gSky.mVOSkyp->getCubeMap() )
	{
		LLGLEnable blend_enable(GL_BLEND);
		if (!invisible && mVertexShaderLevel > 1)
		{
			LLRenderPass::renderTexture(LLRenderPass::PASS_SHINY, sVertexMask);
		}
		else if (!invisible)
		{
			renderGroups(LLRenderPass::PASS_SHINY, sVertexMask);
		}
		else // invisible
		{
			renderGroups(LLRenderPass::PASS_INVISI_SHINY, sVertexMask);
		}
	}
}

void LLDrawPoolBump::endShiny(bool invisible)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_SHINY);
	if (!invisible && !gPipeline.hasRenderBatches(LLRenderPass::PASS_SHINY)|| 
		invisible && !gPipeline.hasRenderBatches(LLRenderPass::PASS_INVISI_SHINY))
	{
		return;
	}

	LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
	if( cube_map )
	{
		cube_map->disable();
		cube_map->restoreMatrix();

		if (!invisible && mVertexShaderLevel > 1)
		{
			shader->disableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, GL_TEXTURE_CUBE_MAP_ARB);
					
			if (LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT) > 0)
			{
				if (diffuse_channel != 0)
				{
					shader->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
				}
			}

			shader->unbind();
			gGL.getTexUnit(0)->activate();
			glEnable(GL_TEXTURE_2D);
		}
		if (cube_channel >= 0)
		{
			gGL.getTexUnit(cube_channel)->setTextureBlendType(LLTexUnit::TB_MULT);
		}
	}
	
	gGL.getTexUnit(0)->activate();
	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
	gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	
	diffuse_channel = -1;
	cube_channel = 0;
	mShiny = FALSE;
}

void LLDrawPoolBump::beginFullbrightShiny()
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_SHINY);
	if (!gPipeline.hasRenderBatches(LLRenderPass::PASS_FULLBRIGHT_SHINY))
	{
		return;
	}

	sVertexMask = VERTEX_MASK_SHINY | LLVertexBuffer::MAP_TEXCOORD;

	// Second pass: environment map
	
	if (LLPipeline::sUnderWaterRender)
	{
		shader = &gObjectShinyWaterProgram;
	}
	else
	{
		shader = &gObjectFullbrightShinyProgram;
	}

	LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
	if( cube_map )
	{
		LLMatrix4 mat;
		mat.initRows(LLVector4(gGLModelView+0),
					 LLVector4(gGLModelView+4),
					 LLVector4(gGLModelView+8),
					 LLVector4(gGLModelView+12));
		shader->bind();
		LLVector3 vec = LLVector3(gShinyOrigin) * mat;
		LLVector4 vec4(vec, gShinyOrigin.mV[3]);
		shader->uniform4fv(LLViewerShaderMgr::SHINY_ORIGIN, 1, vec4.mV);			

		cube_map->setMatrix(1);
		// Make sure that texture coord generation happens for tex unit 1, as that's the one we use for 
		// the cube map in the one pass shiny shaders
		cube_channel = shader->enableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, GL_TEXTURE_CUBE_MAP_ARB);
		cube_map->enableTexture(cube_channel);
		cube_map->enableTextureCoords(1);
		diffuse_channel = shader->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);

		cube_map->bind();
	}
	mShiny = TRUE;
}

void LLDrawPoolBump::renderFullbrightShiny()
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_SHINY);
	if (!gPipeline.hasRenderBatches(LLRenderPass::PASS_FULLBRIGHT_SHINY))
	{
		return;
	}

	sCubeMap = NULL;

	if( gSky.mVOSkyp->getCubeMap() )
	{
		LLGLEnable blend_enable(GL_BLEND);
		LLRenderPass::renderTexture(LLRenderPass::PASS_FULLBRIGHT_SHINY, sVertexMask);
	}
}

void LLDrawPoolBump::endFullbrightShiny()
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_SHINY);
	if (!gPipeline.hasRenderBatches(LLRenderPass::PASS_FULLBRIGHT_SHINY))
	{
		return;
	}

	LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
	if( cube_map )
	{
		cube_map->disable();
		cube_map->restoreMatrix();

		if (diffuse_channel != 0)
		{
			shader->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
		}
		gGL.getTexUnit(0)->activate();
		glEnable(GL_TEXTURE_2D);

		shader->unbind();

		gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	}
	
	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
	gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);

	diffuse_channel = -1;
	cube_channel = 0;
	mShiny = FALSE;
}

void LLDrawPoolBump::renderGroup(LLSpatialGroup* group, U32 type, U32 mask, BOOL texture = TRUE)
{					
	LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[type];	
	
	for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k) 
	{
		LLDrawInfo& params = **k;
		
		applyModelMatrix(params);

		params.mVertexBuffer->setBuffer(mask);
		params.mVertexBuffer->drawRange(LLVertexBuffer::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
		gPipeline.addTrianglesDrawn(params.mCount/3);
	}
}


// static
BOOL LLDrawPoolBump::bindBumpMap(LLDrawInfo& params)
{
	LLImageGL* bump = NULL;

	U8 bump_code = params.mBump;
	LLViewerImage* tex = params.mTexture;

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
			gBumpImageList.addTextureStats(bump_code, tex->getID(), params.mVSize, 1, 1);
		}
		break;
	}

	if (bump)
	{
		bump->bind(1);
		bump->bind(0);
		return TRUE;
	}
	return FALSE;
}

//static
void LLDrawPoolBump::beginBump()
{	
	if (!gPipeline.hasRenderBatches(LLRenderPass::PASS_BUMP))
	{
		return;
	}

	sVertexMask = VERTEX_MASK_BUMP;
	LLFastTimer t(LLFastTimer::FTM_RENDER_BUMP);
	// Optional second pass: emboss bump map
	stop_glerror();

	// TEXTURE UNIT 0
	// Output.rgb = texture at texture coord 0
	gGL.getTexUnit(0)->activate();

	gGL.getTexUnit(0)->setTextureColorBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_TEX_ALPHA);
	gGL.getTexUnit(0)->setTextureAlphaBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_TEX_ALPHA);

	// TEXTURE UNIT 1
	gGL.getTexUnit(1)->activate();
 
	glEnable(GL_TEXTURE_2D); // Texture unit 1

	gGL.getTexUnit(1)->setTextureColorBlend(LLTexUnit::TBO_ADD_SIGNED, LLTexUnit::TBS_PREV_COLOR, LLTexUnit::TBS_ONE_MINUS_TEX_ALPHA);
	gGL.getTexUnit(1)->setTextureAlphaBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_TEX_ALPHA);

	// src	= tex0 + (1 - tex1) - 0.5
	//		= (bump0/2 + 0.5) + (1 - (bump1/2 + 0.5)) - 0.5
	//		= (1 + bump0 - bump1) / 2


	// Blend: src * dst + dst * src
	//		= 2 * src * dst
	//		= 2 * ((1 + bump0 - bump1) / 2) * dst   [0 - 2 * dst]
	//		= (1 + bump0 - bump1) * dst.rgb
	//		= dst.rgb + dst.rgb * (bump0 - bump1)
	gGL.setSceneBlendType(LLRender::BT_MULT_X2);
	gGL.getTexUnit(0)->activate();
	stop_glerror();

	LLViewerImage::unbindTexture(1, GL_TEXTURE_2D);
}

//static
void LLDrawPoolBump::renderBump()
{
	if (!gPipeline.hasRenderBatches(LLRenderPass::PASS_BUMP))
	{
		return;
	}

	LLFastTimer ftm(LLFastTimer::FTM_RENDER_BUMP);
	LLGLDisable fog(GL_FOG);
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_LEQUAL);
	LLGLEnable blend(GL_BLEND);
	glColor4f(1,1,1,1);
	/// Get rid of z-fighting with non-bump pass.
	LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1.0f, -1.0f);
	renderBump(LLRenderPass::PASS_BUMP, sVertexMask);
}

//static
void LLDrawPoolBump::endBump()
{
	if (!gPipeline.hasRenderBatches(LLRenderPass::PASS_BUMP))
	{
		return;
	}

	// Disable texture unit 1
	gGL.getTexUnit(1)->activate();
	glDisable(GL_TEXTURE_2D); // Texture unit 1
	gGL.getTexUnit(1)->setTextureBlendType(LLTexUnit::TB_MULT);

	// Disable texture unit 0
	gGL.getTexUnit(0)->activate();
	gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
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

			//------------------------------
			bump = new LLImageGL( raw, TRUE);
			//immediately assign bump to a global smart pointer in case some local smart pointer
			//accidently releases it.
			(*entries_list)[src_image->getID()] = bump;
			//------------------------------

			bump->setExplicitFormat(GL_ALPHA8, GL_ALPHA);			

			// Note: this may create an LLImageGL immediately
			src_image->setLoadedCallback( callback_func, 0, TRUE, FALSE, new LLUUID(src_image->getID()) );
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

			//---------------------------------------------------
			LLImageGL* bump = new LLImageGL( TRUE);
			//immediately assign bump to a global smart pointer in case some local smart pointer
			//accidently releases it.
			iter->second = bump; // derefs (and deletes) old image
			//---------------------------------------------------

			bump->setExplicitFormat(GL_ALPHA8, GL_ALPHA);
			bump->createGLTexture(0, dst_image);			
		}
		else
		{
			// entry should have been added in LLBumpImageList::getImage().

			// Not a legit assertion - the bump texture could have been flushed by the bump image manager
			//llassert(0);
		}
	}
}

void LLDrawPoolBump::renderBump(U32 type, U32 mask)
{	
	LLCullResult::drawinfo_list_t::iterator begin = gPipeline.beginRenderMap(type);
	LLCullResult::drawinfo_list_t::iterator end = gPipeline.endRenderMap(type);

	for (LLCullResult::drawinfo_list_t::iterator i = begin; i != end; ++i)	
	{
		LLDrawInfo& params = **i;

		if (LLDrawPoolBump::bindBumpMap(params))
		{
			pushBatch(params, mask, FALSE);
		}
	}
}

void LLDrawPoolBump::renderGroupBump(LLSpatialGroup* group, U32 type, U32 mask)
{					
	LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[type];	
	
	for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k) 
	{
		LLDrawInfo& params = **k;
		
		if (LLDrawPoolBump::bindBumpMap(params))
		{
			pushBatch(params, mask, FALSE);
		}
	}
}

void LLDrawPoolBump::pushBatch(LLDrawInfo& params, U32 mask, BOOL texture)
{
	applyModelMatrix(params);

	if (params.mTextureMatrix)
	{
		if (mShiny)
		{
			gGL.getTexUnit(0)->activate();
			glMatrixMode(GL_TEXTURE);
		}
		else
		{
			gGL.getTexUnit(1)->activate();
			glMatrixMode(GL_TEXTURE);
			glLoadMatrixf((GLfloat*) params.mTextureMatrix->mMatrix);
			gPipeline.mTextureMatrixOps++;
			gGL.getTexUnit(0)->activate();
		}

		glLoadMatrixf((GLfloat*) params.mTextureMatrix->mMatrix);
		gPipeline.mTextureMatrixOps++;
	}

	if (mShiny && mVertexShaderLevel > 1 && texture)
	{
		if (params.mTexture.notNull())
		{
			params.mTexture->bind(diffuse_channel);
			params.mTexture->addTextureStats(params.mVSize);
		}
		else
		{
			LLImageGL::unbindTexture(0);
		}
	}
	
	params.mVertexBuffer->setBuffer(mask);
	params.mVertexBuffer->drawRange(LLVertexBuffer::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
	gPipeline.addTrianglesDrawn(params.mCount/3);
	if (params.mTextureMatrix)
	{
		if (mShiny)
		{
			gGL.getTexUnit(0)->activate();
		}
		else
		{
			gGL.getTexUnit(1)->activate();
			glLoadIdentity();
			gGL.getTexUnit(0)->activate();
		}
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
}

void LLDrawPoolInvisible::render(S32 pass)
{ //render invisiprims
	LLFastTimer t(LLFastTimer::FTM_RENDER_INVISIBLE);
  
	U32 invisi_mask = LLVertexBuffer::MAP_VERTEX;
	glStencilMask(0);
	gGL.setColorMask(false, false);
	pushBatches(LLRenderPass::PASS_INVISIBLE, invisi_mask, FALSE);
	gGL.setColorMask(true, false);
	glStencilMask(0xFFFFFFFF);

	if (gPipeline.hasRenderBatches(LLRenderPass::PASS_INVISI_SHINY))
	{
		beginShiny(true);
		renderShiny(true);
		endShiny(true);
	}
}

