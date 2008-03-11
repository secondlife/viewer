/**
 * @file llmimetypes.cpp
 * @brief Translates a MIME type like "video/quicktime" into a
 * localizable user-friendly string like "QuickTime Movie"
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 *
 * Copyright (c) 2007-2008, Linden Research, Inc.
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

#include "llmimetypes.h"

#include "llvieweruictrlfactory.h"

LLMIMETypes::mime_info_map_t LLMIMETypes::sMap;
LLMIMETypes::mime_widget_set_map_t LLMIMETypes::sWidgetMap;

LLString sDefaultLabel;
	// Returned when we don't know what to do with the mime type
LLString sDefaultWidgetType;
	// Returned when we don't know what widget set to use
LLString sDefaultImpl;
	// Returned when we don't know what impl to use

/////////////////////////////////////////////////////////////////////////////

// static
bool LLMIMETypes::parseMIMETypes(const LLString& xml_filename)
{
	LLXMLNodePtr root;
	bool success = LLViewerUICtrlFactory::getLayeredXMLNode(xml_filename, root);
	if ( ! success || root.isNull() || ! root->hasName( "mimetypes" ) )
	{
		llwarns << "Unable to read MIME type file: "
			<< xml_filename << llendl;
		return false;
	}

	for (LLXMLNode* node = root->getFirstChild();
		 node != NULL;
		 node = node->getNextSibling())
	{
		if (node->hasName("defaultlabel"))
		{
			sDefaultLabel = node->getTextContents();
		}
		else if (node->hasName("defaultwidget"))
		{
			sDefaultWidgetType = node->getTextContents();
		}
		else if (node->hasName("defaultimpl"))
		{
			sDefaultImpl = node->getTextContents();
		}
		else if (node->hasName("mimetype") || node->hasName("scheme"))
		{
			LLString mime_type;
			node->getAttributeString("name", mime_type);
			LLMIMEInfo info;
			for (LLXMLNode* child = node->getFirstChild();
				 child != NULL;
				 child = child->getNextSibling())
			{
				if (child->hasName("label"))
				{
					info.mLabel = child->getTextContents();
				}
				else if (child->hasName("widgettype"))
				{
					info.mWidgetType = child->getTextContents();
				}
				else if (child->hasName("impl"))
				{
					info.mImpl = child->getTextContents();
				}
			}
			sMap[mime_type] = info;
		}
		else if (node->hasName("widgetset"))
		{
			LLString set_name;
			node->getAttributeString("name", set_name);
			LLMIMEWidgetSet info;
			for (LLXMLNode* child = node->getFirstChild();
				child != NULL;
				child = child->getNextSibling())
			{
				if (child->hasName("label"))
				{
					info.mLabel = child->getTextContents();
				}
				if (child->hasName("icon"))
				{
					info.mIcon = child->getTextContents();
				}
				if (child->hasName("default_type"))
				{
					info.mDefaultMimeType = child->getTextContents();
				}
				if (child->hasName("tooltip"))
				{
					info.mToolTip = child->getTextContents();
				}
				if (child->hasName("playtip"))
				{
					info.mPlayTip = child->getTextContents();
				}
				if (child->hasName("allow_resize"))
				{
					child->getBoolValue( 1, (BOOL*)&( info.mAllowResize ) );
				}
				if (child->hasName("allow_looping"))
				{
					child->getBoolValue( 1, (BOOL*)&( info.mAllowLooping ) );
				}
			}
			sWidgetMap[set_name] = info;
		}
	}
	return true;
}

// static
LLString LLMIMETypes::translate(const LLString& mime_type)
{
	mime_info_map_t::const_iterator it = sMap.find(mime_type);
	if (it != sMap.end())
	{
		return it->second.mLabel;
	}
	else
	{
		return sDefaultLabel;
	}
}

// static
LLString LLMIMETypes::widgetType(const LLString& mime_type)
{
	mime_info_map_t::const_iterator it = sMap.find(mime_type);
	if (it != sMap.end())
	{
		return it->second.mWidgetType;
	}
	else
	{
		return sDefaultWidgetType;
	}
}

// static
LLString LLMIMETypes::implType(const LLString& mime_type)
{
	mime_info_map_t::const_iterator it = sMap.find(mime_type);
	if (it != sMap.end())
	{
		return it->second.mImpl;
	}
	else
	{
		return sDefaultImpl;
	}
}

// static
LLString LLMIMETypes::findIcon(const LLString& mime_type)
{
	LLString icon = "";
	LLString widget_type = LLMIMETypes::widgetType(mime_type);
	mime_widget_set_map_t::iterator it = sWidgetMap.find(widget_type);
	if(it != sWidgetMap.end())
	{
		icon = it->second.mIcon;
	}
	return icon;
}

// static
LLString LLMIMETypes::findDefaultMimeType(const LLString& widget_type)
{
	LLString mime_type = "none/none";
	mime_widget_set_map_t::iterator it = sWidgetMap.find(widget_type);
	if(it != sWidgetMap.end())
	{
		mime_type = it->second.mDefaultMimeType;
	}
	return mime_type;
}

// static
LLString LLMIMETypes::findToolTip(const LLString& mime_type)
{
	LLString tool_tip = "";
	LLString widget_type = LLMIMETypes::widgetType(mime_type);
	mime_widget_set_map_t::iterator it = sWidgetMap.find(widget_type);
	if(it != sWidgetMap.end())
	{
		tool_tip = it->second.mToolTip;
	}
	return tool_tip;
}

// static
LLString LLMIMETypes::findPlayTip(const LLString& mime_type)
{
	LLString play_tip = "";
	LLString widget_type = LLMIMETypes::widgetType(mime_type);
	mime_widget_set_map_t::iterator it = sWidgetMap.find(widget_type);
	if(it != sWidgetMap.end())
	{
		play_tip = it->second.mPlayTip;
	}
	return play_tip;
}

// static
bool LLMIMETypes::findAllowResize(const LLString& mime_type)
{
	bool allow_resize = false;
	LLString widget_type = LLMIMETypes::widgetType(mime_type);
	mime_widget_set_map_t::iterator it = sWidgetMap.find(widget_type);
	if(it != sWidgetMap.end())
	{
		allow_resize = it->second.mAllowResize;
	}
	return allow_resize;
}

// static
bool LLMIMETypes::findAllowLooping(const LLString& mime_type)
{
	bool allow_looping = false;
	LLString widget_type = LLMIMETypes::widgetType(mime_type);
	mime_widget_set_map_t::iterator it = sWidgetMap.find(widget_type);
	if(it != sWidgetMap.end())
	{
		allow_looping = it->second.mAllowLooping;
	}
	return allow_looping;
}
