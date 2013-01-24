/** 
 * @file lllocaltextureobject.h
 * @brief LLLocalTextureObject class header file
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

#ifndef LL_LOCALTEXTUREOBJECT_H
#define LL_LOCALTEXTUREOBJECT_H

#include <boost/shared_ptr.hpp>

#include "llpointer.h"
#include "llgltexture.h"

class LLTexLayer;
class LLTexLayerTemplate;
class LLWearable;

// Stores all relevant information for a single texture 
// assumed to have ownership of all objects referred to - 
// will delete objects when being replaced or if object is destroyed.
class LLLocalTextureObject
{
public:
	LLLocalTextureObject();
	LLLocalTextureObject(LLGLTexture* image, const LLUUID& id);
	LLLocalTextureObject(const LLLocalTextureObject& lto);
	~LLLocalTextureObject();

	LLGLTexture* getImage() const;
	LLTexLayer* getTexLayer(U32 index) const;
	LLTexLayer* getTexLayer(const std::string &name);
	U32 		getNumTexLayers() const;
	LLUUID		getID() const;
	S32			getDiscard() const;
	BOOL		getBakedReady() const;

	void setImage(LLGLTexture* new_image);
	BOOL setTexLayer(LLTexLayer *new_tex_layer, U32 index);
	BOOL addTexLayer(LLTexLayer *new_tex_layer, LLWearable *wearable);
	BOOL addTexLayer(LLTexLayerTemplate *new_tex_layer, LLWearable *wearable);
	BOOL removeTexLayer(U32 index);

	void setID(LLUUID new_id);
	void setDiscard(S32 new_discard);
	void setBakedReady(BOOL ready);

protected:

private:

	LLPointer<LLGLTexture>			mImage;
	// NOTE: LLLocalTextureObject should be the exclusive owner of mTexEntry and mTexLayer
	// using shared pointers here only for smart assignment & cleanup
	// do NOT create new shared pointers to these objects, or keep pointers to them around
	typedef std::vector<LLTexLayer*> tex_layer_vec_t;
	tex_layer_vec_t mTexLayers;

	LLUUID			mID;

	BOOL mIsBakedReady;
	S32 mDiscard;
};

 #endif // LL_LOCALTEXTUREOBJECT_H

