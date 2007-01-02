/** 
 * @file llmenucommands.h
 * @brief Implementations of menu commands.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
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
