/** 
 * @file llimage.h
 * @brief Object for managing images and their textures.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_LLIMAGE_H
#define LL_LLIMAGE_H

#include "lluuid.h"
#include "llstring.h"
#include "llthread.h"
#include "llmemtype.h"

const S32 MIN_IMAGE_MIP =  2; // 4x4, only used for expand/contract power of 2
const S32 MAX_IMAGE_MIP = 11; // 2048x2048

// *TODO : Use MAX_IMAGE_MIP as max discard level and modify j2c management so that the number 
// of levels is read from the header's file, not inferred from its size.
const S32 MAX_DISCARD_LEVEL = 5;

// JPEG2000 size constraints
// Those are declared here as they are germane to other image constraints used in the viewer
// and declared right here. Some come from the JPEG2000 spec, some conventions specific to SL.
const S32 MAX_DECOMPOSITION_LEVELS = 32;	// Number of decomposition levels cannot exceed 32 according to jpeg2000 spec
const S32 MIN_DECOMPOSITION_LEVELS = 5;		// the SL viewer will *crash* trying to decode images with fewer than 5 decomposition levels (unless image is small that is)
const S32 MAX_PRECINCT_SIZE = 2048;			// No reason to be bigger than MAX_IMAGE_SIZE 
const S32 MIN_PRECINCT_SIZE = 4;			// Can't be smaller than MIN_BLOCK_SIZE
const S32 MAX_BLOCK_SIZE = 64;				// Max total block size is 4096, hence 64x64 when using square blocks
const S32 MIN_BLOCK_SIZE = 4;				// Min block dim is 4 according to jpeg2000 spec
const S32 MIN_LAYER_SIZE = 2000;			// Size of the first quality layer (after header). Must be > to FIRST_PACKET_SIZE!!
const S32 MAX_NB_LAYERS = 64;				// Max number of layers we'll entertain in SL (practical limit)

const S32 MIN_IMAGE_SIZE = (1<<MIN_IMAGE_MIP); // 4, only used for expand/contract power of 2
const S32 MAX_IMAGE_SIZE = (1<<MAX_IMAGE_MIP); // 2048
const S32 MIN_IMAGE_AREA = MIN_IMAGE_SIZE * MIN_IMAGE_SIZE;
const S32 MAX_IMAGE_AREA = MAX_IMAGE_SIZE * MAX_IMAGE_SIZE;
const S32 MAX_IMAGE_COMPONENTS = 8;
const S32 MAX_IMAGE_DATA_SIZE = MAX_IMAGE_AREA * MAX_IMAGE_COMPONENTS; //2048 * 2048 * 8 = 16 MB

// Note!  These CANNOT be changed without modifying simulator code
// *TODO: change both to 1024 when SIM texture fetching is deprecated
const S32 FIRST_PACKET_SIZE = 600;
const S32 MAX_IMG_PACKET_SIZE = 1000;
const S32 HTTP_PACKET_SIZE = 1496;

// Base classes for images.
// There are two major parts for the image:
// The compressed representation, and the decompressed representation.

class LLImageFormatted;
class LLImageRaw;
class LLColor4U;
class LLPrivateMemoryPool;

typedef enum e_image_codec
{
	IMG_CODEC_INVALID  = 0,
	IMG_CODEC_RGB  = 1,
	IMG_CODEC_J2C  = 2,
	IMG_CODEC_BMP  = 3,
	IMG_CODEC_TGA  = 4,
	IMG_CODEC_JPEG = 5,
	IMG_CODEC_DXT  = 6,
	IMG_CODEC_PNG  = 7,
	IMG_CODEC_EOF  = 8
} EImageCodec;

//============================================================================
// library initialization class

class LLImage
{
public:
	static void initClass(bool use_new_byte_range = false, S32 minimal_reverse_byte_range_percent = 75);
	static void cleanupClass();

	static const std::string& getLastError();
	static void setLastError(const std::string& message);
	
	static bool useNewByteRange() { return sUseNewByteRange; }
    static S32  getReverseByteRangePercent() { return sMinimalReverseByteRangePercent; }
	
protected:
	static LLMutex* sMutex;
	static std::string sLastErrorMessage;
	static bool sUseNewByteRange;
    static S32  sMinimalReverseByteRangePercent;
};

//============================================================================
// Image base class

class LLImageBase : public LLThreadSafeRefCount
{
protected:
	virtual ~LLImageBase();
	
public:
	LLImageBase();

	enum
	{
		TYPE_NORMAL = 0,
		TYPE_AVATAR_BAKE = 1,
	};

	virtual void deleteData();
	virtual U8* allocateData(S32 size = -1);
	virtual U8* reallocateData(S32 size = -1);

	virtual void dump();
	virtual void sanityCheck();

	U16 getWidth() const		{ return mWidth; }
	U16 getHeight() const		{ return mHeight; }
	S8	getComponents() const	{ return mComponents; }
	S32 getDataSize() const		{ return mDataSize; }

	const U8 *getData() const	;
	U8 *getData()				;
	bool isBufferInvalid() ;

	void setSize(S32 width, S32 height, S32 ncomponents);
	U8* allocateDataSize(S32 width, S32 height, S32 ncomponents, S32 size = -1); // setSize() + allocateData()
	void enableOverSize() {mAllowOverSize = true ;}
	void disableOverSize() {mAllowOverSize = false; }

protected:
	// special accessor to allow direct setting of mData and mDataSize by LLImageFormatted
	void setDataAndSize(U8 *data, S32 size);
	
public:
	static void generateMip(const U8 *indata, U8* mipdata, int width, int height, S32 nchannels);
	
	// Function for calculating the download priority for textures
	// <= 0 priority means that there's no need for more data.
	static F32 calc_download_priority(F32 virtual_size, F32 visible_area, S32 bytes_sent);

	static EImageCodec getCodecFromExtension(const std::string& exten);
	
	static void createPrivatePool() ;
	static void destroyPrivatePool() ;
	static LLPrivateMemoryPool* getPrivatePool() {return sPrivatePoolp;}

private:
	U8 *mData;
	S32 mDataSize;

	U16 mWidth;
	U16 mHeight;

	S8 mComponents;

	bool mBadBufferAllocation ;
	bool mAllowOverSize ;

	static LLPrivateMemoryPool* sPrivatePoolp ;
public:
	LLMemType::DeclareMemType& mMemType; // debug
};

// Raw representation of an image (used for textures, and other uncompressed formats
class LLImageRaw : public LLImageBase
{
protected:
	/*virtual*/ ~LLImageRaw();
	
public:
	LLImageRaw();
	LLImageRaw(U16 width, U16 height, S8 components);
	LLImageRaw(U8 *data, U16 width, U16 height, S8 components, bool no_copy = false);
	// Construct using createFromFile (used by tools)
	//LLImageRaw(const std::string& filename, bool j2c_lowest_mip_only = false);

	/*virtual*/ void deleteData();
	/*virtual*/ U8* allocateData(S32 size = -1);
	/*virtual*/ U8* reallocateData(S32 size);
	
	BOOL resize(U16 width, U16 height, S8 components);

	//U8 * getSubImage(U32 x_pos, U32 y_pos, U32 width, U32 height) const;
	BOOL setSubImage(U32 x_pos, U32 y_pos, U32 width, U32 height,
					 const U8 *data, U32 stride = 0, BOOL reverse_y = FALSE);

	void clear(U8 r=0, U8 g=0, U8 b=0, U8 a=255);

	void verticalFlip();

	void expandToPowerOfTwo(S32 max_dim = MAX_IMAGE_SIZE, BOOL scale_image = TRUE);
	void contractToPowerOfTwo(S32 max_dim = MAX_IMAGE_SIZE, BOOL scale_image = TRUE);
	void biasedScaleToPowerOfTwo(S32 max_dim = MAX_IMAGE_SIZE);
	BOOL scale( S32 new_width, S32 new_height, BOOL scale_image = TRUE );
	//BOOL scaleDownWithoutBlending( S32 new_width, S32 new_height) ;

	// Fill the buffer with a constant color
	void fill( const LLColor4U& color );

	// Copy operations
	
	// Src and dst can be any size.  Src and dst can each have 3 or 4 components.
	void copy( LLImageRaw* src );

	// Src and dst are same size.  Src and dst have same number of components.
	void copyUnscaled( LLImageRaw* src );
	
	// Src and dst are same size.  Src has 4 components.  Dst has 3 components.
	void copyUnscaled4onto3( LLImageRaw* src );

	// Src and dst are same size.  Src has 3 components.  Dst has 4 components.
	void copyUnscaled3onto4( LLImageRaw* src );

	// Src and dst can be any size.  Src and dst have same number of components.
	void copyScaled( LLImageRaw* src );

	// Src and dst can be any size.  Src has 3 components.  Dst has 4 components.
	void copyScaled3onto4( LLImageRaw* src );

	// Src and dst can be any size.  Src has 4 components.  Dst has 3 components.
	void copyScaled4onto3( LLImageRaw* src );


	// Composite operations

	// Src and dst can be any size.  Src and dst can each have 3 or 4 components.
	void composite( LLImageRaw* src );

	// Src and dst can be any size.  Src has 4 components.  Dst has 3 components.
	void compositeScaled4onto3( LLImageRaw* src );

	// Src and dst are same size.  Src has 4 components.  Dst has 3 components.
	void compositeUnscaled4onto3( LLImageRaw* src );

protected:
	// Create an image from a local file (generally used in tools)
	//bool createFromFile(const std::string& filename, bool j2c_lowest_mip_only = false);

	void copyLineScaled( U8* in, U8* out, S32 in_pixel_len, S32 out_pixel_len, S32 in_pixel_step, S32 out_pixel_step );
	void compositeRowScaled4onto3( U8* in, U8* out, S32 in_pixel_len, S32 out_pixel_len );

	U8	fastFractionalMult(U8 a,U8 b);

	void setDataAndSize(U8 *data, S32 width, S32 height, S8 components) ;

public:
	static S32 sGlobalRawMemory;
	static S32 sRawImageCount;
};

// Compressed representation of image.
// Subclass from this class for the different representations (J2C, bmp)
class LLImageFormatted : public LLImageBase
{
public:
	static LLImageFormatted* createFromType(S8 codec);
	static LLImageFormatted* createFromExtension(const std::string& instring);	

protected:
	/*virtual*/ ~LLImageFormatted();
	
public:
	LLImageFormatted(S8 codec);

	// LLImageBase
	/*virtual*/ void deleteData();
	/*virtual*/ U8* allocateData(S32 size = -1);
	/*virtual*/ U8* reallocateData(S32 size);
	
	/*virtual*/ void dump();
	/*virtual*/ void sanityCheck();

	// New methods
	// subclasses must return a prefered file extension (lowercase without a leading dot)
	virtual std::string getExtension() = 0;
	// calcHeaderSize() returns the maximum size of header;
	//   0 indicates we don't have a header and have to read the entire file
	virtual S32 calcHeaderSize() { return 0; };
	// calcDataSize() returns how many bytes to read to load discard_level (including header)
	virtual S32 calcDataSize(S32 discard_level);
	// calcDiscardLevelBytes() returns the smallest valid discard level based on the number of input bytes
	virtual S32 calcDiscardLevelBytes(S32 bytes);
	// getRawDiscardLevel() by default returns mDiscardLevel, but may be overridden (LLImageJ2C)
	virtual S8  getRawDiscardLevel() { return mDiscardLevel; }
	
	BOOL load(const std::string& filename, int load_size = 0);
	BOOL save(const std::string& filename);

	virtual BOOL updateData() = 0; // pure virtual
 	void setData(U8 *data, S32 size);
 	void appendData(U8 *data, S32 size);

	// Loads first 4 channels.
	virtual BOOL decode(LLImageRaw* raw_image, F32 decode_time) = 0;  
	// Subclasses that can handle more than 4 channels should override this function.
	virtual BOOL decodeChannels(LLImageRaw* raw_image, F32 decode_time, S32 first_channel, S32 max_channel);

	virtual BOOL encode(const LLImageRaw* raw_image, F32 encode_time) = 0;

	S8 getCodec() const;
	BOOL isDecoding() const { return mDecoding ? TRUE : FALSE; }
	BOOL isDecoded()  const { return mDecoded ? TRUE : FALSE; }
	void setDiscardLevel(S8 discard_level) { mDiscardLevel = discard_level; }
	S8 getDiscardLevel() const { return mDiscardLevel; }
	S8 getLevels() const { return mLevels; }
	void setLevels(S8 nlevels) { mLevels = nlevels; }

	// setLastError needs to be deferred for J2C images since it may be called from a DLL
	virtual void resetLastError();
	virtual void setLastError(const std::string& message, const std::string& filename = std::string());
	
protected:
	BOOL copyData(U8 *data, S32 size); // calls updateData()
	
protected:
	S8 mCodec;
	S8 mDecoding;
	S8 mDecoded;  // unused, but changing LLImage layout requires recompiling static Mac/Linux libs. 2009-01-30 JC
	S8 mDiscardLevel;	// Current resolution level worked on. 0 = full res, 1 = half res, 2 = quarter res, etc...
	S8 mLevels;			// Number of resolution levels in that image. Min is 1. 0 means unknown.
	
public:
	static S32 sGlobalFormattedMemory;
};

#endif
