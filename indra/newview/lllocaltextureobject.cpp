/** 
 * @file lllocaltextureobject.cpp
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "lllocaltextureobject.h"

#include "lltexlayer.h"
#include "llviewerimage.h"
#include "lltextureentry.h"
#include "lluuid.h"


LLLocalTextureObject::LLLocalTextureObject() :
	mIsBakedReady(FALSE),
	mDiscard(MAX_DISCARD_LEVEL+1)
{
	mImage = NULL;
}

LLLocalTextureObject::LLLocalTextureObject(LLViewerImage *image, LLTextureEntry *entry, LLTexLayer *layer, LLUUID id)
{
	if (entry)
	{
		LLTextureEntry * te = new LLTextureEntry(*entry);
		mTexEntry = boost::shared_ptr<LLTextureEntry>(te);
	}

	if (layer)
	{
		LLTexLayer *texLayer = new LLTexLayer(*layer);
		mTexLayer = boost::shared_ptr<LLTexLayer>(texLayer);
	}
	mImage = image;
	mID = id;
}

LLLocalTextureObject::LLLocalTextureObject(const LLLocalTextureObject &lto) :
mImage(lto.mImage),
mTexEntry(lto.mTexEntry),
mTexLayer(lto.mTexLayer),
mID(lto.mID),
mIsBakedReady(lto.mIsBakedReady),
mDiscard(lto.mDiscard)
{
}

LLLocalTextureObject::~LLLocalTextureObject()
{
}

LLViewerImage* LLLocalTextureObject::getImage() const
{
	return mImage;
}

LLTextureEntry* LLLocalTextureObject::getTexEntry() const
{
	return mTexEntry.get();
}

LLTexLayer* LLLocalTextureObject::getTexLayer() const
{
	return mTexLayer.get();
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

void LLLocalTextureObject::setImage(LLViewerImage* new_image)
{
	mImage = new_image;
}

void LLLocalTextureObject::setTexEntry(LLTextureEntry *new_te)
{
	LLTextureEntry *ptr = NULL;
	if (new_te)
	{
		ptr = new LLTextureEntry(*new_te);
	}
	mTexEntry = boost::shared_ptr<LLTextureEntry>(ptr);
}

void LLLocalTextureObject::setTexLayer(LLTexLayer *new_tex_layer)
{
	LLTexLayer *ptr = NULL;
	if (new_tex_layer)
	{
		ptr = new LLTexLayer(*new_tex_layer);
	}
	mTexLayer = boost::shared_ptr<LLTexLayer>(ptr);
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

