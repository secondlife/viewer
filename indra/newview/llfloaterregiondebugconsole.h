/** 
 * @file llfloaterregiondebugconsole.h
 * @author Brad Kittenbrink <brad@lindenlab.com>
 * @brief Quick and dirty console for region debug settings
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

#ifndef LL_LLFLOATERREGIONDEBUGCONSOLE_H
#define LL_LLFLOATERREGIONDEBUGCONSOLE_H

#include <boost/signals2.hpp>

#include "llfloater.h"

class LLTextEditor;

typedef boost::signals2::signal<
	void (const std::string& output)> console_reply_signal_t;

class LLFloaterRegionDebugConsole : public LLFloater
{
public:
	LLFloaterRegionDebugConsole(LLSD const & key);
	virtual ~LLFloaterRegionDebugConsole();

	bool postBuild() override;
	
	void onInput(LLUICtrl* ctrl, const LLSD& param);

	LLTextEditor * mOutput;

	static boost::signals2::connection setConsoleReplyCallback(const console_reply_signal_t::slot_type& cb);

 private:
	void onReplyReceived(const std::string& output);

    void onAsyncConsoleError(LLSD result);
    void onConsoleError(LLSD result);
    void onConsoleSuccess(LLSD result);

	boost::signals2::connection mReplySignalConnection;
};

#endif // LL_LLFLOATERREGIONDEBUGCONSOLE_H
