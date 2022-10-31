/**
 * @file llmimetypes.cpp
 * @brief Translates a MIME type like "video/quicktime" into a
 * localizable user-friendly string like "QuickTime Movie"
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llmimetypes.h"
#include "lltrans.h"
#include "llxmlnode.h"

#include "lluictrlfactory.h"

LLMIMETypes::mime_info_map_t LLMIMETypes::sMap;
LLMIMETypes::mime_widget_set_map_t LLMIMETypes::sWidgetMap;

std::string sDefaultLabel;
    // Returned when we don't know what to do with the mime type
std::string sDefaultWidgetType;
    // Returned when we don't know what widget set to use
std::string sDefaultImpl;
    // Returned when we don't know what impl to use
std::string sXMLFilename; 
    // Squirrel away XML filename so we know how to reset
std::string DEFAULT_MIME_TYPE = "none/none";

/////////////////////////////////////////////////////////////////////////////

// static
bool LLMIMETypes::parseMIMETypes(const std::string& xml_filename)
{
    LLXMLNodePtr root;
    bool success = LLUICtrlFactory::getLayeredXMLNode(xml_filename, root);
    if ( ! success || root.isNull() || ! root->hasName( "mimetypes" ) )
    {
        LL_WARNS() << "Unable to read MIME type file: "
            << xml_filename << LL_ENDL;
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
            std::string mime_type;
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
            std::string set_name;
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
                    BOOL allow_resize = FALSE;
                    child->getBoolValue( 1, &allow_resize );
                    info.mAllowResize = (bool)allow_resize;
                }
                if (child->hasName("allow_looping"))
                {
                    BOOL allow_looping = FALSE;
                    child->getBoolValue( 1, &allow_looping );
                    info.mAllowLooping = (bool)allow_looping;
                }
            }
            sWidgetMap[set_name] = info;
        }
    }

    sXMLFilename = xml_filename;
    return true;
}

// static
std::string LLMIMETypes::translate(const std::string& mime_type)
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
std::string LLMIMETypes::widgetType(const std::string& mime_type)
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
std::string LLMIMETypes::implType(const std::string& mime_type)
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
std::string LLMIMETypes::findIcon(const std::string& mime_type)
{
    std::string icon = "";
    std::string widget_type = LLMIMETypes::widgetType(mime_type);
    mime_widget_set_map_t::iterator it = sWidgetMap.find(widget_type);
    if(it != sWidgetMap.end())
    {
        icon = it->second.mIcon;
    }
    return icon;
}

// static
std::string LLMIMETypes::findDefaultMimeType(const std::string& widget_type)
{
    std::string mime_type = getDefaultMimeType();
    mime_widget_set_map_t::iterator it = sWidgetMap.find(widget_type);
    if(it != sWidgetMap.end())
    {
        mime_type = it->second.mDefaultMimeType;
    }
    return mime_type;
}

// static
const std::string& LLMIMETypes::getDefaultMimeType()
{
    return DEFAULT_MIME_TYPE;
}

const std::string& LLMIMETypes::getDefaultMimeTypeTranslation()
{
    static std::string mime_type = LLTrans::getString("DefaultMimeType");
    return mime_type;
}

// static
std::string LLMIMETypes::findToolTip(const std::string& mime_type)
{
    std::string tool_tip = "";
    std::string widget_type = LLMIMETypes::widgetType(mime_type);
    mime_widget_set_map_t::iterator it = sWidgetMap.find(widget_type);
    if(it != sWidgetMap.end())
    {
        tool_tip = it->second.mToolTip;
    }
    return tool_tip;
}

// static
std::string LLMIMETypes::findPlayTip(const std::string& mime_type)
{
    std::string play_tip = "";
    std::string widget_type = LLMIMETypes::widgetType(mime_type);
    mime_widget_set_map_t::iterator it = sWidgetMap.find(widget_type);
    if(it != sWidgetMap.end())
    {
        play_tip = it->second.mPlayTip;
    }
    return play_tip;
}

// static
bool LLMIMETypes::findAllowResize(const std::string& mime_type)
{
    bool allow_resize = false;
    std::string widget_type = LLMIMETypes::widgetType(mime_type);
    mime_widget_set_map_t::iterator it = sWidgetMap.find(widget_type);
    if(it != sWidgetMap.end())
    {
        allow_resize = it->second.mAllowResize;
    }
    return allow_resize;
}

// static
bool LLMIMETypes::findAllowLooping(const std::string& mime_type)
{
    bool allow_looping = false;
    std::string widget_type = LLMIMETypes::widgetType(mime_type);
    mime_widget_set_map_t::iterator it = sWidgetMap.find(widget_type);
    if(it != sWidgetMap.end())
    {
        allow_looping = it->second.mAllowLooping;
    }
    return allow_looping;
}

// static
bool LLMIMETypes::isTypeHandled(const std::string& mime_type)
{
    mime_info_map_t::const_iterator it = sMap.find(mime_type);
    if (it != sMap.end())
    {
        return true;
    }
    return false;
}

// static
void LLMIMETypes::reload(void*)
{
    sMap.clear();
    sWidgetMap.clear();
    (void)LLMIMETypes::parseMIMETypes(sXMLFilename);
}

