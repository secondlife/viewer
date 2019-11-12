/** 
 * @file llviewertexturelist.cpp
 * @brief Object for managing the list of images within a region
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include <sys/stat.h>

#include "llviewertexturelist.h"

#include "llgl.h" // fot gathering stats from GL
#include "llimagegl.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "llimagejpeg.h"
#include "llimagepng.h"

#include "llsdserialize.h"
#include "llsys.h"
#include "llvfs.h"
#include "llvfile.h"
#include "llvfsthread.h"
#include "llxmltree.h"
#include "message.h"

#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llviewermedia.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "pipeline.h"
#include "llappviewer.h"
#include "llxuiparser.h"
#include "lltracerecording.h"
#include "llviewerdisplay.h"
#include "llviewerwindow.h"
#include "llprogressview.h"
#include "llviewertexturemanager.h"

////////////////////////////////////////////////////////////////////////////


static LLTrace::BlockTimerStatHandle FTM_PROCESS_IMAGES("Process Images");

///////////////////////////////////////////////////////////////////////////////
void LLUIImageList::initSingleton()
{
    /*TODO*/ // can call initFromFile here?
}

void LLUIImageList::cleanupSingleton()
{
    cleanUp();
}


// explicitly cleanup resources, as this is a singleton class with process
// lifetime so ability to perform std::map operations in destructor is not
// guaranteed.
void LLUIImageList::cleanUp()
{
	mUIImages.clear();
	mUITextureList.clear() ;
}

LLUIImagePtr LLUIImageList::getUIImageByID(const LLUUID& image_id, S32 priority)
{
	// use id as image name
	std::string image_name = image_id.asString();

	// look for existing image
	uuid_ui_image_map_t::iterator found_it = mUIImages.find(image_name);
	if (found_it != mUIImages.end())
	{
		return found_it->second;
	}

	const BOOL use_mips = FALSE;
	const LLRect scale_rect = LLRect::null;
	const LLRect clip_rect = LLRect::null;
	return loadUIImageByID(image_id, use_mips, scale_rect, clip_rect, (LLViewerTexture::EBoostLevel)priority);
}

LLUIImagePtr LLUIImageList::getUIImage(const std::string& image_name, S32 priority)
{
	// look for existing image
	uuid_ui_image_map_t::iterator found_it = mUIImages.find(image_name);
	if (found_it != mUIImages.end())
	{
		return found_it->second;
	}

	const BOOL use_mips = FALSE;
	const LLRect scale_rect = LLRect::null;
	const LLRect clip_rect = LLRect::null;
	return loadUIImageByName(image_name, image_name, use_mips, scale_rect, clip_rect, (LLViewerTexture::EBoostLevel)priority);
}

LLUIImagePtr LLUIImageList::loadUIImageByName(const std::string& name, const std::string& filename,
											  BOOL use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLViewerTexture::EBoostLevel boost_priority,
											  LLUIImage::EScaleStyle scale_style)
{
    LLViewerTextureManager::FetchParams params;
    if (boost_priority != LLGLTexture::BOOST_NONE)
        params.mBoostPriority = boost_priority;

	LLViewerFetchedTexture::ptr_t imagep = LLViewerTextureManager::instance().getFetchedTextureFromSkin(filename, params);
	return loadUIImage(imagep, name, use_mips, scale_rect, clip_rect, scale_style);
}

LLUIImagePtr LLUIImageList::loadUIImageByID(const LLUUID& id,
											BOOL use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLViewerTexture::EBoostLevel boost_priority,
											LLUIImage::EScaleStyle scale_style)
{
    LLViewerTextureManager::FetchParams params;
    if (boost_priority != LLGLTexture::BOOST_NONE)
        params.mBoostPriority = boost_priority;

    LLViewerFetchedTexture::ptr_t imagep = LLViewerTextureManager::instance().getFetchedTexture(id, params);
	return loadUIImage(imagep, id.asString(), use_mips, scale_rect, clip_rect, scale_style);
}

LLUIImagePtr LLUIImageList::loadUIImage(const LLViewerFetchedTexture::ptr_t &imagep, const std::string& name, BOOL use_mips, const LLRect& scale_rect, const LLRect& clip_rect,
										LLUIImage::EScaleStyle scale_style)
{
	if (!imagep) return NULL;

	imagep->setAddressMode(LLTexUnit::TAM_CLAMP);

	//don't compress UI images
	imagep->getGLTexture()->setAllowCompression(false);

	LLUIImagePtr new_imagep = new LLUIImage(name, imagep);
	new_imagep->setScaleStyle(scale_style);

	if (imagep->getBoostLevel() != LLGLTexture::BOOST_ICON &&
		imagep->getBoostLevel() != LLGLTexture::BOOST_PREVIEW)
	{
		// Don't add downloadable content into this list
		// all UI images are non-deletable and list does not support deletion
		imagep->setNoDelete();
		mUIImages.insert(std::make_pair(name, new_imagep));
		mUITextureList.push_back(imagep);
	}

	//Note:
	//Some other textures such as ICON also through this flow to be fetched.
	//But only UI textures need to set this callback.
	if(imagep->getBoostLevel() == LLGLTexture::BOOST_UI)
	{
		LLUIImageLoadData::ptr_t datap(std::make_shared<LLUIImageLoadData>());

		datap->mImageName = name;
		datap->mImageScaleRegion = scale_rect;
		datap->mImageClipRegion = clip_rect;

        datap->mConnection = imagep->addCallback([datap](bool success, LLViewerFetchedTexture::ptr_t &src_vi, bool final_done){
            onUIImageLoaded(success, src_vi, final_done, datap);
        });
	}
	return new_imagep;
}

LLUIImagePtr LLUIImageList::preloadUIImage(const std::string& name, const std::string& filename, BOOL use_mips, const LLRect& scale_rect, const LLRect& clip_rect, LLUIImage::EScaleStyle scale_style)
{
	// look for existing image
	uuid_ui_image_map_t::iterator found_it = mUIImages.find(name);
	if (found_it != mUIImages.end())
	{
		// image already loaded!
		LL_ERRS() << "UI Image " << name << " already loaded." << LL_ENDL;
	}

	return loadUIImageByName(name, filename, use_mips, scale_rect, clip_rect, LLGLTexture::BOOST_UI, scale_style);
}

//static 
void LLUIImageList::onUIImageLoaded(bool success, LLViewerFetchedTexture::ptr_t &src_vi, bool final_done, const LLUIImageLoadData::ptr_t &image_datap)
{
	if(!success) 
	{
        image_datap->mConnection.disconnect();
		return;
	}

	std::string ui_image_name = image_datap->mImageName;
	LLRect scale_rect = image_datap->mImageScaleRegion;
	LLRect clip_rect = image_datap->mImageClipRegion;

	LLUIImageList* instance = getInstance();

	uuid_ui_image_map_t::iterator found_it = instance->mUIImages.find(ui_image_name);
	if (found_it != instance->mUIImages.end())
	{
		LLUIImagePtr imagep = found_it->second;

		// for images grabbed from local files, apply clipping rectangle to restore original dimensions
		// from power-of-2 gl image
		if (success && imagep.notNull() && src_vi && (src_vi->getUrl().compare(0, 7, "file://")==0))
		{
			F32 full_width = (F32)src_vi->getFullWidth();
			F32 full_height = (F32)src_vi->getFullHeight();
			F32 clip_x = (F32)src_vi->getOriginalWidth() / full_width;
			F32 clip_y = (F32)src_vi->getOriginalHeight() / full_height;
			if (clip_rect != LLRect::null)
			{
				imagep->setClipRegion(LLRectf(llclamp((F32)clip_rect.mLeft / full_width, 0.f, 1.f),
											llclamp((F32)clip_rect.mTop / full_height, 0.f, 1.f),
											llclamp((F32)clip_rect.mRight / full_width, 0.f, 1.f),
											llclamp((F32)clip_rect.mBottom / full_height, 0.f, 1.f)));
			}
			else
			{
				imagep->setClipRegion(LLRectf(0.f, clip_y, clip_x, 0.f));
			}
			if (scale_rect != LLRect::null)
			{
				imagep->setScaleRegion(
					LLRectf(llclamp((F32)scale_rect.mLeft / (F32)imagep->getWidth(), 0.f, 1.f),
						llclamp((F32)scale_rect.mTop / (F32)imagep->getHeight(), 0.f, 1.f),
						llclamp((F32)scale_rect.mRight / (F32)imagep->getWidth(), 0.f, 1.f),
						llclamp((F32)scale_rect.mBottom / (F32)imagep->getHeight(), 0.f, 1.f)));
			}

			imagep->onImageLoaded();
		}
	}
    if (final_done)
    {
        image_datap->mConnection.disconnect();
    }
}

namespace LLInitParam
{
	template<>
	struct TypeValues<LLUIImage::EScaleStyle> : public TypeValuesHelper<LLUIImage::EScaleStyle>
	{
		static void declareValues()
		{
			declare("scale_inner",	LLUIImage::SCALE_INNER);
			declare("scale_outer",	LLUIImage::SCALE_OUTER);
		}
	};
}

struct UIImageDeclaration : public LLInitParam::Block<UIImageDeclaration>
{
	Mandatory<std::string>		name;
	Optional<std::string>		file_name;
	Optional<bool>				preload;
	Optional<LLRect>			scale;
	Optional<LLRect>			clip;
	Optional<bool>				use_mips;
	Optional<LLUIImage::EScaleStyle> scale_type;

	UIImageDeclaration()
	:	name("name"),
		file_name("file_name"),
		preload("preload", false),
		scale("scale"),
		clip("clip"),
		use_mips("use_mips", false),
		scale_type("scale_type", LLUIImage::SCALE_INNER)
	{}
};

struct UIImageDeclarations : public LLInitParam::Block<UIImageDeclarations>
{
	Mandatory<S32>	version;
	Multiple<UIImageDeclaration> textures;

	UIImageDeclarations()
	:	version("version"),
		textures("texture")
	{}
};

bool LLUIImageList::initFromFile()
{
	// Look for textures.xml in all the right places. Pass
	// constraint=LLDir::ALL_SKINS because we want to overlay textures.xml
	// from all the skins directories.
	std::vector<std::string> textures_paths =
		gDirUtilp->findSkinnedFilenames(LLDir::TEXTURES, "textures.xml", LLDir::ALL_SKINS);
	std::vector<std::string>::const_iterator pi(textures_paths.begin()), pend(textures_paths.end());
	if (pi == pend)
	{
		LL_WARNS() << "No textures.xml found in skins directories" << LL_ENDL;
		return false;
	}

	// The first (most generic) file gets special validations
	LLXMLNodePtr root;
	if (!LLXMLNode::parseFile(*pi, root, NULL))
	{
		LL_WARNS() << "Unable to parse UI image list file " << *pi << LL_ENDL;
		return false;
	}
	if (!root->hasAttribute("version"))
	{
		LL_WARNS() << "No valid version number in UI image list file " << *pi << LL_ENDL;
		return false;
	}

	UIImageDeclarations images;
	LLXUIParser parser;
	parser.readXUI(root, images, *pi);

	// add components defined in the rest of the skin paths
	while (++pi != pend)
	{
		LLXMLNodePtr update_root;
		if (LLXMLNode::parseFile(*pi, update_root, NULL))
		{
			parser.readXUI(update_root, images, *pi);
		}
	}

	if (!images.validateBlock()) return false;

	std::map<std::string, UIImageDeclaration> merged_declarations;
	for (LLInitParam::ParamIterator<UIImageDeclaration>::const_iterator image_it = images.textures.begin();
		image_it != images.textures.end();
		++image_it)
	{
		merged_declarations[image_it->name].overwriteFrom(*image_it);
	}

	enum e_decode_pass
	{
		PASS_DECODE_NOW,
		PASS_DECODE_LATER,
		NUM_PASSES
	};

	//for (S32 cur_pass = PASS_DECODE_NOW; cur_pass < NUM_PASSES; cur_pass++)
	{
		for (std::map<std::string, UIImageDeclaration>::const_iterator image_it = merged_declarations.begin();
			image_it != merged_declarations.end();
			++image_it)
		{
			const UIImageDeclaration& image = image_it->second;
			std::string file_name = image.file_name.isProvided() ? image.file_name() : image.name();

			// load high priority textures on first pass (to kick off decode)
			/*enum e_decode_pass decode_pass = image.preload ? PASS_DECODE_NOW : PASS_DECODE_LATER;
			if (decode_pass != cur_pass)
			{
				continue;
			}*/
			preloadUIImage(image.name, file_name, image.use_mips, image.scale, image.clip, image.scale_type);
		}

// 		if (/*cur_pass == PASS_DECODE_NOW &&*/ !gSavedSettings.getBOOL("NoPreload"))
// 		{
// 			LLViewerTextureList::instance().decodeAllImages(4.f); // decode preloaded images
// 		}
	}
	return true;
}

