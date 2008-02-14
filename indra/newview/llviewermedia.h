/** 
 * @file llviewermedia.h
 * @author Callum Prentice & James Cook
 * @brief Client interface to the media engine
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#ifndef LLVIEWERMEDIA_H
#define LLVIEWERMEDIA_H

#include "llmediabase.h"	// for status codes

class LLUUID;

class LLViewerMedia
{
	public:
		static void initClass();
		static void cleanupClass();

		static void play(const std::string& media_url,
						 const std::string& mime_type,
						 const LLUUID& placeholder_texture_id,
						 S32 media_width, S32 media_height, U8 media_auto_scale,
						 U8 media_loop);
		static void stop();
		static void pause();
		static void start();
		static void seek(F32 time);
		static void setVolume(F32 volume);
		static LLMediaBase::EStatus getStatus();

		static LLUUID getMediaTextureID();
		static bool getMediaSize(S32 *media_width, S32 *media_height);
		static bool getTextureSize(S32 *texture_width, S32 *texture_height);
		static bool isMediaPlaying();
		static bool isMediaPaused();
		static bool hasMedia();
		static bool isActiveMediaTexture(const LLUUID& id);

		static std::string getMediaURL();
		static std::string getMimeType();
		static void setMimeType(std::string mime_type);

		static void updateImagesMediaStreams();
};

#endif	// LLVIEWERMEDIA_H
