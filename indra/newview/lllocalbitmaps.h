/** 
 * @file lllocalbitmaps.h
 * @author Vaalith Jinn
 * @brief Local Bitmaps header
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LOCALBITMAPS_H
#define LL_LOCALBITMAPS_H

#include "llavatarappearancedefines.h"
#include "lleventtimer.h"
#include "llimage.h"
#include "llpointer.h"
#include "llwearabletype.h"

class LLScrollListCtrl;
class LLViewerObject;

class LLLocalBitmap
{
	public: /* main */
		LLLocalBitmap(std::string filename);
		~LLLocalBitmap();

	public: /* accessors */
		std::string	getFilename();
		std::string	getShortName();
		LLUUID		getTrackingID();
		LLUUID		getWorldID();
		bool		getValid();

	public: /* self update public section */
		enum EUpdateType
		{
			UT_FIRSTUSE,
			UT_REGUPDATE
		};

		bool updateSelf(EUpdateType = UT_REGUPDATE);

	private: /* self update private section */
		bool decodeBitmap(LLPointer<LLImageRaw> raw);
		void replaceIDs(LLUUID old_id, LLUUID new_id);
		std::vector<LLViewerObject*> prepUpdateObjects(LLUUID old_id);
		void updateUserPrims(LLUUID old_id, LLUUID new_id);
		void updateUserSculpts(LLUUID old_id, LLUUID new_id);
		void updateUserLayers(LLUUID old_id, LLUUID new_id, LLWearableType::EType type);
		LLAvatarAppearanceDefines::ETextureIndex getTexIndex(LLWearableType::EType type, LLAvatarAppearanceDefines::EBakedTextureIndex baked_texind);

	private: /* private enums */
		enum ELinkStatus
		{
			LS_ON,
			LS_BROKEN,
		};

		enum EExtension
		{
			ET_IMG_BMP,
			ET_IMG_TGA,
			ET_IMG_JPG,
			ET_IMG_PNG
		};

	private: /* members */
		std::string mFilename;
		std::string mShortName;
		LLUUID      mTrackingID;
		LLUUID      mWorldID;
		bool        mValid;
		LLSD        mLastModified;
		EExtension  mExtension;
		ELinkStatus mLinkStatus;
		S32         mUpdateRetries;

};

class LLLocalBitmapTimer : public LLEventTimer
{
	public:
		LLLocalBitmapTimer();
		~LLLocalBitmapTimer();

	public:
		void startTimer();
		void stopTimer();
		bool isRunning();
		BOOL tick();

};

class LLLocalBitmapMgr
{
	public:
		LLLocalBitmapMgr();
		~LLLocalBitmapMgr();

	public:
		static bool         addUnit();
		static void         delUnit(LLUUID tracking_id);

		static LLUUID       getWorldID(LLUUID tracking_id);
		static std::string  getFilename(LLUUID tracking_id);
		
		static void         feedScrollList(LLScrollListCtrl* ctrl);
		static void         doUpdates();
		static void         setNeedsRebake();
		static void         doRebake();
		
	private:
		static std::list<LLLocalBitmap*>    sBitmapList;
		static LLLocalBitmapTimer           sTimer;
		static bool                         sNeedsRebake;
		typedef std::list<LLLocalBitmap*>::iterator local_list_iter;
};

#endif

