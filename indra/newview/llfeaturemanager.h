/** 
 * @file llfeaturemanager.h
 * @brief LLFeatureManager class definition
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

#ifndef LL_LLFEATUREMANAGER_H
#define LL_LLFEATUREMANAGER_H

#include "stdtypes.h"

#include "llstring.h"
#include "llskipmap.h"
#include <map>

class LLFeatureInfo
{
public:
	LLFeatureInfo() : mValid(FALSE), mAvailable(FALSE), mRecommendedLevel(-1) {}
	LLFeatureInfo(const char *name, const BOOL available, const S32 level);

	BOOL isValid() const	{ return mValid; };

public:
	BOOL		mValid;
	LLString	mName;
	BOOL		mAvailable;
	S32			mRecommendedLevel;
};


class LLFeatureList
{
public:
	LLFeatureList(const char *name = "default");
	virtual ~LLFeatureList();

	BOOL isFeatureAvailable(const char *name);
	S32 getRecommendedLevel(const char *name);

	void setFeatureAvailable(const char *name, const BOOL available);
	void setRecommendedLevel(const char *name, const S32 level);

	BOOL loadFeatureList(FILE *fp);

	BOOL maskList(LLFeatureList &mask);

	void addFeature(const char *name, const BOOL available, const S32 level);

	void dump();
protected:
	LLString	mName;
	typedef std::map<LLString, LLFeatureInfo> feature_map_t;
	feature_map_t	mFeatures;
};


class LLFeatureManager : public LLFeatureList
{
public:
	LLFeatureManager() : mInited(FALSE), mTableVersion(0), mSafe(FALSE), mGPUClass(0) {}

	void maskCurrentList(const char *name); // Mask the current feature list with the named list

	BOOL loadFeatureTables();

	S32	getGPUClass() 					{ return mGPUClass; }
	std::string& getGPUString() 		{ return mGPUString; }
	
	void cleanupFeatureTables();

	S32 getVersion() const				{ return mTableVersion; }
	void setSafe(const BOOL safe)		{ mSafe = safe; }
	BOOL isSafe() const					{ return mSafe; }

	LLFeatureList *findMask(const char *name);
	BOOL maskFeatures(const char *name);


	void initCPUFeatureMasks();
	void initGraphicsFeatureMasks();
	
	void applyRecommendedFeatures();

protected:
	void loadGPUClass();
	void initBaseMask();

	std::map<LLString, LLFeatureList *> mMaskList;
	BOOL		mInited;
	S32			mTableVersion;
	BOOL		mSafe;					// Reinitialize everything to the "safe" mask
	S32			mGPUClass;
	std::string	mGPUString;
};

extern LLFeatureManager *gFeatureManagerp;

#endif // LL_LLFEATUREMANAGER_H
