/**
 * @file llthumbnailctrl.cpp
 * @brief LLThumbnailCtrl base class
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#include "llthumbnailctrl.h"

#include "linden_common.h"
#include "llagent.h"
#include "lluictrlfactory.h"
#include "lluuid.h"
#include "lltrans.h"
#include "llviewborder.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llwindow.h"

static LLDefaultChildRegistry::Register<LLThumbnailCtrl> r("thumbnail");

LLThumbnailCtrl::Params::Params()
: border("border")
, border_color("border_color")
, fallback_image("fallback_image")
, image_name("image_name")
, border_visible("show_visible", false)
, interactable("interactable", false)
, show_loading("show_loading", true)
{}

LLThumbnailCtrl::LLThumbnailCtrl(const LLThumbnailCtrl::Params& p)
:   LLUICtrl(p)
,   mBorderColor(p.border_color())
,   mBorderVisible(p.border_visible())
,   mFallbackImagep(p.fallback_image)
,   mInteractable(p.interactable())
,   mShowLoadingPlaceholder(p.show_loading())
,   mInited(false)
,   mInitImmediately(true)
{
    mLoadingPlaceholderString = LLTrans::getString("texture_loading");

    LLRect border_rect = getLocalRect();
    LLViewBorder::Params vbparams(p.border);
    vbparams.name("border");
    vbparams.rect(border_rect);
    mBorder = LLUICtrlFactory::create<LLViewBorder> (vbparams);
    addChild(mBorder);

    if (p.image_name.isProvided())
    {
        setValue(p.image_name());
    }
}

LLThumbnailCtrl::~LLThumbnailCtrl()
{
    mTexturep = nullptr;
    mImagep = nullptr;
    mFallbackImagep = nullptr;
}


void LLThumbnailCtrl::draw()
{
    if (!mInited)
    {
        initImage();
    }
    LLRect draw_rect = getLocalRect();

    if (mBorderVisible)
    {
        mBorder->setKeyboardFocusHighlight(hasFocus());

        gl_rect_2d( draw_rect, mBorderColor.get(), false );
        draw_rect.stretch( -1 );
    }

    // If we're in a focused floater, don't apply the floater's alpha to the texture.
    const F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
    if( mTexturep )
    {
        if( mTexturep->getComponents() == 4 )
        {
            const LLColor4 color(.098f, .098f, .098f);
            gl_rect_2d( draw_rect, color, true);
        }

        gl_draw_scaled_image( draw_rect.mLeft, draw_rect.mBottom, draw_rect.getWidth(), draw_rect.getHeight(), mTexturep, UI_VERTEX_COLOR % alpha);

        // Thumbnails are usually 256x256 or smaller, either report that or
        // some high value to get image with higher priority
        mTexturep->setKnownDrawSize(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE);
    }
    else if( mImagep.notNull() )
    {
        mImagep->draw(draw_rect, UI_VERTEX_COLOR % alpha );
    }
    else if (mFallbackImagep.notNull())
    {
        if (draw_rect.getWidth() > mFallbackImagep->getWidth()
            && draw_rect.getHeight() > mFallbackImagep->getHeight())
        {
            S32 img_width = mFallbackImagep->getWidth();
            S32 img_height = mFallbackImagep->getHeight();
            S32 rect_width = draw_rect.getWidth();
            S32 rect_height = draw_rect.getHeight();

            LLRect fallback_rect;
            fallback_rect.mLeft = draw_rect.mLeft + (rect_width - img_width) / 2;
            fallback_rect.mRight = fallback_rect.mLeft + img_width;
            fallback_rect.mBottom = draw_rect.mBottom + (rect_height - img_height) / 2;
            fallback_rect.mTop = fallback_rect.mBottom + img_height;

            mFallbackImagep->draw(fallback_rect, UI_VERTEX_COLOR % alpha);
        }
        else
        {
            mFallbackImagep->draw(draw_rect, UI_VERTEX_COLOR % alpha);
        }
    }
    else
    {
        gl_rect_2d( draw_rect, LLColor4::grey % alpha, true );

        // Draw X
        gl_draw_x( draw_rect, LLColor4::black );
    }

    // Show "Loading..." string on the top left corner while this texture is loading.
    // Using the discard level, do not show the string if the texture is almost but not
    // fully loaded.
    if (mTexturep.notNull()
        && mShowLoadingPlaceholder
        && !mTexturep->isFullyLoaded())
    {
        U32 v_offset = 25;
        LLFontGL* font = LLFontGL::getFontSansSerif();

        // Don't show as loaded if the texture is almost fully loaded (i.e. discard1) unless god
        if ((mTexturep->getDiscardLevel() > 1) || gAgent.isGodlike())
        {
            font->renderUTF8(
                mLoadingPlaceholderString,
                0,
                (draw_rect.mLeft+3),
                (draw_rect.mTop-v_offset),
                LLColor4::white,
                LLFontGL::LEFT,
                LLFontGL::BASELINE,
                LLFontGL::DROP_SHADOW);
        }
    }

    LLUICtrl::draw();
}

void LLThumbnailCtrl::setVisible(bool visible)
{
    if (!visible && mInited)
    {
        unloadImage();
    }
    LLUICtrl::setVisible(visible);
}

void LLThumbnailCtrl::clearTexture()
{
    setValue(LLSD());
    mInited = true; // nothing to do
}

// virtual
// value might be a string or a UUID
void LLThumbnailCtrl::setValue(const LLSD& value)
{
    LLSD tvalue(value);
    if (value.isString() && LLUUID::validate(value.asString()))
    {
        //RN: support UUIDs masquerading as strings
        tvalue = LLSD(LLUUID(value.asString()));
    }

    LLUICtrl::setValue(tvalue);

    unloadImage();

    if (mInitImmediately)
    {
        initImage();
    }
}

bool LLThumbnailCtrl::handleHover(S32 x, S32 y, MASK mask)
{
    if (mInteractable && getEnabled())
    {
        getWindow()->setCursor(UI_CURSOR_HAND);
        return true;
    }
    return LLUICtrl::handleHover(x, y, mask);
}

void LLThumbnailCtrl::initImage()
{
    if (mInited)
    {
        return;
    }
    mInited = true;
    LLSD tvalue = getValue();

    if (tvalue.isUUID())
    {
        mImageAssetID = tvalue.asUUID();
        if (mImageAssetID.notNull())
        {
            // Should it support baked textures?
            mTexturep = LLViewerTextureManager::getFetchedTexture(mImageAssetID, FTT_DEFAULT, MIPMAP_YES, LLGLTexture::BOOST_THUMBNAIL);
            mTexturep->forceToSaveRawImage(0);
            mTexturep->setKnownDrawSize(MAX_IMAGE_SIZE, MAX_IMAGE_SIZE);
        }
    }
    else if (tvalue.isString())
    {
        mImagep = LLUI::getUIImage(tvalue.asString(), LLGLTexture::BOOST_UI);
        if (mImagep)
        {
            LLViewerFetchedTexture* texture = dynamic_cast<LLViewerFetchedTexture*>(mImagep->getImage().get());
            if (texture)
            {
                mImageAssetID = texture->getID();
            }
        }
    }
}

void LLThumbnailCtrl::unloadImage()
{
    mImageAssetID = LLUUID::null;
    mTexturep = nullptr;
    mImagep = nullptr;
    mInited = false;
}


