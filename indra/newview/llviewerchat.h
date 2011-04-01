/** 
 * @file llviewerchat.h
 * @brief wrapper of LLChat in viewer
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLVIEWERCHAT_H
#define LL_LLVIEWERCHAT_H

#include "llchat.h"
#include "llfontgl.h"
#include "v4color.h"


class LLViewerChat 
{
public:
	static void getChatColor(const LLChat& chat, LLColor4& r_color);
	static void getChatColor(const LLChat& chat, std::string& r_color_name, F32& r_color_alpha);
	static LLFontGL* getChatFont();
	static S32 getChatFontSize();
	static void formatChatMsg(const LLChat& chat, std::string& formated_msg);
	static std::string getSenderSLURL(const LLChat& chat, const LLSD& args);

private:
	static std::string getObjectImSLURL(const LLChat& chat, const LLSD& args);

};

#endif
