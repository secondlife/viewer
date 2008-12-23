/** 
 * @file llimagejpeg.cpp
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#include "linden_common.h"
#include "stdtypes.h"

#include "llimagejpeg.h"

#include "llerror.h"

jmp_buf	LLImageJPEG::sSetjmpBuffer ;
LLImageJPEG::LLImageJPEG(S32 quality) 
	:
	LLImageFormatted(IMG_CODEC_JPEG),
	mOutputBuffer( NULL ),
	mOutputBufferSize( 0 ),
	mEncodeQuality( quality ) // on a scale from 1 to 100
{
}

LLImageJPEG::~LLImageJPEG()
{
	llassert( !mOutputBuffer ); // Should already be deleted at end of encode.
	delete[] mOutputBuffer;
}

BOOL LLImageJPEG::updateData()
{
	resetLastError();

	// Check to make sure that this instance has been initialized with data
	if (!getData() || (0 == getDataSize()))
	{
		setLastError("Uninitialized instance of LLImageJPEG");
		return FALSE;
	}

	////////////////////////////////////////
	// Step 1: allocate and initialize JPEG decompression object

	// This struct contains the JPEG decompression parameters and pointers to
	// working space (which is allocated as needed by the JPEG library).
	struct jpeg_decompress_struct cinfo;
	cinfo.client_data = this;

	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	
	// Customize with our own callbacks
	jerr.error_exit =		&LLImageJPEG::errorExit;			// Error exit handler: does not return to caller
	jerr.emit_message =		&LLImageJPEG::errorEmitMessage;		// Conditionally emit a trace or warning message
	jerr.output_message =	&LLImageJPEG::errorOutputMessage;	// Routine that actually outputs a trace or error message
	
	//
	//try/catch will crash on Mac and Linux if LLImageJPEG::errorExit throws an error
	//so as instead, we use setjmp/longjmp to avoid this crash, which is the best we can get. --bao 
	//
	if(setjmp(sSetjmpBuffer))
	{
		jpeg_destroy_decompress(&cinfo);
		return FALSE;
	}
	try
	{
		// Now we can initialize the JPEG decompression object.
		jpeg_create_decompress(&cinfo);

		////////////////////////////////////////
		// Step 2: specify data source
		// (Code is modified version of jpeg_stdio_src();
		if (cinfo.src == NULL)
		{	
			cinfo.src = (struct jpeg_source_mgr *)
				(*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
				sizeof(struct jpeg_source_mgr));
		}
		cinfo.src->init_source	=		&LLImageJPEG::decodeInitSource;
		cinfo.src->fill_input_buffer =	&LLImageJPEG::decodeFillInputBuffer;
		cinfo.src->skip_input_data =	&LLImageJPEG::decodeSkipInputData;
		cinfo.src->resync_to_restart = jpeg_resync_to_restart; // For now, use default method, but we should be able to do better.
		cinfo.src->term_source =		&LLImageJPEG::decodeTermSource;

		cinfo.src->bytes_in_buffer =	getDataSize();
		cinfo.src->next_input_byte =	getData();
		
		////////////////////////////////////////
		// Step 3: read file parameters with jpeg_read_header()
		jpeg_read_header( &cinfo, TRUE );

		// Data set by jpeg_read_header
		setSize(cinfo.image_width, cinfo.image_height, 3); // Force to 3 components (RGB)

		/*
		// More data set by jpeg_read_header
		cinfo.num_components;
		cinfo.jpeg_color_space;	// Colorspace of image
		cinfo.saw_JFIF_marker;		// TRUE if a JFIF APP0 marker was seen
		cinfo.JFIF_major_version;	// Version information from JFIF marker
		cinfo.JFIF_minor_version;  //
		cinfo.density_unit;		// Resolution data from JFIF marker
		cinfo.X_density;
		cinfo.Y_density;
		cinfo.saw_Adobe_marker;	// TRUE if an Adobe APP14 marker was seen
		cinfo.Adobe_transform;     // Color transform code from Adobe marker
		*/
	}
	catch (int)
	{
		jpeg_destroy_decompress(&cinfo);

		return FALSE;
	}
	////////////////////////////////////////
	// Step 4: Release JPEG decompression object 
	jpeg_destroy_decompress(&cinfo);

	return TRUE;
}

// Initialize source --- called by jpeg_read_header
// before any data is actually read.
void LLImageJPEG::decodeInitSource( j_decompress_ptr cinfo )
{
	// no work necessary here
}

// Fill the input buffer --- called whenever buffer is emptied.
boolean LLImageJPEG::decodeFillInputBuffer( j_decompress_ptr cinfo )
{
//	jpeg_source_mgr* src = cinfo->src;
//	LLImageJPEG* self = (LLImageJPEG*) cinfo->client_data;

	// Should never get here, since we provide the entire buffer up front.
	ERREXIT(cinfo, JERR_INPUT_EMPTY);

	return TRUE;
}

// Skip data --- used to skip over a potentially large amount of
// uninteresting data (such as an APPn marker).
//
// Writers of suspendable-input applications must note that skip_input_data
// is not granted the right to give a suspension return.  If the skip extends
// beyond the data currently in the buffer, the buffer can be marked empty so
// that the next read will cause a fill_input_buffer call that can suspend.
// Arranging for additional bytes to be discarded before reloading the input
// buffer is the application writer's problem.
void LLImageJPEG::decodeSkipInputData (j_decompress_ptr cinfo, long num_bytes)
{
	jpeg_source_mgr* src = cinfo->src;
//	LLImageJPEG* self = (LLImageJPEG*) cinfo->client_data;

    src->next_input_byte += (size_t) num_bytes;
    src->bytes_in_buffer -= (size_t) num_bytes;
}

void LLImageJPEG::decodeTermSource (j_decompress_ptr cinfo)
{
  // no work necessary here
}


BOOL LLImageJPEG::decode(LLImageRaw* raw_image, F32 decode_time)
{
	llassert_always(raw_image);
	
	resetLastError();
	
	// Check to make sure that this instance has been initialized with data
	if (!getData() || (0 == getDataSize()))
	{
		setLastError("LLImageJPEG trying to decode an image with no data!");
		return FALSE;
	}
	
	S32 row_stride = 0;
	U8* raw_image_data = NULL;

	////////////////////////////////////////
	// Step 1: allocate and initialize JPEG decompression object

	// This struct contains the JPEG decompression parameters and pointers to
	// working space (which is allocated as needed by the JPEG library).
	struct jpeg_decompress_struct cinfo;

	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	
	// Customize with our own callbacks
	jerr.error_exit =		&LLImageJPEG::errorExit;			// Error exit handler: does not return to caller
	jerr.emit_message =		&LLImageJPEG::errorEmitMessage;		// Conditionally emit a trace or warning message
	jerr.output_message =	&LLImageJPEG::errorOutputMessage;	// Routine that actually outputs a trace or error message
	
	//
	//try/catch will crash on Mac and Linux if LLImageJPEG::errorExit throws an error
	//so as instead, we use setjmp/longjmp to avoid this crash, which is the best we can get. --bao 
	//
	if(setjmp(sSetjmpBuffer))
	{
		jpeg_destroy_decompress(&cinfo);
		return FALSE;
	}
	try
	{
		// Now we can initialize the JPEG decompression object.
		jpeg_create_decompress(&cinfo);

		////////////////////////////////////////
		// Step 2: specify data source
		// (Code is modified version of jpeg_stdio_src();
		if (cinfo.src == NULL)
		{	
			cinfo.src = (struct jpeg_source_mgr *)
				(*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
				sizeof(struct jpeg_source_mgr));
		}
		cinfo.src->init_source	=		&LLImageJPEG::decodeInitSource;
		cinfo.src->fill_input_buffer =	&LLImageJPEG::decodeFillInputBuffer;
		cinfo.src->skip_input_data =	&LLImageJPEG::decodeSkipInputData;
		cinfo.src->resync_to_restart = jpeg_resync_to_restart; // For now, use default method, but we should be able to do better.
		cinfo.src->term_source =		&LLImageJPEG::decodeTermSource;
		cinfo.src->bytes_in_buffer =	getDataSize();
		cinfo.src->next_input_byte =	getData();
		
		////////////////////////////////////////
		// Step 3: read file parameters with jpeg_read_header()
		
		jpeg_read_header(&cinfo, TRUE);

		// We can ignore the return value from jpeg_read_header since
		//   (a) suspension is not possible with our data source, and
		//   (b) we passed TRUE to reject a tables-only JPEG file as an error.
		// See libjpeg.doc for more info.

		setSize(cinfo.image_width, cinfo.image_height, 3); // Force to 3 components (RGB)

		raw_image->resize(getWidth(), getHeight(), getComponents());
		raw_image_data = raw_image->getData();
		
		
		////////////////////////////////////////
		// Step 4: set parameters for decompression 
		cinfo.out_color_components = 3;
		cinfo.out_color_space = JCS_RGB;
   

		////////////////////////////////////////
		// Step 5: Start decompressor 
		
		jpeg_start_decompress(&cinfo);
		// We can ignore the return value since suspension is not possible
		// with our data source.
		
		// We may need to do some setup of our own at this point before reading
		// the data.  After jpeg_start_decompress() we have the correct scaled
		// output image dimensions available, as well as the output colormap
		// if we asked for color quantization.
		// In this example, we need to make an output work buffer of the right size.
    
		// JSAMPLEs per row in output buffer 
		row_stride = cinfo.output_width * cinfo.output_components;
		
		////////////////////////////////////////
		// Step 6: while (scan lines remain to be read) 
		//           jpeg_read_scanlines(...); 
		
		// Here we use the library's state variable cinfo.output_scanline as the
		// loop counter, so that we don't have to keep track ourselves.
		
		// Move pointer to last line
		raw_image_data += row_stride * (cinfo.output_height - 1);

		while (cinfo.output_scanline < cinfo.output_height)
		{
			// jpeg_read_scanlines expects an array of pointers to scanlines.
			// Here the array is only one element long, but you could ask for
			// more than one scanline at a time if that's more convenient.
			
			jpeg_read_scanlines(&cinfo, &raw_image_data, 1);
			raw_image_data -= row_stride;  // move pointer up a line
		}

		////////////////////////////////////////
		// Step 7: Finish decompression 
		jpeg_finish_decompress(&cinfo);

		////////////////////////////////////////
		// Step 8: Release JPEG decompression object 
		jpeg_destroy_decompress(&cinfo);
	}

	catch (int)
	{
		jpeg_destroy_decompress(&cinfo);
		return FALSE;
	}

	// Check to see whether any corrupt-data warnings occurred
	if( jerr.num_warnings != 0 )
	{
		// TODO: extract the warning to find out what went wrong.
		setLastError( "Unable to decode JPEG image.");
		return FALSE;
	}

	return TRUE;
}


// Initialize destination --- called by jpeg_start_compress before any data is actually written.
// static
void LLImageJPEG::encodeInitDestination ( j_compress_ptr cinfo )
{
  LLImageJPEG* self = (LLImageJPEG*) cinfo->client_data;

  cinfo->dest->next_output_byte = self->mOutputBuffer;
  cinfo->dest->free_in_buffer = self->mOutputBufferSize;
}


//  Empty the output buffer --- called whenever buffer fills up.
// 
//  In typical applications, this should write the entire output buffer
//  (ignoring the current state of next_output_byte & free_in_buffer),
//  reset the pointer & count to the start of the buffer, and return TRUE
//  indicating that the buffer has been dumped.
// 
//  In applications that need to be able to suspend compression due to output
//  overrun, a FALSE return indicates that the buffer cannot be emptied now.
//  In this situation, the compressor will return to its caller (possibly with
//  an indication that it has not accepted all the supplied scanlines).  The
//  application should resume compression after it has made more room in the
//  output buffer.  Note that there are substantial restrictions on the use of
//  suspension --- see the documentation.
// 
//  When suspending, the compressor will back up to a convenient restart point
//  (typically the start of the current MCU). next_output_byte & free_in_buffer
//  indicate where the restart point will be if the current call returns FALSE.
//  Data beyond this point will be regenerated after resumption, so do not
//  write it out when emptying the buffer externally.

boolean LLImageJPEG::encodeEmptyOutputBuffer( j_compress_ptr cinfo )
{
  LLImageJPEG* self = (LLImageJPEG*) cinfo->client_data;

  // Should very rarely happen, since our output buffer is
  // as large as the input to start out with.
  
  // Double the buffer size;
  S32 new_buffer_size = self->mOutputBufferSize * 2;
  U8* new_buffer = new U8[ new_buffer_size ];
  if (!new_buffer)
  {
  	llerrs << "Out of memory in LLImageJPEG::encodeEmptyOutputBuffer( j_compress_ptr cinfo )" << llendl;
  	return FALSE;
  }
  memcpy( new_buffer, self->mOutputBuffer, self->mOutputBufferSize );	/* Flawfinder: ignore */
  delete[] self->mOutputBuffer;
  self->mOutputBuffer = new_buffer;

  cinfo->dest->next_output_byte = self->mOutputBuffer + self->mOutputBufferSize;
  cinfo->dest->free_in_buffer = self->mOutputBufferSize;
  self->mOutputBufferSize = new_buffer_size;

  return TRUE;
}

//  Terminate destination --- called by jpeg_finish_compress
//  after all data has been written.  Usually needs to flush buffer.
// 
//  NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
//  application must deal with any cleanup that should happen even
//  for error exit.
void LLImageJPEG::encodeTermDestination( j_compress_ptr cinfo )
{
	LLImageJPEG* self = (LLImageJPEG*) cinfo->client_data;

	S32 file_bytes = (S32)(self->mOutputBufferSize - cinfo->dest->free_in_buffer);
	self->allocateData(file_bytes);

	memcpy( self->getData(), self->mOutputBuffer, file_bytes );	/* Flawfinder: ignore */
}

// static 
void LLImageJPEG::errorExit( j_common_ptr cinfo )	
{
	//LLImageJPEG* self = (LLImageJPEG*) cinfo->client_data;

	// Always display the message
	(*cinfo->err->output_message)(cinfo);

	// Let the memory manager delete any temp files
	jpeg_destroy(cinfo);

	// Return control to the setjmp point
	longjmp(sSetjmpBuffer, 1) ;
}

// Decide whether to emit a trace or warning message.
// msg_level is one of:
//   -1: recoverable corrupt-data warning, may want to abort.
//    0: important advisory messages (always display to user).
//    1: first level of tracing detail.
//    2,3,...: successively more detailed tracing messages.
// An application might override this method if it wanted to abort on warnings
// or change the policy about which messages to display.
// static 
void LLImageJPEG::errorEmitMessage( j_common_ptr cinfo, int msg_level )
{
  struct jpeg_error_mgr * err = cinfo->err;

  if (msg_level < 0) 
  {
	  // It's a warning message.  Since corrupt files may generate many warnings,
	  // the policy implemented here is to show only the first warning,
	  // unless trace_level >= 3.
	  if (err->num_warnings == 0 || err->trace_level >= 3)
	  {
		  (*err->output_message) (cinfo);
	  }
	  // Always count warnings in num_warnings.
	  err->num_warnings++;
  }
  else 
  {
	  // It's a trace message.  Show it if trace_level >= msg_level.
	  if (err->trace_level >= msg_level)
	  {
		  (*err->output_message) (cinfo);
	  }
  }
}

// static 
void LLImageJPEG::errorOutputMessage( j_common_ptr cinfo )
{
	// Create the message
	char buffer[JMSG_LENGTH_MAX];	/* Flawfinder: ignore */
	(*cinfo->err->format_message) (cinfo, buffer);

	std::string error = buffer ;
	LLImage::setLastError(error);

	BOOL is_decode = (cinfo->is_decompressor != 0);
	llwarns << "LLImageJPEG " << (is_decode ? "decode " : "encode ") << " failed: " << buffer << llendl;
}

BOOL LLImageJPEG::encode( const LLImageRaw* raw_image, F32 encode_time )
{
	llassert_always(raw_image);
	
	resetLastError();

	switch( raw_image->getComponents() )
	{
	case 1:
	case 3:
		break;
	default:
		setLastError("Unable to encode a JPEG image that doesn't have 1 or 3 components.");
		return FALSE;
	}

	setSize(raw_image->getWidth(), raw_image->getHeight(), raw_image->getComponents());

	// Allocate a temporary buffer big enough to hold the entire compressed image (and then some)
	// (Note: we make it bigger in emptyOutputBuffer() if we need to)
	delete[] mOutputBuffer;
	mOutputBufferSize = getWidth() * getHeight() * getComponents() + 1024;
	mOutputBuffer = new U8[ mOutputBufferSize ];

	const U8* raw_image_data = NULL;
	S32 row_stride = 0;

	////////////////////////////////////////
	// Step 1: allocate and initialize JPEG compression object

	// This struct contains the JPEG compression parameters and pointers to
	// working space (which is allocated as needed by the JPEG library).
	struct jpeg_compress_struct cinfo;
	cinfo.client_data = this;

	// We have to set up the error handler first, in case the initialization
	// step fails.  (Unlikely, but it could happen if you are out of memory.)
	// This routine fills in the contents of struct jerr, and returns jerr's
	// address which we place into the link field in cinfo.
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);

	// Customize with our own callbacks
	jerr.error_exit =		&LLImageJPEG::errorExit;			// Error exit handler: does not return to caller
	jerr.emit_message =		&LLImageJPEG::errorEmitMessage;		// Conditionally emit a trace or warning message
	jerr.output_message =	&LLImageJPEG::errorOutputMessage;	// Routine that actually outputs a trace or error message

	//
	//try/catch will crash on Mac and Linux if LLImageJPEG::errorExit throws an error
	//so as instead, we use setjmp/longjmp to avoid this crash, which is the best we can get. --bao 
	//
	if( setjmp(sSetjmpBuffer) ) 
	{
		// If we get here, the JPEG code has signaled an error.
		// We need to clean up the JPEG object, close the input file, and return.
		jpeg_destroy_compress(&cinfo);
		delete[] mOutputBuffer;
		mOutputBuffer = NULL;
		mOutputBufferSize = 0;
		return FALSE;
	}

	try
	{

		// Now we can initialize the JPEG compression object.
		jpeg_create_compress(&cinfo);

		////////////////////////////////////////
		// Step 2: specify data destination
		// (code is a modified form of jpeg_stdio_dest() )
		if( cinfo.dest == NULL)
		{	
			cinfo.dest = (struct jpeg_destination_mgr *)
				(*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
				sizeof(struct jpeg_destination_mgr));
		}
		cinfo.dest->next_output_byte =		mOutputBuffer;		// => next byte to write in buffer
		cinfo.dest->free_in_buffer =		mOutputBufferSize;	// # of byte spaces remaining in buffer
		cinfo.dest->init_destination =		&LLImageJPEG::encodeInitDestination;
		cinfo.dest->empty_output_buffer =	&LLImageJPEG::encodeEmptyOutputBuffer;
		cinfo.dest->term_destination =		&LLImageJPEG::encodeTermDestination;

		////////////////////////////////////////
		// Step 3: set parameters for compression 
		//
		// First we supply a description of the input image.
		// Four fields of the cinfo struct must be filled in:
		
		cinfo.image_width = getWidth(); 	// image width and height, in pixels 
		cinfo.image_height = getHeight();

		switch( getComponents() )
		{
		case 1:
			cinfo.input_components = 1;		// # of color components per pixel
			cinfo.in_color_space = JCS_GRAYSCALE; // colorspace of input image
			break;
		case 3:
			cinfo.input_components = 3;		// # of color components per pixel
			cinfo.in_color_space = JCS_RGB; // colorspace of input image
			break;
		default:
			setLastError("Unable to encode a JPEG image that doesn't have 1 or 3 components.");
			return FALSE;
		}

		// Now use the library's routine to set default compression parameters.
		// (You must set at least cinfo.in_color_space before calling this,
		// since the defaults depend on the source color space.)
		jpeg_set_defaults(&cinfo);

		// Now you can set any non-default parameters you wish to.
		jpeg_set_quality(&cinfo, mEncodeQuality, TRUE );  // limit to baseline-JPEG values

		////////////////////////////////////////
		// Step 4: Start compressor 
		//
		// TRUE ensures that we will write a complete interchange-JPEG file.
		// Pass TRUE unless you are very sure of what you're doing.
   
		jpeg_start_compress(&cinfo, TRUE);

		////////////////////////////////////////
		// Step 5: while (scan lines remain to be written) 
		//            jpeg_write_scanlines(...); 

		// Here we use the library's state variable cinfo.next_scanline as the
		// loop counter, so that we don't have to keep track ourselves.
		// To keep things simple, we pass one scanline per call; you can pass
		// more if you wish, though.
   
		row_stride = getWidth() * getComponents();	// JSAMPLEs per row in image_buffer

		// NOTE: For compatibility with LLImage, we need to invert the rows.
		raw_image_data = raw_image->getData();
		
		const U8* last_row_data = raw_image_data + (getHeight()-1) * row_stride;

		JSAMPROW row_pointer[1];				// pointer to JSAMPLE row[s]
		while (cinfo.next_scanline < cinfo.image_height) 
		{
			// jpeg_write_scanlines expects an array of pointers to scanlines.
			// Here the array is only one element long, but you could pass
			// more than one scanline at a time if that's more convenient.

			//Ugly const uncast here (jpeg_write_scanlines should take a const* but doesn't)
			//row_pointer[0] = (JSAMPROW)(raw_image_data + (cinfo.next_scanline * row_stride));
			row_pointer[0] = (JSAMPROW)(last_row_data - (cinfo.next_scanline * row_stride));

			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}

		////////////////////////////////////////
		//   Step 6: Finish compression 
		jpeg_finish_compress(&cinfo);

		// After finish_compress, we can release the temp output buffer. 
		delete[] mOutputBuffer;
		mOutputBuffer = NULL;
		mOutputBufferSize = 0;

		////////////////////////////////////////
		//   Step 7: release JPEG compression object 
		jpeg_destroy_compress(&cinfo);
	}

	catch(int)
	{
		jpeg_destroy_compress(&cinfo);
		delete[] mOutputBuffer;
		mOutputBuffer = NULL;
		mOutputBufferSize = 0;
		return FALSE;
	}

	return TRUE;
}
