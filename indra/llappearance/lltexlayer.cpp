/** 
 * @file lltexlayer.cpp
 * @brief A texture layer. Used for avatars.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "linden_common.h"

#include "lltexlayer.h"

#include "llavatarappearance.h"
#include "llcrc.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "lldir.h"
#include "lltexlayerparams.h"
#include "lltexturemanagerbridge.h"
#include "lllocaltextureobject.h"
#include "../llui/llui.h"
#include "llwearable.h"
#include "llwearabledata.h"
#include "llvertexbuffer.h"
#include "llviewervisualparam.h"
#include "llfasttimer.h"

//#include "../tools/imdebug/imdebug.h"

using namespace LLAvatarAppearanceDefines;

// runway consolidate
extern std::string self_av_string();

class LLTexLayerInfo
{
	friend class LLTexLayer;
	friend class LLTexLayerTemplate;
	friend class LLTexLayerInterface;
public:
	LLTexLayerInfo();
	~LLTexLayerInfo();

	BOOL parseXml(LLXmlTreeNode* node);
	BOOL createVisualParams(LLAvatarAppearance *appearance);
	BOOL isUserSettable() { return mLocalTexture != -1;	}
	S32  getLocalTexture() const { return mLocalTexture; }
	BOOL getOnlyAlpha() const { return mUseLocalTextureAlphaOnly; }
	std::string getName() const { return mName;	}

private:
	std::string				mName;
	
	BOOL					mWriteAllChannels; // Don't use masking.  Just write RGBA into buffer,
	LLTexLayerInterface::ERenderPass mRenderPass;

	std::string				mGlobalColor;
	LLColor4				mFixedColor;

	S32						mLocalTexture;
	std::string				mStaticImageFileName;
	BOOL					mStaticImageIsMask;
	BOOL					mUseLocalTextureAlphaOnly; // Ignore RGB channels from the input texture.  Use alpha as a mask
	BOOL					mIsVisibilityMask;

	typedef std::vector< std::pair< std::string,BOOL > > morph_name_list_t;
	morph_name_list_t		    mMorphNameList;
	param_color_info_list_t		mParamColorInfoList;
	param_alpha_info_list_t		mParamAlphaInfoList;
};

//-----------------------------------------------------------------------------
// LLTexLayerSetBuffer
// The composite image that a LLViewerTexLayerSet writes to.  Each LLViewerTexLayerSet has one.
//-----------------------------------------------------------------------------

LLTexLayerSetBuffer::LLTexLayerSetBuffer(LLTexLayerSet* const owner) :
	mTexLayerSet(owner)
{
}

LLTexLayerSetBuffer::~LLTexLayerSetBuffer()
{
}

void LLTexLayerSetBuffer::pushProjection() const
{
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadIdentity();
	gGL.ortho(0.0f, getCompositeWidth(), 0.0f, getCompositeHeight(), -1.0f, 1.0f);

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	gGL.loadIdentity();
}

void LLTexLayerSetBuffer::popProjection() const
{
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.popMatrix();
}

// virtual
void LLTexLayerSetBuffer::preRenderTexLayerSet()
{
	// Set up an ortho projection
	pushProjection();
}

// virtual
void LLTexLayerSetBuffer::postRenderTexLayerSet(BOOL success)
{
	popProjection();
}

BOOL LLTexLayerSetBuffer::renderTexLayerSet(LLRenderTarget* bound_target)
{
	// Default color mask for tex layer render
	gGL.setColorMask(true, true);

	BOOL success = TRUE;
	
	gAlphaMaskProgram.bind();
	gAlphaMaskProgram.setMinimumAlpha(0.004f);

	LLVertexBuffer::unbind();

	// Composite the color data
	LLGLSUIDefault gls_ui;
	success &= mTexLayerSet->render( getCompositeOriginX(), getCompositeOriginY(), 
									 getCompositeWidth(), getCompositeHeight(), bound_target );
	gGL.flush();

	midRenderTexLayerSet(success);

	gAlphaMaskProgram.unbind();

	LLVertexBuffer::unbind();
	
	// reset GL state
	gGL.setColorMask(true, true);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);

	return success;
}

//-----------------------------------------------------------------------------
// LLTexLayerSetInfo
// An ordered set of texture layers that get composited into a single texture.
//-----------------------------------------------------------------------------

LLTexLayerSetInfo::LLTexLayerSetInfo() :
	mBodyRegion( "" ),
	mWidth( 512 ),
	mHeight( 512 ),
	mClearAlpha( TRUE )
{
}

LLTexLayerSetInfo::~LLTexLayerSetInfo( )
{
	std::for_each(mLayerInfoList.begin(), mLayerInfoList.end(), DeletePointer());
	mLayerInfoList.clear();
}

BOOL LLTexLayerSetInfo::parseXml(LLXmlTreeNode* node)
{
	llassert( node->hasName( "layer_set" ) );
	if( !node->hasName( "layer_set" ) )
	{
		return FALSE;
	}

	// body_region
	static LLStdStringHandle body_region_string = LLXmlTree::addAttributeString("body_region");
	if( !node->getFastAttributeString( body_region_string, mBodyRegion ) )
	{
		LL_WARNS() << "<layer_set> is missing body_region attribute" << LL_ENDL;
		return FALSE;
	}

	// width, height
	static LLStdStringHandle width_string = LLXmlTree::addAttributeString("width");
	if( !node->getFastAttributeS32( width_string, mWidth ) )
	{
		return FALSE;
	}

	static LLStdStringHandle height_string = LLXmlTree::addAttributeString("height");
	if( !node->getFastAttributeS32( height_string, mHeight ) )
	{
		return FALSE;
	}

	// Optional alpha component to apply after all compositing is complete.
	static LLStdStringHandle alpha_tga_file_string = LLXmlTree::addAttributeString("alpha_tga_file");
	node->getFastAttributeString( alpha_tga_file_string, mStaticAlphaFileName );

	static LLStdStringHandle clear_alpha_string = LLXmlTree::addAttributeString("clear_alpha");
	node->getFastAttributeBOOL( clear_alpha_string, mClearAlpha );

	// <layer>
	for (LLXmlTreeNode* child = node->getChildByName( "layer" );
		 child;
		 child = node->getNextNamedChild())
	{
		LLTexLayerInfo* info = new LLTexLayerInfo();
		if( !info->parseXml( child ))
		{
			delete info;
			return FALSE;
		}
		mLayerInfoList.push_back( info );		
	}
	return TRUE;
}

// creates visual params without generating layersets or layers
void LLTexLayerSetInfo::createVisualParams(LLAvatarAppearance *appearance)
{
	//layer_info_list_t		mLayerInfoList;
	for (LLTexLayerInfo* layer_info : mLayerInfoList)
	{
		layer_info->createVisualParams(appearance);
	}
}

//-----------------------------------------------------------------------------
// LLTexLayerSet
// An ordered set of texture layers that get composited into a single texture.
//-----------------------------------------------------------------------------

BOOL LLTexLayerSet::sHasCaches = FALSE;

LLTexLayerSet::LLTexLayerSet(LLAvatarAppearance* const appearance) :
	mAvatarAppearance( appearance ),
	mIsVisible( TRUE ),
	mBakedTexIndex(LLAvatarAppearanceDefines::BAKED_HEAD),
	mInfo( NULL )
{
}

// virtual
LLTexLayerSet::~LLTexLayerSet()
{
	deleteCaches();
	std::for_each(mLayerList.begin(), mLayerList.end(), DeletePointer());
	mLayerList.clear();

	std::for_each(mMaskLayerList.begin(), mMaskLayerList.end(), DeletePointer());
	mMaskLayerList.clear();
}

//-----------------------------------------------------------------------------
// setInfo
//-----------------------------------------------------------------------------

BOOL LLTexLayerSet::setInfo(const LLTexLayerSetInfo *info)
{
	llassert(mInfo == NULL);
	mInfo = info;
	//mID = info->mID; // No ID

	mLayerList.reserve(info->mLayerInfoList.size());
	for (LLTexLayerInfo* layer_info : info->mLayerInfoList)
	{
		LLTexLayerInterface *layer = NULL;
		if (layer_info->isUserSettable())
		{
			layer = new LLTexLayerTemplate( this, getAvatarAppearance() );
		}
		else
		{
			layer = new LLTexLayer(this);
		}
		// this is the first time this layer (of either type) is being created - make sure you add the parameters to the avatar appearance
		if (!layer->setInfo(layer_info, NULL))
		{
			mInfo = NULL;
			return FALSE;
		}
		if (!layer->isVisibilityMask())
		{
			mLayerList.push_back( layer );
		}
		else
		{
			mMaskLayerList.push_back(layer);
		}
	}

	requestUpdate();

	stop_glerror();

	return TRUE;
}

#if 0 // obsolete
//-----------------------------------------------------------------------------
// parseData
//-----------------------------------------------------------------------------

BOOL LLTexLayerSet::parseData(LLXmlTreeNode* node)
{
	LLTexLayerSetInfo *info = new LLTexLayerSetInfo;

	if (!info->parseXml(node))
	{
		delete info;
		return FALSE;
	}
	if (!setInfo(info))
	{
		delete info;
		return FALSE;
	}
	return TRUE;
}
#endif

void LLTexLayerSet::deleteCaches()
{
	for(LLTexLayerInterface* layer : mLayerList)
	{
		layer->deleteCaches();
	}
	for (LLTexLayerInterface* layer : mMaskLayerList)
	{
		layer->deleteCaches();
	}
}


BOOL LLTexLayerSet::render( S32 x, S32 y, S32 width, S32 height, LLRenderTarget* bound_target )
{
	BOOL success = TRUE;
	mIsVisible = TRUE;

	if (mMaskLayerList.size() > 0)
	{
		for (LLTexLayerInterface* layer : mMaskLayerList)
		{
			if (layer->isInvisibleAlphaMask())
			{
				mIsVisible = FALSE;
			}
		}
	}

	LLGLSUIDefault gls_ui;
	LLGLDepthTest gls_depth(GL_FALSE, GL_FALSE);
	gGL.setColorMask(true, true);

	// clear buffer area to ensure we don't pick up UI elements
	{
		gGL.flush();
		LLGLDisable no_alpha(GL_ALPHA_TEST);
		gAlphaMaskProgram.setMinimumAlpha(0.0f);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4f( 0.f, 0.f, 0.f, 1.f );

		gl_rect_2d_simple( width, height );

		gGL.flush();
		gAlphaMaskProgram.setMinimumAlpha(0.004f);
	}

	if (mIsVisible)
	{
		// composite color layers
		for(LLTexLayerInterface* layer : mLayerList)
		{
			if (layer->getRenderPass() == LLTexLayer::RP_COLOR)
			{
				gGL.flush();
				success &= layer->render(x, y, width, height, bound_target);
				gGL.flush();
			}
		}
		
		renderAlphaMaskTextures(x, y, width, height, bound_target, false);
	
		stop_glerror();
	}
	else
	{
		gGL.flush();

		gGL.setSceneBlendType(LLRender::BT_REPLACE);
		LLGLDisable no_alpha(GL_ALPHA_TEST);
		gAlphaMaskProgram.setMinimumAlpha(0.f);

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4f( 0.f, 0.f, 0.f, 0.f );

		gl_rect_2d_simple( width, height );
		gGL.setSceneBlendType(LLRender::BT_ALPHA);

		gGL.flush();
		gAlphaMaskProgram.setMinimumAlpha(0.004f);
	}

	return success;
}


BOOL LLTexLayerSet::isBodyRegion(const std::string& region) const 
{ 
	return mInfo->mBodyRegion == region; 
}

const std::string LLTexLayerSet::getBodyRegionName() const 
{ 
	return mInfo->mBodyRegion; 
}

void LLTexLayerSet::destroyComposite()
{
	if( mComposite )
	{
		mComposite = NULL;
	}
}

LLTexLayerSetBuffer* LLTexLayerSet::getComposite()
{
	if (!mComposite)
	{
		createComposite();
	}
	return mComposite;
}

const LLTexLayerSetBuffer* LLTexLayerSet::getComposite() const
{
	return mComposite;
}

void LLTexLayerSet::gatherMorphMaskAlpha(U8 *data, S32 origin_x, S32 origin_y, S32 width, S32 height, LLRenderTarget* bound_target)
{
    LL_PROFILE_ZONE_SCOPED;
	memset(data, 255, width * height);

	for(LLTexLayerInterface* layer : mLayerList)
	{
		layer->gatherAlphaMasks(data, origin_x, origin_y, width, height, bound_target);
	}
	
	// Set alpha back to that of our alpha masks.
	renderAlphaMaskTextures(origin_x, origin_y, width, height, bound_target, true);
}

void LLTexLayerSet::renderAlphaMaskTextures(S32 x, S32 y, S32 width, S32 height, LLRenderTarget* bound_target, bool forceClear)
{
    LL_PROFILE_ZONE_SCOPED;
	const LLTexLayerSetInfo *info = getInfo();
	
	gGL.setColorMask(false, true);
	gGL.setSceneBlendType(LLRender::BT_REPLACE);
	
	// (Optionally) replace alpha with a single component image from a tga file.
	if (!info->mStaticAlphaFileName.empty())
	{
		gGL.flush();
		{
			LLGLTexture* tex = LLTexLayerStaticImageList::getInstance()->getTexture(info->mStaticAlphaFileName, TRUE);
			if( tex )
			{
				LLGLSUIDefault gls_ui;
				gGL.getTexUnit(0)->bind(tex);
				gl_rect_2d_simple_tex( width, height );
			}
		}
		gGL.flush();
	}
	else if (forceClear || info->mClearAlpha || (mMaskLayerList.size() > 0))
	{
		// Set the alpha channel to one (clean up after previous blending)
		gGL.flush();
		LLGLDisable no_alpha(GL_ALPHA_TEST);
		gAlphaMaskProgram.setMinimumAlpha(0.f);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4f( 0.f, 0.f, 0.f, 1.f );
		
		gl_rect_2d_simple( width, height );
		
		gGL.flush();
		gAlphaMaskProgram.setMinimumAlpha(0.004f);
	}
	
	// (Optional) Mask out part of the baked texture with alpha masks
	// will still have an effect even if mClearAlpha is set or the alpha component was replaced
	if (mMaskLayerList.size() > 0)
	{
		gGL.setSceneBlendType(LLRender::BT_MULT_ALPHA);
		for (LLTexLayerInterface* layer : mMaskLayerList)
		{
			gGL.flush();
			layer->blendAlphaTexture(x,y,width, height);
			gGL.flush();
		}
		
	}
	
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	
	gGL.setColorMask(true, true);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);
}

void LLTexLayerSet::applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components)
{
	mAvatarAppearance->applyMorphMask(tex_data, width, height, num_components, mBakedTexIndex);
}

BOOL LLTexLayerSet::isMorphValid() const
{
	for(const LLTexLayerInterface* layer : mLayerList)
	{
		if (layer && !layer->isMorphValid())
		{
			return FALSE;
		}
	}
	return TRUE;
}

void LLTexLayerSet::invalidateMorphMasks()
{
	for(LLTexLayerInterface* layer : mLayerList)
	{
		if (layer)
		{
			layer->invalidateMorphMasks();
		}
	}
}


//-----------------------------------------------------------------------------
// LLTexLayerInfo
//-----------------------------------------------------------------------------
LLTexLayerInfo::LLTexLayerInfo() :
	mWriteAllChannels( FALSE ),
	mRenderPass(LLTexLayer::RP_COLOR),
	mFixedColor( 0.f, 0.f, 0.f, 0.f ),
	mLocalTexture( -1 ),
	mStaticImageIsMask( FALSE ),
	mUseLocalTextureAlphaOnly(FALSE),
	mIsVisibilityMask(FALSE)
{
}

LLTexLayerInfo::~LLTexLayerInfo( )
{
	std::for_each(mParamColorInfoList.begin(), mParamColorInfoList.end(), DeletePointer());
	mParamColorInfoList.clear();
	std::for_each(mParamAlphaInfoList.begin(), mParamAlphaInfoList.end(), DeletePointer());
	mParamAlphaInfoList.clear();
}

BOOL LLTexLayerInfo::parseXml(LLXmlTreeNode* node)
{
	llassert( node->hasName( "layer" ) );

	// name attribute
	static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
	if( !node->getFastAttributeString( name_string, mName ) )
	{
		return FALSE;
	}
	
	static LLStdStringHandle write_all_channels_string = LLXmlTree::addAttributeString("write_all_channels");
	node->getFastAttributeBOOL( write_all_channels_string, mWriteAllChannels );

	std::string render_pass_name;
	static LLStdStringHandle render_pass_string = LLXmlTree::addAttributeString("render_pass");
	if( node->getFastAttributeString( render_pass_string, render_pass_name ) )
	{
		if( render_pass_name == "bump" )
		{
			mRenderPass = LLTexLayer::RP_BUMP;
		}
	}

	// Note: layers can have either a "global_color" attrib, a "fixed_color" attrib, or a <param_color> child.
	// global color attribute (optional)
	static LLStdStringHandle global_color_string = LLXmlTree::addAttributeString("global_color");
	node->getFastAttributeString( global_color_string, mGlobalColor );

	// Visibility mask (optional)
	BOOL is_visibility;
	static LLStdStringHandle visibility_mask_string = LLXmlTree::addAttributeString("visibility_mask");
	if (node->getFastAttributeBOOL(visibility_mask_string, is_visibility))
	{
		mIsVisibilityMask = is_visibility;
	}

	// color attribute (optional)
	LLColor4U color4u;
	static LLStdStringHandle fixed_color_string = LLXmlTree::addAttributeString("fixed_color");
	if( node->getFastAttributeColor4U( fixed_color_string, color4u ) )
	{
		mFixedColor.setVec( color4u );
	}

		// <texture> optional sub-element
	for (LLXmlTreeNode* texture_node = node->getChildByName( "texture" );
		 texture_node;
		 texture_node = node->getNextNamedChild())
	{
		std::string local_texture_name;
		static LLStdStringHandle tga_file_string = LLXmlTree::addAttributeString("tga_file");
		static LLStdStringHandle local_texture_string = LLXmlTree::addAttributeString("local_texture");
		static LLStdStringHandle file_is_mask_string = LLXmlTree::addAttributeString("file_is_mask");
		static LLStdStringHandle local_texture_alpha_only_string = LLXmlTree::addAttributeString("local_texture_alpha_only");
		if( texture_node->getFastAttributeString( tga_file_string, mStaticImageFileName ) )
		{
			texture_node->getFastAttributeBOOL( file_is_mask_string, mStaticImageIsMask );
		}
		else if (texture_node->getFastAttributeString(local_texture_string, local_texture_name))
		{
			texture_node->getFastAttributeBOOL( local_texture_alpha_only_string, mUseLocalTextureAlphaOnly );

			/* if ("upper_shirt" == local_texture_name)
				mLocalTexture = TEX_UPPER_SHIRT; */
			mLocalTexture = TEX_NUM_INDICES;
			for (const LLAvatarAppearanceDictionary::Textures::value_type& dict_pair : LLAvatarAppearance::getDictionary()->getTextures())
			{
				const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = dict_pair.second;
				if (local_texture_name == texture_dict->mName)
			{
					mLocalTexture = dict_pair.first;
					break;
			}
			}
			if (mLocalTexture == TEX_NUM_INDICES)
			{
				LL_WARNS() << "<texture> element has invalid local_texture attribute: " << mName << " " << local_texture_name << LL_ENDL;
				return FALSE;
			}
		}
		else	
		{
			LL_WARNS() << "<texture> element is missing a required attribute. " << mName << LL_ENDL;
			return FALSE;
		}
	}

	for (LLXmlTreeNode* maskNode = node->getChildByName( "morph_mask" );
		 maskNode;
		 maskNode = node->getNextNamedChild())
	{
		std::string morph_name;
		static LLStdStringHandle morph_name_string = LLXmlTree::addAttributeString("morph_name");
		if (maskNode->getFastAttributeString(morph_name_string, morph_name))
		{
			BOOL invert = FALSE;
			static LLStdStringHandle invert_string = LLXmlTree::addAttributeString("invert");
			maskNode->getFastAttributeBOOL(invert_string, invert);			
			mMorphNameList.push_back(std::pair<std::string,BOOL>(morph_name,invert));
		}
	}

	// <param> optional sub-element (color or alpha params)
	for (LLXmlTreeNode* child = node->getChildByName( "param" );
		 child;
		 child = node->getNextNamedChild())
	{
		if( child->getChildByName( "param_color" ) )
		{
			// <param><param_color/></param>
			LLTexLayerParamColorInfo* info = new LLTexLayerParamColorInfo();
			if (!info->parseXml(child))
			{
				delete info;
				return FALSE;
			}
			mParamColorInfoList.push_back(info);
		}
		else if( child->getChildByName( "param_alpha" ) )
		{
			// <param><param_alpha/></param>
			LLTexLayerParamAlphaInfo* info = new LLTexLayerParamAlphaInfo( );
			if (!info->parseXml(child))
			{
				delete info;
				return FALSE;
			}
 			mParamAlphaInfoList.push_back(info);
		}
	}
	
	return TRUE;
}

BOOL LLTexLayerInfo::createVisualParams(LLAvatarAppearance *appearance)
{
	BOOL success = TRUE;
	for (LLTexLayerParamColorInfo* color_info : mParamColorInfoList)
	{
		LLTexLayerParamColor* param_color = new LLTexLayerParamColor(appearance);
		if (!param_color->setInfo(color_info, TRUE))
		{
			LL_WARNS() << "NULL TexLayer Color Param could not be added to visual param list. Deleting." << LL_ENDL;
			delete param_color;
			success = FALSE;
		}
	}

	for (LLTexLayerParamAlphaInfo* alpha_info : mParamAlphaInfoList)
	{
		LLTexLayerParamAlpha* param_alpha = new LLTexLayerParamAlpha(appearance);
		if (!param_alpha->setInfo(alpha_info, TRUE))
		{
			LL_WARNS() << "NULL TexLayer Alpha Param could not be added to visual param list. Deleting." << LL_ENDL;
			delete param_alpha;
			success = FALSE;
		}
	}

	return success;
}

LLTexLayerInterface::LLTexLayerInterface(LLTexLayerSet* const layer_set):
	mTexLayerSet( layer_set ),
	mMorphMasksValid( FALSE ),
	mInfo(NULL),
	mHasMorph(FALSE)
{
}

LLTexLayerInterface::LLTexLayerInterface(const LLTexLayerInterface &layer, LLWearable *wearable):
	mTexLayerSet( layer.mTexLayerSet ),
	mInfo(NULL)
{
	// don't add visual params for cloned layers
	setInfo(layer.getInfo(), wearable);

	mHasMorph = layer.mHasMorph;
}

BOOL LLTexLayerInterface::setInfo(const LLTexLayerInfo *info, LLWearable* wearable  ) // This sets mInfo and calls initialization functions
{
	// setInfo should only be called once. Code is not robust enough to handle redefinition of a texlayer.
	// Not a critical warning, but could be useful for debugging later issues. -Nyx
	if (mInfo != NULL) 
	{
			LL_WARNS() << "mInfo != NULL" << LL_ENDL;
	}
	mInfo = info;
	//mID = info->mID; // No ID

	mParamColorList.reserve(mInfo->mParamColorInfoList.size());
	for (LLTexLayerParamColorInfo* color_info : mInfo->mParamColorInfoList)
	{
		LLTexLayerParamColor* param_color;
		if (!wearable)
			{
				param_color = new LLTexLayerParamColor(this);
				if (!param_color->setInfo(color_info, TRUE))
				{
					mInfo = NULL;
					return FALSE;
				}
			}
			else
			{
				param_color = (LLTexLayerParamColor*)wearable->getVisualParam(color_info->getID());
				if (!param_color)
				{
					mInfo = NULL;
					return FALSE;
				}
			}
			mParamColorList.push_back( param_color );
		}

	mParamAlphaList.reserve(mInfo->mParamAlphaInfoList.size());
	for (LLTexLayerParamAlphaInfo* alpha_info : mInfo->mParamAlphaInfoList)
		{
			LLTexLayerParamAlpha* param_alpha;
			if (!wearable)
			{
				param_alpha = new LLTexLayerParamAlpha( this );
				if (!param_alpha->setInfo(alpha_info, TRUE))
				{
					mInfo = NULL;
					return FALSE;
				}
			}
			else
			{
				param_alpha = (LLTexLayerParamAlpha*) wearable->getVisualParam(alpha_info->getID());
				if (!param_alpha)
				{
					mInfo = NULL;
					return FALSE;
				}
			}
			mParamAlphaList.push_back( param_alpha );
		}

	return TRUE;
}

/*virtual*/ void LLTexLayerInterface::requestUpdate()
{
	mTexLayerSet->requestUpdate();
}

const std::string& LLTexLayerInterface::getName() const
{
	return mInfo->mName; 
}

ETextureIndex LLTexLayerInterface::getLocalTextureIndex() const
{
	return (ETextureIndex) mInfo->mLocalTexture;
}

LLWearableType::EType LLTexLayerInterface::getWearableType() const
{
	ETextureIndex te = getLocalTextureIndex();
	if (TEX_INVALID == te)
	{
		LLWearableType::EType type = LLWearableType::WT_INVALID;

		for (LLTexLayerParamColor* param : mParamColorList)
		{
			if (param) 
			{
				LLWearableType::EType new_type = (LLWearableType::EType)param->getWearableType();
				if (new_type != LLWearableType::WT_INVALID && new_type != type) 
				{
					if (type != LLWearableType::WT_INVALID) 
					{
						return LLWearableType::WT_INVALID;
					}
					type = new_type;
				}
			}
		}

		for (LLTexLayerParamAlpha* param : mParamAlphaList)
		{
			if (param) 
			{
				LLWearableType::EType new_type = (LLWearableType::EType)param->getWearableType();
				if (new_type != LLWearableType::WT_INVALID && new_type != type) 
				{
					if (type != LLWearableType::WT_INVALID) 
					{
						return LLWearableType::WT_INVALID;
					}
					type = new_type;
				}
			}
		}

		return type;
	}
	return LLAvatarAppearance::getDictionary()->getTEWearableType(te);
}

LLTexLayerInterface::ERenderPass LLTexLayerInterface::getRenderPass() const
{
	return mInfo->mRenderPass; 
}

const std::string& LLTexLayerInterface::getGlobalColor() const
{
	return mInfo->mGlobalColor; 
}

BOOL LLTexLayerInterface::isVisibilityMask() const
{
	return mInfo->mIsVisibilityMask;
}

void LLTexLayerInterface::invalidateMorphMasks()
{
	mMorphMasksValid = FALSE;
}

LLViewerVisualParam* LLTexLayerInterface::getVisualParamPtr(S32 index) const
{
	LLViewerVisualParam *result = NULL;
	for (LLTexLayerParamColor* param : mParamColorList)
	{
		if (param->getID() == index)
		{
			result = param;
		}
	}
	for (LLTexLayerParamAlpha* param : mParamAlphaList)
	{
		if (param->getID() == index)
		{
			result = param;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// LLTexLayer
// A single texture layer, consisting of:
//		* color, consisting of either
//			* one or more color parameters (weighted colors)
//			* a reference to a global color
//			* a fixed color with non-zero alpha
//			* opaque white (the default)
//		* (optional) a texture defined by either
//			* a GUID
//			* a texture entry index (TE)
//		* (optional) one or more alpha parameters (weighted alpha textures)
//-----------------------------------------------------------------------------
LLTexLayer::LLTexLayer(LLTexLayerSet* const layer_set) :
	LLTexLayerInterface( layer_set ),
	mLocalTextureObject(NULL)
{
}

LLTexLayer::LLTexLayer(const LLTexLayer &layer, LLWearable *wearable) :
	LLTexLayerInterface( layer, wearable ),
	mLocalTextureObject(NULL)
{
}

LLTexLayer::LLTexLayer(const LLTexLayerTemplate &layer_template, LLLocalTextureObject *lto, LLWearable *wearable) :
	LLTexLayerInterface( layer_template, wearable ),
	mLocalTextureObject(lto)
{
}

LLTexLayer::~LLTexLayer()
{
	// mParamAlphaList and mParamColorList are LLViewerVisualParam's and get
	// deleted with ~LLCharacter()
	//std::for_each(mParamAlphaList.begin(), mParamAlphaList.end(), DeletePointer());
	//std::for_each(mParamColorList.begin(), mParamColorList.end(), DeletePointer());
	
	for (alpha_cache_t::value_type& alpha_pair : mAlphaCache)
	{
		U8* alpha_data = alpha_pair.second;
		ll_aligned_free_32(alpha_data);
	}

}

void LLTexLayer::asLLSD(LLSD& sd) const
{
	// *TODO: Finish
	sd["id"] = getUUID();
}

//-----------------------------------------------------------------------------
// setInfo
//-----------------------------------------------------------------------------

BOOL LLTexLayer::setInfo(const LLTexLayerInfo* info, LLWearable* wearable  )
{
	return LLTexLayerInterface::setInfo(info, wearable);
}

//static 
void LLTexLayer::calculateTexLayerColor(const param_color_list_t &param_list, LLColor4 &net_color)
{
	for (const LLTexLayerParamColor* param : param_list)
	{
		LLColor4 param_net = param->getNetColor();
		const LLTexLayerParamColorInfo *info = (LLTexLayerParamColorInfo *)param->getInfo();
		switch(info->getOperation())
		{
			case LLTexLayerParamColor::OP_ADD:
				net_color += param_net;
				break;
			case LLTexLayerParamColor::OP_MULTIPLY:
				net_color = net_color * param_net;
				break;
			case LLTexLayerParamColor::OP_BLEND:
				net_color = lerp(net_color, param_net, param->getWeight());
				break;
			default:
				llassert(0);
				break;
		}
	}
	net_color.clamp();
}

/*virtual*/ void LLTexLayer::deleteCaches()
{
	// Only need to delete caches for alpha params. Color params don't hold extra memory
	for (LLTexLayerParamAlpha* param : mParamAlphaList)
	{
		param->deleteCaches();
	}
}

BOOL LLTexLayer::render(S32 x, S32 y, S32 width, S32 height, LLRenderTarget* bound_target)
{
	LLGLEnable color_mat(GL_COLOR_MATERIAL);
	// *TODO: Is this correct?
	//gPipeline.disableLights();
	stop_glerror();

	LLColor4 net_color;
	BOOL color_specified = findNetColor(&net_color);
	
	if (mTexLayerSet->getAvatarAppearance()->mIsDummy)
	{
		color_specified = true;
		net_color = LLAvatarAppearance::getDummyColor();
	}

	BOOL success = TRUE;
	
	// If you can't see the layer, don't render it.
	if( is_approx_zero( net_color.mV[VW] ) )
	{
		return success;
	}

	BOOL alpha_mask_specified = FALSE;
	param_alpha_list_t::const_iterator iter = mParamAlphaList.begin();
	if( iter != mParamAlphaList.end() )
	{
		// If we have alpha masks, but we're skipping all of them, skip the whole layer.
		// However, we can't do this optimization if we have morph masks that need updating.
/*		if (!mHasMorph)
		{
			BOOL skip_layer = TRUE;

			while( iter != mParamAlphaList.end() )
			{
				const LLTexLayerParamAlpha* param = *iter;
		
				if( !param->getSkip() )
				{
					skip_layer = FALSE;
					break;
				}

				iter++;
			} 

			if( skip_layer )
			{
				return success;
			}
		}//*/

		const bool force_render = true;
		renderMorphMasks(x, y, width, height, net_color, bound_target, force_render);
		alpha_mask_specified = TRUE;
		gGL.flush();
		gGL.blendFunc(LLRender::BF_DEST_ALPHA, LLRender::BF_ONE_MINUS_DEST_ALPHA);
	}

	gGL.color4fv( net_color.mV);

	if( getInfo()->mWriteAllChannels )
	{
		gGL.flush();
		gGL.setSceneBlendType(LLRender::BT_REPLACE);
	}

	if( (getInfo()->mLocalTexture != -1) && !getInfo()->mUseLocalTextureAlphaOnly )
	{
		{
			LLGLTexture* tex = NULL;
			if (mLocalTextureObject && mLocalTextureObject->getImage())
			{
				tex = mLocalTextureObject->getImage();
				if (mLocalTextureObject->getID() == IMG_DEFAULT_AVATAR)
				{
					tex = NULL;
				}
			}
			else
			{
				LL_INFOS() << "lto not defined or image not defined: " << getInfo()->getLocalTexture() << " lto: " << mLocalTextureObject << LL_ENDL;
			}
//			if( mTexLayerSet->getAvatarAppearance()->getLocalTextureGL((ETextureIndex)getInfo()->mLocalTexture, &image_gl ) )
			{
				if( tex )
				{
					bool no_alpha_test = getInfo()->mWriteAllChannels;
					LLGLDisable alpha_test(no_alpha_test ? GL_ALPHA_TEST : 0);
					if (no_alpha_test)
					{
						gAlphaMaskProgram.setMinimumAlpha(0.f);
					}
					
					LLTexUnit::eTextureAddressMode old_mode = tex->getAddressMode();
					
					gGL.getTexUnit(0)->bind(tex, TRUE);
					gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

					gl_rect_2d_simple_tex( width, height );

					gGL.getTexUnit(0)->setTextureAddressMode(old_mode);
					gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
					if (no_alpha_test)
					{
						gAlphaMaskProgram.setMinimumAlpha(0.004f);
					}
				}
			}
//			else
//			{
//				success = FALSE;
//			}
		}
	}

	if( !getInfo()->mStaticImageFileName.empty() )
	{
		{
			LLGLTexture* tex = LLTexLayerStaticImageList::getInstance()->getTexture(getInfo()->mStaticImageFileName, getInfo()->mStaticImageIsMask);
			if( tex )
			{
				gGL.getTexUnit(0)->bind(tex, TRUE);
				gl_rect_2d_simple_tex( width, height );
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			}
			else
			{
				success = FALSE;
			}
		}
	}

	if(((-1 == getInfo()->mLocalTexture) ||
		 getInfo()->mUseLocalTextureAlphaOnly) &&
		getInfo()->mStaticImageFileName.empty() &&
		color_specified )
	{
		LLGLDisable no_alpha(GL_ALPHA_TEST);
		gAlphaMaskProgram.setMinimumAlpha(0.000f);

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4fv( net_color.mV );
		gl_rect_2d_simple( width, height );
		gAlphaMaskProgram.setMinimumAlpha(0.004f);
	}

	if( alpha_mask_specified || getInfo()->mWriteAllChannels )
	{
		// Restore standard blend func value
		gGL.flush();
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
		stop_glerror();
	}

	if( !success )
	{
		LL_INFOS() << "LLTexLayer::render() partial: " << getInfo()->mName << LL_ENDL;
	}
	return success;
}

const U8*	LLTexLayer::getAlphaData() const
{
	LLCRC alpha_mask_crc;
	const LLUUID& uuid = getUUID();
	alpha_mask_crc.update((U8*)(&uuid.mData), UUID_BYTES);

	for (const LLTexLayerParamAlpha* param : mParamAlphaList)
	{
		// MULTI-WEARABLE: verify visual parameters used here
		F32 param_weight = param->getWeight();
		alpha_mask_crc.update((U8*)&param_weight, sizeof(F32));
	}

	U32 cache_index = alpha_mask_crc.getCRC();

	alpha_cache_t::const_iterator iter2 = mAlphaCache.find(cache_index);
	return (iter2 == mAlphaCache.end()) ? 0 : iter2->second;
}

BOOL LLTexLayer::findNetColor(LLColor4* net_color) const
{
	// Color is either:
	//	* one or more color parameters (weighted colors)  (which may make use of a global color or fixed color)
	//	* a reference to a global color
	//	* a fixed color with non-zero alpha
	//	* opaque white (the default)

	if( !mParamColorList.empty() )
	{
		if( !getGlobalColor().empty() )
		{
			net_color->setVec( mTexLayerSet->getAvatarAppearance()->getGlobalColor( getInfo()->mGlobalColor ) );
		}
		else if (getInfo()->mFixedColor.mV[VW])
		{
			net_color->setVec( getInfo()->mFixedColor );
		}
		else
		{
			net_color->setVec( 0.f, 0.f, 0.f, 0.f );
		}
		
		calculateTexLayerColor(mParamColorList, *net_color);
		return TRUE;
	}

	if( !getGlobalColor().empty() )
	{
		net_color->setVec( mTexLayerSet->getAvatarAppearance()->getGlobalColor( getGlobalColor() ) );
		return TRUE;
	}

	if( getInfo()->mFixedColor.mV[VW] )
	{
		net_color->setVec( getInfo()->mFixedColor );
		return TRUE;
	}

	net_color->setToWhite();

	return FALSE; // No need to draw a separate colored polygon
}

BOOL LLTexLayer::blendAlphaTexture(S32 x, S32 y, S32 width, S32 height)
{
	BOOL success = TRUE;

	gGL.flush();
	
	if( !getInfo()->mStaticImageFileName.empty() )
	{
		LLGLTexture* tex = LLTexLayerStaticImageList::getInstance()->getTexture( getInfo()->mStaticImageFileName, getInfo()->mStaticImageIsMask );
		if( tex )
		{
			LLGLSNoAlphaTest gls_no_alpha_test;
			gAlphaMaskProgram.setMinimumAlpha(0.f);
			gGL.getTexUnit(0)->bind(tex, TRUE);
			gl_rect_2d_simple_tex( width, height );
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			gAlphaMaskProgram.setMinimumAlpha(0.004f);
		}
		else
		{
			success = FALSE;
		}
	}
	else
	{
		if (getInfo()->mLocalTexture >=0 && getInfo()->mLocalTexture < TEX_NUM_INDICES)
		{
			LLGLTexture* tex = mLocalTextureObject->getImage();
			if (tex)
			{
				LLGLSNoAlphaTest gls_no_alpha_test;
				gAlphaMaskProgram.setMinimumAlpha(0.f);
				gGL.getTexUnit(0)->bind(tex);
				gl_rect_2d_simple_tex( width, height );
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				gAlphaMaskProgram.setMinimumAlpha(0.004f);
			}
		}
	}
	
	return success;
}

/*virtual*/ void LLTexLayer::gatherAlphaMasks(U8 *data, S32 originX, S32 originY, S32 width, S32 height, LLRenderTarget* bound_target)
{
	addAlphaMask(data, originX, originY, width, height, bound_target);
}

void LLTexLayer::renderMorphMasks(S32 x, S32 y, S32 width, S32 height, const LLColor4 &layer_color, LLRenderTarget* bound_target, bool force_render)
{
	if (!force_render && !hasMorph())
	{
		LL_DEBUGS() << "skipping renderMorphMasks for " << getUUID() << LL_ENDL;
		return;
	}
    LL_PROFILE_ZONE_SCOPED;
	BOOL success = TRUE;

	llassert( !mParamAlphaList.empty() );

	gAlphaMaskProgram.setMinimumAlpha(0.f);
	gGL.setColorMask(false, true);

	LLTexLayerParamAlpha* first_param = *mParamAlphaList.begin();
	// Note: if the first param is a mulitply, multiply against the current buffer's alpha
	if( !first_param || !first_param->getMultiplyBlend() )
	{
		LLGLDisable no_alpha(GL_ALPHA_TEST);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	
		// Clear the alpha
		gGL.flush();
		gGL.setSceneBlendType(LLRender::BT_REPLACE);

		gGL.color4f( 0.f, 0.f, 0.f, 0.f );
		gl_rect_2d_simple( width, height );
	}

	// Accumulate alphas
	LLGLSNoAlphaTest gls_no_alpha_test;
	gGL.color4f( 1.f, 1.f, 1.f, 1.f );
	for (LLTexLayerParamAlpha* param : mParamAlphaList)
	{
		success &= param->render( x, y, width, height );
		if (!success && !force_render)
		{
			LL_DEBUGS() << "Failed to render param " << param->getID() << " ; skipping morph mask." << LL_ENDL;
			return;
		}
	}

	// Approximates a min() function
	gGL.flush();
	gGL.setSceneBlendType(LLRender::BT_MULT_ALPHA);

	// Accumulate the alpha component of the texture
	if( getInfo()->mLocalTexture != -1 )
	{
		LLGLTexture* tex = mLocalTextureObject->getImage();
		if( tex && (tex->getComponents() == 4) )
		{
			LLGLSNoAlphaTest gls_no_alpha_test;
			LLTexUnit::eTextureAddressMode old_mode = tex->getAddressMode();
			
			gGL.getTexUnit(0)->bind(tex, TRUE);
			gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

			gl_rect_2d_simple_tex( width, height );

			gGL.getTexUnit(0)->setTextureAddressMode(old_mode);
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		}
	}

	if( !getInfo()->mStaticImageFileName.empty() && getInfo()->mStaticImageIsMask )
	{
		LLGLTexture* tex = LLTexLayerStaticImageList::getInstance()->getTexture(getInfo()->mStaticImageFileName, getInfo()->mStaticImageIsMask);
		if( tex )
		{
			if(	(tex->getComponents() == 4) || (tex->getComponents() == 1) )
			{
				LLGLSNoAlphaTest gls_no_alpha_test;
				gGL.getTexUnit(0)->bind(tex, TRUE);
				gl_rect_2d_simple_tex( width, height );
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			}
			else
			{
				LL_WARNS() << "Skipping rendering of " << getInfo()->mStaticImageFileName 
						<< "; expected 1 or 4 components." << LL_ENDL;
			}
		}
	}

	// Draw a rectangle with the layer color to multiply the alpha by that color's alpha.
	// Note: we're still using gGL.blendFunc( GL_DST_ALPHA, GL_ZERO );
	if ( !is_approx_equal(layer_color.mV[VW], 1.f) )
	{
		LLGLDisable no_alpha(GL_ALPHA_TEST);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4fv(layer_color.mV);
		gl_rect_2d_simple( width, height );
	}

	gAlphaMaskProgram.setMinimumAlpha(0.004f);

	LLGLSUIDefault gls_ui;

	gGL.setColorMask(true, true);
	
	if (hasMorph() && success)
	{
		LLCRC alpha_mask_crc;
		const LLUUID& uuid = getUUID();
		alpha_mask_crc.update((U8*)(&uuid.mData), UUID_BYTES);
		
		for (const LLTexLayerParamAlpha* param : mParamAlphaList)
		{
			F32 param_weight = param->getWeight();
			alpha_mask_crc.update((U8*)&param_weight, sizeof(F32));
		}

		U32 cache_index = alpha_mask_crc.getCRC();
		U8* alpha_data = NULL; 
                // We believe we need to generate morph masks, do not assume that the cached version is accurate.
                // We can get bad morph masks during login, on minimize, and occasional gl errors.
                // We should only be doing this when we believe something has changed with respect to the user's appearance.
		{
                       LL_DEBUGS("Avatar") << "gl alpha cache of morph mask not found, doing readback: " << getName() << LL_ENDL;
                        // clear out a slot if we have filled our cache
			S32 max_cache_entries = getTexLayerSet()->getAvatarAppearance()->isSelf() ? 4 : 1;
			while ((S32)mAlphaCache.size() >= max_cache_entries)
			{
				alpha_cache_t::iterator iter2 = mAlphaCache.begin(); // arbitrarily grab the first entry
				alpha_data = iter2->second;
                ll_aligned_free_32(alpha_data);
				mAlphaCache.erase(iter2);
			}
			
            // GPUs tend to be very uptight about memory alignment as the DMA used to convey
            // said data to the card works better when well-aligned so plain old default-aligned heap mem is a no-no
            //new U8[width * height];
            size_t bytes_per_pixel = 1; // unsigned byte alpha channel only...
            size_t row_size        = (width + 3) & ~0x3; // OpenGL 4-byte row align (even for things < 4 bpp...)
            size_t pixels          = (row_size * height);
            size_t mem_size        = pixels * bytes_per_pixel;

            alpha_data = (U8*)ll_aligned_malloc_32(mem_size);

            bool skip_readback = LLRender::sNsightDebugSupport; // nSight doesn't support use of glReadPixels

			if (!skip_readback)
			{
                if (gGLManager.mIsIntel)
                { // work-around for broken intel drivers which cannot do glReadPixels on an RGBA FBO
                  // returning only the alpha portion without locking up downstream 
                    U8* temp = (U8*)ll_aligned_malloc_32(mem_size << 2); // allocate same size, but RGBA

                    if (bound_target)
                    {
                        gGL.getTexUnit(0)->bind(bound_target);
                    }
                    else
                    {
                        gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, 0);
                    }

                    glGetTexImage(LLTexUnit::getInternalType(LLTexUnit::TT_TEXTURE), 0, GL_RGBA, GL_UNSIGNED_BYTE, temp);

                    U8* alpha_cursor = alpha_data;
                    U8* pixel        = temp;
                    for (int i = 0; i < pixels; i++)
                    {
                        *alpha_cursor++ = pixel[3];
                        pixel += 4;
                    }

                    gGL.getTexUnit(0)->disable();

                    ll_aligned_free_32(temp);
                }
                else
                { // platforms with working drivers...
				    glReadPixels(x, y, width, height, GL_ALPHA, GL_UNSIGNED_BYTE, alpha_data);                
                }
			}
            else
            {
                ll_aligned_free_32(alpha_data);
                alpha_data = nullptr;
            }

            mAlphaCache[cache_index] = alpha_data;
		}
		
		getTexLayerSet()->getAvatarAppearance()->dirtyMesh();

		mMorphMasksValid = TRUE;
		getTexLayerSet()->applyMorphMask(alpha_data, width, height, 1);
	}
}

void LLTexLayer::addAlphaMask(U8 *data, S32 originX, S32 originY, S32 width, S32 height, LLRenderTarget* bound_target)
{
    LL_PROFILE_ZONE_SCOPED;
	S32 size = width * height;
	const U8* alphaData = getAlphaData();
	if (!alphaData && hasAlphaParams())
	{
		LLColor4 net_color;
		findNetColor( &net_color );
		// TODO: eliminate need for layer morph mask valid flag
		invalidateMorphMasks();
		const bool force_render = false;
		renderMorphMasks(originX, originY, width, height, net_color, bound_target, force_render);
		alphaData = getAlphaData();
	}
	if (alphaData)
	{
		for( S32 i = 0; i < size; i++ )
		{
			U8 curAlpha = data[i];
			U16 resultAlpha = curAlpha;
			resultAlpha *= ( ((U16)alphaData[i]) + 1);
			resultAlpha = resultAlpha >> 8;
			data[i] = (U8)resultAlpha;
		}
	}
}

/*virtual*/ BOOL LLTexLayer::isInvisibleAlphaMask() const
{
	if (mLocalTextureObject)
	{
		if (mLocalTextureObject->getID() == IMG_INVISIBLE)
		{
			return TRUE;
		}
	}

	return FALSE;
}

LLUUID LLTexLayer::getUUID() const
{
	LLUUID uuid;
	if( getInfo()->mLocalTexture != -1 )
	{
			LLGLTexture* tex = mLocalTextureObject->getImage();
			if (tex)
			{
				uuid = mLocalTextureObject->getID();
			}
	}
	if( !getInfo()->mStaticImageFileName.empty() )
	{
			LLGLTexture* tex = LLTexLayerStaticImageList::getInstance()->getTexture(getInfo()->mStaticImageFileName, getInfo()->mStaticImageIsMask);
			if( tex )
			{
				uuid = tex->getID();
			}
	}
	return uuid;
}


//-----------------------------------------------------------------------------
// LLTexLayerTemplate
// A single texture layer, consisting of:
//		* color, consisting of either
//			* one or more color parameters (weighted colors)
//			* a reference to a global color
//			* a fixed color with non-zero alpha
//			* opaque white (the default)
//		* (optional) a texture defined by either
//			* a GUID
//			* a texture entry index (TE)
//		* (optional) one or more alpha parameters (weighted alpha textures)
//-----------------------------------------------------------------------------
LLTexLayerTemplate::LLTexLayerTemplate(LLTexLayerSet* layer_set, LLAvatarAppearance* const appearance) :
	LLTexLayerInterface(layer_set),
	mAvatarAppearance( appearance )
{
}

LLTexLayerTemplate::LLTexLayerTemplate(const LLTexLayerTemplate &layer) :
	LLTexLayerInterface(layer),
	mAvatarAppearance(layer.getAvatarAppearance())
{
}

LLTexLayerTemplate::~LLTexLayerTemplate()
{
}

//-----------------------------------------------------------------------------
// setInfo
//-----------------------------------------------------------------------------

/*virtual*/ BOOL LLTexLayerTemplate::setInfo(const LLTexLayerInfo* info, LLWearable* wearable  )
{
	return LLTexLayerInterface::setInfo(info, wearable);
}

U32 LLTexLayerTemplate::updateWearableCache() const
{
	mWearableCache.clear();

	LLWearableType::EType wearable_type = getWearableType();
	if (LLWearableType::WT_INVALID == wearable_type)
	{
		//this isn't a cloneable layer 
		return 0;
	}
	U32 num_wearables = getAvatarAppearance()->getWearableData()->getWearableCount(wearable_type);
	U32 added = 0;
	for (U32 i = 0; i < num_wearables; i++)
	{
		LLWearable*  wearable = getAvatarAppearance()->getWearableData()->getWearable(wearable_type, i);
		if (!wearable)
		{
			continue;
		}
		mWearableCache.push_back(wearable);
		added++;
	}
	return added;
}
LLTexLayer* LLTexLayerTemplate::getLayer(U32 i) const
{
	if (mWearableCache.size() <= i)
	{
		return NULL;
	}
	LLWearable *wearable = mWearableCache[i];
	LLLocalTextureObject *lto = NULL;
	LLTexLayer *layer = NULL;
	if (wearable)
	{
		 lto = wearable->getLocalTextureObject(mInfo->mLocalTexture);
	}
	if (lto)
	{
		layer = lto->getTexLayer(getName());
	}
	return layer;
}

/*virtual*/ BOOL LLTexLayerTemplate::render(S32 x, S32 y, S32 width, S32 height, LLRenderTarget* bound_target)
{
	if(!mInfo)
	{
		return FALSE ;
	}

	BOOL success = TRUE;
	updateWearableCache();
	for (LLWearable* wearable : mWearableCache)
	{
		LLLocalTextureObject *lto = NULL;
		LLTexLayer *layer = NULL;
		if (wearable)
		{
			lto = wearable->getLocalTextureObject(mInfo->mLocalTexture);
		}
		if (lto)
		{
			layer = lto->getTexLayer(getName());
		}
		if (layer)
		{
			wearable->writeToAvatar(mAvatarAppearance);
			layer->setLTO(lto);
			success &= layer->render(x, y, width, height, bound_target);
		}
	}

	return success;
}

/*virtual*/ BOOL LLTexLayerTemplate::blendAlphaTexture( S32 x, S32 y, S32 width, S32 height) // Multiplies a single alpha texture against the frame buffer
{
	BOOL success = TRUE;
	U32 num_wearables = updateWearableCache();
	for (U32 i = 0; i < num_wearables; i++)
	{
		LLTexLayer *layer = getLayer(i);
		if (layer)
		{
			success &= layer->blendAlphaTexture(x,y,width,height);
		}
	}
	return success;
}

/*virtual*/ void LLTexLayerTemplate::gatherAlphaMasks(U8 *data, S32 originX, S32 originY, S32 width, S32 height, LLRenderTarget* bound_target)
{
	U32 num_wearables = updateWearableCache();
	U32 i = num_wearables - 1; // For rendering morph masks, we only want to use the top wearable
	LLTexLayer *layer = getLayer(i);
	if (layer)
	{
		layer->addAlphaMask(data, originX, originY, width, height, bound_target);
	}
}

/*virtual*/ void LLTexLayerTemplate::setHasMorph(BOOL newval)
{ 
	mHasMorph = newval;
	U32 num_wearables = updateWearableCache();
	for (U32 i = 0; i < num_wearables; i++)
	{
		LLTexLayer *layer = getLayer(i);
		if (layer)
		{	
			layer->setHasMorph(newval);
		}
	}
}

/*virtual*/ void LLTexLayerTemplate::deleteCaches()
{
	U32 num_wearables = updateWearableCache();
	for (U32 i = 0; i < num_wearables; i++)
	{
		LLTexLayer *layer = getLayer(i);
		if (layer)
		{
			layer->deleteCaches();
		}
	}
}

/*virtual*/ BOOL LLTexLayerTemplate::isInvisibleAlphaMask() const
{
	U32 num_wearables = updateWearableCache();
	for (U32 i = 0; i < num_wearables; i++)
	{
		LLTexLayer *layer = getLayer(i);
		if (layer)
		{
			 if (layer->isInvisibleAlphaMask())
			 {
				 return TRUE;
			 }
		}
	}

	return FALSE;
}


//-----------------------------------------------------------------------------
// finds a specific layer based on a passed in name
//-----------------------------------------------------------------------------
LLTexLayerInterface*  LLTexLayerSet::findLayerByName(const std::string& name)
{
	for (LLTexLayerInterface* layer : mLayerList)
	{
		if (layer->getName() == name)
		{
			return layer;
		}
	}
	for (LLTexLayerInterface* layer : mMaskLayerList)
	{
		if (layer->getName() == name)
		{
			return layer;
		}
	}
	return NULL;
}

void LLTexLayerSet::cloneTemplates(LLLocalTextureObject *lto, LLAvatarAppearanceDefines::ETextureIndex tex_index, LLWearable *wearable)
{
	// initialize all texlayers with this texture type for this LTO
	for(LLTexLayerInterface* layer : mLayerList)
	{
		LLTexLayerTemplate* layer_template = (LLTexLayerTemplate*)layer;
		if (layer_template->getInfo()->getLocalTexture() == (S32)tex_index)
		{
			lto->addTexLayer(layer_template, wearable);
		}
	}
	for(LLTexLayerInterface* layer : mMaskLayerList)
	{
		LLTexLayerTemplate* layer_template = (LLTexLayerTemplate*)layer;
		if (layer_template->getInfo()->getLocalTexture() == (S32)tex_index)
		{
			lto->addTexLayer(layer_template, wearable);
		}
	}
}
//-----------------------------------------------------------------------------
// LLTexLayerStaticImageList
//-----------------------------------------------------------------------------

LLTexLayerStaticImageList::LLTexLayerStaticImageList() :
	mGLBytes(0),
	mTGABytes(0),
	mImageNames(16384)
{
}

LLTexLayerStaticImageList::~LLTexLayerStaticImageList()
{
	deleteCachedImages();
}

void LLTexLayerStaticImageList::dumpByteCount() const
{
	LL_INFOS() << "Avatar Static Textures " <<
		"KB GL:" << (mGLBytes / 1024) <<
		"KB TGA:" << (mTGABytes / 1024) << "KB" << LL_ENDL;
}

void LLTexLayerStaticImageList::deleteCachedImages()
{
	if( mGLBytes || mTGABytes )
	{
		LL_INFOS() << "Clearing Static Textures " <<
			"KB GL:" << (mGLBytes / 1024) <<
			"KB TGA:" << (mTGABytes / 1024) << "KB" << LL_ENDL;

		//mStaticImageLists uses LLPointers, clear() will cause deletion
		
		mStaticImageListTGA.clear();
		mStaticImageList.clear();
		
		mGLBytes = 0;
		mTGABytes = 0;
	}
}

// Note: in general, for a given image image we'll call either getImageTga() or getTexture().
// We call getImageTga() if the image is used as an alpha gradient.
// Otherwise, we call getTexture()

// Returns an LLImageTGA that contains the encoded data from a tga file named file_name.
// Caches the result to speed identical subsequent requests.
LLImageTGA* LLTexLayerStaticImageList::getImageTGA(const std::string& file_name)
{
    LL_PROFILE_ZONE_SCOPED;
	const char *namekey = mImageNames.addString(file_name);
	image_tga_map_t::const_iterator iter = mStaticImageListTGA.find(namekey);
	if( iter != mStaticImageListTGA.end() )
	{
		return iter->second;
	}
	else
	{
		std::string path;
		path = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,file_name);
		LLPointer<LLImageTGA> image_tga = new LLImageTGA( path );
		if( image_tga->getDataSize() > 0 )
		{
			mStaticImageListTGA[ namekey ] = image_tga;
			mTGABytes += image_tga->getDataSize();
			return image_tga;
		}
		else
		{
			return NULL;
		}
	}
}

// Returns a GL Image (without a backing ImageRaw) that contains the decoded data from a tga file named file_name.
// Caches the result to speed identical subsequent requests.
LLGLTexture* LLTexLayerStaticImageList::getTexture(const std::string& file_name, BOOL is_mask)
{
    LL_PROFILE_ZONE_SCOPED;
	LLPointer<LLGLTexture> tex;
	const char *namekey = mImageNames.addString(file_name);

	texture_map_t::const_iterator iter = mStaticImageList.find(namekey);
	if( iter != mStaticImageList.end() )
	{
		tex = iter->second;
	}
	else
	{
		llassert(gTextureManagerBridgep);
		tex = gTextureManagerBridgep->getLocalTexture( FALSE );
		LLPointer<LLImageRaw> image_raw = new LLImageRaw;
		if( loadImageRaw( file_name, image_raw ) )
		{
			if( (image_raw->getComponents() == 1) && is_mask )
			{
				// Convert grayscale alpha masks from single channel into RGBA.
				// Fill RGB with black to allow fixed function gl calls
				// to match shader implementation.
				LLPointer<LLImageRaw> alpha_image_raw = image_raw;
				image_raw = new LLImageRaw(image_raw->getWidth(),
										   image_raw->getHeight(),
										   4);

				image_raw->copyUnscaledAlphaMask(alpha_image_raw, LLColor4U::black);
			}
			tex->createGLTexture(0, image_raw, 0, TRUE, LLGLTexture::LOCAL);

			gGL.getTexUnit(0)->bind(tex);
			tex->setAddressMode(LLTexUnit::TAM_CLAMP);

			mStaticImageList [ namekey ] = tex;
			mGLBytes += (S32)tex->getWidth() * tex->getHeight() * tex->getComponents();
		}
		else
		{
			tex = NULL;
		}
	}

	return tex;
}

// Reads a .tga file, decodes it, and puts the decoded data in image_raw.
// Returns TRUE if successful.
BOOL LLTexLayerStaticImageList::loadImageRaw(const std::string& file_name, LLImageRaw* image_raw)
{
    LL_PROFILE_ZONE_SCOPED;
	BOOL success = FALSE;
	std::string path;
	path = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,file_name);
	LLPointer<LLImageTGA> image_tga = new LLImageTGA( path );
	if( image_tga->getDataSize() > 0 )
	{
		// Copy data from tga to raw.
		success = image_tga->decode( image_raw );
	}

	return success;
}

