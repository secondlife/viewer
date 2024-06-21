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
#include "llstartup.h"
#include "lltextureentry.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llviewershadermgr.h"
#include "llmodel.h"

//#include "llimagebmp.h"
//#include "../tools/imdebug/imdebug.h"

// static
LLStandardBumpmap gStandardBumpmapList[TEM_BUMPMAP_COUNT];
LL::WorkQueue::weak_t LLBumpImageList::sMainQueue;
LL::WorkQueue::weak_t LLBumpImageList::sTexUpdateQueue;
LLRenderTarget LLBumpImageList::sRenderTarget;

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
static bool shiny = false;

// Enabled after changing LLViewerTexture::mNeedsCreateTexture to an
// LLAtomicBool; this should work just fine, now. HB
#define LL_BUMPLIST_MULTITHREADED 1


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

    if (LLStartUp::getStartupState() < STATE_SEED_CAP_GRANTED)
    {
        // Not ready, need caps for images
        return;
    }

    // can't assert; we destroyGL and restoreGL a lot during *first* startup, which populates this list already, THEN we explicitly init the list as part of *normal* startup.  Sigh.  So clear the list every time before we (re-)add the standard bumpmaps.
    //llassert( LLStandardBumpmap::sStandardBumpmapCount == 0 );
    clear();
    LL_INFOS() << "Adding standard bumpmaps." << LL_ENDL;
    gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount++] = LLStandardBumpmap("None");       // BE_NO_BUMP
    gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount++] = LLStandardBumpmap("Brightness"); // BE_BRIGHTNESS
    gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount++] = LLStandardBumpmap("Darkness");   // BE_DARKNESS

    std::string file_name = gDirUtilp->getExpandedFilename( LL_PATH_APP_SETTINGS, "std_bump.ini" );
    LLFILE* file = LLFile::fopen( file_name, "rt" );     /*Flawfinder: ignore*/
    if( !file )
    {
        LL_WARNS() << "Could not open std_bump <" << file_name << ">" << LL_ENDL;
        return;
    }

    S32 file_version = 0;

    S32 fields_read = fscanf( file, "LLStandardBumpmap version %d", &file_version );
    if( fields_read != 1 )
    {
        LL_WARNS() << "Bad LLStandardBumpmap header" << LL_ENDL;
        return;
    }

    if( file_version > STD_BUMP_LATEST_FILE_VERSION )
    {
        LL_WARNS() << "LLStandardBumpmap has newer version (" << file_version << ") than viewer (" << STD_BUMP_LATEST_FILE_VERSION << ")" << LL_ENDL;
        return;
    }

    while( !feof(file) && (LLStandardBumpmap::sStandardBumpmapCount < (U32)TEM_BUMPMAP_COUNT) )
    {
        // *NOTE: This buffer size is hard coded into scanf() below.
        char label[2048] = "";  /* Flawfinder: ignore */
        char bump_image_id[2048] = "";  /* Flawfinder: ignore */
        fields_read = fscanf(   /* Flawfinder: ignore */
            file, "\n%2047s %2047s", label, bump_image_id);
        if( EOF == fields_read )
        {
            break;
        }
        if( fields_read != 2 )
        {
            LL_WARNS() << "Bad LLStandardBumpmap entry" << LL_ENDL;
            return;
        }

//      LL_INFOS() << "Loading bumpmap: " << bump_image_id << " from viewerart" << LL_ENDL;
        gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mLabel = label;
        gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mImage =
            LLViewerTextureManager::getFetchedTexture(LLUUID(bump_image_id));
        gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mImage->setBoostLevel(LLGLTexture::LOCAL) ;
        gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mImage->setLoadedCallback(LLBumpImageList::onSourceStandardLoaded, 0, true, false, NULL, NULL );
        gStandardBumpmapList[LLStandardBumpmap::sStandardBumpmapCount].mImage->forceToSaveRawImage(0, 30.f) ;
        LLStandardBumpmap::sStandardBumpmapCount++;
    }

    fclose( file );
}

// static
void LLStandardBumpmap::clear()
{
    LL_INFOS() << "Clearing standard bumpmaps." << LL_ENDL;
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
    shiny = false;
}


void LLDrawPoolBump::prerender()
{
    mShaderLevel = LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT);
}

// static
S32 LLDrawPoolBump::numBumpPasses()
{
    return 1;
}


//static
void LLDrawPoolBump::bindCubeMap(LLGLSLShader* shader, S32 shader_level, S32& diffuse_channel, S32& cube_channel)
{
    LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
    if( cube_map && !LLPipeline::sReflectionProbesEnabled )
    {
        if (shader )
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
                diffuse_channel = shader->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
            }
            else
            {
                cube_map->setMatrix(0);
                cube_channel = shader->enableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
                diffuse_channel = -1;
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
        }
    }
}

//static
void LLDrawPoolBump::unbindCubeMap(LLGLSLShader* shader, S32 shader_level, S32& diffuse_channel, S32& cube_channel)
{
    LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
    if( cube_map && !LLPipeline::sReflectionProbesEnabled)
    {
        if (shader_level > 1)
        {
            shader->disableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);

            if (LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_OBJECT) > 0)
            {
                if (diffuse_channel != 0)
                {
                    shader->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
                }
            }
        }
        // Moved below shader->disableTexture call to avoid false alarms from auto-re-enable of textures on stage 0
        // MAINT-755
        cube_map->disable();
        cube_map->restoreMatrix();
    }
}

void LLDrawPoolBump::beginFullbrightShiny()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_SHINY);

    sVertexMask = VERTEX_MASK_SHINY | LLVertexBuffer::MAP_TEXCOORD0;

    // Second pass: environment map
    shader = &gDeferredFullbrightShinyProgram;
    if (LLPipeline::sRenderingHUDs)
    {
        shader = &gHUDFullbrightShinyProgram;
    }

    if (mRigged)
    {
        llassert(shader->mRiggedVariant);
        shader = shader->mRiggedVariant;
    }

    // bind exposure map so fullbright shader can cancel out exposure
    S32 channel = shader->enableTexture(LLShaderMgr::EXPOSURE_MAP);
    if (channel > -1)
    {
        gGL.getTexUnit(channel)->bind(&gPipeline.mExposureMap);
    }

    LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;

    if (cube_map && !LLPipeline::sReflectionProbesEnabled)
    {
        // Make sure that texture coord generation happens for tex unit 1, as that's the one we use for
        // the cube map in the one pass shiny shaders
        gGL.getTexUnit(1)->disable();
        cube_channel = shader->enableTexture(LLViewerShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
        cube_map->enableTexture(cube_channel);
        diffuse_channel = shader->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);

        gGL.getTexUnit(cube_channel)->bind(cube_map);
        gGL.getTexUnit(0)->activate();
    }

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

        if (LLPipeline::sReflectionProbesEnabled)
        {
            gPipeline.bindReflectionProbes(*shader);
        }
        else
        {
            gPipeline.setEnvMat(*shader);
        }
    }

    if (mShaderLevel > 1)
    { //indexed texture rendering, channel 0 is always diffuse
        diffuse_channel = 0;
    }

    shiny = true;
}

void LLDrawPoolBump::renderFullbrightShiny()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_SHINY);

    {
        LLGLEnable blend_enable(GL_BLEND);

        if (mShaderLevel > 1)
        {
            if (mRigged)
            {
                LLRenderPass::pushRiggedBatches(LLRenderPass::PASS_FULLBRIGHT_SHINY_RIGGED, true, true);
            }
            else
            {
                LLRenderPass::pushBatches(LLRenderPass::PASS_FULLBRIGHT_SHINY, true, true);
            }
        }
        else
        {
            if (mRigged)
            {
                LLRenderPass::pushRiggedBatches(LLRenderPass::PASS_FULLBRIGHT_SHINY_RIGGED);
            }
            else
            {
                LLRenderPass::pushBatches(LLRenderPass::PASS_FULLBRIGHT_SHINY);
            }
        }
    }
}

void LLDrawPoolBump::endFullbrightShiny()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_SHINY);

    LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
    if( cube_map && !LLPipeline::sReflectionProbesEnabled )
    {
        cube_map->disable();
        if (shader->mFeatures.hasReflectionProbes)
        {
            gPipeline.unbindReflectionProbes(*shader);
        }
        shader->unbind();
    }

    diffuse_channel = -1;
    cube_channel = 0;
    shiny = false;
}

void LLDrawPoolBump::renderGroup(LLSpatialGroup* group, U32 type, bool texture = true)
{
    LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[type];

    for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
    {
        LLDrawInfo& params = **k;

        applyModelMatrix(params);

        params.mVertexBuffer->setBuffer();
        params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
    }
}


// static
bool LLDrawPoolBump::bindBumpMap(LLDrawInfo& params, S32 channel)
{
    U8 bump_code = params.mBump;

    return bindBumpMap(bump_code, params.mTexture, channel);
}

//static
bool LLDrawPoolBump::bindBumpMap(LLFace* face, S32 channel)
{
    const LLTextureEntry* te = face->getTextureEntry();
    if (te)
    {
        U8 bump_code = te->getBumpmap();
        return bindBumpMap(bump_code, face->getTexture(), channel);
    }

    return false;
}

//static
bool LLDrawPoolBump::bindBumpMap(U8 bump_code, LLViewerTexture* texture, S32 channel)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    //Note: texture atlas does not support bump texture now.
    LLViewerFetchedTexture* tex = LLViewerTextureManager::staticCastToFetchedTexture(texture) ;
    if(!tex)
    {
        //if the texture is not a fetched texture
        return false;
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
            gBumpImageList.addTextureStats(bump_code, tex->getID(), tex->getMaxVirtualSize());
        }
        break;
    }

    if (bump)
    {
        if (channel == -2)
        {
            gGL.getTexUnit(1)->bindFast(bump);
            gGL.getTexUnit(0)->bindFast(bump);
        }
        else
        {
            // NOTE: do not use bindFast here (see SL-16222)
            gGL.getTexUnit(channel)->bind(bump);
        }

        return true;
    }

    return false;
}

//static
void LLDrawPoolBump::beginBump()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_BUMP);
    sVertexMask = VERTEX_MASK_BUMP;
    // Optional second pass: emboss bump map
    stop_glerror();

    shader = &gObjectBumpProgram;

    if (mRigged)
    {
        llassert(shader->mRiggedVariant);
        shader = shader->mRiggedVariant;
    }

    shader->bind();

    gGL.setSceneBlendType(LLRender::BT_MULT_X2);
    stop_glerror();
}

//static
void LLDrawPoolBump::renderBump(U32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_BUMP);
    LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_LEQUAL);
    LLGLEnable blend(GL_BLEND);
    gGL.diffuseColor4f(1,1,1,1);
    /// Get rid of z-fighting with non-bump pass.
    LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    pushBumpBatches(pass);
}

//static
void LLDrawPoolBump::endBump(U32 pass)
{
    LLGLSLShader::unbind();

    gGL.setSceneBlendType(LLRender::BT_ALPHA);
}

S32 LLDrawPoolBump::getNumDeferredPasses()
{
    return 1;
}

void LLDrawPoolBump::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_BUMP);

    shiny = true;
    for (int i = 0; i < 2; ++i)
    {
        bool rigged = i == 1;
        gDeferredBumpProgram.bind(rigged);
        diffuse_channel = LLGLSLShader::sCurBoundShaderPtr->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
        bump_channel = LLGLSLShader::sCurBoundShaderPtr->enableTexture(LLViewerShaderMgr::BUMP_MAP);
        gGL.getTexUnit(diffuse_channel)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(bump_channel)->unbind(LLTexUnit::TT_TEXTURE);

        U32 type = rigged ? LLRenderPass::PASS_BUMP_RIGGED : LLRenderPass::PASS_BUMP;
        LLCullResult::drawinfo_iterator begin = gPipeline.beginRenderMap(type);
        LLCullResult::drawinfo_iterator end = gPipeline.endRenderMap(type);

        LLVOAvatar* avatar = nullptr;
        U64 skin = 0;

        for (LLCullResult::drawinfo_iterator i = begin; i != end; )
        {
            LLDrawInfo& params = **i;

            LLCullResult::increment_iterator(i, end);

            LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(params.mAlphaMaskCutoff);
            LLDrawPoolBump::bindBumpMap(params, bump_channel);

            if (rigged)
            {
                if (avatar != params.mAvatar || skin != params.mSkinInfo->mHash)
                {
                    uploadMatrixPalette(params);
                    avatar = params.mAvatar;
                    skin = params.mSkinInfo->mHash;
                }
                pushBumpBatch(params, true, false);
            }
            else
            {
                pushBumpBatch(params, true, false);
            }
        }

        LLGLSLShader::sCurBoundShaderPtr->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
        LLGLSLShader::sCurBoundShaderPtr->disableTexture(LLViewerShaderMgr::BUMP_MAP);
        LLGLSLShader::sCurBoundShaderPtr->unbind();
        gGL.getTexUnit(0)->activate();
    }

    shiny = false;
}


void LLDrawPoolBump::renderPostDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    S32 num_passes = LLPipeline::sRenderingHUDs ? 1 : 2; // skip rigged pass when rendering HUDs

    for (int i = 0; i < num_passes; ++i)
    { // two passes -- static and rigged
        mRigged = (i == 1);

        // render shiny
        beginFullbrightShiny();
        renderFullbrightShiny();
        endFullbrightShiny();

        //render bump
        beginBump();
        renderBump(LLRenderPass::PASS_POST_BUMP);
        endBump();
    }
}


////////////////////////////////////////////////////////////////
// List of bump-maps created from other textures.


//const LLUUID TEST_BUMP_ID("3d33eaf2-459c-6f97-fd76-5fce3fc29447");

void LLBumpImageList::init()
{
    llassert( mBrightnessEntries.size() == 0 );
    llassert( mDarknessEntries.size() == 0 );

    LLStandardBumpmap::restoreGL();
    sMainQueue = LL::WorkQueue::getInstance("mainloop");
    sTexUpdateQueue = LL::WorkQueue::getInstance("LLImageGL"); // Share work queue with tex loader.
}

void LLBumpImageList::clear()
{
    LL_INFOS() << "Clearing dynamic bumpmaps." << LL_ENDL;
    // these will be re-populated on-demand
    mBrightnessEntries.clear();
    mDarknessEntries.clear();

    sRenderTarget.release();

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
    llassert(LLCoros::on_main_thread_main_coro()); // This code is not thread safe

    for (bump_image_map_t::iterator iter = mBrightnessEntries.begin(); iter != mBrightnessEntries.end(); )
    {
        LLViewerTexture* image = iter->second;
        if( image )
        {
            bool destroy = true;
            if( image->hasGLTexture())
            {
                if( image->getBoundRecently() )
                {
                    destroy = false;
                }
                else
                {
                    image->destroyGLTexture();
                }
            }

            if( destroy )
            {
                //LL_INFOS() << "*** Destroying bright " << (void*)image << LL_ENDL;
                iter = mBrightnessEntries.erase(iter);   // deletes the image thanks to reference counting
                continue;
            }
        }
        ++iter;
    }

    for (bump_image_map_t::iterator iter = mDarknessEntries.begin(); iter != mDarknessEntries.end(); )
    {
        bump_image_map_t::iterator curiter = iter++;
        LLViewerTexture* image = curiter->second;
        if( image )
        {
            bool destroy = true;
            if( image->hasGLTexture())
            {
                if( image->getBoundRecently() )
                {
                    destroy = false;
                }
                else
                {
                    image->destroyGLTexture();
                }
            }

            if( destroy )
            {
                //LL_INFOS() << "*** Destroying dark " << (void*)image << LL_ENDL;;
                mDarknessEntries.erase(curiter);  // deletes the image thanks to reference counting
            }
        }
    }
}


// Note: the caller SHOULD NOT keep the pointer that this function returns.  It may be updated as more data arrives.
LLViewerTexture* LLBumpImageList::getBrightnessDarknessImage(LLViewerFetchedTexture* src_image, U8 bump_code )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    llassert( (bump_code == BE_BRIGHTNESS) || (bump_code == BE_DARKNESS) );

    LLViewerTexture* bump = NULL;

    bump_image_map_t* entries_list = NULL;
    void (*callback_func)( bool success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, bool final, void* userdata ) = NULL;

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
        (*entries_list)[src_image->getID()] = LLViewerTextureManager::getLocalTexture( true );
        bump = (*entries_list)[src_image->getID()]; // In case callback was called immediately and replaced the image
    }

    if (!src_image->hasCallbacks())
    { //if image has no callbacks but resolutions don't match, trigger raw image loaded callback again
        if (src_image->getWidth() != bump->getWidth() ||
            src_image->getHeight() != bump->getHeight())// ||
            //(LLPipeline::sRenderDeferred && bump->getComponents() != 4))
        {
            src_image->setBoostLevel(LLGLTexture::BOOST_BUMP) ;
            src_image->setLoadedCallback( callback_func, 0, true, false, new LLUUID(src_image->getID()), NULL );
            src_image->forceToSaveRawImage(0) ;
        }
    }

    return bump;
}


// static
void LLBumpImageList::onSourceBrightnessLoaded( bool success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, bool final, void* userdata )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLUUID* source_asset_id = (LLUUID*)userdata;
    LLBumpImageList::onSourceLoaded( success, src_vi, src, *source_asset_id, BE_BRIGHTNESS );
    if( final )
    {
        delete source_asset_id;
    }
}

// static
void LLBumpImageList::onSourceDarknessLoaded( bool success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, bool final, void* userdata )
{
    LLUUID* source_asset_id = (LLUUID*)userdata;
    LLBumpImageList::onSourceLoaded( success, src_vi, src, *source_asset_id, BE_DARKNESS );
    if( final )
    {
        delete source_asset_id;
    }
}

void LLBumpImageList::onSourceStandardLoaded( bool success, LLViewerFetchedTexture* src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, bool final, void* userdata)
{
    if (success && LLPipeline::sRenderDeferred)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
        LLPointer<LLImageRaw> nrm_image = new LLImageRaw(src->getWidth(), src->getHeight(), 4);
        {
            generateNormalMapFromAlpha(src, nrm_image);
        }
        src_vi->setExplicitFormat(GL_RGBA, GL_RGBA);
        {
            src_vi->createGLTexture(src_vi->getDiscardLevel(), nrm_image);
        }
    }
}

void LLBumpImageList::generateNormalMapFromAlpha(LLImageRaw* src, LLImageRaw* nrm_image)
{
    LLImageDataSharedLock lockIn(src);
    LLImageDataLock lockOut(nrm_image);

    U8* nrm_data = nrm_image->getData();
    S32 resX = src->getWidth();
    S32 resY = src->getHeight();

    const U8* src_data = src->getData();

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

// static
void LLBumpImageList::onSourceLoaded( bool success, LLViewerTexture *src_vi, LLImageRaw* src, LLUUID& source_asset_id, EBumpEffect bump_code )
{
    LL_PROFILE_ZONE_SCOPED;

    if( success )
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

        LLImageDataSharedLock lock(src);

        bump_image_map_t& entries_list(bump_code == BE_BRIGHTNESS ? gBumpImageList.mBrightnessEntries : gBumpImageList.mDarknessEntries );
        bump_image_map_t::iterator iter = entries_list.find(source_asset_id);

        {
            if (iter == entries_list.end() ||
                iter->second.isNull() ||
                            iter->second->getWidth() != src->getWidth() ||
                            iter->second->getHeight() != src->getHeight()) // bump not cached yet or has changed resolution
            { //make sure an entry exists for this image
                entries_list[src_vi->getID()] = LLViewerTextureManager::getLocalTexture(true);
                iter = entries_list.find(src_vi->getID());
            }
        }

        if (iter->second->getWidth() != src->getWidth() ||
            iter->second->getHeight() != src->getHeight()) // bump not cached yet or has changed resolution
        {
            LLPointer<LLImageRaw> dst_image = new LLImageRaw(src->getWidth(), src->getHeight(), 1);
            U8* dst_data = dst_image->getData();
            S32 dst_data_size = dst_image->getDataSize();

            const U8* src_data = src->getData();
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
                U8 bias_and_scale_lut[256];
                F32 twice_one_over_range = 2.f / (maximum - minimum);
                S32 i;

                const F32 ARTIFICIAL_SCALE = 2.f;  // Advantage: exaggerates the effect in midrange.  Disadvantage: clamps at the extremes.
                if (BE_DARKNESS == bump_code)
                {
                    for( i = minimum; i <= maximum; i++ )
                    {
                        F32 minus_one_to_one = F32(maximum - i) * twice_one_over_range - 1.f;
                        bias_and_scale_lut[i] = llclampb(ll_round(127 * minus_one_to_one * ARTIFICIAL_SCALE + 128));
                    }
                }
                else
                {
                    for( i = minimum; i <= maximum; i++ )
                    {
                        F32 minus_one_to_one = F32(i - minimum) * twice_one_over_range - 1.f;
                        bias_and_scale_lut[i] = llclampb(ll_round(127 * minus_one_to_one * ARTIFICIAL_SCALE + 128));
                    }
                }

                for( i = 0; i < dst_data_size; i++ )
                {
                    dst_data[i] = bias_and_scale_lut[dst_data[i]];
                }
            }

            //---------------------------------------------------
            // immediately assign bump to a smart pointer in case some local smart pointer
            // accidentally releases it.
            LLPointer<LLViewerTexture> bump = iter->second;

            if (!LLPipeline::sRenderDeferred)
            {
                bump->setExplicitFormat(GL_ALPHA8, GL_ALPHA);

#if LL_BUMPLIST_MULTITHREADED
                auto tex_queue = LLImageGLThread::sEnabledTextures ? sTexUpdateQueue.lock() : nullptr;

                if (tex_queue)
                { //dispatch creation to background thread
                    LLImageRaw* dst_ptr = dst_image;
                    LLViewerTexture* bump_ptr = bump;
                    dst_ptr->ref();
                    bump_ptr->ref();
                    tex_queue->post(
                        [=]()
                        {
                            LL_PROFILE_ZONE_NAMED("bil - create texture");
                            bump_ptr->createGLTexture(0, dst_ptr);
                            bump_ptr->unref();
                            dst_ptr->unref();
                        });

                }
                else
#endif
                {
                    bump->createGLTexture(0, dst_image);
                }
            }
            else
            { //convert to normal map
                LL_PROFILE_ZONE_NAMED("bil - create normal map");
                LLImageGL* img = bump->getGLTexture();
                LLImageRaw* dst_ptr = dst_image.get();
                LLGLTexture* bump_ptr = bump.get();

                dst_ptr->ref();
                img->ref();
                bump_ptr->ref();
                auto create_func = [=]()
                {
                    img->setUseMipMaps(true);
                    // upload dst_image to GPU (greyscale in red channel)
                    img->setExplicitFormat(GL_RED, GL_RED);

                    bump_ptr->createGLTexture(0, dst_ptr);
                    dst_ptr->unref();
                };

                auto generate_func = [=]()
                {
                    // Allocate an empty RGBA texture at "tex_name" the same size as bump
                    //  Note: bump will still point at GPU copy of dst_image
                    bump_ptr->setExplicitFormat(GL_RGBA, GL_RGBA);
                    LLGLuint tex_name;
                    img->createGLTexture(0, nullptr, false, 0, true, &tex_name);

                    // point render target at empty buffer
                    sRenderTarget.setColorAttachment(img, tex_name);

                    // generate normal map in empty texture
                    {
                        sRenderTarget.bindTarget();

                        LLGLDepthTest depth(GL_FALSE);
                        LLGLDisable cull(GL_CULL_FACE);
                        LLGLDisable blend(GL_BLEND);
                        gGL.setColorMask(true, true);

                        gNormalMapGenProgram.bind();

                        static LLStaticHashedString sNormScale("norm_scale");
                        static LLStaticHashedString sStepX("stepX");
                        static LLStaticHashedString sStepY("stepY");

                        gNormalMapGenProgram.uniform1f(sNormScale, gSavedSettings.getF32("RenderNormalMapScale"));
                        gNormalMapGenProgram.uniform1f(sStepX, 1.f / bump_ptr->getWidth());
                        gNormalMapGenProgram.uniform1f(sStepY, 1.f / bump_ptr->getHeight());

                        gGL.getTexUnit(0)->bind(bump_ptr);

                        gGL.begin(LLRender::TRIANGLE_STRIP);
                        gGL.texCoord2f(0, 0);
                        gGL.vertex2f(0, 0);

                        gGL.texCoord2f(0, 1);
                        gGL.vertex2f(0, 1);

                        gGL.texCoord2f(1, 0);
                        gGL.vertex2f(1, 0);

                        gGL.texCoord2f(1, 1);
                        gGL.vertex2f(1, 1);

                        gGL.end();

                        gGL.flush();

                        gNormalMapGenProgram.unbind();

                        sRenderTarget.flush();
                        sRenderTarget.releaseColorAttachment();
                    }

                    // point bump at normal map and free gpu copy of dst_image
                    img->syncTexName(tex_name);

                    // generate mipmap
                    gGL.getTexUnit(0)->bind(img);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    gGL.getTexUnit(0)->disable();

                    bump_ptr->unref();
                    img->unref();
                };

#if LL_BUMPLIST_MULTITHREADED
                auto main_queue = LLImageGLThread::sEnabledTextures ? sMainQueue.lock() : nullptr;

                if (main_queue)
                { //dispatch texture upload to background thread, issue GPU commands to generate normal map on main thread
                    main_queue->postTo(
                        sTexUpdateQueue,
                        create_func,
                        generate_func);
                }
                else
#endif
                { // immediate upload texture and generate normal map
                    create_func();
                    generate_func();
                }


            }

            iter->second = bump; // derefs (and deletes) old image
            //---------------------------------------------------
        }
    }
}

void LLDrawPoolBump::pushBumpBatches(U32 type)
{
    LLVOAvatar* avatar = nullptr;
    U64 skin = 0;

    if (mRigged)
    { // nudge type enum and include skinweights for rigged pass
        type += 1;
    }

    LLCullResult::drawinfo_iterator begin = gPipeline.beginRenderMap(type);
    LLCullResult::drawinfo_iterator end = gPipeline.endRenderMap(type);

    for (LLCullResult::drawinfo_iterator i = begin; i != end; ++i)
    {
        LLDrawInfo& params = **i;

        if (LLDrawPoolBump::bindBumpMap(params))
        {
            if (mRigged)
            {
                if (avatar != params.mAvatar || skin != params.mSkinInfo->mHash)
                {
                    if (uploadMatrixPalette(params))
                    {
                        avatar = params.mAvatar;
                        skin = params.mSkinInfo->mHash;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
            pushBumpBatch(params, false);
        }
    }
}

void LLRenderPass::pushBumpBatch(LLDrawInfo& params, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    applyModelMatrix(params);

    bool tex_setup = false;

    if (batch_textures && params.mTextureList.size() > 1)
    {
        for (U32 i = 0; i < params.mTextureList.size(); ++i)
        {
            if (params.mTextureList[i].notNull())
            {
                gGL.getTexUnit(i)->bindFast(params.mTextureList[i]);
            }
        }
    }
    else
    { //not batching textures or batch has only 1 texture -- might need a texture matrix
        if (params.mTextureMatrix)
        {
            if (shiny)
            {
                gGL.getTexUnit(0)->activate();
                gGL.matrixMode(LLRender::MM_TEXTURE);
            }
            else
            {
                gGL.getTexUnit(0)->activate();
                gGL.matrixMode(LLRender::MM_TEXTURE);
                gGL.loadMatrix((GLfloat*) params.mTextureMatrix->mMatrix);
                gPipeline.mTextureMatrixOps++;
            }

            gGL.loadMatrix((GLfloat*) params.mTextureMatrix->mMatrix);
            gPipeline.mTextureMatrixOps++;

            tex_setup = true;
        }

        if (shiny && mShaderLevel > 1 && texture)
        {
            if (params.mTexture.notNull())
            {
                gGL.getTexUnit(diffuse_channel)->bindFast(params.mTexture);
            }
            else
            {
                gGL.getTexUnit(diffuse_channel)->unbind(LLTexUnit::TT_TEXTURE);
            }
        }
    }

    params.mVertexBuffer->setBuffer();
    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);

    if (tex_setup)
    {
        if (shiny)
        {
            gGL.getTexUnit(0)->activate();
        }
        else
        {
            gGL.getTexUnit(0)->activate();
            gGL.matrixMode(LLRender::MM_TEXTURE);
        }
        gGL.loadIdentity();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
    }
}

