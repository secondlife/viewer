/**
 * @file llprocesstexture.cpp
 * @brief Implementation of LLProcessTexture class.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

// linden includes
#include "linden_common.h"

#include "llfasttimer.h"
#include "llgl.h"
#include "llimagej2c.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llsdutil.h"

// appearance includes
#include "llwearabletype.h"
#include "llavatarappearancedefines.h"

// project includes
#include "llappappearanceutility.h"
#include "llbakingavatar.h"
#include "llbakingtexlayer.h"
#include "llbakingtexture.h"
#include "llbakingwearable.h"
#include "llbakingwearablesdata.h"
#include "llbakingwindow.h"
#include "llprocesstexture.h"

static const S32 MAX_SIZE_LLSD_HEADER = 1024 * 1024;
static const BOOL USE_MIP_MAPS = TRUE;

static const S32 EYES_SLOT_DIMENSIONS=512;

using namespace LLAvatarAppearanceDefines;

LLProcessTexture::LLProcessTexture(LLAppAppearanceUtility* app)
    : LLBakingProcess(app),
      mWindow(NULL),
      mInputRaw(NULL),
      mBakeSize(512)
{
}

void LLProcessTexture::cleanup()
{
    delete mWindow;
}

static LLFastTimer::DeclareTimer FTM_CREATE_TEXTURE_FROM_STREAM("Create texture from stream.");
static LLPointer<LLImageRaw> create_texture_from_stream(std::istream& input,
                                                             S32 texture_size,
                                                             const LLUUID& id)
{
    LL_RECORD_BLOCK_TIME(FTM_CREATE_TEXTURE_FROM_STREAM);
    // Read compressed j2c texture data from the input stream.
    U8* buffer = (U8*) ll_aligned_malloc_16(texture_size);
    input.read((char*) buffer, texture_size);

    if (input.gcount() < texture_size)
    {
        ll_aligned_free_16(buffer);
        throw LLAppException(RV_UNABLE_TO_DECODE, " Early EOF in input stream.");
    }
    if (input.gcount() > texture_size)
    {
        ll_aligned_free_16(buffer);
        throw LLAppException(RV_UNABLE_TO_DECODE, " Read too much data from input stream: Programming Error.");
    }

    const S32 DISCARD_FULL_TEXTURE_RESOLUTION = 0;
    LLPointer<LLImageJ2C> j2c = new LLImageJ2C();
    j2c->setDiscardLevel(DISCARD_FULL_TEXTURE_RESOLUTION);
    // Transfer the j2c buffer to an LLImageJ2C object.
    // This gives memory ownership of buffer to LLImageJ2C
    if (!j2c->validate(buffer, texture_size))
    {
        throw LLAppException(RV_UNABLE_TO_DECODE, " Unable to validate J2C: " + LLImage::getLastThreadError());
    }
    if (!(j2c->getWidth() * j2c->getHeight() * j2c->getComponents()))
    {
        throw LLAppException(RV_UNABLE_TO_DECODE, " Invalid dimensions.");
    }

    // Decompress the J2C image into a raw image.
    LLPointer<LLImageRaw> image_raw = new LLImageRaw(j2c->getWidth(),
                                                     j2c->getHeight(),
                                                     j2c->getComponents());
    const F32 MAX_DECODE_TIME = 60.f;
    j2c->setDiscardLevel(0);
    if (!j2c->decode(image_raw, MAX_DECODE_TIME))
    {
        throw LLAppException(RV_UNABLE_TO_DECODE, " Decoding timeout.");
    }
    // *TODO: Check for decode failure?
    LL_DEBUGS() << "ID: " << id << ", Raw Width: " << image_raw->getWidth() << ", Raw Height: " << image_raw->getHeight()
        << ", Raw Components: " << (S32) image_raw->getComponents()
        << ", J2C Width: " << j2c->getWidth() << ", J2C Height: " << j2c->getHeight()
        << ", J2C Components: " << (S32) j2c->getComponents() << LL_ENDL;

    return image_raw;
}

void LLProcessTexture::parseInput(std::istream& input)
{
    mInputRaw = &input;
    LL_DEBUGS() << "Reading..." << LL_ENDL;
    // Parse LLSD header.
    mInputData = LLSDSerialize::fromBinary( *mInputRaw, MAX_SIZE_LLSD_HEADER );
    if (mInputData.isUndefined()) throw LLAppException(RV_UNABLE_TO_PARSE);
    if (!mInputData.has("slot_id")) throw LLAppException(RV_UNABLE_TO_PARSE, " Missing slot id");
    if (!mInputData.has("textures")) throw LLAppException(RV_UNABLE_TO_PARSE, " Missing texture header");
    if (!mInputData.has("wearables")) throw LLAppException(RV_UNABLE_TO_PARSE, " Missing wearables");

    // Verify the slot_id is valid.
    if (BAKED_NUM_INDICES == LLAvatarAppearance::getDictionary()->findBakedByImageName(mInputData["slot_id"].asString()))
    {
        throw LLAppException(RV_UNABLE_TO_PARSE, " Invalid slot id");
    }
}

void LLProcessTexture::init()
{
    if (mApp->isDebugMode()) gDebugGL = TRUE;

    S32 maxTextureDecodedWidth = 0;
    S32 maxTextureDecodedHeight = 0;

    // Extract texture data.
    LLSD::array_const_iterator iter = mInputData["textures"].beginArray();
    LLSD::array_const_iterator end = mInputData["textures"].endArray();
    std::map< LLUUID, LLPointer<LLImageRaw> > imageRawMap;

    for (; iter != end; ++iter)
    {
        LLUUID texture_id = (*iter)[0].asUUID();
        S32 texture_size = (*iter)[1];
        imageRawMap[texture_id] = create_texture_from_stream(*mInputRaw, texture_size, texture_id);
        maxTextureDecodedWidth = imageRawMap[texture_id]->getWidth() > maxTextureDecodedWidth ? imageRawMap[texture_id]->getWidth() : maxTextureDecodedWidth;
        maxTextureDecodedHeight = imageRawMap[texture_id]->getHeight() > maxTextureDecodedHeight ? imageRawMap[texture_id]->getHeight() : maxTextureDecodedHeight;
    }

    if ("eyes" == mInputData["slot_id"].asString())
    {
        mBakeSize = 512;
    }
    else
    {
        if ((maxTextureDecodedWidth > 512) || (maxTextureDecodedHeight > 512))
        {
            mBakeSize = 1024;
        }
        else
        {
            mBakeSize = 512;
        }
    }

    mWindow = new LLBakingWindow(mBakeSize, mBakeSize);

    for (iter = mInputData["textures"].beginArray(); iter != mInputData["textures"].endArray(); ++iter)
    {
        LLUUID texture_id = (*iter)[0].asUUID();

        LLPointer<LLBakingTexture> texture = new LLBakingTexture(texture_id, imageRawMap[texture_id]);
        texture->forceActive();
        texture->setGLTextureCreated(true);

        mTextureData[texture_id] = texture;
    }

}

LLPointer<LLGLTexture> LLProcessTexture::getLocalTexture(BOOL usemipmaps, BOOL generate_gl_tex)
{
    LLPointer<LLBakingTexture> tex = new LLBakingTexture(usemipmaps);
    if(generate_gl_tex)
    {
        tex->generateGLTexture() ;
        tex->setCategory(LLGLTexture::LOCAL) ;
    }
    return tex ;
}

LLPointer<LLGLTexture> LLProcessTexture::getLocalTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps, BOOL generate_gl_tex)
{
    LLPointer<LLBakingTexture> tex = new LLBakingTexture(width, height, components, usemipmaps) ;
    if(generate_gl_tex)
    {
        tex->generateGLTexture() ;
        tex->setCategory(LLGLTexture::LOCAL) ;
    }
    return tex ;
}

LLGLTexture* LLProcessTexture::getFetchedTexture(const LLUUID &image_id)
{
    texture_map_t::iterator texture_iter = mTextureData.find(image_id);
    if (mTextureData.end() == texture_iter)
    {
        LL_DEBUGS() << "Ignoring unused texture id: " << image_id << LL_ENDL;
        return NULL;
    }

    return texture_iter->second;
}

void LLProcessTexture::process(std::ostream& output)
{
    LL_DEBUGS() << "Building avatar..." << LL_ENDL;
    // Set ourself as the texture bridge.
    gTextureManagerBridgep = this;

    // Construct the avatar.
    LLBakingWearablesData wearable_data;
    LLBakingAvatar avatar(&wearable_data, mBakeSize);
    avatar.initInstance();
    wearable_data.setAvatarAppearance(&avatar);

    // Extract and parse wearables.
    wearable_data.setWearableOutfit(mInputData["wearables"]);

    avatar.setSex( (avatar.getVisualParamWeight( "male" ) > 0.5f) ? SEX_MALE : SEX_FEMALE );

    avatar.updateVisualParams();

    // Prepare gl for avatar baking
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    LLGLEnable color_mat(GL_COLOR_MATERIAL);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    EBakedTextureIndex bake_type = LLAvatarAppearance::getDictionary()->findBakedByImageName(mInputData["slot_id"].asString());
    LLTexLayerSet* layer_set = avatar.getAvatarLayerSet(bake_type);
    LLBakingTexLayerSetBuffer* composite = dynamic_cast<LLBakingTexLayerSetBuffer*> (layer_set->getComposite());
    if (!composite)
    {
        throw LLAppException(RV_UNABLE_TO_BAKE, " Could not build composite.");
    }

    LL_DEBUGS() << "Rendering..." << LL_ENDL;
    if (!composite->render())
    {
        throw LLAppException(RV_UNABLE_TO_BAKE, " Failed to render composite.");
    }

    mWindow->swapBuffers();

    LL_DEBUGS() << "Compressing..." << LL_ENDL;
    LLImageJ2C* j2c = composite->getCompressedImage();
    if (!j2c)
    {
        throw LLAppException(RV_UNABLE_TO_BAKE, " Could not build image.");
    }

    LL_DEBUGS() << "Writing..." << LL_ENDL;
    output.write((char*)j2c->getData(), j2c->getDataSize());
    LL_DEBUGS() << "Done." << LL_ENDL;
}
