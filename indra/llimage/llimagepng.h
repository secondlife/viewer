/* 
 * @file llimagepng.h
 *
 * Copyright (c) 2007 Peekay Semyorka.
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLIMAGEPNG_H
#define LL_LLIMAGEPNG_H

#include "stdtypes.h"
#include "llimage.h"

class LLImagePNG : public LLImageFormatted
{
protected:
	~LLImagePNG();

public:
	LLImagePNG();

	BOOL updateData();
	BOOL decode(LLImageRaw* raw_image, F32 decode_time = 0.0);
	BOOL encode(const LLImageRaw* raw_image, F32 encode_time = 0.0);

private:
	U8* mTmpWriteBuffer;
};

#endif
