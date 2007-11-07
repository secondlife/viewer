/** 
 * @file llvieweraudio.h
 * @brief Audio functions originally in viewer.cpp
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_VIEWERAUDIO_H
#define LL_VIEWERAUDIO_H

void init_audio();
void audio_update_volume(bool force_update = true);
void audio_update_listener();
void audio_update_wind(bool force_update = true);

#endif //LL_VIEWER_H
