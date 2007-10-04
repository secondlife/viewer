/** 
 * @file llmenucommands.h
 * @brief Implementations of menu commands.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

#ifndef LL_LLMENUCOMMANDS_H
#define LL_LLMENUCOMMANDS_H

class LLUUID;

void handle_track_avatar(const LLUUID& agent_id, const std::string& name);
void handle_pay_by_id(const LLUUID& agent_id);
void handle_mouselook(void*);
void handle_map(void*);
void handle_mini_map(void*);
void handle_find(void*);
void handle_events(void*);
void handle_inventory(void*);
void handle_clothing(void*);
void handle_chat(void*);
void handle_return_key(void*);
void handle_slash_key(void*);

#endif
