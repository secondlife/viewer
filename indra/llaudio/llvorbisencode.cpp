/** 
 * @file vorbisencode.cpp
 * @brief Vorbis encoding routine routine for Indra.
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

#include "linden_common.h"

#include "vorbis/vorbisenc.h"

#include "llvorbisencode.h"
#include "llerror.h"
#include "llrand.h"
#include "llmath.h"
#include "llapr.h"

//#if LL_DARWIN
// MBW -- XXX -- Getting rid of SecondLifeVorbis for now -- no fmod means no name collisions.
#if 0
#include "VorbisFramework.h"

#define vorbis_analysis				mac_vorbis_analysis
#define vorbis_analysis_headerout	mac_vorbis_analysis_headerout
#define vorbis_analysis_init		mac_vorbis_analysis_init
#define vorbis_encode_ctl			mac_vorbis_encode_ctl
#define vorbis_encode_setup_init	mac_vorbis_encode_setup_init
#define vorbis_encode_setup_managed	mac_vorbis_encode_setup_managed

#define	vorbis_info_init			mac_vorbis_info_init
#define	vorbis_info_clear			mac_vorbis_info_clear
#define	vorbis_comment_init			mac_vorbis_comment_init
#define	vorbis_comment_clear		mac_vorbis_comment_clear
#define	vorbis_block_init			mac_vorbis_block_init
#define	vorbis_block_clear			mac_vorbis_block_clear
#define	vorbis_dsp_clear			mac_vorbis_dsp_clear
#define	vorbis_analysis_buffer		mac_vorbis_analysis_buffer
#define	vorbis_analysis_wrote		mac_vorbis_analysis_wrote
#define	vorbis_analysis_blockout	mac_vorbis_analysis_blockout

#define ogg_stream_packetin			mac_ogg_stream_packetin
#define	ogg_stream_init				mac_ogg_stream_init
#define	ogg_stream_flush			mac_ogg_stream_flush
#define	ogg_stream_pageout			mac_ogg_stream_pageout
#define	ogg_page_eos				mac_ogg_page_eos
#define	ogg_stream_clear			mac_ogg_stream_clear

#endif

S32 check_for_invalid_wav_formats(const std::string& in_fname, std::string& error_msg)
{
	U16 num_channels = 0;
	U32 sample_rate = 0;
	U32 bits_per_sample = 0;
	U32 physical_file_size = 0;
	U32 chunk_length = 0;
	U32 raw_data_length = 0;
	U32 bytes_per_sec = 0;
	BOOL uncompressed_pcm = FALSE;

	unsigned char wav_header[44];		/*Flawfinder: ignore*/

	error_msg.clear();

	//********************************
	LLAPRFile infile ;
    infile.open(in_fname,LL_APR_RB);
	//********************************
	if (!infile.getFileHandle())
	{
		error_msg = "CannotUploadSoundFile";
		return(LLVORBISENC_SOURCE_OPEN_ERR);
	}

	infile.read(wav_header, 44);
	physical_file_size = infile.seek(APR_END,0);

	if (strncmp((char *)&(wav_header[0]),"RIFF",4))
	{
		error_msg = "SoundFileNotRIFF";
		return(LLVORBISENC_WAV_FORMAT_ERR);
	}

	if (strncmp((char *)&(wav_header[8]),"WAVE",4))
	{
		error_msg = "SoundFileNotRIFF";
		return(LLVORBISENC_WAV_FORMAT_ERR);
	}
	
	// parse the chunks
	
	U32 file_pos = 12;  // start at the first chunk (usually fmt but not always)
	
	while ((file_pos + 8)< physical_file_size)
	{
		infile.seek(APR_SET,file_pos);
		infile.read(wav_header, 44);

		chunk_length = ((U32) wav_header[7] << 24) 
			+ ((U32) wav_header[6] << 16) 
			+ ((U32) wav_header[5] << 8) 
			+ wav_header[4];

		if (chunk_length > physical_file_size - file_pos - 4)
		{
			infile.close();
			error_msg = "SoundFileInvalidChunkSize";
			return(LLVORBISENC_CHUNK_SIZE_ERR);
		}

//		llinfos << "chunk found: '" << wav_header[0] << wav_header[1] << wav_header[2] << wav_header[3] << "'" << llendl;

		if (!(strncmp((char *)&(wav_header[0]),"fmt ",4)))
		{
			if ((wav_header[8] == 0x01) && (wav_header[9] == 0x00))
			{
				uncompressed_pcm = TRUE;
			}
			num_channels = ((U16) wav_header[11] << 8) + wav_header[10];
			sample_rate = ((U32) wav_header[15] << 24) 
				+ ((U32) wav_header[14] << 16) 
				+ ((U32) wav_header[13] << 8) 
				+ wav_header[12];
			bits_per_sample = ((U16) wav_header[23] << 8) + wav_header[22];
			bytes_per_sec = ((U32) wav_header[19] << 24) 
				+ ((U32) wav_header[18] << 16) 
				+ ((U32) wav_header[17] << 8) 
				+ wav_header[16];
		}
		else if (!(strncmp((char *)&(wav_header[0]),"data",4)))
		{
			raw_data_length = chunk_length;			
		}
		file_pos += (chunk_length + 8);
		chunk_length = 0;
	} 
	//****************
	infile.close();
	//****************

	if (!uncompressed_pcm)
	{	
		 error_msg = "SoundFileNotPCM";
		  return(LLVORBISENC_PCM_FORMAT_ERR);
	}
	
	if ((num_channels < 1) || (num_channels > LLVORBIS_CLIP_MAX_CHANNELS))
	{	
		error_msg = "SoundFileInvalidChannelCount";
		return(LLVORBISENC_MULTICHANNEL_ERR);
	}

	if (sample_rate != LLVORBIS_CLIP_SAMPLE_RATE)
	{	
		error_msg = "SoundFileInvalidSampleRate";
		return(LLVORBISENC_UNSUPPORTED_SAMPLE_RATE);
	}
	
	if ((bits_per_sample != 16) && (bits_per_sample != 8))
	{		 
		error_msg = "SoundFileInvalidWordSize";
		return(LLVORBISENC_UNSUPPORTED_WORD_SIZE);
	}

	if (!raw_data_length)
	{
		error_msg = "SoundFileInvalidHeader";
		return(LLVORBISENC_CLIP_TOO_LONG);		 
	}

	F32 clip_length = (F32)raw_data_length/(F32)bytes_per_sec;
		
	if (clip_length > LLVORBIS_CLIP_MAX_TIME)
	{
		error_msg = "SoundFileInvalidTooLong";
		return(LLVORBISENC_CLIP_TOO_LONG);		 
	}

    return(LLVORBISENC_NOERR);
}

S32 encode_vorbis_file(const std::string& in_fname, const std::string& out_fname)
{
#define READ_BUFFER 1024
	unsigned char readbuffer[READ_BUFFER*4+44];   /* out of the data segment, not the stack */	/*Flawfinder: ignore*/

	ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
	ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet       op; /* one raw packet of data for decode */
	
	vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
	vorbis_comment   vc; /* struct that stores all the user comments */
	
	vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
	vorbis_block     vb; /* local working space for packet->PCM decode */
	
	int eos=0;
	int result;

	U16 num_channels = 0;
	U32 sample_rate = 0;
	U32 bits_per_sample = 0;

	S32 format_error = 0;
	std::string error_msg;
	if ((format_error = check_for_invalid_wav_formats(in_fname, error_msg)))
	{
		llwarns << error_msg << ": " << in_fname << llendl;
		return(format_error);
	}

#if 1
	unsigned char wav_header[44];	/*Flawfinder: ignore*/

	S32 data_left = 0;

	LLAPRFile infile ;
	infile.open(in_fname,LL_APR_RB);
	if (!infile.getFileHandle())
	{
		llwarns << "Couldn't open temporary ogg file for writing: " << in_fname
			<< llendl;
		return(LLVORBISENC_SOURCE_OPEN_ERR);
	}

	LLAPRFile outfile ;
	outfile.open(out_fname,LL_APR_WPB);
	if (!outfile.getFileHandle())
	{
		llwarns << "Couldn't open upload sound file for reading: " << in_fname
			<< llendl;
		return(LLVORBISENC_DEST_OPEN_ERR);
	}
	
	 // parse the chunks
	 U32 chunk_length = 0;
	 U32 file_pos = 12;  // start at the first chunk (usually fmt but not always)
	 
	 while (infile.eof() != APR_EOF)
	 {
		 infile.seek(APR_SET,file_pos);
		 infile.read(wav_header, 44);
		 
		 chunk_length = ((U32) wav_header[7] << 24) 
			 + ((U32) wav_header[6] << 16) 
			 + ((U32) wav_header[5] << 8) 
			 + wav_header[4];
		 
//		 llinfos << "chunk found: '" << wav_header[0] << wav_header[1] << wav_header[2] << wav_header[3] << "'" << llendl;
		 
		 if (!(strncmp((char *)&(wav_header[0]),"fmt ",4)))
		 {
			 num_channels = ((U16) wav_header[11] << 8) + wav_header[10];
			 sample_rate = ((U32) wav_header[15] << 24) 
				 + ((U32) wav_header[14] << 16) 
				 + ((U32) wav_header[13] << 8) 
				 + wav_header[12];
			 bits_per_sample = ((U16) wav_header[23] << 8) + wav_header[22];
		 }
	 	 else if (!(strncmp((char *)&(wav_header[0]),"data",4)))
		 {
			 infile.seek(APR_SET,file_pos+8);
			 // leave the file pointer at the beginning of the data chunk data
			 data_left = chunk_length;			
			 break;
		 }
		 file_pos += (chunk_length + 8);
		 chunk_length = 0;
	 } 
	 

	 /********** Encode setup ************/
	 
	 /* choose an encoding mode */
	 /* (mode 0: 44kHz stereo uncoupled, roughly 128kbps VBR) */
	 vorbis_info_init(&vi);

	 // always encode to mono

	 // SL-52913 & SL-53779 determined this quality level to be our 'good
	 // enough' general-purpose quality level with a nice low bitrate.
	 // Equivalent to oggenc -q0.5
	 F32 quality = 0.05f;
//	 quality = (bitrate==128000 ? 0.4f : 0.1);

//	 if (vorbis_encode_init(&vi, /* num_channels */ 1 ,sample_rate, -1, bitrate, -1))
	 if (vorbis_encode_init_vbr(&vi, /* num_channels */ 1 ,sample_rate, quality))
//	 if (vorbis_encode_setup_managed(&vi,1,sample_rate,-1,bitrate,-1) ||
//		vorbis_encode_ctl(&vi,OV_ECTL_RATEMANAGE_AVG,NULL) ||
//		vorbis_encode_setup_init(&vi))
	{
		llwarns << "unable to initialize vorbis codec at quality " << quality << llendl;
		//		llwarns << "unable to initialize vorbis codec at bitrate " << bitrate << llendl;
		return(LLVORBISENC_DEST_OPEN_ERR);
	}
	 
	 /* add a comment */
	 vorbis_comment_init(&vc);
//	 vorbis_comment_add(&vc,"Linden");
	 
	 /* set up the analysis state and auxiliary encoding storage */
	 vorbis_analysis_init(&vd,&vi);
	 vorbis_block_init(&vd,&vb);
	 
	 /* set up our packet->stream encoder */
	 /* pick a random serial number; that way we can more likely build
		chained streams just by concatenation */
	 ogg_stream_init(&os, ll_rand());
	 
	 /* Vorbis streams begin with three headers; the initial header (with
		most of the codec setup parameters) which is mandated by the Ogg
		bitstream spec.  The second header holds any comment fields.  The
		third header holds the bitstream codebook.  We merely need to
		make the headers, then pass them to libvorbis one at a time;
		libvorbis handles the additional Ogg bitstream constraints */
	 
	 {
		 ogg_packet header;
		 ogg_packet header_comm;
		 ogg_packet header_code;
		 
		 vorbis_analysis_headerout(&vd,&vc,&header,&header_comm,&header_code);
		 ogg_stream_packetin(&os,&header); /* automatically placed in its own
											  page */
		 ogg_stream_packetin(&os,&header_comm);
		 ogg_stream_packetin(&os,&header_code);
		 
		 /* We don't have to write out here, but doing so makes streaming 
		  * much easier, so we do, flushing ALL pages. This ensures the actual
		  * audio data will start on a new page
		  */
		 while(!eos){
			 int result=ogg_stream_flush(&os,&og);
			 if(result==0)break;
			 outfile.write(og.header, og.header_len);
			 outfile.write(og.body, og.body_len);
		 }
		 
	 }
	 
	 
	 while(!eos)
	 {
		 long bytes_per_sample = bits_per_sample/8;

		 long bytes=(long)infile.read(readbuffer,llclamp((S32)(READ_BUFFER*num_channels*bytes_per_sample),0,data_left)); /* stereo hardwired here */
		 
		 if (bytes==0)
		 {
			 /* end of file.  this can be done implicitly in the mainline,
				but it's easier to see here in non-clever fashion.
				Tell the library we're at end of stream so that it can handle
				the last frame and mark end of stream in the output properly */

			 vorbis_analysis_wrote(&vd,0);
//			 eos = 1;
			 
		 }
		 else
		 {
			 long i;
			 long samples;
			 int temp;

			 data_left -= bytes;
             /* data to encode */
			 
			 /* expose the buffer to submit data */
			 float **buffer=vorbis_analysis_buffer(&vd,READ_BUFFER);
			
			 i = 0;
			 samples = bytes / (num_channels * bytes_per_sample);

			 if (num_channels == 2)
			 {
				 if (bytes_per_sample == 2)
				 {
					 /* uninterleave samples */
					 for(i=0; i<samples ;i++)
					 {
					 	 temp =  ((signed char *)readbuffer)[i*4+1];	/*Flawfinder: ignore*/
						 temp += ((signed char *)readbuffer)[i*4+3];	/*Flawfinder: ignore*/
						 temp <<= 8;
						 temp += readbuffer[i*4];
						 temp += readbuffer[i*4+2];

						 buffer[0][i] = ((float)temp) / 65536.f;
					 }
				 }
				 else // presume it's 1 byte per which is unsigned (F#@%ing wav "standard")
				 {
					 /* uninterleave samples */
					 for(i=0; i<samples ;i++)
					 {
					 	 temp  = readbuffer[i*2+0];
						 temp += readbuffer[i*2+1];
						 temp -= 256;
						 buffer[0][i] = ((float)temp) / 256.f;
					 }
				 } 
			 }
			 else if (num_channels == 1)
			 {
				 if (bytes_per_sample == 2)
				 {
					 for(i=0; i < samples ;i++)
					 {
					 	 temp = ((signed char*)readbuffer)[i*2+1];
						 temp <<= 8;
						 temp += readbuffer[i*2];
						 buffer[0][i] = ((float)temp) / 32768.f;
					 }
				 }
				 else // presume it's 1 byte per which is unsigned (F#@%ing wav "standard")
				 {
					 for(i=0; i < samples ;i++)
					 {
						 temp = readbuffer[i];
						 temp -= 128;
						 buffer[0][i] = ((float)temp) / 128.f;
					 }
				 }
			 }
				
			 /* tell the library how much we actually submitted */
			 vorbis_analysis_wrote(&vd,i);
		 }
			 
		 /* vorbis does some data preanalysis, then divvies up blocks for
			more involved (potentially parallel) processing.  Get a single
			block for encoding now */
		 while(vorbis_analysis_blockout(&vd,&vb)==1)
		 {
			 
			 /* analysis */
			/* Do the main analysis, creating a packet */
			vorbis_analysis(&vb, NULL);
			vorbis_bitrate_addblock(&vb);

			while(vorbis_bitrate_flushpacket(&vd, &op)) 
			{
			 
			 /* weld the packet into the bitstream */
			 ogg_stream_packetin(&os,&op);
			 
			 /* write out pages (if any) */
			 while(!eos)
			 {
				 result = ogg_stream_pageout(&os,&og);

				 if(result==0)
				 	break;

				 outfile.write(og.header, og.header_len);
				 outfile.write(og.body, og.body_len);
				 
				 /* this could be set above, but for illustrative purposes, I do
					it here (to show that vorbis does know where the stream ends) */
				 
				 if(ogg_page_eos(&og))
				 	eos=1;
				 
			 }
			}
		 }
	 }
	 
	 
	 
	 /* clean up and exit.  vorbis_info_clear() must be called last */
	 
	 ogg_stream_clear(&os);
	 vorbis_block_clear(&vb);
	 vorbis_dsp_clear(&vd);
	 vorbis_comment_clear(&vc);
	 vorbis_info_clear(&vi);
	 
	 /* ogg_page and ogg_packet structs always point to storage in
		libvorbis.  They're never freed or manipulated directly */
	 
//	 fprintf(stderr,"Vorbis encoding: Done.\n");
	 llinfos << "Vorbis encoding: Done." << llendl;
	 
#endif
	 return(LLVORBISENC_NOERR);
	 
}
