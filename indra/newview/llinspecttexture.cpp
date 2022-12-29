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

#include "llfloaterreg.h"
#include "llinspect.h"
#include "llinspecttexture.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewertexturelist.h"

// ============================================================================
// LLInspectTexture class
//

class LLInspectTexture : public LLInspect
{
	friend class LLFloaterReg;
public:
	LLInspectTexture(const LLSD& sdKey);
	~LLInspectTexture();

public:
	void onOpen(const LLSD& sdData) override;
	BOOL postBuild() override;

public:
	const LLUUID& getAssetId() const { return mAssetId; }
	const LLUUID& getItemId() const  { return mItemId; }

protected:
	LLUUID         mAssetId;
	LLUUID         mItemId;		// Item UUID relative to gInventoryModel (or null if not displaying an inventory texture)
	LLUUID         mNotecardId;
	LLTextureCtrl* mTextureCtrl = nullptr;
	LLTextBox*     mTextureName = nullptr;
};

LLInspectTexture::LLInspectTexture(const LLSD& sdKey)
	: LLInspect(LLSD())
{
}

LLInspectTexture::~LLInspectTexture()
{
}

void LLInspectTexture::onOpen(const LLSD& sdData)
{
	// Start fade animation
	LLInspect::onOpen(sdData);

	bool fIsAsset = sdData.has("asset_id");
	bool fIsInventory = sdData.has("item_id");

	// Skip if we're being asked to display the same thing
	const LLUUID idAsset = (fIsAsset) ? sdData["asset_id"].asUUID() : LLUUID::null;
	const LLUUID idItem = (fIsInventory) ? sdData["item_id"].asUUID() : LLUUID::null;
	if ( (getVisible()) && ( ((fIsAsset) && (idAsset == mAssetId)) || ((fIsInventory) && (idItem == mItemId)) ) )
	{
		return;
	}

	// Position the inspector relative to the mouse cursor
	// Similar to how tooltips are positioned [see LLToolTipMgr::createToolTip()]
	if (sdData.has("pos"))
		LLUI::instance().positionViewNearMouse(this, sdData["pos"]["x"].asInteger(), sdData["pos"]["y"].asInteger());
	else
		LLUI::instance().positionViewNearMouse(this);

	std::string strName = sdData["name"].asString();
	if (fIsAsset)
	{
		mAssetId = idAsset;
		mItemId = idItem;		// Will be non-null in the case of a notecard
		mNotecardId = sdData["notecard_id"].asUUID();
	}
	else if (fIsInventory)
	{
		const LLViewerInventoryItem* pItem = gInventory.getItem(idItem);
		if ( (pItem) && (LLAssetType::AT_TEXTURE == pItem->getType()) )
		{
			if (strName.empty())
				strName = pItem->getName();
			mAssetId = pItem->getAssetUUID();
			mItemId = idItem;
		}
		else
		{
			mAssetId.setNull();
			mItemId.setNull();
		}
		mNotecardId = LLUUID::null;
	}

	mTextureCtrl->setImageAssetID(mAssetId);
	mTextureName->setText(strName);
}

BOOL LLInspectTexture::postBuild()
{
	mTextureCtrl = getChild<LLTextureCtrl>("texture_ctrl");
	mTextureName = getChild<LLTextBox>("texture_name");

	return TRUE;
}

// ============================================================================
// Helper functions
//

LLToolTip* LLInspectTextureUtil::createInventoryToolTip(LLToolTip::Params p)
{
	const LLSD& sdTooltip = p.create_params;

	LLInventoryType::EType eInvType = (sdTooltip.has("inv_type")) ? (LLInventoryType::EType)sdTooltip["inv_type"].asInteger() : LLInventoryType::IT_NONE;
	switch (eInvType)
	{
		case LLInventoryType::IT_SNAPSHOT:
		case LLInventoryType::IT_TEXTURE:
			return LLUICtrlFactory::create<LLTextureToolTip>(p);
		case LLInventoryType::IT_CATEGORY:
			{
				if (sdTooltip.has("item_id"))
				{
					const LLUUID idCategory = sdTooltip["item_id"].asUUID();

					LLInventoryModel::cat_array_t cats;
					LLInventoryModel::item_array_t items;
					LLIsOfAssetType f(LLAssetType::AT_TEXTURE);
					gInventory.getDirectDescendentsOf(idCategory, cats, items, f);

					// Exactly one texture found => show the texture tooltip
					if (1 == items.size())
					{
						p.create_params.getValue()["asset_id"] = items.front()->getAssetUUID();
						return LLUICtrlFactory::create<LLTextureToolTip>(p);
					}
				}

				// No or more than one texture found => show default tooltip
				return LLUICtrlFactory::create<LLToolTip>(p);
			}
		default:
			return LLUICtrlFactory::create<LLToolTip>(p);
	}
}

void LLInspectTextureUtil::registerFloater()
{
	LLFloaterReg::add("inspect_texture", "inspect_texture.xml", &LLFloaterReg::build<LLInspectTexture>);
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
	if (m_Image)
	{
		LLRect rctClient = getLocalRect();

		gl_rect_2d(rctClient, LLColor4::black);
		rctClient.stretch(-2);
		if (4 == m_Image->getComponents())
			gl_rect_2d_checkerboard(rctClient);
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
	if (sdTextureParams.has("asset_id"))
		mPreviewView->setImageFromAssetId(sdTextureParams["asset_id"].asUUID());
	else if (sdTextureParams.has("item_id"))
		mPreviewView->setImageFromItemId(sdTextureParams["item_id"].asUUID());

	snapToChildren();
}

// ============================================================================
