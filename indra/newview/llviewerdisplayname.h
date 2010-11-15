/** 
 * @file llviewerdisplayname.h
 * @brief Wrapper for display name functionality
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LLVIEWERDISPLAYNAME_H
#define LLVIEWERDISPLAYNAME_H

#include <boost/signals2.hpp>

class LLSD;
class LLUUID;

namespace LLViewerDisplayName
{
	typedef boost::signals2::signal<
		void (bool success, const std::string& reason, const LLSD& content)>
			set_name_signal_t;
	typedef set_name_signal_t::slot_type set_name_slot_t;
	
	typedef boost::signals2::signal<void (void)> name_changed_signal_t;
	typedef name_changed_signal_t::slot_type name_changed_slot_t;

	// Sends an update to the server to change a display name
	// and call back when done.  May not succeed due to service
	// unavailable or name not available.
	void set(const std::string& display_name, const set_name_slot_t& slot); 
	
	void addNameChangedCallback(const name_changed_signal_t::slot_type& cb);
}

#endif // LLVIEWERDISPLAYNAME_H
