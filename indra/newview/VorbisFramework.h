/** 
 * @file VorbisFramework.h
 * @author Dave Camp
 * @date Fri Oct 10 2003
 * @brief For the Macview project
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifdef __cplusplus
extern "C" {
#endif

#include "ogg/ogg.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisenc.h"

extern int      mac_vorbis_analysis(vorbis_block *vb,ogg_packet *op);

extern int      mac_vorbis_analysis_headerout(vorbis_dsp_state *v,
					  vorbis_comment *vc,
					  ogg_packet *op,
					  ogg_packet *op_comm,
					  ogg_packet *op_code);

extern int mac_vorbis_analysis_init(vorbis_dsp_state *v,vorbis_info *vi);

extern int mac_vorbis_encode_ctl(vorbis_info *vi,int number,void *arg);

extern int mac_vorbis_encode_setup_init(vorbis_info *vi);

extern int mac_vorbis_encode_setup_managed(vorbis_info *vi,
				       long channels,
				       long rate,
				       
				       long max_bitrate,
				       long nominal_bitrate,
				       long min_bitrate);

extern void     mac_vorbis_info_init(vorbis_info *vi);
extern void     mac_vorbis_info_clear(vorbis_info *vi);
extern void     mac_vorbis_comment_init(vorbis_comment *vc);
extern void     mac_vorbis_comment_clear(vorbis_comment *vc);
extern int      mac_vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb);
extern int      mac_vorbis_block_clear(vorbis_block *vb);
extern void     mac_vorbis_dsp_clear(vorbis_dsp_state *v);
extern float  **mac_vorbis_analysis_buffer(vorbis_dsp_state *v,int vals);
extern int      mac_vorbis_analysis_wrote(vorbis_dsp_state *v,int vals);
extern int      mac_vorbis_analysis_blockout(vorbis_dsp_state *v,vorbis_block *vb);

extern int		mac_ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op);
extern int      mac_ogg_stream_init(ogg_stream_state *os,int serialno);
extern int      mac_ogg_stream_flush(ogg_stream_state *os, ogg_page *og);
extern int      mac_ogg_stream_pageout(ogg_stream_state *os, ogg_page *og);
extern int      mac_ogg_page_eos(ogg_page *og);
extern int      mac_ogg_stream_clear(ogg_stream_state *os);


#ifdef __cplusplus
}
#endif
