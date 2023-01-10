/** 
 * @file lltexlayerglobalcolor.cpp
 * @brief Color for texture layers.
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

#include "linden_common.h"
#include "llavatarappearance.h"
#include "lltexlayer.h"
#include "lltexglobalcolor.h"

class LLWearable;

//-----------------------------------------------------------------------------
// LLTexGlobalColor
//-----------------------------------------------------------------------------

LLTexGlobalColor::LLTexGlobalColor(LLAvatarAppearance* appearance)
	:
	mAvatarAppearance(appearance),
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
	for (LLTexLayerParamColorInfo* color_info : mInfo->mParamColorInfoList)
	{
		LLTexParamGlobalColor* param_color = new LLTexParamGlobalColor(this);
		if (!param_color->setInfo(color_info, TRUE))
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
LLTexParamGlobalColor::LLTexParamGlobalColor(LLTexGlobalColor* tex_global_color)
	: LLTexLayerParamColor(tex_global_color->getAvatarAppearance()),
	mTexGlobalColor(tex_global_color)
{
}

//-----------------------------------------------------------------------------
// LLTexParamGlobalColor
//-----------------------------------------------------------------------------
LLTexParamGlobalColor::LLTexParamGlobalColor(const LLTexParamGlobalColor& pOther)
	: LLTexLayerParamColor(pOther),
	mTexGlobalColor(pOther.mTexGlobalColor)
{
}

//-----------------------------------------------------------------------------
// ~LLTexParamGlobalColor
//-----------------------------------------------------------------------------
LLTexParamGlobalColor::~LLTexParamGlobalColor()
{
}

/*virtual*/ LLViewerVisualParam* LLTexParamGlobalColor::cloneParam(LLWearable* wearable) const
{
	return new LLTexParamGlobalColor(*this);
}

void LLTexParamGlobalColor::onGlobalColorChanged()
{
	mAvatarAppearance->onGlobalColorChanged(mTexGlobalColor);
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
	mParamColorInfoList.clear();
}

BOOL LLTexGlobalColorInfo::parseXml(LLXmlTreeNode* node)
{
	// name attribute
	static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
	if (!node->getFastAttributeString(name_string, mName))
	{
		LL_WARNS() << "<global_color> element is missing name attribute." << LL_ENDL;
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
