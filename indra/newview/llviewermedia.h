/**
 * @file llviewermedia.h
 * @brief Client interface to the media engine
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LLVIEWERMEDIA_H
#define LLVIEWERMEDIA_H

#include "llmediabase.h"	// for status codes

class LLMediaManagerData;
class LLUUID;

class LLViewerMedia
{
	public:
		// Special case early init for just web browser component
		// so we can show login screen.  See .cpp file for details. JC
		static void initBrowser();

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
		static bool isMusicPlaying();

		static std::string getMediaURL();
		static std::string getMimeType();
		static void setMimeType(std::string mime_type);

		static void updateImagesMediaStreams();

		static void toggleMusicPlay(void*);
		static void toggleMediaPlay(void*);
		static void mediaStop(void*);

	private:
		// Fill in initialization data for LLMediaManager::initClass()
		static void buildMediaManagerData( LLMediaManagerData* init_data );
		
		enum { STOPPED=0, PLAYING=1, PAUSED=2 };
		static S32 mMusicState;
};

#endif	// LLVIEWERMEDIA_H
