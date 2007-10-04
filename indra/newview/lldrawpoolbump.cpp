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

#include "llagent.h"
#include "llcubemap.h"
#include "lldrawable.h"
#include "lldrawpoolsimple.h"
#include "llface.h"
#include "llsky.h"
#include "lltextureentry.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "pipeline.h"
#include "llglslshader.h"

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
static LLCubeMap* sCubeMap = NULL;

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
	FILE* file = LLFile::fopen( file_name.c_str(), "rt" );	 /*Flawfinder: ignore*/
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

LLDrawPoolBump::LLDrawPoolBump() 
: LLRenderPass(LLDrawPool::POOL_BUMP)
{
}


void LLDrawPoolBump::prerender()
{
	mVertexShaderLevel = LLShaderMgr::getVertexShaderLevel(LLShaderMgr::SHADER_OBJECT);
}

// static
S32 LLDrawPoolBump::numBumpPasses()
{
	if (gSavedSettings.getBOOL("RenderObjectBump"))
	{
		return 2;
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
	switch( pass )
	{
		case 0:
			beginShiny();
			break;
		case 1:
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
	  {
		  renderShiny();
		  break;
	  }
	  case 1:
	  {
		  renderBump();
		  break;
	  }
	  default:
	  {
		  llassert(0);
		  break;
	  }
	}
}

void LLDrawPoolBump::endRenderPass(S32 pass)
{
	switch( pass )
	{
		case 0:
			endShiny();
			break;
		case 1:
			endBump();
			break;
		default:
			llassert(0);
			break;
	}
}

//static
void LLDrawPoolBump::beginShiny()
{
	sVertexMask = VERTEX_MASK_SHINY;
	// Second pass: environment map
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	LLCubeMap* cube_map = gSky.mVOSkyp->getCubeMap();
	if( cube_map )
	{
		cube_map->enable(0);
		cube_map->setMatrix(0);
		cube_map->bind();

		if (LLShaderMgr::getVertexShaderLevel(LLShaderMgr::SHADER_OBJECT) > 0)
		{
			LLMatrix4 mat;
			mat.initRows(LLVector4(gGLModelView+0),
						 LLVector4(gGLModelView+4),
						 LLVector4(gGLModelView+8),
						 LLVector4(gGLModelView+12));
			gObjectShinyProgram.bind();
			LLVector3 vec = LLVector3(gShinyOrigin) * mat;
			LLVector4 vec4(vec, gShinyOrigin.mV[3]);
			glUniform4fvARB(gObjectShinyProgram.mUniform[LLShaderMgr::SHINY_ORIGIN], 1,
				vec4.mV);
		}
		else
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,	GL_COMBINE_ARB);
			
			//use RGB from texture
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,	GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,	GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,	GL_SRC_COLOR);

			// use alpha from color
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_PRIMARY_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);
		}
	}
}

void LLDrawPoolBump::renderShiny()
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_SHINY);
	
	sCubeMap = NULL;

	if( gSky.mVOSkyp->getCubeMap() )
	{
		LLGLEnable blend_enable(GL_BLEND);
		renderStatic(LLRenderPass::PASS_SHINY, sVertexMask);
		renderActive(LLRenderPass::PASS_SHINY, sVertexMask);
	}
}

void LLDrawPoolBump::renderActive(U32 type, U32 mask, BOOL texture)
{
#if !LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkClientArrays(mask);
#endif

	LLSpatialBridge* last_bridge = NULL;
	glPushMatrix();
	
	for (LLSpatialGroup::sg_vector_t::iterator i = gPipeline.mActiveGroups.begin(); i != gPipeline.mActiveGroups.end(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (!group->isDead() &&
			gPipeline.hasRenderType(group->mSpatialPartition->mDrawableType) &&
			group->mDrawMap.find(type) != group->mDrawMap.end())
		{
			LLSpatialBridge* bridge = (LLSpatialBridge*) group->mSpatialPartition;
			if (bridge != last_bridge)
			{
				glPopMatrix();
				glPushMatrix();
				glMultMatrixf((F32*) bridge->mDrawable->getRenderMatrix().mMatrix);
				last_bridge = bridge;

				if (LLPipeline::sDynamicReflections)
				{
					LLSpatialPartition* part = gPipeline.getSpatialPartition(LLPipeline::PARTITION_VOLUME);
					LLSpatialGroup::OctreeNode* node = part->mOctree->getNodeAt(LLVector3d(bridge->mDrawable->getPositionAgent()), 32.0);
					if (node)
					{
						sCubeMap = ((LLSpatialGroup*) node->getListener(0))->mReflectionMap;
					}
				}
			}

			renderGroup(group,type,mask,texture);
		}
	}
	
	glPopMatrix();
}



void LLDrawPoolBump::renderGroup(LLSpatialGroup* group, U32 type, U32 mask, BOOL texture = TRUE)
{					
	LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[type];	
	
	for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k) 
	{
		LLDrawInfo& params = **k;
		if (LLPipeline::sDynamicReflections)
		{
			if (params.mReflectionMap.notNull())
			{
				params.mReflectionMap->bind();
			}
			else
			{
				if (sCubeMap)
				{
					sCubeMap->bind();
				}
				else
				{
					gSky.mVOSkyp->getCubeMap()->bind();
				}
			}
		}
		
		params.mVertexBuffer->setBuffer(mask);
		U32* indices_pointer = (U32*) params.mVertexBuffer->getIndicesPointer();
		glDrawRangeElements(GL_TRIANGLES, params.mStart, params.mEnd, params.mCount,
							GL_UNSIGNED_INT, indices_pointer+params.mOffset);
		gPipeline.mTrianglesDrawn += params.mCount/3;
	}
}

void LLDrawPoolBump::endShiny()
{
	LLCubeMap* cube_map = gSky.mVOSkyp->getCubeMap();
	if( cube_map )
	{
		cube_map->disable();
		cube_map->restoreMatrix();

		if (LLShaderMgr::getVertexShaderLevel(LLShaderMgr::SHADER_OBJECT) > 0)
		{
			gObjectShinyProgram.unbind();
		}

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_MODULATE);
	}
	
	LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
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
	sVertexMask = VERTEX_MASK_BUMP;
	LLFastTimer t(LLFastTimer::FTM_RENDER_BUMP);
	// Optional second pass: emboss bump map
	stop_glerror();

	// TEXTURE UNIT 0
	// Output.rgb = texture at texture coord 0
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

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
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

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
	glActiveTextureARB(GL_TEXTURE0_ARB);
	stop_glerror();
}

//static
void LLDrawPoolBump::renderBump()
{
	LLFastTimer ftm(LLFastTimer::FTM_RENDER_BUMP);
	LLGLDisable fog(GL_FOG);
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_EQUAL);
	LLGLEnable tex2d(GL_TEXTURE_2D);
	LLGLEnable blend(GL_BLEND);
	glColor4f(1,1,1,1);
	renderBump(LLRenderPass::PASS_BUMP, sVertexMask);
	renderBumpActive(LLRenderPass::PASS_BUMP, sVertexMask);
}

//static
void LLDrawPoolBump::endBump()
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

void LLDrawPoolBump::renderBumpActive(U32 type, U32 mask)
{
#if !LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkClientArrays(mask);
#endif

	LLSpatialBridge* last_bridge = NULL;
	glPushMatrix();
	
	for (LLSpatialGroup::sg_vector_t::iterator i = gPipeline.mActiveGroups.begin(); i != gPipeline.mActiveGroups.end(); ++i)
	{
		LLSpatialGroup* group = *i;
		if (!group->isDead() && 
			group->mSpatialPartition->mRenderByGroup &&
			group->mDrawMap.find(type) != group->mDrawMap.end())
		{
			LLSpatialBridge* bridge = (LLSpatialBridge*) group->mSpatialPartition;
			if (bridge != last_bridge)
			{
				glPopMatrix();
				glPushMatrix();
				glMultMatrixf((F32*) bridge->mDrawable->getRenderMatrix().mMatrix);
				last_bridge = bridge;
			}

			renderGroupBump(group,type,mask);
		}
	}
	
	glPopMatrix();
}

void LLDrawPoolBump::renderBump(U32 type, U32 mask)
{	
#if !LL_RELEASE_FOR_DOWNLOAD
	LLGLState::checkClientArrays(mask);
#endif

	LLSpatialGroup::drawmap_elem_t& draw_info = gPipeline.mRenderMap[type];	

	for (LLSpatialGroup::drawmap_elem_t::iterator i = draw_info.begin(); i != draw_info.end(); ++i)	
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
	if (params.mTextureMatrix)
	{
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glMatrixMode(GL_TEXTURE);
		glLoadMatrixf((GLfloat*) params.mTextureMatrix->mMatrix);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glLoadMatrixf((GLfloat*) params.mTextureMatrix->mMatrix);
	}
	params.mVertexBuffer->setBuffer(mask);
	U32* indices_pointer = (U32*) params.mVertexBuffer->getIndicesPointer();
	glDrawRangeElements(GL_TRIANGLES, params.mStart, params.mEnd, params.mCount,
						GL_UNSIGNED_INT, indices_pointer+params.mOffset);
	gPipeline.mTrianglesDrawn += params.mCount/3;
	if (params.mTextureMatrix)
	{
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glLoadIdentity();
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
}
