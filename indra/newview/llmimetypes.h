/**
 * @file llmimetypes.h
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

#ifndef LLMIMETYPES_H
#define LLMIMETYPES_H

#include <string>
#include <map>

class LLMIMETypes
{
public:
    static bool parseMIMETypes(const std::string& xml_file_path);
        // Loads the MIME string definition XML file, usually
        // from the application skins directory

    static std::string translate(const std::string& mime_type);
        // Returns "QuickTime Movie" from "video/quicktime"

    static std::string widgetType(const std::string& mime_type);
        // Type of control widgets for this MIME type
        // Returns "movie" from "video/quicktime"

    static std::string implType(const std::string& mime_type);
        // Type of Impl to use for decoding media.

    static std::string findIcon(const std::string& mime_type);
        // Icon from control widget type for this MIME type

    static std::string findToolTip(const std::string& mime_type);
        // Tool tip from control widget type for this MIME type

    static std::string findPlayTip(const std::string& mime_type);
        // Play button tool tip from control widget type for this MIME type

    static std::string findDefaultMimeType(const std::string& widget_type);
        // Canonical mime type associated with this widget set

    static const std::string& getDefaultMimeType();

    static const std::string& getDefaultMimeTypeTranslation();

    static bool findAllowResize(const std::string& mime_type);
        // accessor for flag to enable/disable media size edit fields

    static bool findAllowLooping(const std::string& mime_type);
        // accessor for flag to enable/disable media looping checkbox

    static bool isTypeHandled(const std::string& mime_type);
        // determines if the specific mime type is handled by the media system

    static void reload(void*);
        // re-loads the MIME types file from the file path last passed into parseMIMETypes

public:
    struct LLMIMEInfo
    {
        std::string mLabel;
            // friendly label like "QuickTime Movie"

        std::string mWidgetType;
            // "web" means use web media UI widgets

        std::string mImpl;
            // which impl to use with this mime type
    };
    struct LLMIMEWidgetSet
    {
        std::string mLabel;
            // friendly label like "QuickTime Movie"

        std::string mIcon;
            // Name of icon asset to display in toolbar

        std::string mDefaultMimeType;
            // Mime type string to use in absence of a specific one

        std::string mToolTip;
            // custom tool tip for this mime type

        std::string mPlayTip;
            // custom tool tip to display for Play button

        BOOL mAllowResize;
            // enable/disable media size edit fields

        BOOL mAllowLooping;
            // enable/disable media looping checkbox
    };
    typedef std::map< std::string, LLMIMEInfo > mime_info_map_t;
    typedef std::map< std::string, LLMIMEWidgetSet > mime_widget_set_map_t;

    // Public so users can iterate over it
    static mime_info_map_t sMap;
    static mime_widget_set_map_t sWidgetMap;
private:
};

#endif
