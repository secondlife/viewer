/**
 * @file   llpipelinelistener.h
 * @author Don Kjer
 * @date   2012-07-09
 * @brief  Implementation for LLPipelineListener
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

// Precompiled header
#include "llviewerprecompiledheaders.h"

#include "llpipelinelistener.h"

#include "pipeline.h"
#include "stringize.h"
#include <sstream>
#include "llviewermenu.h"


namespace {
	// Render Types
	void toggle_render_types_wrapper(LLSD const& request)
	{
		for (LLSD::array_const_iterator iter = request["types"].beginArray();
			iter != request["types"].endArray();
			++iter)
		{
			U32 render_type = render_type_from_string( iter->asString() );
			if ( render_type != 0 )
			{
				LLPipeline::toggleRenderTypeControl( (void*) render_type );
			}
		}
	}

	void has_render_type_wrapper(LLSD const& request)
	{
		LLEventAPI::Response response(LLSD(), request);
		U32 render_type = render_type_from_string( request["type"].asString() );
		if ( render_type != 0 )
		{
			response["value"] = LLPipeline::hasRenderTypeControl( (void*) render_type );
		}
		else
		{
			response.error(STRINGIZE("unknown type '" << request["type"].asString() << "'"));
		}
	}

	void disable_all_render_types_wrapper(LLSD const& request)
	{
		gPipeline.clearAllRenderTypes();
	}

	void enable_all_render_types_wrapper(LLSD const& request)
	{
		gPipeline.setAllRenderTypes();
	}

	// Render Features
	void toggle_render_features_wrapper(LLSD const& request)
	{
		for (LLSD::array_const_iterator iter = request["features"].beginArray();
			iter != request["features"].endArray();
			++iter)
		{
			U32 render_feature = feature_from_string( iter->asString() );
			if ( render_feature != 0 )
			{
				LLPipeline::toggleRenderDebugControl( (void*) render_feature );
			}
		}
	}

	void has_render_feature_wrapper(LLSD const& request)
	{
		LLEventAPI::Response response(LLSD(), request);
		U32 render_feature = feature_from_string( request["feature"].asString() );
		if ( render_feature != 0 )
		{
			response["value"] = gPipeline.hasRenderDebugFeatureMask(render_feature);
		}
		else
		{
			response.error(STRINGIZE("unknown feature '" << request["feature"].asString() << "'"));
		}
	}

	void disable_all_render_features_wrapper(LLSD const& request)
	{
		gPipeline.clearAllRenderDebugFeatures();
	}

	void enable_all_render_features_wrapper(LLSD const& request)
	{
		gPipeline.setAllRenderDebugFeatures();
	}

	// Render Info Displays
	void toggle_info_displays_wrapper(LLSD const& request)
	{
		for (LLSD::array_const_iterator iter = request["displays"].beginArray();
			iter != request["displays"].endArray();
			++iter)
		{
			U32 info_display = info_display_from_string( iter->asString() );
			if ( info_display != 0 )
			{
				LLPipeline::toggleRenderDebug( (void*) info_display );
			}
		}
	}

	void has_info_display_wrapper(LLSD const& request)
	{
		LLEventAPI::Response response(LLSD(), request);
		U32 info_display = info_display_from_string( request["display"].asString() );
		if ( info_display != 0 )
		{
			response["value"] = gPipeline.hasRenderDebugMask(info_display);
		}
		else
		{
			response.error(STRINGIZE("unknown display '" << request["display"].asString() << "'"));
		}
	}

	void disable_all_info_displays_wrapper(LLSD const& request)
	{
		gPipeline.clearAllRenderDebugDisplays();
	}

	void enable_all_info_displays_wrapper(LLSD const& request)
	{
		gPipeline.setAllRenderDebugDisplays();
	}

}


LLPipelineListener::LLPipelineListener():
	LLEventAPI("LLPipeline",
			   "API to te rendering pipeline.")
{
	// Render Types
	add("toggleRenderTypes",
		"Toggle rendering [\"types\"]:\n"
		"See: llviewermenu.cpp:render_type_from_string for list of available types.",
		&toggle_render_types_wrapper);
	add("hasRenderType",
		"Check if rendering [\"type\"] is enabled:\n"
		"See: llviewermenu.cpp:render_type_from_string for list of available types.",
		&has_render_type_wrapper,
		LLSDMap("reply", LLSD()));
	add("disableAllRenderTypes",
		"Turn off all rendering types.",
		&disable_all_render_types_wrapper);
	add("enableAllRenderTypes",
		"Turn on all rendering types.",
		&enable_all_render_types_wrapper);

	// Render Features
	add("toggleRenderFeatures",
		"Toggle rendering [\"features\"]:\n"
		"See: llviewermenu.cpp:feature_from_string for list of available features.",
		&toggle_render_features_wrapper);
	add("hasRenderFeature",
		"Check if rendering [\"feature\"] is enabled:\n"
		"See: llviewermenu.cpp:render_feature_from_string for list of available features.",
		&has_render_feature_wrapper,
		LLSDMap("reply", LLSD()));
	add("disableAllRenderFeatures",
		"Turn off all rendering features.",
		&disable_all_render_features_wrapper);
	add("enableAllRenderFeatures",
		"Turn on all rendering features.",
		&enable_all_render_features_wrapper);

	// Render Info Displays
	add("toggleRenderInfoDisplays",
		"Toggle info [\"displays\"]:\n"
		"See: llviewermenu.cpp:info_display_from_string for list of available displays.",
		&toggle_info_displays_wrapper);
	add("hasRenderInfoDisplay",
		"Check if info [\"display\"] is enabled:\n"
		"See: llviewermenu.cpp:info_display_from_string for list of available displays.",
		&has_info_display_wrapper,
		LLSDMap("reply", LLSD()));
	add("disableAllRenderInfoDisplays",
		"Turn off all info displays.",
		&disable_all_info_displays_wrapper);
	add("enableAllRenderInfoDisplays",
		"Turn on all info displays.",
		&enable_all_info_displays_wrapper);
}

