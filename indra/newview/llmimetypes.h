/**
 * @file llmimetypes.h
 * @brief Translates a MIME type like "video/quicktime" into a
 * localizable user-friendly string like "QuickTime Movie"
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
