/** 
 * @file lltexglobalcolor.h
 * @brief This is global texture color info used by llavatarappearance.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#ifndef LL_LLTEXGLOBALCOLOR_H
#define LL_LLTEXGLOBALCOLOR_H

#include "lltexlayer.h"
#include "lltexlayerparams.h"

class LLAvatarAppearance;
class LLWearable;
class LLTexGlobalColorInfo;

class LLTexGlobalColor
{
public:
	LLTexGlobalColor( LLAvatarAppearance* appearance );
	~LLTexGlobalColor();

	LLTexGlobalColorInfo*	getInfo() const { return mInfo; }
	//   This sets mInfo and calls initialization functions
	BOOL					setInfo(LLTexGlobalColorInfo *info);
	
	LLAvatarAppearance*		getAvatarAppearance()	const	   	{ return mAvatarAppearance; }
	LLColor4				getColor() const;
	const std::string&		getName() const;

private:
	param_color_list_t		mParamGlobalColorList;
	LLAvatarAppearance*		mAvatarAppearance;  // just backlink, don't LLPointer 
	LLTexGlobalColorInfo	*mInfo;
};

// Used by llavatarappearance to determine skin/eye/hair color.
class LLTexGlobalColorInfo
{
	friend class LLTexGlobalColor;
public:
	LLTexGlobalColorInfo();
	~LLTexGlobalColorInfo();

	BOOL parseXml(LLXmlTreeNode* node);

private:
	param_color_info_list_t		mParamColorInfoList;
	std::string				mName;
};

class LLTexParamGlobalColor : public LLTexLayerParamColor
{
public:
	LLTexParamGlobalColor(LLTexGlobalColor *tex_color);
	/*virtual*/ LLViewerVisualParam* cloneParam(LLWearable* wearable) const;
protected:
	/*virtual*/ void onGlobalColorChanged(bool upload_bake);
private:
	LLTexGlobalColor*		mTexGlobalColor;
};

#endif
