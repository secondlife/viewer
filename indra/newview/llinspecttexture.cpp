/**
 * @file llinspecttexture.cpp
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llinspect.h"
#include "llinspecttexture.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewertexturelist.h"


// ============================================================================
// Helper functions
//

LLToolTip* LLInspectTextureUtil::createInventoryToolTip(LLToolTip::Params p)
{
    const LLSD& sdTooltip = p.create_params;
    
    if (sdTooltip.has("thumbnail_id") && sdTooltip["thumbnail_id"].asUUID().notNull())
    {
        // go straight for thumbnail regardless of type
        // TODO: make a tooltip factory?
        return LLUICtrlFactory::create<LLTextureToolTip>(p);
    }

	LLInventoryType::EType eInvType = (sdTooltip.has("inv_type")) ? (LLInventoryType::EType)sdTooltip["inv_type"].asInteger() : LLInventoryType::IT_NONE;
	switch (eInvType)
	{
		case LLInventoryType::IT_CATEGORY:
			{
				if (sdTooltip.has("item_id"))
				{
					const LLUUID idCategory = sdTooltip["item_id"].asUUID();
                    LLViewerInventoryCategory* cat = gInventory.getCategory(idCategory);
                    if (cat && cat->getPreferredType() == LLFolderType::FT_OUTFIT)
                    {
                        LLInventoryModel::cat_array_t cats;
                        LLInventoryModel::item_array_t items;
                        // Not LLIsOfAssetType, because we allow links
                        LLIsTextureType f;
                        gInventory.getDirectDescendentsOf(idCategory, cats, items, f);

                        // Exactly one texture found => show the texture tooltip
                        if (1 == items.size())
                        {
                            LLViewerInventoryItem* item = items.front();
                            if (item && item->getIsLinkType())
                            {
                                item = item->getLinkedItem();
                            }
                            if (item)
                            {
                                // Note: LLFloaterChangeItemThumbnail will attempt to write this
                                // into folder's thumbnail id when opened
                                p.create_params.getValue()["thumbnail_id"] = item->getAssetUUID();
                                return LLUICtrlFactory::create<LLTextureToolTip>(p);
                            }
                        }
                    }
				}

				// No or more than one texture found => show default tooltip
				return LLUICtrlFactory::create<LLToolTip>(p);
			}
		default:
			return LLUICtrlFactory::create<LLToolTip>(p);
	}
}

// ============================================================================
// LLTexturePreviewView helper class
//

class LLTexturePreviewView : public LLView
{
public:
	LLTexturePreviewView(const LLView::Params& p);
	~LLTexturePreviewView();

public:
	void draw() override;

public:
	void setImageFromAssetId(const LLUUID& idAsset);
	void setImageFromItemId(const LLUUID& idItem);

protected:
	LLPointer<LLViewerFetchedTexture> m_Image;
	S32         mImageBoostLevel = LLGLTexture::BOOST_NONE;
	std::string mLoadingText;
};


LLTexturePreviewView::LLTexturePreviewView(const LLView::Params& p)
	: LLView(p)
{
	mLoadingText = LLTrans::getString("texture_loading");
}

LLTexturePreviewView::~LLTexturePreviewView()
{
	if (m_Image)
	{
		m_Image->setBoostLevel(mImageBoostLevel);
		m_Image = nullptr;
	}
}

void LLTexturePreviewView::draw()
{
    LLView::draw();

	if (m_Image)
	{
		LLRect rctClient = getLocalRect();

        if (4 == m_Image->getComponents())
        {
            const LLColor4 color(.098f, .098f, .098f);
            gl_rect_2d(rctClient, color, TRUE);
        }
		gl_draw_scaled_image(rctClient.mLeft, rctClient.mBottom, rctClient.getWidth(), rctClient.getHeight(), m_Image);

		bool isLoading = (!m_Image->isFullyLoaded()) && (m_Image->getDiscardLevel() > 0);
		if (isLoading)
			LLFontGL::getFontSansSerif()->renderUTF8(mLoadingText, 0, llfloor(rctClient.mLeft + 3),  llfloor(rctClient.mTop - 25), LLColor4::white, LLFontGL::LEFT, LLFontGL::BASELINE, LLFontGL::DROP_SHADOW);
		m_Image->addTextureStats((isLoading) ? MAX_IMAGE_AREA : (F32)(rctClient.getWidth() * rctClient.getHeight()));
	}
}

void LLTexturePreviewView::setImageFromAssetId(const LLUUID& idAsset)
{
	m_Image = LLViewerTextureManager::getFetchedTexture(idAsset, FTT_DEFAULT, MIPMAP_TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	if (m_Image)
	{
		mImageBoostLevel = m_Image->getBoostLevel();
		m_Image->setBoostLevel(LLGLTexture::BOOST_PREVIEW);
		m_Image->forceToSaveRawImage(0);
		if ( (!m_Image->isFullyLoaded()) && (!m_Image->hasFetcher()) )
		{
			if (m_Image->isInFastCacheList())
			{
				m_Image->loadFromFastCache();
			}
			gTextureList.forceImmediateUpdate(m_Image);
		}
	}
}

void LLTexturePreviewView::setImageFromItemId(const LLUUID& idItem)
{
	const LLViewerInventoryItem* pItem = gInventory.getItem(idItem);
	setImageFromAssetId( (pItem) ? pItem->getAssetUUID() : LLUUID::null );
}

// ============================================================================
// LLTextureToolTip class
//

LLTextureToolTip::LLTextureToolTip(const LLToolTip::Params& p)
	: LLToolTip(p)
	, mPreviewView(nullptr)
	, mPreviewSize(256)
{
	mMaxWidth = llmax(mMaxWidth, mPreviewSize);

    // Currently has to share params with LLToolTip, override values
    setBackgroundColor(LLColor4::black);
    setTransparentColor(LLColor4::black);
    setBorderVisible(true);
}

LLTextureToolTip::~LLTextureToolTip()
{
}

void LLTextureToolTip::initFromParams(const LLToolTip::Params& p)
{
    LLToolTip::initFromParams(p);
    
    // Create and add the preview control
    LLView::Params p_preview;
    p_preview.name = "texture_preview";
    LLRect rctPreview;
    rctPreview.setOriginAndSize(mPadding, mTextBox->getRect().mTop, mPreviewSize, mPreviewSize);
    p_preview.rect = rctPreview;
    mPreviewView = LLUICtrlFactory::create<LLTexturePreviewView>(p_preview);
    addChild(mPreviewView);
    
    // Parse the control params
    const LLSD& sdTextureParams = p.create_params;
    if (sdTextureParams.has("thumbnail_id"))
    {
        mPreviewView->setImageFromAssetId(sdTextureParams["thumbnail_id"].asUUID());
    }
    else if (sdTextureParams.has("item_id"))
    {
        mPreviewView->setImageFromItemId(sdTextureParams["item_id"].asUUID());
    }

    // Currently has to share params with LLToolTip, override values manually
    // Todo: provide from own params instead, may be like object inspector does it
    LLViewBorder::Params border_params;
    border_params.border_thickness(LLPANEL_BORDER_WIDTH);
    border_params.highlight_light_color(LLColor4::white);
    border_params.highlight_dark_color(LLColor4::white);
    border_params.shadow_light_color(LLColor4::white);
    border_params.shadow_dark_color(LLColor4::white);
    addBorder(border_params);
    setBorderVisible(true);

    setBackgroundColor(LLColor4::black);
    setBackgroundVisible(true);
    setBackgroundOpaque(true);
    setBackgroundImage(nullptr);
    setTransparentImage(nullptr);

    mTextBox->setColor(LLColor4::white);

	snapToChildren();
}

// ============================================================================
