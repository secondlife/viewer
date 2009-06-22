/** 
 * @file lltexglobalcolor.h
 * @brief This is global texture color info used by llvoavatar.
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLTEXGLOBALCOLOR_H
#define LL_LLTEXGLOBALCOLOR_H

#include "lltexlayer.h"
#include "lltexlayerparams.h"

class LLVOAvatar;
class LLTexGlobalColorInfo;

class LLTexGlobalColor
{
public:
	LLTexGlobalColor( LLVOAvatar* avatar );
	~LLTexGlobalColor();

	LLTexGlobalColorInfo*	getInfo() const { return mInfo; }
	//   This sets mInfo and calls initialization functions
	BOOL					setInfo(LLTexGlobalColorInfo *info);
	
	LLVOAvatar*				getAvatar()	const			   	{ return mAvatar; }
	LLColor4				getColor() const;
	const std::string&		getName() const;

private:
	param_color_list_t		mParamGlobalColorList;
	LLVOAvatar*				mAvatar;  // just backlink, don't LLPointer 
	LLTexGlobalColorInfo	*mInfo;
};

// Used by llvoavatar to determine skin/eye/hair color.
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
protected:
	/*virtual*/ void onGlobalColorChanged(bool set_by_user);
private:
	LLTexGlobalColor*		mTexGlobalColor;
};

#endif
