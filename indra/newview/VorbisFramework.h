/** 
 * @file VorbisFramework.h
 * @author Dave Camp
 * @date Fri Oct 10 2003
 * @brief For the Macview project
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
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
