/** 
 * @file lllocaltextureobject.h
 * @brief LLLocalTextureObject class header file
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

#ifndef LL_LOCALTEXTUREOBJECT_H
#define LL_LOCALTEXTUREOBJECT_H

#include <boost/shared_ptr.hpp>

class LLViewerImage;
class LLUUID;
class LLTexLayer;
class LLTextureEntry;

// Stores all relevant information for a single texture 
// assumed to have ownership of all objects referred to - 
// will delete objects when being replaced or if object is destroyed.
class LLLocalTextureObject
{
public:
	LLLocalTextureObject();
	LLLocalTextureObject(LLViewerImage *image, LLTextureEntry *entry, LLTexLayer *layer, LLUUID id);
	LLLocalTextureObject(const LLLocalTextureObject &lto);
	~LLLocalTextureObject();

	LLViewerImage* getImage() const;
	LLTextureEntry* getTexEntry() const;
	LLTexLayer* getTexLayer() const;
	LLUUID		getID() const;
	S32			getDiscard() const;
	BOOL		getBakedReady() const;

	void setImage(LLViewerImage* new_image);
	void setTexEntry(LLTextureEntry *new_te);
	void setTexLayer(LLTexLayer *new_tex_layer);
	void setID(LLUUID new_id);
	void setDiscard(S32 new_discard);
	void setBakedReady(BOOL ready);

protected:

private:

	LLPointer<LLViewerImage>  			mImage;
	// NOTE: LLLocalTextureObject should be the exclusive owner of mTexEntry and mTexLayer
	// using shared pointers here only for smart assignment & cleanup
	// do NOT create new shared pointers to these objects, or keep pointers to them around
	boost::shared_ptr<LLTextureEntry> 	mTexEntry;
	boost::shared_ptr<LLTexLayer>	   	mTexLayer;
	LLUUID			mID;

	BOOL mIsBakedReady;
	S32 mDiscard;
};

 #endif // LL_LOCALTEXTUREOBJECT_H

