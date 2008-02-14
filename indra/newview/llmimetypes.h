/** 
 * @file llmimetypes.h
 * @author James Cook
 * @brief Translates a MIME type like "video/quicktime" into a
 * localizable user-friendly string like "QuickTime Movie"
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
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

	static bool LLMIMETypes::findAllowLooping(const LLString& mime_type);
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
