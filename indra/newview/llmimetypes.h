/**
 * @file llmimetypes.h
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

#ifndef LLMIMETYPES_H
#define LLMIMETYPES_H

#include "llstring.h"	// because XML parsing lib uses LLString, ugh
#include <map>

class LLMIMETypes
{
public:
	static bool parseMIMETypes(const LLString& xml_file_path);
		// Loads the MIME string definition XML file, usually
		// from the application skins directory

	static LLString translate(const LLString& mime_type);
		// Returns "QuickTime Movie" from "video/quicktime"

	static LLString widgetType(const LLString& mime_type);
		// Type of control widgets for this MIME type
		// Returns "movie" from "video/quicktime"

	static LLString implType(const LLString& mime_type);
		// Type of Impl to use for decoding media.

	static LLString findIcon(const LLString& mime_type);
		// Icon from control widget type for this MIME type

	static LLString findToolTip(const LLString& mime_type);
		// Tool tip from control widget type for this MIME type

	static LLString findPlayTip(const LLString& mime_type);
		// Play button tool tip from control widget type for this MIME type

	static LLString findDefaultMimeType(const LLString& widget_type);
		// Canonical mime type associated with this widget set

	static bool findAllowResize(const LLString& mime_type);
		// accessor for flag to enable/disable media size edit fields

	static bool findAllowLooping(const LLString& mime_type);
		// accessor for flag to enable/disable media looping checkbox

public:
	struct LLMIMEInfo
	{
		LLString mLabel;
			// friendly label like "QuickTime Movie"

		LLString mWidgetType;
			// "web" means use web media UI widgets

		LLString mImpl;
			// which impl to use with this mime type
	};
	struct LLMIMEWidgetSet
	{
		LLString mLabel;
			// friendly label like "QuickTime Movie"

		LLString mIcon;
			// Name of icon asset to display in toolbar

		LLString mDefaultMimeType;
			// Mime type string to use in absence of a specific one

		LLString mToolTip;
			// custom tool tip for this mime type

		LLString mPlayTip;
			// custom tool tip to display for Play button

		bool mAllowResize;
			// enable/disable media size edit fields

		bool mAllowLooping;
			// enable/disable media looping checkbox
	};
	typedef std::map< LLString, LLMIMEInfo > mime_info_map_t;
	typedef std::map< LLString, LLMIMEWidgetSet > mime_widget_set_map_t;

	// Public so users can iterate over it
	static mime_info_map_t sMap;
	static mime_widget_set_map_t sWidgetMap;
private:
};

#endif
