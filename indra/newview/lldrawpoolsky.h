/** 
 * @file lldrawpoolsky.h
 * @brief LLDrawPoolSky class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDRAWPOOLSKY_H
#define LL_LLDRAWPOOLSKY_H

#include "lldrawpool.h"

class LLSkyTex;
class LLHeavenBody;

class LLDrawPoolSky : public LLDrawPool
{
private:
	LLSkyTex			*mSkyTex;
	LLHeavenBody		*mHB[2]; // Sun and Moon

public:
	LLDrawPoolSky();

	/*virtual*/ LLDrawPool *instancePool();

	/*virtual*/ void prerender();
	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void renderForSelect();
	void setSkyTex(LLSkyTex* const st) { mSkyTex = st; }
	void setSun(LLHeavenBody* sun) { mHB[0] = sun; }
	void setMoon(LLHeavenBody* moon) { mHB[1] = moon; }

	void renderSkyCubeFace(U8 side);
	void renderHeavenlyBody(U8 hb, const LLFace* face);
	void renderSunHalo(const LLFace* face);

	virtual S32 getMaterialAttribIndex() { return 0; }
};

#endif // LL_LLDRAWPOOLSKY_H
