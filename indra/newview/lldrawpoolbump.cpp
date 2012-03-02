/** 
 * @file lldrawpoolbump.cpp
 * @brief LLDrawPoolBump class implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolbump.h"

#include "llstl.h"
#include "llviewercontrol.h"
#include "lldir.h"
#include "m3math.h"
#include "m4math.h"
#include "v4math.h"
#include "llglheaders.h"
#include "llrender.h"

#include "llcubemap.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "lltextureentry.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
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
const U32 VERTEX_MASK_BUMP = LLVertexBuffer::MAP_VERTEX |LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_TEXCOORD1;

U32 LLDrawPoolBump::sVertexMask = VERTEX_MASK_SHINY;


static LLGLSLShader* shader = NULL;
static S32 cube_channel = -1;
static S32 diffuse_channel = -1;
static S32 bump_channel = -1;

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
	addstandard();
}

// static
void LLStandardBumpmap::addstandard()
{
	if(!gTextureList.isInitialized())
	{
		//Note: loading pre-configuration sometimes triggers this call.
		//But it is safe to return here because bump images will be reloaded during initialization later.
		return ;
	}

	// can't assert; we destroyGL and restoreGL a lot during *first* startup, which populates this list already, THEN we explicitly init the list as part of *normal* startup.  Sigh.  So clear the list every time before we (re-)add the standard bumpmaps.
	//llassert( LLStandardBumpmap::sStandardBumpmapCount == 0 );
	clear();
	llinfos << "Adding standard bumpmaps." << llendl;
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
		char bump_image_id[2048] = "";	/* Flawfinder: ignore */
		fields_read = fscanf(	/* Flawfinder: ignore */
			file, "\n%2047s %2047s", label, bump_image_id);
		if( EOF == fields_read )
		{
			break;
		}
		if( fields_read != 2 )
		{
			llwarns << "Bad LLStandardBumpmap entry" << llendl;
			return;
		}

// 		llinfos << "Loading bumpmap: " << bump_image_id << " from viewerart" << llendl;
		gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mLabel = label;
		gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mImage = 
			LLViewerTextureManager::getFetchedTexture(LLUUID(bump_image_id));	
		gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mImage->setBoostLevel(LLViewerTexture::BOOST_BUMP) ;
		gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mImage->setLoadedCallback(LLBumpImageList::onSourceStandardLoaded, 0, TRUE, FALSE, NULL, NULL );
		gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mImage->forceToSaveRawImage(0) ;
		LLStandardBumpmap::sStandardBumpmapCount++;
	}

	fclose( file );
}

// static
void LLStandardBumpmap::clear()
{
	llinfos << "Clearing standard bumpmaps." << llendl;
	for( U32 i = 0; i < LLStandardBumpmap::sStandardBumpmapCount; i++ )
	{
		gStandardBumpmapList[i].mLabel.assign("");
		gStandardBumpmapList[i].mImage = NULL;
	}
	sStandardBumpmapCount = 0;
}

// static
void LLStandardBumpmap::destroyGL()
{
	clear();
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
	LLFastTimer t(FTM_RENDER_BUMP);
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
	LLFastTimer t(FTM_RENDER_BUMP);
	
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
	LLFastTimer t(FTM_RENDER_BUMP);
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

	//to cleanup texture channels
	LLRenderPass::endRenderPass(pass);
}

//static
void LLDrawPoolBump::beginShiny(bool invisible)
{
	LLFastTimer t(FTM_RENDER_SHINY);
	if ((!invisible && !gPipeline.hasRenderBatches(LLRenderPass::PASS_SHINY))|| 
		(invisible && !gPipeline.hasRenderBatches(LLRenderPass::PASS_INVISI_SHINY)))
	{
		return;
	}

	mShiny = TRUE;
	sVertexMask = VERTEX_MASK_SHINY;
	// Second pass: environment map
	if (!invisible && mVertexShaderLevel > 1)
	{
		sVertexMask = VERTEX_MASK_SHINY | LLVertexBuffer::MAP_TEXCOORD0;
	}
	
	if (getVertexShaderLevel() > 0)
	{
		if (LLPipeline::sUnderWaterRender)
		{
			shader = &gObjectShinyWaterProgram;
		}
		else
		{
			shader = &gObjectShinyProgram;
		}
		shader->bind();
	}
	else
	{
		shader = NULL;
	}

	bindCubeMap(shader, mVertexShaderLevel, diffuse_channel, cube_channel, invisible);

	if (mVertexShaderLevel > 1)
	{ //indexed texture rendering, channel 0 is always diffuse
		diffuse_channel = 0;
	}
}

//static
void LLDrawPoolBump::bindCubeMap(LLGLSLShader* shader, S32 shader_level, S32& diffuse_channel, S32& cube_channel, bool invisible)
{
	LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
	if( cube_map )
	{
		if (!invisible && shader )
		{
			LLMatrix4 mat;
			mat.initRows(LLVector4(gGLModelView+0),
						 LLVector4(gGLModelView+4),
						 LLVector4(gGLModelView+8),
						 LLVector4(gGLModelView+12));
			LLVector3 vec = LLVector3(gShinyOrigin) * mat;
			LLVector4 vec4(vec, gShinyOrigin.mV[3]);
			shader->uniform4fv(LLViewerShaderMgr::SHINY_ORIGIN, 1, vec4.mV);			
			if (shader_level > 1)
			{
				cube_map->setMatrix(1);
				// Make sure that texture coord generation happens for tex unit 1, as that's the one we use for 
				// the cube map in the one pass shiny shaders
				cube_channel = shader->enableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
				cube_map->enableTexture(cube_channel);
				cube_map->enableTextureCoords(1);
				diffuse_channel = shader->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
			}
			else
			{
				cube_channel = shader->enableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
				diffuse_channel = -1;
				cube_map->setMatrix(0);
				cube_map->enable(cube_channel);
			}			
			gGL.getTexUnit(cube_channel)->bind(cube_map);
			gGL.getTexUnit(0)->activate();
		}
		else
		{
			cube_channel = 0;
			diffuse_channel = -1;
			gGL.getTexUnit(0)->disable();
			cube_map->enable(0);
			cube_map->setMatrix(0);
			gGL.getTexUnit(0)->bind(cube_map);

			gGL.getTexUnit(0)->setTextureColorBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_TEX_COLOR);
			gGL.getTexUnit(0)->setTextureAlphaBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_VERT_ALPHA);
		}
	}
}

void LLDrawPoolBump::renderShiny(bool invisible)
{
	LLFastTimer t(FTM_RENDER_SHINY);
	if ((!invisible && !gPipeline.hasRenderBatches(LLRenderPass::PASS_SHINY))|| 
		(invisible && !gPipeline.hasRenderBatches(LLRenderPass::PASS_INVISI_SHINY)))
	{
		return;
	}

	if( gSky.mVOSkyp->getCubeMap() )
	{
		LLGLEnable blend_enable(GL_BLEND);
		if (!invisible && mVertexShaderLevel > 1)
		{
			LLRenderPass::pushBatches(LLRenderPass::PASS_SHINY, sVertexMask | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
		}
		else if (!invisible)
		{
			renderGroups(LLRenderPass::PASS_SHINY, sVertexMask);
		}
		//else // invisible (deprecated)
		//{
			//renderGroups(LLRenderPass::PASS_INVISI_SHINY, sVertexMask);
		//}
	}
}

//static
void LLDrawPoolBump::unbindCubeMap(LLGLSLShader* shader, S32 shader_level, S32& diffuse_channel, S32& cube_channel, bool invisible)
{
	LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
	if( cube_map )
	{
		cube_map->disable();
		cube_map->restoreMatrix();

		if (!invisible && shader_level > 1)
		{
			shader->disableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
					
			if (LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_OBJECT) > 0)
			{
				if (diffuse_channel != 0)
				{
					shader->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
				}
			}
		}
	}

	if (!LLGLSLShader::sNoFixedFunction)
	{
		gGL.getTexUnit(diffuse_channel)->disable();
		gGL.getTexUnit(cube_channel)->disable();

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	}
}

void LLDrawPoolBump::endShiny(bool invisible)
{
	LLFastTimer t(FTM_RENDER_SHINY);
	if ((!invisible && !gPipeline.hasRenderBatches(LLRenderPass::PASS_SHINY))|| 
		(invisible && !gPipeline.hasRenderBatches(LLRenderPass::PASS_INVISI_SHINY)))
	{
		return;
	}

	unbindCubeMap(shader, mVertexShaderLevel, diffuse_channel, cube_channel, invisible);
	if (shader)
	{
		shader->unbind();
	}

	diffuse_channel = -1;
	cube_channel = 0;
	mShiny = FALSE;
}

void LLDrawPoolBump::beginFullbrightShiny()
{
	LLFastTimer t(FTM_RENDER_SHINY);
	if (!gPipeline.hasRenderBatches(LLRenderPass::PASS_FULLBRIGHT_SHINY))
	{
		return;
	}

	sVertexMask = VERTEX_MASK_SHINY | LLVertexBuffer::MAP_TEXCOORD0;

	// Second pass: environment map
	
	if (LLPipeline::sUnderWaterRender)
	{
		shader = &gObjectFullbrightShinyWaterProgram;
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
		gGL.getTexUnit(1)->disable();
		cube_channel = shader->enableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
		cube_map->enableTexture(cube_channel);
		cube_map->enableTextureCoords(1);
		diffuse_channel = shader->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);

		gGL.getTexUnit(cube_channel)->bind(cube_map);
		gGL.getTexUnit(0)->activate();
	}

	if (mVertexShaderLevel > 1)
	{ //indexed texture rendering, channel 0 is always diffuse
		diffuse_channel = 0;
	}

	mShiny = TRUE;
}

void LLDrawPoolBump::renderFullbrightShiny()
{
	LLFastTimer t(FTM_RENDER_SHINY);
	if (!gPipeline.hasRenderBatches(LLRenderPass::PASS_FULLBRIGHT_SHINY))
	{
		return;
	}

	if( gSky.mVOSkyp->getCubeMap() )
	{
		LLGLEnable blend_enable(GL_BLEND);

		if (mVertexShaderLevel > 1)
		{
			LLRenderPass::pushBatches(LLRenderPass::PASS_FULLBRIGHT_SHINY, sVertexMask | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, TRUE);
		}
		else
		{
			LLRenderPass::renderTexture(LLRenderPass::PASS_FULLBRIGHT_SHINY, sVertexMask);
		}
	}
}

void LLDrawPoolBump::endFullbrightShiny()
{
	LLFastTimer t(FTM_RENDER_SHINY);
	if (!gPipeline.hasRenderBatches(LLRenderPass::PASS_FULLBRIGHT_SHINY))
	{
		return;
	}

	LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
	if( cube_map )
	{
		cube_map->disable();
		cube_map->restoreMatrix();

		/*if (diffuse_channel != 0)
		{
			shader->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
		}
		gGL.getTexUnit(0)->activate();
		gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);*/

		shader->unbind();
		//gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	}
	
	//gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	//gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);

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

		if (params.mGroup)
		{
			params.mGroup->rebuildMesh();
		}
		params.mVertexBuffer->setBuffer(mask);
		params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
		gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);
	}
}


// static
BOOL LLDrawPoolBump::bindBumpMap(LLDrawInfo& params, S32 channel)
{
	U8 bump_code = params.mBump;

	return bindBumpMap(bump_code, params.mTexture, params.mVSize, channel);
}

//static
BOOL LLDrawPoolBump::bindBumpMap(LLFace* face, S32 channel)
{
	const LLTextureEntry* te = face->getTextureEntry();
	if (te)
	{
		U8 bump_code = te->getBumpmap();
		return bindBumpMap(bump_code, face->getTexture(), face->getVirtualSize(), channel);
	}

	return FALSE;
}

//static
BOOL LLDrawPoolBump::bindBumpMap(U8 bump_code, LLViewerTexture* texture, F32 vsize, S32 channel)
{
	//Note: texture atlas does not support bump texture now.
	LLViewerFetchedTexture* tex = LLViewerTextureManager::staticCastToFetchedTexture(texture) ;
	if(!tex)
	{
		//if the texture is not a fetched texture
		return FALSE;
	}

	LLViewerTexture* bump = NULL;

	switch( bump_code )
	{
	case BE_NO_BUMP:		
		break;
	case BE_BRIGHTNESS: 
	case BE_DARKNESS:
		bump = gBumpImageList.getBrightnessDarknessImage( tex, bump_code );		
		break;

	default:
		if( bump_code < LLStandardBumpmap::sStandardBumpmapCount )
		{
			bump = gStandardBumpmapList[bump_code].mImage;
			gBumpImageList.addTextureStats(bump_code, tex->getID(), vsize);
		}
		break;
	}

	if (bump)
	{
		if (channel == -2)
		{
			gGL.getTexUnit(1)->bind(bump);
			gGL.getTexUnit(0)->bind(bump);
		}
		else
		{
			gGL.getTexUnit(channel)->bind(bump);
		}

		return TRUE;
	}

	return FALSE;
}

//static
void LLDrawPoolBump::beginBump(U32 pass)
{	
	if (!gPipeline.hasRenderBatches(pass))
	{
		return;
	}

	sVertexMask = VERTEX_MASK_BUMP;
	LLFastTimer t(FTM_RENDER_BUMP);
	// Optional second pass: emboss bump map
	stop_glerror();

	if (LLGLSLShader::sNoFixedFunction)
	{
		gObjectBumpProgram.bind();
	}
	else
	{
		// TEXTURE UNIT 0
		// Output.rgb = texture at texture coord 0
		gGL.getTexUnit(0)->activate();

		gGL.getTexUnit(0)->setTextureColorBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_TEX_ALPHA);
		gGL.getTexUnit(0)->setTextureAlphaBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_TEX_ALPHA);

		// TEXTURE UNIT 1
		gGL.getTexUnit(1)->activate();
 
		gGL.getTexUnit(1)->enable(LLTexUnit::TT_TEXTURE);

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

		gGL.getTexUnit(0)->activate();
		gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);
	}

	gGL.setSceneBlendType(LLRender::BT_MULT_X2);
	stop_glerror();
}

//static
void LLDrawPoolBump::renderBump(U32 pass)
{
	if (!gPipeline.hasRenderBatches(pass))
	{
		return;
	}

	LLFastTimer ftm(FTM_RENDER_BUMP);
	LLGLDisable fog(GL_FOG);
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_LEQUAL);
	LLGLEnable blend(GL_BLEND);
	gGL.diffuseColor4f(1,1,1,1);
	/// Get rid of z-fighting with non-bump pass.
	LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1.0f, -1.0f);
	renderBump(pass, sVertexMask);
}

//static
void LLDrawPoolBump::endBump(U32 pass)
{
	if (!gPipeline.hasRenderBatches(pass))
	{
		return;
	}

	if (LLGLSLShader::sNoFixedFunction)
	{
		gObjectBumpProgram.unbind();
	}
	else
	{
		// Disable texture blending on unit 1
		gGL.getTexUnit(1)->activate();
		gGL.getTexUnit(1)->disable();
		gGL.getTexUnit(1)->setTextureBlendType(LLTexUnit::TB_MULT);

		// Disable texture blending on unit 0
		gGL.getTexUnit(0)->activate();
		gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
	}
	
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
}

S32 LLDrawPoolBump::getNumDeferredPasses()
{ 
	if (gSavedSettings.getBOOL("RenderObjectBump"))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void LLDrawPoolBump::beginDeferredPass(S32 pass)
{
	if (!gPipeline.hasRenderBatches(LLRenderPass::PASS_BUMP))
	{
		return;
	}
	LLFastTimer ftm(FTM_RENDER_BUMP);
	mShiny = TRUE;
	gDeferredBumpProgram.bind();
	diffuse_channel = gDeferredBumpProgram.enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	bump_channel = gDeferredBumpProgram.enableTexture(LLViewerShaderMgr::BUMP_MAP);
	gGL.getTexUnit(diffuse_channel)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.getTexUnit(bump_channel)->unbind(LLTexUnit::TT_TEXTURE);
}

void LLDrawPoolBump::endDeferredPass(S32 pass)
{
	if (!gPipeline.hasRenderBatches(LLRenderPass::PASS_BUMP))
	{
		return;
	}
	LLFastTimer ftm(FTM_RENDER_BUMP);
	mShiny = FALSE;
	gDeferredBumpProgram.disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	gDeferredBumpProgram.disableTexture(LLViewerShaderMgr::BUMP_MAP);
	gDeferredBumpProgram.unbind();
	gGL.getTexUnit(0)->activate();
}

void LLDrawPoolBump::renderDeferred(S32 pass)
{
	if (!gPipeline.hasRenderBatches(LLRenderPass::PASS_BUMP))
	{
		return;
	}
	LLFastTimer ftm(FTM_RENDER_BUMP);

	U32 type = LLRenderPass::PASS_BUMP;
	LLCullResult::drawinfo_list_t::iterator begin = gPipeline.beginRenderMap(type);
	LLCullResult::drawinfo_list_t::iterator end = gPipeline.endRenderMap(type);

	U32 mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_BINORMAL | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_COLOR;
	
	for (LLCullResult::drawinfo_list_t::iterator i = begin; i != end; ++i)	
	{
		LLDrawInfo& params = **i;

		LLDrawPoolBump::bindBumpMap(params, bump_channel);
		pushBatch(params, mask, TRUE);
	}
}

void LLDrawPoolBump::beginPostDeferredPass(S32 pass)
{
	switch (pass)
	{
	case 0:
		beginFullbrightShiny();
		break;
	case 1:
		beginBump(LLRenderPass::PASS_POST_BUMP);
		break;
	}
}

void LLDrawPoolBump::endPostDeferredPass(S32 pass)
{
	switch (pass)
	{
	case 0:
		endFullbrightShiny();
		break;
	case 1:
		endBump(LLRenderPass::PASS_POST_BUMP);
		break;
	}

	//to disable texture channels
	LLRenderPass::endRenderPass(pass);
}

void LLDrawPoolBump::renderPostDeferred(S32 pass)
{
	switch (pass)
	{
	case 0:
		renderFullbrightShiny();
		break;
	case 1:
		renderBump(LLRenderPass::PASS_POST_BUMP);
		break;
	}
}

////////////////////////////////////////////////////////////////
// List of bump-maps created from other textures.


//const LLUUID TEST_BUMP_ID("3d33eaf2-459c-6f97-fd76-5fce3fc29447");

void LLBumpImageList::init()
{
	llassert( mBrightnessEntries.size() == 0 );
	llassert( mDarknessEntries.size() == 0 );

	LLStandardBumpmap::init();
}

void LLBumpImageList::clear()
{
	llinfos << "Clearing dynamic bumpmaps." << llendl;
	// these will be re-populated on-demand
	mBrightnessEntries.clear();
	mDarknessEntries.clear();

	LLStandardBumpmap::clear();
}

void LLBumpImageList::shutdown()
{
	clear();
	LLStandardBumpmap::shutdown();
}

void LLBumpImageList::destroyGL()
{
	clear();
	LLStandardBumpmap::destroyGL();
}

void LLBumpImageList::restoreGL()
{
	if(!gTextureList.isInitialized())
	{
		//safe to return here because bump images will be reloaded during initialization later.
		return ;
	}

	LLStandardBumpmap::restoreGL();
	// Images will be recreated as they are needed.
}


LLBumpImageList::~LLBumpImageList()
{
	// Shutdown should have already been called.
	llassert( mBrightnessEntries.size() == 0 );
	llassert( mDarknessEntries.size() == 0 );
}


// Note: Does nothing for entries in gStandardBumpmapList that are not actually standard bump images (e.g. none, brightness, and darkness)
void LLBumpImageList::addTextureStats(U8 bump, const LLUUID& base_image_id, F32 virtual_size)
{
	bump &= TEM_BUMP_MASK;
	LLViewerFetchedTexture* bump_image = gStandardBumpmapList[bump].mImage;
	if( bump_image )
	{		
		bump_image->addTextureStats(virtual_size);
	}
}


void LLBumpImageList::updateImages()
{	
	for (bump_image_map_t::iterator iter = mBrightnessEntries.begin(); iter != mBrightnessEntries.end(); )
	{
		bump_image_map_t::iterator curiter = iter++;
		LLViewerTexture* image = curiter->second;
		if( image )
		{
			BOOL destroy = TRUE;
			if( image->hasGLTexture())
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
		LLViewerTexture* image = curiter->second;
		if( image )
		{
			BOOL destroy = TRUE;
			if( image->hasGLTexture())
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
LLViewerTexture* LLBumpImageList::getBrightnessDarknessImage(LLViewerFetchedTexture* src_image, U8 bump_code )
{
	llassert( (bump_code == BE_BRIGHTNESS) || (bump_code == BE_DARKNESS) );

	LLViewerTexture* bump = NULL;
	
	bump_image_map_t* entries_list = NULL;
	void (*callback_func)( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata ) = NULL;

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
	if (iter != entries_list->end() && iter->second.notNull())
	{
		bump = iter->second;
	}
	else
	{
		LLPointer<LLImageRaw> raw = new LLImageRaw(1,1,1);
		raw->clear(0x77, 0x77, 0xFF, 0xFF);

		(*entries_list)[src_image->getID()] = LLViewerTextureManager::getLocalTexture( raw.get(), TRUE);
		bump = (*entries_list)[src_image->getID()]; // In case callback was called immediately and replaced the image
	}

	if (!src_image->hasCallbacks())
	{ //if image has no callbacks but resolutions don't match, trigger raw image loaded callback again
		if (src_image->getWidth() != bump->getWidth() ||
			src_image->getHeight() != bump->getHeight())// ||
			//(LLPipeline::sRenderDeferred && bump->getComponents() != 4))
		{
			src_image->setBoostLevel(LLViewerTexture::BOOST_BUMP) ;
			src_image->setLoadedCallback( callback_func, 0, TRUE, FALSE, new LLUUID(src_image->getID()), NULL );
			src_image->forceToSaveRawImage(0) ;
		}
	}

	return bump;
}


static LLFastTimer::DeclareTimer FTM_BUMP_SOURCE_STANDARD_LOADED("Bump Standard Callback");

// static
void LLBumpImageList::onSourceBrightnessLoaded( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata )
{
	LLUUID* source_asset_id = (LLUUID*)userdata;
	LLBumpImageList::onSourceLoaded( success, src_vi, src, *source_asset_id, BE_BRIGHTNESS );
	if( final )
	{
		delete source_asset_id;
	}
}

// static
void LLBumpImageList::onSourceDarknessLoaded( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata )
{
	LLUUID* source_asset_id = (LLUUID*)userdata;
	LLBumpImageList::onSourceLoaded( success, src_vi, src, *source_asset_id, BE_DARKNESS );
	if( final )
	{
		delete source_asset_id;
	}
}

static LLFastTimer::DeclareTimer FTM_BUMP_GEN_NORMAL("Generate Normal Map");
static LLFastTimer::DeclareTimer FTM_BUMP_CREATE_TEXTURE("Create GL Normal Map");

void LLBumpImageList::onSourceStandardLoaded( BOOL success, LLViewerFetchedTexture* src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata)
{
	if (success && LLPipeline::sRenderDeferred)
	{
		LLFastTimer t(FTM_BUMP_SOURCE_STANDARD_LOADED);
		LLPointer<LLImageRaw> nrm_image = new LLImageRaw(src->getWidth(), src->getHeight(), 4);
		{
			LLFastTimer t(FTM_BUMP_GEN_NORMAL);
			generateNormalMapFromAlpha(src, nrm_image);
		}
		src_vi->setExplicitFormat(GL_RGBA, GL_RGBA);
		{
			LLFastTimer t(FTM_BUMP_CREATE_TEXTURE);
			src_vi->createGLTexture(src_vi->getDiscardLevel(), nrm_image);
		}
	}
}

void LLBumpImageList::generateNormalMapFromAlpha(LLImageRaw* src, LLImageRaw* nrm_image)
{
	U8* nrm_data = nrm_image->getData();
	S32 resX = src->getWidth();
	S32 resY = src->getHeight();

	U8* src_data = src->getData();

	S32 src_cmp = src->getComponents();

	F32 norm_scale = gSavedSettings.getF32("RenderNormalMapScale");

	U32 idx = 0;
	//generate normal map from pseudo-heightfield
	for (S32 j = 0; j < resY; ++j)
	{
		for (S32 i = 0; i < resX; ++i)
		{
			S32 rX = (i+1)%resX;
			S32 rY = (j+1)%resY;
			S32 lX = (i-1)%resX;
			S32 lY = (j-1)%resY;

			if (lX < 0)
			{
				lX += resX;
			}
			if (lY < 0)
			{
				lY += resY;
			}

			F32 cH = (F32) src_data[(j*resX+i)*src_cmp+src_cmp-1];

			LLVector3 right = LLVector3(norm_scale, 0, (F32) src_data[(j*resX+rX)*src_cmp+src_cmp-1]-cH);
			LLVector3 left = LLVector3(-norm_scale, 0, (F32) src_data[(j*resX+lX)*src_cmp+src_cmp-1]-cH);
			LLVector3 up = LLVector3(0, -norm_scale, (F32) src_data[(lY*resX+i)*src_cmp+src_cmp-1]-cH);
			LLVector3 down = LLVector3(0, norm_scale, (F32) src_data[(rY*resX+i)*src_cmp+src_cmp-1]-cH);

			LLVector3 norm = right%down + down%left + left%up + up%right;
		
			norm.normVec();
			
			norm *= 0.5f;
			norm += LLVector3(0.5f,0.5f,0.5f);

			idx = (j*resX+i)*4;
			nrm_data[idx+0]= (U8) (norm.mV[0]*255);
			nrm_data[idx+1]= (U8) (norm.mV[1]*255);
			nrm_data[idx+2]= (U8) (norm.mV[2]*255);
			nrm_data[idx+3]= src_data[(j*resX+i)*src_cmp+src_cmp-1];
		}
	}
}


static LLFastTimer::DeclareTimer FTM_BUMP_SOURCE_LOADED("Bump Source Loaded");
static LLFastTimer::DeclareTimer FTM_BUMP_SOURCE_ENTRIES_UPDATE("Entries Update");
static LLFastTimer::DeclareTimer FTM_BUMP_SOURCE_MIN_MAX("Min/Max");
static LLFastTimer::DeclareTimer FTM_BUMP_SOURCE_RGB2LUM("RGB to Luminance");
static LLFastTimer::DeclareTimer FTM_BUMP_SOURCE_RESCALE("Rescale");
static LLFastTimer::DeclareTimer FTM_BUMP_SOURCE_GEN_NORMAL("Generate Normal");
static LLFastTimer::DeclareTimer FTM_BUMP_SOURCE_CREATE("Create");

// static
void LLBumpImageList::onSourceLoaded( BOOL success, LLViewerTexture *src_vi, LLImageRaw* src, LLUUID& source_asset_id, EBumpEffect bump_code )
{
	if( success )
	{
		LLFastTimer t(FTM_BUMP_SOURCE_LOADED);


		bump_image_map_t& entries_list(bump_code == BE_BRIGHTNESS ? gBumpImageList.mBrightnessEntries : gBumpImageList.mDarknessEntries );
		bump_image_map_t::iterator iter = entries_list.find(source_asset_id);

		{
			LLFastTimer t(FTM_BUMP_SOURCE_ENTRIES_UPDATE);
			if (iter == entries_list.end() ||
				iter->second.isNull() ||
							iter->second->getWidth() != src->getWidth() ||
							iter->second->getHeight() != src->getHeight()) // bump not cached yet or has changed resolution
			{ //make sure an entry exists for this image
				LLPointer<LLImageRaw> raw = new LLImageRaw(1,1,1);
				raw->clear(0x77, 0x77, 0xFF, 0xFF);

				entries_list[src_vi->getID()] = LLViewerTextureManager::getLocalTexture( raw.get(), TRUE);
				iter = entries_list.find(src_vi->getID());
			}
		}

		//if (iter->second->getWidth() != src->getWidth() ||
		//	iter->second->getHeight() != src->getHeight()) // bump not cached yet or has changed resolution
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
				{
					LLFastTimer t(FTM_BUMP_SOURCE_MIN_MAX);
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
				}
				break;
			case 3:
			case 4:
				{
					LLFastTimer t(FTM_BUMP_SOURCE_RGB2LUM);
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
				}
				break;
			default:
				llassert(0);
				dst_image->clear();
				break;
			}

			if( maximum > minimum )
			{
				LLFastTimer t(FTM_BUMP_SOURCE_RESCALE);
				U8 bias_and_scale_lut[256];
				F32 twice_one_over_range = 2.f / (maximum - minimum);
				S32 i;

				const F32 ARTIFICIAL_SCALE = 2.f;  // Advantage: exaggerates the effect in midrange.  Disadvantage: clamps at the extremes.
				if (BE_DARKNESS == bump_code)
				{
					for( i = minimum; i <= maximum; i++ )
					{
						F32 minus_one_to_one = F32(maximum - i) * twice_one_over_range - 1.f;
						bias_and_scale_lut[i] = llclampb(llround(127 * minus_one_to_one * ARTIFICIAL_SCALE + 128));
					}
				}
				else
				{
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
			// immediately assign bump to a global smart pointer in case some local smart pointer
			// accidentally releases it.
			LLPointer<LLViewerTexture> bump = LLViewerTextureManager::getLocalTexture( TRUE );

			if (!LLPipeline::sRenderDeferred)
			{
				LLFastTimer t(FTM_BUMP_SOURCE_CREATE);
				bump->setExplicitFormat(GL_ALPHA8, GL_ALPHA);
				bump->createGLTexture(0, dst_image);
			}
			else 
			{ //convert to normal map
				
				//disable compression on normal maps to prevent errors below
				bump->getGLTexture()->setAllowCompression(false);

				{
					LLFastTimer t(FTM_BUMP_SOURCE_CREATE);
					bump->setExplicitFormat(GL_RGBA8, GL_ALPHA);
					bump->createGLTexture(0, dst_image);
				}

				{
					LLFastTimer t(FTM_BUMP_SOURCE_GEN_NORMAL);
					gPipeline.mScreen.bindTarget();
					
					LLGLDepthTest depth(GL_FALSE);
					LLGLDisable cull(GL_CULL_FACE);
					LLGLDisable blend(GL_BLEND);
					gGL.setColorMask(TRUE, TRUE);
					gNormalMapGenProgram.bind();
					gNormalMapGenProgram.uniform1f("norm_scale", gSavedSettings.getF32("RenderNormalMapScale"));
					gNormalMapGenProgram.uniform1f("stepX", 1.f/bump->getWidth());
					gNormalMapGenProgram.uniform1f("stepY", 1.f/bump->getHeight());

					LLVector2 v((F32) bump->getWidth()/gPipeline.mScreen.getWidth(),
								(F32) bump->getHeight()/gPipeline.mScreen.getHeight());

					gGL.getTexUnit(0)->bind(bump);
					
					S32 width = bump->getWidth();
					S32 height = bump->getHeight();

					S32 screen_width = gPipeline.mScreen.getWidth();
					S32 screen_height = gPipeline.mScreen.getHeight();

					glViewport(0, 0, screen_width, screen_height);

					for (S32 left = 0; left < width; left += screen_width)
					{
						S32 right = left + screen_width;
						right = llmin(right, width);
						
						F32 left_tc = (F32) left/ width;
						F32 right_tc = (F32) right/width;

						for (S32 bottom = 0; bottom < height; bottom += screen_height)
						{
							S32 top = bottom+screen_height;
							top = llmin(top, height);

							F32 bottom_tc = (F32) bottom/height;
							F32 top_tc = (F32)(bottom+screen_height)/height;
							top_tc = llmin(top_tc, 1.f);

							F32 screen_right = (F32) (right-left)/screen_width;
							F32 screen_top = (F32) (top-bottom)/screen_height;

							gGL.begin(LLRender::TRIANGLE_STRIP);
							gGL.texCoord2f(left_tc, bottom_tc);
							gGL.vertex2f(0, 0);

							gGL.texCoord2f(left_tc, top_tc);
							gGL.vertex2f(0, screen_top);

							gGL.texCoord2f(right_tc, bottom_tc);
							gGL.vertex2f(screen_right, 0);

							gGL.texCoord2f(right_tc, top_tc);
							gGL.vertex2f(screen_right, screen_top);

							gGL.end();

							gGL.flush();

							S32 w = right-left;
							S32 h = top-bottom;

							glCopyTexSubImage2D(GL_TEXTURE_2D, 0, left, bottom, 0, 0, w, h);
						}
					}

					glGenerateMipmap(GL_TEXTURE_2D);

					gPipeline.mScreen.flush();

					gNormalMapGenProgram.unbind();
										
					//generateNormalMapFromAlpha(dst_image, nrm_image);
				}
			}
		
			iter->second = bump; // derefs (and deletes) old image
			//---------------------------------------------------
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

void LLDrawPoolBump::pushBatch(LLDrawInfo& params, U32 mask, BOOL texture, BOOL batch_textures)
{
	applyModelMatrix(params);

	bool tex_setup = false;

	if (batch_textures && params.mTextureList.size() > 1)
	{
		for (U32 i = 0; i < params.mTextureList.size(); ++i)
		{
			if (params.mTextureList[i].notNull())
			{
				gGL.getTexUnit(i)->bind(params.mTextureList[i], TRUE);
			}
		}
	}
	else
	{ //not batching textures or batch has only 1 texture -- might need a texture matrix
		if (params.mTextureMatrix)
		{
			if (mShiny)
			{
				gGL.getTexUnit(0)->activate();
				gGL.matrixMode(LLRender::MM_TEXTURE);
			}
			else
			{
				if (!LLGLSLShader::sNoFixedFunction)
				{
					gGL.getTexUnit(1)->activate();
					gGL.matrixMode(LLRender::MM_TEXTURE);
					gGL.loadMatrix((GLfloat*) params.mTextureMatrix->mMatrix);
				}

				gGL.getTexUnit(0)->activate();
				gGL.matrixMode(LLRender::MM_TEXTURE);
				gGL.loadMatrix((GLfloat*) params.mTextureMatrix->mMatrix);
				gPipeline.mTextureMatrixOps++;
			}

			gGL.loadMatrix((GLfloat*) params.mTextureMatrix->mMatrix);
			gPipeline.mTextureMatrixOps++;

			tex_setup = true;
		}

		if (mShiny && mVertexShaderLevel > 1 && texture)
		{
			if (params.mTexture.notNull())
			{
				gGL.getTexUnit(diffuse_channel)->bind(params.mTexture);
				params.mTexture->addTextureStats(params.mVSize);		
			}
			else
			{
				gGL.getTexUnit(diffuse_channel)->unbind(LLTexUnit::TT_TEXTURE);
			}
		}
	}

	if (params.mGroup)
	{
		params.mGroup->rebuildMesh();
	}
	params.mVertexBuffer->setBuffer(mask);
	params.mVertexBuffer->drawRange(params.mDrawMode, params.mStart, params.mEnd, params.mCount, params.mOffset);
	gPipeline.addTrianglesDrawn(params.mCount, params.mDrawMode);
	if (tex_setup)
	{
		if (mShiny)
		{
			gGL.getTexUnit(0)->activate();
		}
		else
		{
			if (!LLGLSLShader::sNoFixedFunction)
			{
				gGL.getTexUnit(1)->activate();
				gGL.matrixMode(LLRender::MM_TEXTURE);
				gGL.loadIdentity();
			}
			gGL.getTexUnit(0)->activate();
			gGL.matrixMode(LLRender::MM_TEXTURE);
		}
		gGL.loadIdentity();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}
}

void LLDrawPoolInvisible::render(S32 pass)
{ //render invisiprims
	LLFastTimer t(FTM_RENDER_INVISIBLE);
  
	if (gPipeline.canUseVertexShaders())
	{
		gOcclusionProgram.bind();
	}

	U32 invisi_mask = LLVertexBuffer::MAP_VERTEX;
	glStencilMask(0);
	gGL.setColorMask(false, false);
	pushBatches(LLRenderPass::PASS_INVISIBLE, invisi_mask, FALSE);
	gGL.setColorMask(true, false);
	glStencilMask(0xFFFFFFFF);

	if (gPipeline.canUseVertexShaders())
	{
		gOcclusionProgram.unbind();
	}

	if (gPipeline.hasRenderBatches(LLRenderPass::PASS_INVISI_SHINY))
	{
		beginShiny(true);
		renderShiny(true);
		endShiny(true);
	}
}

void LLDrawPoolInvisible::beginDeferredPass(S32 pass)
{
	beginRenderPass(pass);
}

void LLDrawPoolInvisible::endDeferredPass( S32 pass )
{
	endRenderPass(pass);
}

void LLDrawPoolInvisible::renderDeferred( S32 pass )
{ //render invisiprims; this doesn't work becaue it also blocks all the post-deferred stuff
#if 0 
	LLFastTimer t(FTM_RENDER_INVISIBLE);
  
	U32 invisi_mask = LLVertexBuffer::MAP_VERTEX;
	glStencilMask(0);
	glStencilOp(GL_ZERO, GL_KEEP, GL_REPLACE);
	gGL.setColorMask(false, false);
	pushBatches(LLRenderPass::PASS_INVISIBLE, invisi_mask, FALSE);
	gGL.setColorMask(true, true);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(0xFFFFFFFF);
	
	if (gPipeline.hasRenderBatches(LLRenderPass::PASS_INVISI_SHINY))
	{
		beginShiny(true);
		renderShiny(true);
		endShiny(true);
	}
#endif
}
