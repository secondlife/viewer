/** 
 * @file llvowater.h
 * @brief Description of LLVOWater class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_VOWATER_H
#define LL_VOWATER_H

#include "llviewerobject.h"
#include "llviewerimage.h"
#include "v2math.h"
#include "llfft.h"

#include "llwaterpatch.h"


const U32 N_RES	= 16; //32			// number of subdivisions of wave tile
const U8  WAVE_STEP		= 8;

/*
#define N_DET		32			// number of subdivisions of wave tile for details

class LLWaterDetail
{
protected:
	S32				mResolution;
	LLViewerImage	*mTex;
	U8				*mTexData;

public:
	LLWaterDetail() : mResolution(N_DET), mTex(0), mTexData(0) {init();}
	void init();

	~LLWaterDetail()
	{
		delete[] mTexData;
		mTexData = NULL;
	}


	//void initEmpty();

	void setPixel(const LLVector3 &norm, const S32 i, const S32 j)
	{
		S32 offset = (i * mResolution + j) * 3;
		mTexData[offset]   = llround(norm.mV[VX] * 255);
		mTexData[offset+1] = llround(norm.mV[VX] * 255);
		mTexData[offset+2] = llround(norm.mV[VX] * 255);
	}
	void setPixel(F32 x, F32 y, F32 z, const S32 i, const S32 j)
	{
		S32 offset = (i * mResolution + j) * 3;
		mTexData[offset]   = llround(x * 255);
		mTexData[offset+1] = llround(y * 255);
		mTexData[offset+2] = llround(z * 255);
	}

	S32 getResolution()						{ return mResolution; }

	void createDetailBumpmap(F32* u, F32* v);
	void createTexture() const			{ mTex->createTexture(); }
	void bindTexture() const			{ mTex->bindTexture(); }
	LLViewerImage* getTexture() const	{ return mTex; }
};
*/

class LLWaterSurface
{
protected:
	BOOL mInitialized;
	LLVector3 mWind;
	F32 mA;
	F32 mVisc; // viscosity of the fluid
	F32 mShininess;

	//LLWaterDetail* mDetail;

	LLFFTPlan mPlan;
	F32 mOmega[N_RES+1][N_RES+1];		// wave frequency
	COMPLEX mHtilda0[N_RES+1][N_RES+1];	// wave amplitudes and phases at time 0.
	LLVector3 mNorms[N_RES][N_RES];
	COMPLEX mHtilda[N_RES * N_RES];

public:
	LLWaterSurface();
	~LLWaterSurface() {}

	//void initSpecularLookup();

	F32 phillips(const LLVector2& k, const LLVector2& wind_n, F32 L, F32 L_small);

	void initAmplitudes();

	F32 height(S32 i, S32 j) const { return mHtilda[i * N_RES + j].re; }
	F32 heightWrapped(S32 i, S32 j) const
	{
		return height((i + N_RES) % N_RES, (j + N_RES) % N_RES);
	}

	void generateWaterHeightField(F64 time);
	void calcNormals();

	void getHeightAndNormal(F32 i, F32 j, F32& height, LLVector3& normal) const;
	void getIntegerHeightAndNormal(S32 i, S32 j, F32& height, LLVector3& normal) const;
	F32 agentDepth() const;

//	const LLWaterHeightField* hField() const { return &mHeightField; }

	//void generateDetail(F64 t);
	//void fluidSolver(F32* u, F32* v, F32* u0, F32* v0, F64 dt);

	const LLVector3& getWind() const { return mWind; }
	void setWind(const LLVector3& w) { mWind = w; } // initialized = 0?
	F32 A() const { return mA; }
	void setA(F32 a) { mA = a; }
	F32 getShininess() const { return mShininess; }
	//LLViewerImage* getSpecularLookup() const { return mSpecularLookup; }
	//LLViewerImage* getDetail() const { return mDetail->getTexture(); }
};

class LLVOWater;

class LLWaterGrid
{
public:
	LLWaterGrid();

	void init();
	void cleanup();

	LLWaterSurface* getWaterSurface() { return &mWater; }

	void update();
	void updateTree(const LLVector3 &camera_pos, const LLVector3 &look_at, F32 clip_far,
		BOOL restart);
	void updateVisibility(const LLVector3 &camera_pos, const LLVector3 &look_at, F32 clip_far);

	LLVector3 mRegionOrigin;

	LLVector3* mVtx;
	LLVector3* mNorms;
	U32 mRegionWidth;
	U32 mMaxGridSize;
	U32 mPatchRes;
	U32 mMinStep;
	U32 mStepsInRegion;
	LLWaterPatch* mPatches;
	LLRoam mRoam;
	F32 mResIncrease;
	U32 mResDecrease;

	LLVOWater* mTab[5][5];

	U32 gridDim() const					{ return mGridDim; }
	U32 rowSize() const					{ return mGridDim1; }
	void setGridDim(U32 gd)				{ mGridDim = gd; mGridDim1 = mGridDim + 1; }
	U32 index(const LL2Coord& c) const	{ return c.y() * mGridDim1 + c.x(); }
	U32 index(U32 x, U32 y) const		{ return y * mGridDim1 + x; }

	LLVector3 vtx(const LL2Coord& c, F32 raised) const
	{
		LLVector3 v = mVtx[index(c)];
		v.mV[VZ] += raised;
		return v;
	}

	LLVector3 vtx(U32 x, U32 y, F32 raised) const
	{
		LLVector3 v = mVtx[index(x, y)];
		v.mV[VZ] += raised;
		return v;
	}

	void setVertex(const U32 x, const U32 y, const F32 raised, LLVector3 &vertex) const
	{
		vertex = mVtx[index(x, y)];
		vertex.mV[VZ] += raised;
	}

	void setCamPosition(LL2Coord& cam, const LLVector3& cam_pos)
	{
		cam.x() = llround((cam_pos.mV[VX] - mRegionOrigin.mV[VX]) / mMinStep);
		cam.y() = llround((cam_pos.mV[VY] - mRegionOrigin.mV[VY]) / mMinStep);
	}

	LLVector3 vtx(const LL2Coord& c) const		{ return mVtx[index(c)]; }
	LLVector3 norm(const LL2Coord& c) const		{ return mNorms[index(c)]; }
	LLVector3 vtx(U32 x, U32 y) const			{ return mVtx[index(x, y)]; }
	LLVector3 norm(U32 x, U32 y) const			{ return mNorms[index(x, y)]; }

protected:
	LLWaterSurface mWater;
	U32 mGridDim;
	U32 mGridDim1;
};

class LLSurface;
class LLHeavenBody;
class LLVOSky;
class LLFace;

class LLVOWater : public LLViewerObject
{
public:
	LLVOWater(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	virtual ~LLVOWater() {}

	/*virtual*/ void markDead();

	// Initialize data that's only inited once per class.
	static void initClass();
	static void cleanupClass();

	/*virtual*/ BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL        updateGeometry(LLDrawable *drawable);
				BOOL		updateGeometryFlat(LLDrawable *drawable);
				BOOL		updateGeometryHeightFieldSimple(LLDrawable *drawable);
				BOOL		updateGeometryHeightFieldRoam(LLDrawable *drawable);

	/*virtual*/ void updateDrawable(BOOL force_damped);
	/*virtual*/ void updateTextures(LLAgent &agent);
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent); // generate accurate apparent angle and area

	/*virtual*/ BOOL isActive() const; // Whether this object needs to do an idleUpdate.

	void setUseTexture(const BOOL use_texture);

	static void generateNewWaves(F64 time);
	static LLWaterSurface* getWaterSurface() { return sGrid->getWaterSurface(); }//return &mWater; }
	static const LLWaterGrid* getGrid() { return sGrid; }
protected:
	BOOL mUseTexture;
	static LLWaterGrid *sGrid;
};

#endif // LL_VOSURFACEPATCH_H
