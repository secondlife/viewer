/** 
 * @file llfeaturemanager.h
 * @brief LLFeatureManager class definition
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	S32	getGPUClass();
	S32	loadGPUClass();

	void cleanupFeatureTables();

	S32 getVersion() const				{ return mTableVersion; }
	void setSafe(const BOOL safe)		{ mSafe = safe; }
	BOOL isSafe() const					{ return mSafe; }

	LLFeatureList *findMask(const char *name);
	BOOL maskFeatures(const char *name);


	void initCPUFeatureMasks();
	void initGraphicsFeatureMasks();
	BOOL initPCIFeatureMasks();

	void applyRecommendedFeatures();
protected:
	void initBaseMask();

	std::map<LLString, LLFeatureList *> mMaskList;
	BOOL		mInited;
	S32			mTableVersion;
	BOOL		mSafe;					// Reinitialize everything to the "safe" mask
	S32			mGPUClass;
};

extern LLFeatureManager *gFeatureManagerp;

#endif // LL_LLFEATUREMANAGER_H
