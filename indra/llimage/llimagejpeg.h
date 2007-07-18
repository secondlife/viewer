/** 
 * @file llimagejpeg.h
 * @brief This class compresses and decompresses JPEG files
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLIMAGEJPEG_H
#define LL_LLIMAGEJPEG_H

#include <setjmp.h>

#include "llimage.h"

extern "C" {
#ifdef LL_STANDALONE
# include <jpeglib.h>
# include <jerror.h>
#else
# include "jpeglib/jpeglib.h"
# include "jpeglib/jerror.h"
#endif
}

class LLImageJPEG : public LLImageFormatted
{
protected:
	virtual ~LLImageJPEG();
	
public:
	LLImageJPEG();

	/*virtual*/ BOOL updateData();
	/*virtual*/ BOOL decode(LLImageRaw* raw_image, F32 time=0.0);
	/*virtual*/ BOOL encode(const LLImageRaw* raw_image, F32 time=0.0);

	void			setEncodeQuality( S32 q )	{ mEncodeQuality = q; } // on a scale from 1 to 100
	S32				getEncodeQuality()			{ return mEncodeQuality; }

	// Callbacks registered with jpeglib
	static void		encodeInitDestination ( j_compress_ptr cinfo );
	static boolean	encodeEmptyOutputBuffer(j_compress_ptr cinfo);
	static void		encodeTermDestination(j_compress_ptr cinfo);

	static void		decodeInitSource(j_decompress_ptr cinfo);
	static boolean	decodeFillInputBuffer(j_decompress_ptr cinfo);
	static void		decodeSkipInputData(j_decompress_ptr cinfo, long num_bytes);
	static void		decodeTermSource(j_decompress_ptr cinfo);


	static void		errorExit(j_common_ptr cinfo);
	static void		errorEmitMessage(j_common_ptr cinfo, int msg_level);
	static void		errorOutputMessage(j_common_ptr cinfo);

	static BOOL		decompress(LLImageJPEG* imagep);

protected:
	U8*				mOutputBuffer;		// temp buffer used during encoding
	S32				mOutputBufferSize;	// bytes in mOuputBuffer

	S32				mEncodeQuality;		// on a scale from 1 to 100

	jmp_buf			mSetjmpBuffer;		// To allow the library to abort.
};

#endif  // LL_LLIMAGEJPEG_H
