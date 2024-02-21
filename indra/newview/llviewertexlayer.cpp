/** 
 * @file llviewertexlayer.cpp
 * @brief Viewer texture layer. Used for avatars.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#include "llviewertexlayer.h"

#include "llagent.h"
#include "llimagej2c.h"
#include "llnotificationsutil.h"
#include "llviewerregion.h"
#include "llglslshader.h"
#include "llvoavatarself.h"
#include "pipeline.h"
#include "llviewercontrol.h"

// runway consolidate
extern std::string self_av_string();

//-----------------------------------------------------------------------------
// LLViewerTexLayerSetBuffer
// The composite image that a LLViewerTexLayerSet writes to.  Each LLViewerTexLayerSet has one.
//-----------------------------------------------------------------------------

// static
S32 LLViewerTexLayerSetBuffer::sGLByteCount = 0;

LLViewerTexLayerSetBuffer::LLViewerTexLayerSetBuffer(LLTexLayerSet* const owner, 
										 S32 width, S32 height) :
	// ORDER_LAST => must render these after the hints are created.
	LLTexLayerSetBuffer(owner),
    LLViewerDynamicTexture(width, height, 4, LLViewerDynamicTexture::ORDER_LAST, false),
	mNeedsUpdate(true),
	mNumLowresUpdates(0)
{
	mGLTexturep->setNeedsAlphaAndPickMask(false);

	LLViewerTexLayerSetBuffer::sGLByteCount += getSize();
	mNeedsUpdateTimer.start();
}

LLViewerTexLayerSetBuffer::~LLViewerTexLayerSetBuffer()
{
	LLViewerTexLayerSetBuffer::sGLByteCount -= getSize();
	destroyGLTexture();
	for( S32 order = 0; order < ORDER_COUNT; order++ )
	{
		LLViewerDynamicTexture::sInstances[order].erase(this);  // will fail in all but one case.
	}
}

//virtual 
S8 LLViewerTexLayerSetBuffer::getType() const 
{
	return LLViewerDynamicTexture::LL_TEX_LAYER_SET_BUFFER ;
}

//virtual 
void LLViewerTexLayerSetBuffer::restoreGLTexture() 
{	
	LLViewerDynamicTexture::restoreGLTexture() ;
}

//virtual 
void LLViewerTexLayerSetBuffer::destroyGLTexture() 
{
	LLViewerDynamicTexture::destroyGLTexture() ;
}

// static
void LLViewerTexLayerSetBuffer::dumpTotalByteCount()
{
	LL_INFOS() << "Composite System GL Buffers: " << (LLViewerTexLayerSetBuffer::sGLByteCount/1024) << "KB" << LL_ENDL;
}

void LLViewerTexLayerSetBuffer::requestUpdate()
{
	restartUpdateTimer();
	mNeedsUpdate = true;
	mNumLowresUpdates = 0;
}

void LLViewerTexLayerSetBuffer::restartUpdateTimer()
{
	mNeedsUpdateTimer.reset();
	mNeedsUpdateTimer.start();
}

// virtual
bool LLViewerTexLayerSetBuffer::needsRender()
{
	llassert(mTexLayerSet->getAvatarAppearance() == gAgentAvatarp);
	if (!isAgentAvatarValid()) return false;

	const bool update_now = mNeedsUpdate && isReadyToUpdate();

	// Don't render if we don't want to (or aren't ready to) update.
	if (!update_now)
	{
		return false;
	}

	// Don't render if we're animating our appearance.
	if (gAgentAvatarp->getIsAppearanceAnimating())
	{
		return false;
	}

	// Don't render if we are trying to create a skirt texture but aren't wearing a skirt.
	if (gAgentAvatarp->getBakedTE(getViewerTexLayerSet()) == LLAvatarAppearanceDefines::TEX_SKIRT_BAKED && 
		!gAgentAvatarp->isWearingWearableType(LLWearableType::WT_SKIRT))
	{
		return false;
	}

	// Render if we have at least minimal level of detail for each local texture.
	return getViewerTexLayerSet()->isLocalTextureDataAvailable();
}

// virtual
void LLViewerTexLayerSetBuffer::preRenderTexLayerSet()
{
	LLTexLayerSetBuffer::preRenderTexLayerSet();
	
	// keep depth buffer, we don't need to clear it
	LLViewerDynamicTexture::preRender(false);
}

// virtual
void LLViewerTexLayerSetBuffer::postRenderTexLayerSet(bool success)
{

	LLTexLayerSetBuffer::postRenderTexLayerSet(success);
	LLViewerDynamicTexture::postRender(success);
}

// virtual
void LLViewerTexLayerSetBuffer::midRenderTexLayerSet(bool success)
{
	const bool update_now = mNeedsUpdate && isReadyToUpdate();
	if (update_now)
	{
		doUpdate();
	}

	// *TODO: Old logic does not check success before setGLTextureCreated
	// we have valid texture data now
	mGLTexturep->setGLTextureCreated(true);
}

bool LLViewerTexLayerSetBuffer::isInitialized(void) const
{
	return mGLTexturep.notNull() && mGLTexturep->isGLTextureCreated();
}

bool LLViewerTexLayerSetBuffer::isReadyToUpdate() const
{
	// If we requested an update and have the final LOD ready, then update.
	if (getViewerTexLayerSet()->isLocalTextureDataFinal()) return true;

	// If we haven't done an update yet, then just do one now regardless of state of textures.
	if (mNumLowresUpdates == 0) return true;

	// Update if we've hit a timeout.  Unlike for uploads, we can make this timeout fairly small
	// since render unnecessarily doesn't cost much.
	const U32 texture_timeout = gSavedSettings.getU32("AvatarBakedLocalTextureUpdateTimeout");
	if (texture_timeout != 0)
	{
		// If we hit our timeout and have textures available at even lower resolution, then update.
		const bool is_update_textures_timeout = mNeedsUpdateTimer.getElapsedTimeF32() >= texture_timeout;
		const bool has_lower_lod = getViewerTexLayerSet()->isLocalTextureDataAvailable();
		if (has_lower_lod && is_update_textures_timeout) return true; 
	}

	return false;
}

bool LLViewerTexLayerSetBuffer::requestUpdateImmediate()
{
	mNeedsUpdate = true;
	bool result = false;

	if (needsRender())
	{
		preRender(false);
		result = render();
		postRender(result);
	}

	return result;
}

// Mostly bookkeeping; don't need to actually "do" anything since
// render() will actually do the update.
void LLViewerTexLayerSetBuffer::doUpdate()
{
	LLViewerTexLayerSet* layer_set = getViewerTexLayerSet();
	const bool highest_lod = layer_set->isLocalTextureDataFinal();
	if (highest_lod)
	{
		mNeedsUpdate = false;
	}
	else
	{
		mNumLowresUpdates++;
	}

	restartUpdateTimer();

	// need to switch to using this layerset if this is the first update
	// after getting the lowest LOD
	layer_set->getAvatar()->updateMeshTextures();
	
	// Print out notification that we updated this texture.
	if (gSavedSettings.getBOOL("DebugAvatarRezTime"))
	{
		const bool highest_lod = layer_set->isLocalTextureDataFinal();
		const std::string lod_str = highest_lod ? "HighRes" : "LowRes";
		LLSD args;
		args["EXISTENCE"] = llformat("%d",(U32)layer_set->getAvatar()->debugGetExistenceTimeElapsedF32());
		args["TIME"] = llformat("%d",(U32)mNeedsUpdateTimer.getElapsedTimeF32());
		args["BODYREGION"] = layer_set->getBodyRegionName();
		args["RESOLUTION"] = lod_str;
		LLNotificationsUtil::add("AvatarRezSelfBakedTextureUpdateNotification",args);
		LL_DEBUGS("Avatar") << self_av_string() << "Locally updating [ name: " << layer_set->getBodyRegionName() << " res:" << lod_str << " time:" << (U32)mNeedsUpdateTimer.getElapsedTimeF32() << " ]" << LL_ENDL;
	}
}

//-----------------------------------------------------------------------------
// LLViewerTexLayerSet
// An ordered set of texture layers that get composited into a single texture.
//-----------------------------------------------------------------------------

LLViewerTexLayerSet::LLViewerTexLayerSet(LLAvatarAppearance* const appearance) :
	LLTexLayerSet(appearance),
	mUpdatesEnabled( false )
{
}

// virtual
LLViewerTexLayerSet::~LLViewerTexLayerSet()
{
}

// Returns true if at least one packet of data has been received for each of the textures that this layerset depends on.
bool LLViewerTexLayerSet::isLocalTextureDataAvailable() const
{
	if (!mAvatarAppearance->isSelf()) return false;
	return getAvatar()->isLocalTextureDataAvailable(this);
}


// Returns true if all of the data for the textures that this layerset depends on have arrived.
bool LLViewerTexLayerSet::isLocalTextureDataFinal() const
{
	if (!mAvatarAppearance->isSelf()) return false;
	return getAvatar()->isLocalTextureDataFinal(this);
}

// virtual
void LLViewerTexLayerSet::requestUpdate()
{
	if( mUpdatesEnabled )
	{
		createComposite();
		getViewerComposite()->requestUpdate(); 
	}
}

void LLViewerTexLayerSet::updateComposite()
{
	createComposite();
	getViewerComposite()->requestUpdateImmediate();
}

// virtual
void LLViewerTexLayerSet::createComposite()
{
	if(!mComposite)
	{
		S32 width = mInfo->getWidth();
		S32 height = mInfo->getHeight();
		// Composite other avatars at reduced resolution
		if( !mAvatarAppearance->isSelf() )
		{
			LL_ERRS() << "composites should not be created for non-self avatars!" << LL_ENDL;
		}
		mComposite = new LLViewerTexLayerSetBuffer( this, width, height );
	}
}

void LLViewerTexLayerSet::setUpdatesEnabled( bool b )
{
	mUpdatesEnabled = b; 
}

LLVOAvatarSelf* LLViewerTexLayerSet::getAvatar()
{
	return dynamic_cast<LLVOAvatarSelf*> (mAvatarAppearance);
}

const LLVOAvatarSelf* LLViewerTexLayerSet::getAvatar() const
{
	return dynamic_cast<const LLVOAvatarSelf*> (mAvatarAppearance);
}

LLViewerTexLayerSetBuffer* LLViewerTexLayerSet::getViewerComposite()
{
	return dynamic_cast<LLViewerTexLayerSetBuffer*> (getComposite());
}

const LLViewerTexLayerSetBuffer* LLViewerTexLayerSet::getViewerComposite() const
{
	return dynamic_cast<const LLViewerTexLayerSetBuffer*> (getComposite());
}


const std::string LLViewerTexLayerSetBuffer::dumpTextureInfo() const
{
	if (!isAgentAvatarValid()) return "";

	const bool is_high_res = true; 
	const U32 num_low_res = 0;
	const std::string local_texture_info = gAgentAvatarp->debugDumpLocalTextureDataInfo(getViewerTexLayerSet());

	std::string text = llformat("[HiRes:%d LoRes:%d] %s",
								is_high_res, num_low_res,
								local_texture_info.c_str());
	return text;
}
