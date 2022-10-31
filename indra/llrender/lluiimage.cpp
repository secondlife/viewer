/** 
 * @file lluiimage.cpp
 * @brief UI implementation
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

// Utilities functions the user interface needs

//#include "llviewerprecompiledheaders.h"
#include "linden_common.h"

// Project includes
#include "lluiimage.h"

LLUIImage::LLUIImage(const std::string& name, LLPointer<LLTexture> image)
:   mName(name),
    mImage(image),
    mScaleRegion(0.f, 1.f, 1.f, 0.f),
    mClipRegion(0.f, 1.f, 1.f, 0.f),
    mImageLoaded(NULL),
    mScaleStyle(SCALE_INNER),
    mCachedW(-1),
    mCachedH(-1)
{
    getTextureWidth();
    getTextureHeight();
}

LLUIImage::~LLUIImage()
{
    delete mImageLoaded;
}

S32 LLUIImage::getWidth() const
{ 
    // return clipped dimensions of actual image area
    return ll_round((F32)mImage->getWidth(0) * mClipRegion.getWidth()); 
}

S32 LLUIImage::getHeight() const
{ 
    // return clipped dimensions of actual image area
    return ll_round((F32)mImage->getHeight(0) * mClipRegion.getHeight()); 
}

void LLUIImage::draw3D(const LLVector3& origin_agent, const LLVector3& x_axis, const LLVector3& y_axis, 
                        const LLRect& rect, const LLColor4& color)
{
    F32 border_scale = 1.f;
    F32 border_height = (1.f - mScaleRegion.getHeight()) * getHeight();
    F32 border_width = (1.f - mScaleRegion.getWidth()) * getWidth();
    if (rect.getHeight() < border_height || rect.getWidth() < border_width)
    {
         if(border_height - rect.getHeight() > border_width - rect.getWidth())
         {
             border_scale = (F32)rect.getHeight() / border_height;
         }
         else
         {
            border_scale = (F32)rect.getWidth() / border_width;
         }
    }

    LLRender2D::getInstance()->pushMatrix();
    { 
        LLVector3 rect_origin = origin_agent + (rect.mLeft * x_axis) + (rect.mBottom * y_axis); 
        LLRender2D::getInstance()->translate(rect_origin.mV[VX],
                                            rect_origin.mV[VY], 
                                            rect_origin.mV[VZ]);
        gGL.getTexUnit(0)->bind(getImage());
        gGL.color4fv(color.mV);

        LLRectf center_uv_rect(mClipRegion.mLeft + mScaleRegion.mLeft * mClipRegion.getWidth(),
                            mClipRegion.mBottom + mScaleRegion.mTop * mClipRegion.getHeight(),
                            mClipRegion.mLeft + mScaleRegion.mRight * mClipRegion.getWidth(),
                            mClipRegion.mBottom + mScaleRegion.mBottom * mClipRegion.getHeight());
        gl_segmented_rect_3d_tex(mClipRegion,
                                center_uv_rect,
                                LLRectf(border_width * border_scale * 0.5f / (F32)rect.getWidth(),
                                        (rect.getHeight() - (border_height * border_scale * 0.5f)) / (F32)rect.getHeight(),
                                        (rect.getWidth() - (border_width * border_scale * 0.5f)) / (F32)rect.getWidth(),
                                        (border_height * border_scale * 0.5f) / (F32)rect.getHeight()),
                                rect.getWidth() * x_axis, 
                                rect.getHeight() * y_axis);
        
    } LLRender2D::getInstance()->popMatrix();
}

//#include "lluiimage.inl"

boost::signals2::connection LLUIImage::addLoadedCallback( const image_loaded_signal_t::slot_type& cb ) 
{
    if (!mImageLoaded) 
    {
        mImageLoaded = new image_loaded_signal_t();
    }
    return mImageLoaded->connect(cb);
}


void LLUIImage::onImageLoaded()
{
    if (mImageLoaded)
    {
        (*mImageLoaded)();
    }
}

namespace LLInitParam
{
    void ParamValue<LLUIImage*>::updateValueFromBlock()
    {
        // The keyword "none" is specifically requesting a null image
        // do not default to current value. Used to overwrite template images. 
        if (name() == "none")
        {
            updateValue(NULL);
            return;
        }

        LLUIImage* imagep =  LLRender2D::getInstance()->getUIImage(name());
        if (imagep)
        {
            updateValue(imagep);
        }
    }
    
    void ParamValue<LLUIImage*>::updateBlockFromValue(bool make_block_authoritative)
    {
        if (getValue() == NULL)
        {
            name.set("none", make_block_authoritative);
        }
        else
        {
            name.set(getValue()->getName(), make_block_authoritative);
        }
    }

    
    bool ParamCompare<LLUIImage*, false>::equals(
        LLUIImage* const &a,
        LLUIImage* const &b)
    {
        // force all LLUIImages for XML UI export to be "non-default"
        if (!a && !b)
            return false;
        else
            return (a == b);
    }
}

