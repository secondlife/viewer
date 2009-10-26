/** 
 * @file lltexlayerglobalcolor.cpp
 * @brief SERAPH - ADD IN
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

#include "llviewerprecompiledheaders.h"
#include "llagent.h"
#include "lltexlayer.h"
#include "llvoavatar.h"
#include "llwearable.h"
#include "lltexglobalcolor.h"

//-----------------------------------------------------------------------------
// LLTexGlobalColor
//-----------------------------------------------------------------------------

LLTexGlobalColor::LLTexGlobalColor(LLVOAvatar* avatar) 
	:
	mAvatar(avatar),
	mInfo(NULL)
{
}

LLTexGlobalColor::~LLTexGlobalColor()
{
	// mParamColorList are LLViewerVisualParam's and get deleted with ~LLCharacter()
	//std::for_each(mParamColorList.begin(), mParamColorList.end(), DeletePointer());
}

BOOL LLTexGlobalColor::setInfo(LLTexGlobalColorInfo *info)
{
	llassert(mInfo == NULL);
	mInfo = info;
	//mID = info->mID; // No ID

	mParamGlobalColorList.reserve(mInfo->mParamColorInfoList.size());
	for (param_color_info_list_t::iterator iter = mInfo->mParamColorInfoList.begin(); 
		 iter != mInfo->mParamColorInfoList.end(); 
		 iter++)
	{
		LLTexParamGlobalColor* param_color = new LLTexParamGlobalColor(this);
		if (!param_color->setInfo(*iter, TRUE))
		{
			mInfo = NULL;
			return FALSE;
		}
		mParamGlobalColorList.push_back(param_color);
	}
	
	return TRUE;
}

LLColor4 LLTexGlobalColor::getColor() const
{
	// Sum of color params
	if (mParamGlobalColorList.empty())
		return LLColor4(1.f, 1.f, 1.f, 1.f);

	LLColor4 net_color(0.f, 0.f, 0.f, 0.f);
	LLTexLayer::calculateTexLayerColor(mParamGlobalColorList, net_color);
	return net_color;
}

const std::string& LLTexGlobalColor::getName() const
{ 
	return mInfo->mName; 
}

//-----------------------------------------------------------------------------
// LLTexParamGlobalColor
//-----------------------------------------------------------------------------
LLTexParamGlobalColor::LLTexParamGlobalColor(LLTexGlobalColor* tex_global_color) :
	LLTexLayerParamColor(tex_global_color->getAvatar()),
	mTexGlobalColor(tex_global_color)
{
}

/*virtual*/ LLViewerVisualParam* LLTexParamGlobalColor::cloneParam(LLWearable* wearable) const
{
	LLTexParamGlobalColor *new_param = new LLTexParamGlobalColor(mTexGlobalColor);
	*new_param = *this;
	return new_param;
}

void LLTexParamGlobalColor::onGlobalColorChanged(bool set_by_user)
{
	mAvatar->onGlobalColorChanged(mTexGlobalColor, set_by_user);
}

//-----------------------------------------------------------------------------
// LLTexGlobalColorInfo
//-----------------------------------------------------------------------------

LLTexGlobalColorInfo::LLTexGlobalColorInfo()
{
}


LLTexGlobalColorInfo::~LLTexGlobalColorInfo()
{
	for_each(mParamColorInfoList.begin(), mParamColorInfoList.end(), DeletePointer());
}

BOOL LLTexGlobalColorInfo::parseXml(LLXmlTreeNode* node)
{
	// name attribute
	static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
	if (!node->getFastAttributeString(name_string, mName))
	{
		llwarns << "<global_color> element is missing name attribute." << llendl;
		return FALSE;
	}
	// <param> sub-element
	for (LLXmlTreeNode* child = node->getChildByName("param");
		 child;
		 child = node->getNextNamedChild())
	{
		if (child->getChildByName("param_color"))
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
	}
	return TRUE;
}
