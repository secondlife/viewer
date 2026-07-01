/**
 * @file lluiimage.h
 * @brief wrapper for images used in the UI that handles smart scaling, etc.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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
#include "llrect.h"
#include "llinitparam.h"
#include "lltexture.h"
#include "llrender2dutils.h"
#include "llvertexbuffer.h"

#include <boost/signals2.hpp>

#include <bit>
#include <chrono>
#include <type_traits>
#include <unordered_map>
#include <vector>

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
        // This happens when image becomes loaded
        invalidateDisplayLists();
    }

    LL_FORCE_INLINE void setScaleRegion(const LLRectf& region)
    {
        mScaleRegion = region;
        // This happens when image becomes loaded
        invalidateDisplayLists();
    }

    LL_FORCE_INLINE void setScaleStyle(EScaleStyle style)
    {
        mScaleStyle = style;
        // This happens when image becomes loaded
        invalidateDisplayLists();
    }

    LL_FORCE_INLINE LLPointer<LLTexture> getImage() { return mImage; }
    LL_FORCE_INLINE const LLPointer<LLTexture>& getImage() const { return mImage; }

    LL_FORCE_INLINE void draw(S32 x, S32 y, S32 width, S32 height, const LLColor4& color, bool solid_color) const;
    LL_FORCE_INLINE void draw(S32 x, S32 y, S32 width, S32 height, const LLColor4& color = UI_VERTEX_COLOR) const;
    LL_FORCE_INLINE void draw(S32 x, S32 y, const LLColor4& color = UI_VERTEX_COLOR) const;
    LL_FORCE_INLINE void draw(const LLRect& rect, const LLColor4& color = UI_VERTEX_COLOR) const { draw(rect.mLeft, rect.mBottom, rect.getWidth(), rect.getHeight(), color); }

    LL_FORCE_INLINE void drawSolid(S32 x, S32 y, S32 width, S32 height, const LLColor4& color) const;
    LL_FORCE_INLINE void drawSolid(const LLRect& rect, const LLColor4& color) const { drawSolid(rect.mLeft, rect.mBottom, rect.getWidth(), rect.getHeight(), color); }
    LL_FORCE_INLINE void drawSolid(S32 x, S32 y, const LLColor4& color) const { drawSolid(x, y, getWidth(), getHeight(), color); }

    LL_FORCE_INLINE void drawBorder(S32 x, S32 y, S32 width, S32 height, const LLColor4& color, S32 border_width) const;
    LL_FORCE_INLINE void drawBorder(const LLRect& rect, const LLColor4& color, S32 border_width) const { drawBorder(rect.mLeft, rect.mBottom, rect.getWidth(), rect.getHeight(), color, border_width); }
    LL_FORCE_INLINE void drawBorder(S32 x, S32 y, const LLColor4& color, S32 border_width) const { drawBorder(x, y, getWidth(), getHeight(), color, border_width); }

    // Note: draw3D is not cached with display lists because it uses world-space rendering
    // with dynamic transforms (gl_segmented_rect_3d_tex). These calls are infrequent and
    // highly dynamic, making caching ineffective. The 2D UI methods benefit from caching
    // because they're called many times per frame with the same dimensions.
    void draw3D(const LLVector3& origin_agent, const LLVector3& x_axis, const LLVector3& y_axis, const LLRect& rect, const LLColor4& color);

    LL_FORCE_INLINE const std::string& getName() const { return mName; }

    virtual S32 getWidth() const;
    virtual S32 getHeight() const;

    // returns dimensions of underlying textures, which might not be equal to ui image portion
    LL_FORCE_INLINE S32 getTextureWidth() const;
    LL_FORCE_INLINE S32 getTextureHeight() const;

    boost::signals2::connection addLoadedCallback( const image_loaded_signal_t::slot_type& cb );

    void onImageLoaded();

    // Global cleanup of unused display lists across all LLUIImage instances
    // Should be called periodically (e.g., once per frame or when memory pressure is detected)
    static void updateClass();
    static void cleanupClass();

    static void enableDisplayListsCollection(bool enable) { sEnableDisplayListsCollection = enable; }

protected:
    // Packed key for identifying unique display list configurations
    struct PackedKey
    {
        uint64_t position;    // x and y coordinates
        uint64_t color_flags; // RGBA color + solid_color flag
        uint64_t dimensions;  // width and height
        uint64_t translate;   // UI offset
        uint64_t scale;       // UI scale

        constexpr bool operator==(const PackedKey& other) const
        {
            return position == other.position &&
                color_flags == other.color_flags &&
                dimensions == other.dimensions &&
                translate == other.translate &&
                scale == other.scale;
        }

        struct Hash
        {
            std::size_t operator()(const PackedKey& key) const
            {
                return static_cast<std::size_t>(key.position ^ key.color_flags ^
                                               key.dimensions ^ key.translate ^ key.scale);
            }
        };

        // Static factory function to create PackedKey from parameters
        static constexpr PackedKey create(S32 x, S32 y, S32 width, S32 height,
                                         const LLColor4& color, bool solid_color,
                                         const LLVector3& translate, const LLVector3& scale)
        {
            auto float_to_u8 = [](F32 f) -> uint8_t {
                return static_cast<uint8_t>(llclamp(f * 255.0f, 0.0f, 255.0f));
            };

            auto float_to_bits = [](F32 f) -> uint32_t {
                return std::bit_cast<uint32_t>(f);
            };

            uint8_t r = float_to_u8(color.mV[VRED]);
            uint8_t g = float_to_u8(color.mV[VGREEN]);
            uint8_t b = float_to_u8(color.mV[VBLUE]);
            uint8_t a = float_to_u8(color.mV[VALPHA]);

            uint64_t pos = (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) |
                static_cast<uint64_t>(static_cast<uint32_t>(y));

            uint64_t col = (static_cast<uint64_t>(r) << 56) |
                (static_cast<uint64_t>(g) << 48) |
                (static_cast<uint64_t>(b) << 40) |
                (static_cast<uint64_t>(a) << 32) |
                (solid_color ? 1ULL : 0ULL);

            uint64_t dim = (static_cast<uint64_t>(static_cast<uint32_t>(width)) << 32) |
                static_cast<uint64_t>(static_cast<uint32_t>(height));

            uint64_t trns = (static_cast<uint64_t>(float_to_bits(translate.mV[VX])) << 32) |
                static_cast<uint64_t>(float_to_bits(translate.mV[VY]));

            uint64_t scl = (static_cast<uint64_t>(float_to_bits(scale.mV[VX])) << 32) |
                static_cast<uint64_t>(float_to_bits(scale.mV[VY]));

            return PackedKey{ pos, col, dim, trns, scl };
        }
    };

    // Cached display list for a specific configuration
    struct CachedDisplayList
    {
        buffer_data_list_t list;
        std::chrono::steady_clock::time_point last_used;
    };

    // Get a display list for the given configuration
    buffer_data_list_t* findDisplayList(S32 x, S32 y, S32 width, S32 height, const LLColor4& color, bool solid_color) const;
    // Generate a new display list for the given configuration, draws immediately.
    buffer_data_list_t* genDisplayList(S32 x, S32 y, S32 width, S32 height, const LLColor4& color, bool solid_color) const;

    // Invalidate all cached display lists (called when image properties change)
    void invalidateDisplayLists();

    // Clean up old display lists for this image (called by updateClass)
    void cleanupDisplayLists();
    void unregisterFromGlobalCleanup();

    image_loaded_signal_t* mImageLoaded;

    std::string             mName;
    LLRectf                 mScaleRegion;
    LLRectf                 mClipRegion;
    LLPointer<LLTexture>    mImage;
    EScaleStyle             mScaleStyle;
    mutable S32             mCachedW;
    mutable S32             mCachedH;

    // Display list cache - now using PackedKey directly
    mutable std::unordered_map<PackedKey, CachedDisplayList, PackedKey::Hash> mDisplayLists;

    // Track all LLUIImage cache instances for global cleanup
    static std::vector<LLPointer<LLUIImage> > sImageList;
    static size_t sCleanupIndex;  // Round-robin cleanup position
    static bool sEnableDisplayListsCollection;
};

#include "lluiimage.inl"

namespace LLInitParam
{
    template<>
    class ParamValue<LLUIImage*>
    :   public CustomParamValue<LLUIImage*>
    {
        typedef std::add_lvalue_reference<std::add_const<LLUIImage*>::type>::type  T_const_ref;
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
