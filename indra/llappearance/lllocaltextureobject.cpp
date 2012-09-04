/** 
 * @file lllocaltextureobject.cpp
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

#include "linden_common.h"

#include "lllocaltextureobject.h"

#include "llimage.h"
#include "llrender.h"
#include "lltexlayer.h"
#include "llgltexture.h"
#include "lluuid.h"
#include "llwearable.h"


LLLocalTextureObject::LLLocalTextureObject() :
	mIsBakedReady(FALSE),
	mDiscard(MAX_DISCARD_LEVEL+1)
{
	mImage = NULL;
}

LLLocalTextureObject::LLLocalTextureObject(LLGLTexture* image, const LLUUID& id) :
	mIsBakedReady(FALSE),
	mDiscard(MAX_DISCARD_LEVEL+1)
{
	mImage = image;
	gGL.getTexUnit(0)->bind(mImage);
	mID = id;
}

LLLocalTextureObject::LLLocalTextureObject(const LLLocalTextureObject& lto) :
	mImage(lto.mImage),
	mID(lto.mID),
	mIsBakedReady(lto.mIsBakedReady),
	mDiscard(lto.mDiscard)
{
	U32 num_layers = lto.getNumTexLayers();
	mTexLayers.reserve(num_layers);
	for (U32 index = 0; index < num_layers; index++)
	{
		LLTexLayer* original_layer = lto.getTexLayer(index);
		if (!original_layer)
		{
			llerrs << "could not clone Local Texture Object: unable to extract texlayer!" << llendl;
			continue;
		}

		LLTexLayer* new_layer = new LLTexLayer(*original_layer);
		new_layer->setLTO(this);
		mTexLayers.push_back(new_layer);
	}
}

LLLocalTextureObject::~LLLocalTextureObject()
{
}

LLGLTexture* LLLocalTextureObject::getImage() const
{
	return mImage;
}

LLTexLayer* LLLocalTextureObject::getTexLayer(U32 index) const
{
	if (index >= getNumTexLayers())
	{
		return NULL;
	}

	return mTexLayers[index];
}

LLTexLayer* LLLocalTextureObject::getTexLayer(const std::string &name)
{
	for( tex_layer_vec_t::iterator iter = mTexLayers.begin(); iter != mTexLayers.end(); iter++)
	{
		LLTexLayer *layer = *iter;
		if (layer->getName().compare(name) == 0)
		{
			return layer;
		}
	}

	return NULL;
}

U32 LLLocalTextureObject::getNumTexLayers() const
{
	return mTexLayers.size();
}

LLUUID LLLocalTextureObject::getID() const
{
	return mID;
}

S32 LLLocalTextureObject::getDiscard() const
{
	return mDiscard;
}

BOOL LLLocalTextureObject::getBakedReady() const
{
	return mIsBakedReady;
}

void LLLocalTextureObject::setImage(LLGLTexture* new_image)
{
	mImage = new_image;
}

BOOL LLLocalTextureObject::setTexLayer(LLTexLayer *new_tex_layer, U32 index)
{
	if (index >= getNumTexLayers() )
	{
		return FALSE;
	}

	if (new_tex_layer == NULL)
	{
		return removeTexLayer(index);
	}

	LLTexLayer *layer = new LLTexLayer(*new_tex_layer);
	layer->setLTO(this);

	if (mTexLayers[index])
	{
		delete mTexLayers[index];
	}
	mTexLayers[index] = layer;

	return TRUE;
}

BOOL LLLocalTextureObject::addTexLayer(LLTexLayer *new_tex_layer, LLWearable *wearable)
{
	if (new_tex_layer == NULL)
	{
		return FALSE;
	}

	LLTexLayer *layer = new LLTexLayer(*new_tex_layer, wearable);
	layer->setLTO(this);
	mTexLayers.push_back(layer);
	return TRUE;
}

BOOL LLLocalTextureObject::addTexLayer(LLTexLayerTemplate *new_tex_layer, LLWearable *wearable)
{
	if (new_tex_layer == NULL)
	{
		return FALSE;
	}

	LLTexLayer *layer = new LLTexLayer(*new_tex_layer, this, wearable);
	layer->setLTO(this);
	mTexLayers.push_back(layer);
	return TRUE;
}

BOOL LLLocalTextureObject::removeTexLayer(U32 index)
{
	if (index >= getNumTexLayers())
	{
		return FALSE;
	}
	tex_layer_vec_t::iterator iter = mTexLayers.begin();
	iter += index;

	delete *iter;
	mTexLayers.erase(iter);
	return TRUE;
}

void LLLocalTextureObject::setID(LLUUID new_id)
{
	mID = new_id;
}

void LLLocalTextureObject::setDiscard(S32 new_discard)
{
	mDiscard = new_discard;
}

void LLLocalTextureObject::setBakedReady(BOOL ready)
{
	mIsBakedReady = ready;
}

