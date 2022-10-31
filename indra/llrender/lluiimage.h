/** 
 * @file lluiimage.h
 * @brief wrapper for images used in the UI that handles smart scaling, etc.
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

#ifndef LL_LLUIIMAGE_H
#define LL_LLUIIMAGE_H

#include "v4color.h"
#include "llpointer.h"
#include "llrefcount.h"
#include "llrefcount.h"
#include "llrect.h"
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include "llinitparam.h"
#include "lltexture.h"
#include "llrender2dutils.h"

extern const LLColor4 UI_VERTEX_COLOR;

class LLUIImage : public LLRefCount
{
public:
    enum EScaleStyle
    {
        SCALE_INNER,
        SCALE_OUTER
    };

    typedef boost::signals2::signal<void (void)> image_loaded_signal_t;

    LLUIImage(const std::string& name, LLPointer<LLTexture> image);
    virtual ~LLUIImage();

    LL_FORCE_INLINE void setClipRegion(const LLRectf& region)
    { 
        mClipRegion = region; 
    }

    LL_FORCE_INLINE void setScaleRegion(const LLRectf& region)
    { 
        mScaleRegion = region; 
    }

    LL_FORCE_INLINE void setScaleStyle(EScaleStyle style)
    {
        mScaleStyle = style;
    }

    LL_FORCE_INLINE LLPointer<LLTexture> getImage() { return mImage; }
    LL_FORCE_INLINE const LLPointer<LLTexture>& getImage() const { return mImage; }

    LL_FORCE_INLINE void draw(S32 x, S32 y, S32 width, S32 height, const LLColor4& color = UI_VERTEX_COLOR) const;
    LL_FORCE_INLINE void draw(S32 x, S32 y, const LLColor4& color = UI_VERTEX_COLOR) const;
    LL_FORCE_INLINE void draw(const LLRect& rect, const LLColor4& color = UI_VERTEX_COLOR) const { draw(rect.mLeft, rect.mBottom, rect.getWidth(), rect.getHeight(), color); }
    
    LL_FORCE_INLINE void drawSolid(S32 x, S32 y, S32 width, S32 height, const LLColor4& color) const;
    LL_FORCE_INLINE void drawSolid(const LLRect& rect, const LLColor4& color) const { drawSolid(rect.mLeft, rect.mBottom, rect.getWidth(), rect.getHeight(), color); }
    LL_FORCE_INLINE void drawSolid(S32 x, S32 y, const LLColor4& color) const { drawSolid(x, y, getWidth(), getHeight(), color); }

    LL_FORCE_INLINE void drawBorder(S32 x, S32 y, S32 width, S32 height, const LLColor4& color, S32 border_width) const;
    LL_FORCE_INLINE void drawBorder(const LLRect& rect, const LLColor4& color, S32 border_width) const { drawBorder(rect.mLeft, rect.mBottom, rect.getWidth(), rect.getHeight(), color, border_width); }
    LL_FORCE_INLINE void drawBorder(S32 x, S32 y, const LLColor4& color, S32 border_width) const { drawBorder(x, y, getWidth(), getHeight(), color, border_width); }

    void draw3D(const LLVector3& origin_agent, const LLVector3& x_axis, const LLVector3& y_axis, const LLRect& rect, const LLColor4& color);

    LL_FORCE_INLINE const std::string& getName() const { return mName; }

    virtual S32 getWidth() const;
    virtual S32 getHeight() const;

    // returns dimensions of underlying textures, which might not be equal to ui image portion
    LL_FORCE_INLINE S32 getTextureWidth() const;
    LL_FORCE_INLINE S32 getTextureHeight() const;

    boost::signals2::connection addLoadedCallback( const image_loaded_signal_t::slot_type& cb );

    void onImageLoaded();

protected:
    image_loaded_signal_t* mImageLoaded;

    std::string             mName;
    LLRectf                 mScaleRegion;
    LLRectf                 mClipRegion;
    LLPointer<LLTexture>    mImage;
    EScaleStyle             mScaleStyle;
    mutable S32             mCachedW;
    mutable S32             mCachedH;
};

#include "lluiimage.inl"

namespace LLInitParam
{
    template<>
    class ParamValue<LLUIImage*> 
    :   public CustomParamValue<LLUIImage*>
    {
        typedef boost::add_reference<boost::add_const<LLUIImage*>::type>::type  T_const_ref;
        typedef CustomParamValue<LLUIImage*> super_t;
    public:
        Optional<std::string> name;

        ParamValue(LLUIImage* const& image = NULL)
        :   super_t(image)
        {
            updateBlockFromValue(false);
            addSynonym(name, "name");
        }

        void updateValueFromBlock();
        void updateBlockFromValue(bool make_block_authoritative);
    };

    // Need custom comparison function for our test app, which only loads
    // LLUIImage* as NULL.
    template<>
    struct ParamCompare<LLUIImage*, false>
    {
        static bool equals(LLUIImage* const &a, LLUIImage* const &b);
    };
}

typedef LLPointer<LLUIImage> LLUIImagePtr;
#endif
