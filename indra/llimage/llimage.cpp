/** 
 * @file llimage.cpp
 * @brief Base class for images.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llimageworker.h"
#include "llimage.h"

#include "llmath.h"
#include "v4coloru.h"

#include "llimagebmp.h"
#include "llimagetga.h"
#include "llimagej2c.h"
#include "llimagejpeg.h"
#include "llimagepng.h"
#include "llimagedxt.h"
#include "llmemory.h"

#include <boost/preprocessor.hpp>

//..................................................................................
//..................................................................................
// Helper macrose's for generate cycle unwrap templates
//..................................................................................
#define _UNROL_GEN_TPL_arg_0(arg)
#define _UNROL_GEN_TPL_arg_1(arg) arg

#define _UNROL_GEN_TPL_comma_0
#define _UNROL_GEN_TPL_comma_1 BOOST_PP_COMMA()
//..................................................................................
#define _UNROL_GEN_TPL_ARGS_macro(z,n,seq) \
	BOOST_PP_CAT(_UNROL_GEN_TPL_arg_, BOOST_PP_MOD(n, 2))(BOOST_PP_SEQ_ELEM(n, seq)) BOOST_PP_CAT(_UNROL_GEN_TPL_comma_, BOOST_PP_AND(BOOST_PP_MOD(n, 2), BOOST_PP_NOT_EQUAL(BOOST_PP_INC(n), BOOST_PP_SEQ_SIZE(seq))))

#define _UNROL_GEN_TPL_ARGS(seq) \
	BOOST_PP_REPEAT(BOOST_PP_SEQ_SIZE(seq), _UNROL_GEN_TPL_ARGS_macro, seq)
//..................................................................................

#define _UNROL_GEN_TPL_TYPE_ARGS_macro(z,n,seq) \
	BOOST_PP_SEQ_ELEM(n, seq) BOOST_PP_CAT(_UNROL_GEN_TPL_comma_, BOOST_PP_AND(BOOST_PP_MOD(n, 2), BOOST_PP_NOT_EQUAL(BOOST_PP_INC(n), BOOST_PP_SEQ_SIZE(seq))))

#define _UNROL_GEN_TPL_TYPE_ARGS(seq) \
	BOOST_PP_REPEAT(BOOST_PP_SEQ_SIZE(seq), _UNROL_GEN_TPL_TYPE_ARGS_macro, seq)
//..................................................................................
#define _UNROLL_GEN_TPL_foreach_ee(z, n, seq) \
	executor<n>(_UNROL_GEN_TPL_ARGS(seq));

#define _UNROLL_GEN_TPL(name, args_seq, operation, spec) \
	template<> struct name<spec> { \
	private: \
		template<S32 _idx> inline void executor(_UNROL_GEN_TPL_TYPE_ARGS(args_seq)) { \
			BOOST_PP_SEQ_ENUM(operation) ; \
		} \
	public: \
		inline void operator()(_UNROL_GEN_TPL_TYPE_ARGS(args_seq)) { \
			BOOST_PP_REPEAT(spec, _UNROLL_GEN_TPL_foreach_ee, args_seq) \
		} \
};
//..................................................................................
#define _UNROLL_GEN_TPL_foreach_seq_macro(r, data, elem) \
	_UNROLL_GEN_TPL(BOOST_PP_SEQ_ELEM(0, data), BOOST_PP_SEQ_ELEM(1, data), BOOST_PP_SEQ_ELEM(2, data), elem)

#define UNROLL_GEN_TPL(name, args_seq, operation, spec_seq) \
	/*general specialization - should not be implemented!*/ \
	template<U8> struct name { inline void operator()(_UNROL_GEN_TPL_TYPE_ARGS(args_seq)) { /*static_assert(!"Should not be instantiated.");*/  } }; \
	BOOST_PP_SEQ_FOR_EACH(_UNROLL_GEN_TPL_foreach_seq_macro, (name)(args_seq)(operation), spec_seq)
//..................................................................................
//..................................................................................


//..................................................................................
// Generated unrolling loop templates with specializations
//..................................................................................
//example: for(c = 0; c < ch; ++c) comp[c] = cx[0] = 0;
UNROLL_GEN_TPL(uroll_zeroze_cx_comp, (S32 *)(cx)(S32 *)(comp), (cx[_idx] = comp[_idx] = 0), (1)(3)(4));
//example: for(c = 0; c < ch; ++c) comp[c] >>= 4;
UNROLL_GEN_TPL(uroll_comp_rshftasgn_constval, (S32 *)(comp)(const S32)(cval), (comp[_idx] >>= cval), (1)(3)(4));
//example: for(c = 0; c < ch; ++c) comp[c] = (cx[c] >> 5) * yap;
UNROLL_GEN_TPL(uroll_comp_asgn_cx_rshft_cval_all_mul_val, (S32 *)(comp)(S32 *)(cx)(const S32)(cval)(S32)(val), (comp[_idx] = (cx[_idx] >> cval) * val), (1)(3)(4));
//example: for(c = 0; c < ch; ++c) comp[c] += (cx[c] >> 5) * Cy;
UNROLL_GEN_TPL(uroll_comp_plusasgn_cx_rshft_cval_all_mul_val, (S32 *)(comp)(S32 *)(cx)(const S32)(cval)(S32)(val), (comp[_idx] += (cx[_idx] >> cval) * val), (1)(3)(4));
//example: for(c = 0; c < ch; ++c) comp[c] += pix[c] * info.xapoints[x];
UNROLL_GEN_TPL(uroll_inp_plusasgn_pix_mul_val, (S32 *)(comp)(const U8 *)(pix)(S32)(val), (comp[_idx] += pix[_idx] * val), (1)(3)(4));
//example: for(c = 0; c < ch; ++c) cx[c] = pix[c] * info.xapoints[x];
UNROLL_GEN_TPL(uroll_inp_asgn_pix_mul_val, (S32 *)(comp)(const U8 *)(pix)(S32)(val), (comp[_idx] = pix[_idx] * val), (1)(3)(4));
//example: for(c = 0; c < ch; ++c) comp[c] = ((cx[c] * info.yapoints[y]) + (comp[c] * (256 - info.yapoints[y]))) >> 16;
UNROLL_GEN_TPL(uroll_comp_asgn_cx_mul_apoint_plus_comp_mul_inv_apoint_allshifted_16_r, (S32 *)(comp)(S32 *)(cx)(S32)(apoint), (comp[_idx] = ((cx[_idx] * apoint) + (comp[_idx] * (256 - apoint))) >> 16), (1)(3)(4));
//example: for(c = 0; c < ch; ++c) comp[c] = (comp[c] + pix[c] * info.yapoints[y]) >> 8;
UNROLL_GEN_TPL(uroll_comp_asgn_comp_plus_pix_mul_apoint_allshifted_8_r, (S32 *)(comp)(const U8 *)(pix)(S32)(apoint), (comp[_idx] = (comp[_idx] + pix[_idx] * apoint) >> 8), (1)(3)(4));
//example: for(c = 0; c < ch; ++c) comp[c] = ((comp[c]*(256 - info.xapoints[x])) + ((cx[c] * info.xapoints[x]))) >> 12;
UNROLL_GEN_TPL(uroll_comp_asgn_comp_mul_inv_apoint_plus_cx_mul_apoint_allshifted_12_r, (S32 *)(comp)(S32)(apoint)(S32 *)(cx), (comp[_idx] = ((comp[_idx] * (256-apoint)) + (cx[_idx] * apoint)) >> 12), (1)(3)(4));
//example: for(c = 0; c < ch; ++c) *dptr++ = comp[c]&0xff;
UNROLL_GEN_TPL(uroll_uref_dptr_inc_asgn_comp_and_ff, (U8 *&)(dptr)(S32 *)(comp), (*dptr++ = comp[_idx]&0xff), (1)(3)(4));
//example: for(c = 0; c < ch; ++c) *dptr++ = (sptr[info.xpoints[x]*ch + c])&0xff;
UNROLL_GEN_TPL(uroll_uref_dptr_inc_asgn_sptr_apoint_plus_idx_alland_ff, (U8 *&)(dptr)(const U8 *)(sptr)(S32)(apoint), (*dptr++ = sptr[apoint + _idx]&0xff), (1)(3)(4));
//example: for(c = 0; c < ch; ++c) *dptr++ = (comp[c]>>10)&0xff;
UNROLL_GEN_TPL(uroll_uref_dptr_inc_asgn_comp_rshft_cval_and_ff, (U8 *&)(dptr)(S32 *)(comp)(const S32)(cval), (*dptr++ = (comp[_idx]>>cval)&0xff), (1)(3)(4));
//..................................................................................


template<U8 ch>
struct scale_info 
{
public:
	std::vector<S32> xpoints;
	std::vector<const U8*> ystrides;
	std::vector<S32> xapoints, yapoints;
	S32 xup_yup;

public:
	//unrolling loop types declaration
	typedef uroll_zeroze_cx_comp<ch>														uroll_zeroze_cx_comp_t;
	typedef uroll_comp_rshftasgn_constval<ch>												uroll_comp_rshftasgn_constval_t;
	typedef uroll_comp_asgn_cx_rshft_cval_all_mul_val<ch>									uroll_comp_asgn_cx_rshft_cval_all_mul_val_t;
	typedef uroll_comp_plusasgn_cx_rshft_cval_all_mul_val<ch>								uroll_comp_plusasgn_cx_rshft_cval_all_mul_val_t;
	typedef uroll_inp_plusasgn_pix_mul_val<ch>												uroll_inp_plusasgn_pix_mul_val_t;
	typedef uroll_inp_asgn_pix_mul_val<ch>													uroll_inp_asgn_pix_mul_val_t;
	typedef uroll_comp_asgn_cx_mul_apoint_plus_comp_mul_inv_apoint_allshifted_16_r<ch>		uroll_comp_asgn_cx_mul_apoint_plus_comp_mul_inv_apoint_allshifted_16_r_t;
	typedef uroll_comp_asgn_comp_plus_pix_mul_apoint_allshifted_8_r<ch>						uroll_comp_asgn_comp_plus_pix_mul_apoint_allshifted_8_r_t;
	typedef uroll_comp_asgn_comp_mul_inv_apoint_plus_cx_mul_apoint_allshifted_12_r<ch>		uroll_comp_asgn_comp_mul_inv_apoint_plus_cx_mul_apoint_allshifted_12_r_t;
	typedef uroll_uref_dptr_inc_asgn_comp_and_ff<ch>										uroll_uref_dptr_inc_asgn_comp_and_ff_t;
	typedef uroll_uref_dptr_inc_asgn_sptr_apoint_plus_idx_alland_ff<ch>						uroll_uref_dptr_inc_asgn_sptr_apoint_plus_idx_alland_ff_t;
	typedef uroll_uref_dptr_inc_asgn_comp_rshft_cval_and_ff<ch>								uroll_uref_dptr_inc_asgn_comp_rshft_cval_and_ff_t;

public:
	scale_info(const U8 *src, U32 srcW, U32 srcH, U32 dstW, U32 dstH, U32 srcStride)
		: xup_yup((dstW >= srcW) + ((dstH >= srcH) << 1))
	{
		calc_x_points(srcW, dstW);
		calc_y_strides(src, srcStride, srcH, dstH);
		calc_aa_points(srcW, dstW, xup_yup&1, xapoints);
		calc_aa_points(srcH, dstH, xup_yup&2, yapoints);
	}

private:
	//...........................................................................................
	void calc_x_points(U32 srcW, U32 dstW)
	{
		xpoints.resize(dstW+1);

		S32 val = dstW >= srcW ? 0x8000 * srcW / dstW - 0x8000 : 0;
		S32 inc = (srcW << 16) / dstW;

		for(U32 i = 0, j = 0; i < dstW; ++i, ++j, val += inc)
		{
			xpoints[j] = llmax(0, val >> 16);
		}
	}
	//...........................................................................................
	void calc_y_strides(const U8 *src, U32 srcStride, U32 srcH, U32 dstH)
	{
		ystrides.resize(dstH+1);

		S32 val = dstH >= srcH ? 0x8000 * srcH / dstH - 0x8000 : 0;
		S32 inc = (srcH << 16) / dstH;

		for(U32 i = 0, j = 0; i < dstH; ++i, ++j, val += inc)
		{
			ystrides[j] = src + llmax(0, val >> 16) * srcStride;
		}
	}
	//...........................................................................................
	void calc_aa_points(U32 srcSz, U32 dstSz, bool scale_up, std::vector<S32> &vp)
	{
		vp.resize(dstSz);

		if(scale_up)
		{
			S32 val = 0x8000 * srcSz / dstSz - 0x8000;
			S32 inc = (srcSz << 16) / dstSz;
			U32 pos;

			for(U32 i = 0, j = 0; i < dstSz; ++i, ++j, val += inc)
			{
				pos = val >> 16;

				if (pos >= (srcSz - 1))
					vp[j] = 0;
				else
					vp[j] = (val >> 8) - ((val >> 8) & 0xffffff00);
			}
		}
		else
		{ 
			S32 inc = (srcSz << 16) / dstSz;
			S32 Cp = ((dstSz << 14) / srcSz) + 1;
			S32 ap;

			for(U32 i = 0, j = 0, val = 0; i < dstSz; ++i, ++j, val += inc)
			{
				ap = ((0x100 - ((val >> 8) & 0xff)) * Cp) >> 8;
				vp[j] = ap | (Cp << 16);
			}
		}
	}
};


template<U8 ch>
inline void bilinear_scale(
	const U8 *src, U32 srcW, U32 srcH, U32 srcStride
	, U8 *dst, U32 dstW, U32 dstH, U32 dstStride
	)
{
	typedef scale_info<ch> scale_info_t;

	scale_info_t info(src, srcW, srcH, dstW, dstH, srcStride);

	const U8 *sptr;
	U8 *dptr;
	U32 x, y;
	const U8 *pix;

	S32 cx[ch], comp[ch];


	if(3 == info.xup_yup)
	{ //scale x/y - up
		for(y = 0; y < dstH; ++y)
		{
			dptr = dst + (y * dstStride);
			sptr = info.ystrides[y];

			if(0 < info.yapoints[y])
			{
				for(x = 0; x < dstW; ++x)
				{
					//for(c = 0; c < ch; ++c) cx[c] = comp[c] = 0;
					typename scale_info_t::uroll_zeroze_cx_comp_t()(cx, comp);

					if(0 < info.xapoints[x])
					{
						pix = info.ystrides[y] + info.xpoints[x] * ch;

						//for(c = 0; c < ch; ++c) comp[c] = pix[c] * (256 - info.xapoints[x]);
						typename scale_info_t::uroll_inp_asgn_pix_mul_val_t()(comp, pix, 256 - info.xapoints[x]);

						pix += ch;

						//for(c = 0; c < ch; ++c) comp[c] += pix[c] * info.xapoints[x];
						typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(comp, pix, info.xapoints[x]);

						pix += srcStride;

						//for(c = 0; c < ch; ++c) cx[c] = pix[c] * info.xapoints[x];
						typename scale_info_t::uroll_inp_asgn_pix_mul_val_t()(cx, pix, info.xapoints[x]);

						pix -= ch;

						//for(c = 0; c < ch; ++c) { 
						//	cx[c] += pix[c] * (256 - info.xapoints[x]);
						//	comp[c] = ((cx[c] * info.yapoints[y]) + (comp[c] * (256 - info.yapoints[y]))) >> 16;
						//	*dptr++ = comp[c]&0xff;
						//}
						typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(cx, pix, 256 - info.xapoints[x]);
						typename scale_info_t::uroll_comp_asgn_cx_mul_apoint_plus_comp_mul_inv_apoint_allshifted_16_r_t()(comp, cx, info.yapoints[y]);
						typename scale_info_t::uroll_uref_dptr_inc_asgn_comp_and_ff_t()(dptr, comp);
					}
					else
					{
						pix = info.ystrides[y] + info.xpoints[x] * ch;

						//for(c = 0; c < ch; ++c) comp[c] = pix[c] * (256 - info.yapoints[y]);
						typename scale_info_t::uroll_inp_asgn_pix_mul_val_t()(comp, pix, 256-info.yapoints[y]);

						pix += srcStride;

						//for(c = 0; c < ch; ++c) { 
						//	comp[c] = (comp[c] + pix[c] * info.yapoints[y]) >> 8;
						//	*dptr++ = comp[c]&0xff;
						//}
						typename scale_info_t::uroll_comp_asgn_comp_plus_pix_mul_apoint_allshifted_8_r_t()(comp, pix, info.yapoints[y]);
						typename scale_info_t::uroll_uref_dptr_inc_asgn_comp_and_ff_t()(dptr, comp);
					}
				}
			}
			else
			{
				for(x = 0; x < dstW; ++x)
				{
					if(0 < info.xapoints[x])
					{
						pix = info.ystrides[y] + info.xpoints[x] * ch;

						//for(c = 0; c < ch; ++c) {
						//	comp[c] = pix[c] * (256 - info.xapoints[x]);
						//	comp[c] = (comp[c] + pix[c] * info.xapoints[x]) >> 8;
						//	*dptr++ = comp[c]&0xff;
						//}
						typename scale_info_t::uroll_inp_asgn_pix_mul_val_t()(comp, pix, 256 - info.xapoints[x]);
						typename scale_info_t::uroll_comp_asgn_comp_plus_pix_mul_apoint_allshifted_8_r_t()(comp, pix, info.xapoints[x]);
						typename scale_info_t::uroll_uref_dptr_inc_asgn_comp_and_ff_t()(dptr, comp);
					}
					else 
					{
						//for(c = 0; c < ch; ++c) *dptr++ = (sptr[info.xpoints[x]*ch + c])&0xff;
						typename scale_info_t::uroll_uref_dptr_inc_asgn_sptr_apoint_plus_idx_alland_ff_t()(dptr, sptr, info.xpoints[x]*ch);
					}
				}
			}
		}
	}
	else if(info.xup_yup == 1)
	{ //scaling down vertically
		S32 Cy, j;
		S32 yap;

		for(y = 0; y < dstH; y++)
		{
			Cy = info.yapoints[y] >> 16;
			yap = info.yapoints[y] & 0xffff;

			dptr = dst + (y * dstStride);

			for(x = 0; x < dstW; x++)
			{
				pix = info.ystrides[y] + info.xpoints[x] * ch;

				//for(c = 0; c < ch; ++c) comp[c] = pix[c] * yap;
				typename scale_info_t::uroll_inp_asgn_pix_mul_val_t()(comp, pix, yap);

				pix += srcStride;

				for(j = (1 << 14) - yap; j > Cy; j -= Cy, pix += srcStride)
				{
					//for(c = 0; c < ch; ++c) comp[c] += pix[c] * Cy;
					typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(comp, pix, Cy);
				}

				if(j > 0)
				{
					//for(c = 0; c < ch; ++c) comp[c] += pix[c] * j;
					typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(comp, pix, j);
				}

				if(info.xapoints[x] > 0)
				{
					pix = info.ystrides[y] + info.xpoints[x]*ch + ch;
					//for(c = 0; c < ch; ++c) cx[c] = pix[c] * yap;
					typename scale_info_t::uroll_inp_asgn_pix_mul_val_t()(cx, pix, yap);

					pix += srcStride;
					for(j = (1 << 14) - yap; j > Cy; j -= Cy)
					{
						//for(c = 0; c < ch; ++c) cx[c] += pix[c] * Cy;
						typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(cx, pix, Cy);
						pix += srcStride;
					}

					if(j > 0)
					{
						//for(c = 0; c < ch; ++c) cx[c] += pix[c] * j;
						typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(cx, pix, j);
					}

					//for(c = 0; c < ch; ++c) comp[c] = ((comp[c]*(256 - info.xapoints[x])) + ((cx[c] * info.xapoints[x]))) >> 12;
					typename scale_info_t::uroll_comp_asgn_comp_mul_inv_apoint_plus_cx_mul_apoint_allshifted_12_r_t()(comp, info.xapoints[x], cx);
				}
				else
				{
					//for(c = 0; c < ch; ++c) comp[c] >>= 4;
					typename scale_info_t::uroll_comp_rshftasgn_constval_t()(comp, 4);
				}

				//for(c = 0; c < ch; ++c) *dptr++ = (comp[c]>>10)&0xff;
				typename scale_info_t::uroll_uref_dptr_inc_asgn_comp_rshft_cval_and_ff_t()(dptr, comp, 10);
			}
		}
	}
	else if(info.xup_yup == 2)
	{ // scaling down horizontally
		S32 Cx, j;
		S32 xap;

		for(y = 0; y < dstH; y++)
		{
			dptr = dst + (y * dstStride);

			for(x = 0; x < dstW; x++)
			{
				Cx = info.xapoints[x] >> 16;
				xap = info.xapoints[x] & 0xffff;

				pix = info.ystrides[y] + info.xpoints[x] * ch;

				//for(c = 0; c < ch; ++c) comp[c] = pix[c] * xap;
				typename scale_info_t::uroll_inp_asgn_pix_mul_val_t()(comp, pix, xap);

				pix+=ch;
				for(j = (1 << 14) - xap; j > Cx; j -= Cx)
				{
					//for(c = 0; c < ch; ++c) comp[c] += pix[c] * Cx;
					typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(comp, pix, Cx);
					pix+=ch;
				}

				if(j > 0)
				{
					//for(c = 0; c < ch; ++c) comp[c] += pix[c] * j;
					typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(comp, pix, j);
				}

				if(info.yapoints[y] > 0)
				{
					pix = info.ystrides[y] + info.xpoints[x]*ch + srcStride;
					//for(c = 0; c < ch; ++c) cx[c] = pix[c] * xap;
					typename scale_info_t::uroll_inp_asgn_pix_mul_val_t()(cx, pix, xap);

					pix+=ch;
					for(j = (1 << 14) - xap; j > Cx; j -= Cx)
					{
						//for(c = 0; c < ch; ++c) cx[c] += pix[c] * Cx;
						typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(cx, pix, Cx);
						pix+=ch;
					}

					if(j > 0)
					{
						//for(c = 0; c < ch; ++c) cx[c] += pix[c] * j;
						typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(cx, pix, j);
					}

					//for(c = 0; c < ch; ++c) comp[c] = ((comp[c] * (256 - info.yapoints[y])) + ((cx[c] * info.yapoints[y]))) >> 12;
					typename scale_info_t::uroll_comp_asgn_comp_mul_inv_apoint_plus_cx_mul_apoint_allshifted_12_r_t()(comp, info.yapoints[y], cx);
				}
				else
				{
					//for(c = 0; c < ch; ++c) comp[c] >>= 4;
					typename scale_info_t::uroll_comp_rshftasgn_constval_t()(comp, 4);
				}

				//for(c = 0; c < ch; ++c) *dptr++ = (comp[c]>>10)&0xff;
				typename scale_info_t::uroll_uref_dptr_inc_asgn_comp_rshft_cval_and_ff_t()(dptr, comp, 10);
			}
		}
	}
	else 
	{ //scale x/y - down
		S32 Cx, Cy, i, j;
		S32 xap, yap;

		for(y = 0; y < dstH; y++)
		{
			Cy = info.yapoints[y] >> 16;
			yap = info.yapoints[y] & 0xffff;

			dptr = dst + (y * dstStride);
			for(x = 0; x < dstW; x++)
			{
				Cx = info.xapoints[x] >> 16;
				xap = info.xapoints[x] & 0xffff;

				sptr = info.ystrides[y] + info.xpoints[x] * ch;
				pix = sptr;
				sptr += srcStride;

				//for(c = 0; c < ch; ++c) cx[c] = pix[c] * xap;
				typename scale_info_t::uroll_inp_asgn_pix_mul_val_t()(cx, pix, xap);

				pix+=ch;
				for(i = (1 << 14) - xap; i > Cx; i -= Cx)
				{
					//for(c = 0; c < ch; ++c) cx[c] += pix[c] * Cx;
					typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(cx, pix, Cx);
					pix+=ch;
				}

				if(i > 0)
				{
					//for(c = 0; c < ch; ++c) cx[c] += pix[c] * i;
					typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(cx, pix, i);
				}

				//for(c = 0; c < ch; ++c) comp[c] = (cx[c] >> 5) * yap;
				typename scale_info_t::uroll_comp_asgn_cx_rshft_cval_all_mul_val_t()(comp, cx, 5, yap);

				for(j = (1 << 14) - yap; j > Cy; j -= Cy)
				{
					pix = sptr;
					sptr += srcStride;

					//for(c = 0; c < ch; ++c) cx[c] = pix[c] * xap;
					typename scale_info_t::uroll_inp_asgn_pix_mul_val_t()(cx, pix, xap);

					pix+=ch;
					for(i = (1 << 14) - xap; i > Cx; i -= Cx)
					{
						//for(c = 0; c < ch; ++c) cx[c] += pix[c] * Cx;
						typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(cx, pix, Cx);
						pix+=ch;
					}

					if(i > 0)
					{
						//for(c = 0; c < ch; ++c) cx[c] += pix[c] * i;
						typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(cx, pix, i);
					}

					//for(c = 0; c < ch; ++c) comp[c] += (cx[c] >> 5) * Cy;
					typename scale_info_t::uroll_comp_plusasgn_cx_rshft_cval_all_mul_val_t()(comp, cx, 5, Cy);
				}

				if(j > 0)
				{
					pix = sptr;
					sptr += srcStride;

					//for(c = 0; c < ch; ++c) cx[c] = pix[c] * xap;
					typename scale_info_t::uroll_inp_asgn_pix_mul_val_t()(cx, pix, xap);

					pix+=ch;
					for(i = (1 << 14) - xap; i > Cx; i -= Cx)
					{
						//for(c = 0; c < ch; ++c) cx[c] += pix[c] * Cx;
						typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(cx, pix, Cx);
						pix+=ch;
					}

					if(i > 0)
					{
						//for(c = 0; c < ch; ++c) cx[c] += pix[c] * i;
						typename scale_info_t::uroll_inp_plusasgn_pix_mul_val_t()(cx, pix, i);
					}

					//for(c = 0; c < ch; ++c) comp[c] += (cx[c] >> 5) * j;
					typename scale_info_t::uroll_comp_plusasgn_cx_rshft_cval_all_mul_val_t()(comp, cx, 5, j);
				}

				//for(c = 0; c < ch; ++c) *dptr++ = (comp[c]>>23)&0xff;
				typename scale_info_t::uroll_uref_dptr_inc_asgn_comp_rshft_cval_and_ff_t()(dptr, comp, 23);
			}
		}
	} //else
}

//wrapper
static void bilinear_scale(const U8 *src, U32 srcW, U32 srcH, U32 srcCh, U32 srcStride, U8 *dst, U32 dstW, U32 dstH, U32 dstCh, U32 dstStride)
{
	llassert(srcCh == dstCh);

	switch(srcCh)
	{
	case 1:
		bilinear_scale<1>(src, srcW, srcH, srcStride, dst, dstW, dstH, dstStride);
		break;
	case 3:
		bilinear_scale<3>(src, srcW, srcH, srcStride, dst, dstW, dstH, dstStride);
		break;
	case 4:
		bilinear_scale<4>(src, srcW, srcH, srcStride, dst, dstW, dstH, dstStride);
		break;
	default:
		llassert(!"Implement if need");
		break;
	}

}

//---------------------------------------------------------------------------
// LLImage
//---------------------------------------------------------------------------

//static
std::string LLImage::sLastErrorMessage;
LLMutex* LLImage::sMutex = NULL;
bool LLImage::sUseNewByteRange = false;
S32  LLImage::sMinimalReverseByteRangePercent = 75;
LLPrivateMemoryPool* LLImageBase::sPrivatePoolp = NULL ;

//static
void LLImage::initClass(bool use_new_byte_range, S32 minimal_reverse_byte_range_percent)
{
	sUseNewByteRange = use_new_byte_range;
    sMinimalReverseByteRangePercent = minimal_reverse_byte_range_percent;
	sMutex = new LLMutex(NULL);

	LLImageBase::createPrivatePool() ;
}

//static
void LLImage::cleanupClass()
{
	delete sMutex;
	sMutex = NULL;

	LLImageBase::destroyPrivatePool() ;
}

//static
const std::string& LLImage::getLastError()
{
	static const std::string noerr("No Error");
	return sLastErrorMessage.empty() ? noerr : sLastErrorMessage;
}

//static
void LLImage::setLastError(const std::string& message)
{
	LLMutexLock m(sMutex);
	sLastErrorMessage = message;
}

//---------------------------------------------------------------------------
// LLImageBase
//---------------------------------------------------------------------------

LLImageBase::LLImageBase()
:	LLTrace::MemTrackable<LLImageBase>("LLImage"),
	mData(NULL),
	mDataSize(0),
	mWidth(0),
	mHeight(0),
	mComponents(0),
	mBadBufferAllocation(false),
	mAllowOverSize(false)
{}

// virtual
LLImageBase::~LLImageBase()
{
	deleteData(); // virtual
}

//static 
void LLImageBase::createPrivatePool() 
{
	if(!sPrivatePoolp)
	{
		sPrivatePoolp = LLPrivateMemoryPoolManager::getInstance()->newPool(LLPrivateMemoryPool::STATIC_THREADED) ;
	}
}
	
//static 
void LLImageBase::destroyPrivatePool() 
{
	if(sPrivatePoolp)
	{
		LLPrivateMemoryPoolManager::getInstance()->deletePool(sPrivatePoolp) ;
		sPrivatePoolp = NULL ;
	}
}

// virtual
void LLImageBase::dump()
{
	LL_INFOS() << "LLImageBase mComponents " << mComponents
		<< " mData " << mData
		<< " mDataSize " << mDataSize
		<< " mWidth " << mWidth
		<< " mHeight " << mHeight
		<< LL_ENDL;
}

// virtual
void LLImageBase::sanityCheck()
{
	if (mWidth > MAX_IMAGE_SIZE
		|| mHeight > MAX_IMAGE_SIZE
		|| mDataSize > (S32)MAX_IMAGE_DATA_SIZE
		|| mComponents > (S8)MAX_IMAGE_COMPONENTS
		)
	{
		LL_ERRS() << "Failed LLImageBase::sanityCheck "
			   << "width " << mWidth
			   << "height " << mHeight
			   << "datasize " << mDataSize
			   << "components " << mComponents
			   << "data " << mData
			   << LL_ENDL;
	}
}

// virtual
void LLImageBase::deleteData()
{
	FREE_MEM(sPrivatePoolp, mData) ;
	disclaimMem(mDataSize);
	mDataSize = 0;
	mData = NULL;
}

// virtual
U8* LLImageBase::allocateData(S32 size)
{
	//make this function thread-safe.
	static const U32 MAX_BUFFER_SIZE = 4096 * 4096 * 16; //256 MB
	mBadBufferAllocation = false;

	if (size < 0)
	{
		size = mWidth * mHeight * mComponents;
		if (size <= 0)
		{
			LL_WARNS() << llformat("LLImageBase::allocateData called with bad dimensions: %dx%dx%d",mWidth,mHeight,(S32)mComponents) << LL_ENDL;
			mBadBufferAllocation = true;
		}
	}	

	if (!mBadBufferAllocation && (size < 1 || size > MAX_BUFFER_SIZE))
	{
		LL_INFOS() << "width: " << mWidth << " height: " << mHeight << " components: " << mComponents << LL_ENDL ;
		if(mAllowOverSize)
		{
			LL_INFOS() << "Oversize: " << size << LL_ENDL ;
		}
		else
		{
			LL_WARNS() << "LLImageBase::allocateData: bad size: " << size << LL_ENDL;
			mBadBufferAllocation = true;
		}
	}

	if (!mBadBufferAllocation && (!mData || size != mDataSize))
	{
		deleteData(); // virtual
		mData = (U8*)ALLOCATE_MEM(sPrivatePoolp, size);
		if (!mData)
		{
			LL_WARNS() << "Failed to allocate image data size [" << size << "]" << LL_ENDL;
			mBadBufferAllocation = true;
		}
	}

	if (mBadBufferAllocation)
	{
		size = 0;
		mWidth = mHeight = 0;
		mData = NULL;
	}
	mDataSize = size;
	claimMem(mDataSize);

	return mData;
}

// virtual
U8* LLImageBase::reallocateData(S32 size)
{
	U8 *new_datap = (U8*)ALLOCATE_MEM(sPrivatePoolp, size);
	if (!new_datap)
	{
		LL_WARNS() << "Out of memory in LLImageBase::reallocateData" << LL_ENDL;
		return 0;
	}
	if (mData)
	{
		S32 bytes = llmin(mDataSize, size);
		memcpy(new_datap, mData, bytes);	/* Flawfinder: ignore */
		FREE_MEM(sPrivatePoolp, mData) ;
	}
	mData = new_datap;
	disclaimMem(mDataSize);
	mDataSize = size;
	claimMem(mDataSize);
	return mData;
}

const U8* LLImageBase::getData() const	
{ 
	if(mBadBufferAllocation)
	{
		LL_WARNS() << "Bad memory allocation for the image buffer!" << LL_ENDL ;
		return NULL;
	}

	return mData; 
} // read only

U8* LLImageBase::getData()				
{ 
	if(mBadBufferAllocation)
	{
		LL_WARNS() << "Bad memory allocation for the image buffer!" << LL_ENDL;
		return NULL;
	}

	return mData; 
}

bool LLImageBase::isBufferInvalid() const
{
	return mBadBufferAllocation || mData == NULL ;
}

void LLImageBase::setSize(S32 width, S32 height, S32 ncomponents)
{
	mWidth = width;
	mHeight = height;
	mComponents = ncomponents;
}

U8* LLImageBase::allocateDataSize(S32 width, S32 height, S32 ncomponents, S32 size)
{
	setSize(width, height, ncomponents);
	return allocateData(size); // virtual
}

//---------------------------------------------------------------------------
// LLImageRaw
//---------------------------------------------------------------------------

S32 LLImageRaw::sGlobalRawMemory = 0;
S32 LLImageRaw::sRawImageCount = 0;

LLImageRaw::LLImageRaw()
	: LLImageBase()
{
	++sRawImageCount;
}

LLImageRaw::LLImageRaw(U16 width, U16 height, S8 components)
	: LLImageBase()
{
	//llassert( S32(width) * S32(height) * S32(components) <= MAX_IMAGE_DATA_SIZE );
	allocateDataSize(width, height, components);
	++sRawImageCount;
}

LLImageRaw::LLImageRaw(U8 *data, U16 width, U16 height, S8 components, bool no_copy)
	: LLImageBase()
{
	if(no_copy)
	{
		setDataAndSize(data, width, height, components);
	}
	else if(allocateDataSize(width, height, components))
	{
		memcpy(getData(), data, width*height*components);
	}
	++sRawImageCount;
}

//LLImageRaw::LLImageRaw(const std::string& filename, bool j2c_lowest_mip_only)
//	: LLImageBase()
//{
//	createFromFile(filename, j2c_lowest_mip_only);
//}

LLImageRaw::~LLImageRaw()
{
	// NOTE: ~LLimageBase() call to deleteData() calls LLImageBase::deleteData()
	//        NOT LLImageRaw::deleteData()
	deleteData();
	--sRawImageCount;
}

// virtual
U8* LLImageRaw::allocateData(S32 size)
{
	U8* res = LLImageBase::allocateData(size);
	sGlobalRawMemory += getDataSize();
	return res;
}

// virtual
U8* LLImageRaw::reallocateData(S32 size)
{
	sGlobalRawMemory -= getDataSize();
	U8* res = LLImageBase::reallocateData(size);
	sGlobalRawMemory += getDataSize();
	return res;
}

// virtual
void LLImageRaw::deleteData()
{
	sGlobalRawMemory -= getDataSize();
	LLImageBase::deleteData();
}

void LLImageRaw::setDataAndSize(U8 *data, S32 width, S32 height, S8 components) 
{ 
	if(data == getData())
	{
		return ;
	}

	deleteData();

	LLImageBase::setSize(width, height, components) ;
	LLImageBase::setDataAndSize(data, width * height * components) ;
	
	sGlobalRawMemory += getDataSize();
}

bool LLImageRaw::resize(U16 width, U16 height, S8 components)
{
	if ((getWidth() == width) && (getHeight() == height) && (getComponents() == components))
	{
		return true;
	}
	// Reallocate the data buffer.
	deleteData();

	allocateDataSize(width,height,components);

	return true;
}

bool LLImageRaw::setSubImage(U32 x_pos, U32 y_pos, U32 width, U32 height,
							 const U8 *data, U32 stride, bool reverse_y)
{
	if (!getData())
	{
		return false;
	}
	if (!data)
	{
		return false;
	}

	// Should do some simple bounds checking

	U32 i;
	for (i = 0; i < height; i++)
	{
		const U32 row = reverse_y ? height - 1 - i : i;
		const U32 from_offset = row * ((stride == 0) ? width*getComponents() : stride);
		const U32 to_offset = (y_pos + i)*getWidth() + x_pos;
		memcpy(getData() + to_offset*getComponents(),		/* Flawfinder: ignore */
				data + from_offset, getComponents()*width);
	}

	return true;
}

void LLImageRaw::clear(U8 r, U8 g, U8 b, U8 a)
{
	llassert( getComponents() <= 4 );
	// This is fairly bogus, but it'll do for now.
	if (isBufferInvalid())
	{
		LL_WARNS() << "Invalid image buffer" << LL_ENDL;
		return;
	}

	U8 *pos = getData();
	U32 x, y;
	for (x = 0; x < getWidth(); x++)
	{
		for (y = 0; y < getHeight(); y++)
		{
			*pos = r;
			pos++;
			if (getComponents() == 1)
			{
				continue;
			}
			*pos = g;
			pos++;
			if (getComponents() == 2)
			{
				continue;
			}
			*pos = b;
			pos++;
			if (getComponents() == 3)
			{
				continue;
			}
			*pos = a;
			pos++;
		}
	}
}

// Reverses the order of the rows in the image
void LLImageRaw::verticalFlip()
{
	S32 row_bytes = getWidth() * getComponents();
	llassert(row_bytes > 0);
	std::vector<U8> line_buffer(row_bytes);
	S32 mid_row = getHeight() / 2;
	for( S32 row = 0; row < mid_row; row++ )
	{
		U8* row_a_data = getData() + row * row_bytes;
		U8* row_b_data = getData() + (getHeight() - 1 - row) * row_bytes;
		memcpy( &line_buffer[0], row_a_data,  row_bytes );
		memcpy( row_a_data,  row_b_data,  row_bytes );
		memcpy( row_b_data,  &line_buffer[0], row_bytes );
	}
}


void LLImageRaw::expandToPowerOfTwo(S32 max_dim, bool scale_image)
{
	// Find new sizes
	S32 new_width  = expandDimToPowerOfTwo(getWidth(), max_dim);
	S32 new_height = expandDimToPowerOfTwo(getHeight(), max_dim);

	scale( new_width, new_height, scale_image );
}

void LLImageRaw::contractToPowerOfTwo(S32 max_dim, bool scale_image)
{
	// Find new sizes
	S32 new_width  = contractDimToPowerOfTwo(getWidth(), MIN_IMAGE_SIZE);
	S32 new_height = contractDimToPowerOfTwo(getHeight(), MIN_IMAGE_SIZE);

	scale( new_width, new_height, scale_image );
}

// static
S32 LLImageRaw::biasedDimToPowerOfTwo(S32 curr_dim, S32 max_dim)
{
	// Strong bias towards rounding down (to save bandwidth)
	// No bias would mean THRESHOLD == 1.5f;
	const F32 THRESHOLD = 1.75f;
    
	// Find new sizes
	S32 larger_dim  = max_dim;	// 2^n >= curr_dim
	S32 smaller_dim = max_dim;	// 2^(n-1) <= curr_dim
	while( (smaller_dim > curr_dim) && (smaller_dim > MIN_IMAGE_SIZE) )
	{
		larger_dim = smaller_dim;
		smaller_dim >>= 1;
	}
	return ( ((F32)curr_dim / (F32)smaller_dim) > THRESHOLD ) ? larger_dim : smaller_dim;
}

// static
S32 LLImageRaw::expandDimToPowerOfTwo(S32 curr_dim, S32 max_dim)
{
	S32 new_dim = MIN_IMAGE_SIZE;
	while( (new_dim < curr_dim) && (new_dim < max_dim) )
	{
		new_dim <<= 1;
	}
    return new_dim;
}

// static
S32 LLImageRaw::contractDimToPowerOfTwo(S32 curr_dim, S32 min_dim)
{
	S32 new_dim = MAX_IMAGE_SIZE;
	while( (new_dim > curr_dim) && (new_dim > min_dim) )
	{
		new_dim >>= 1;
	}
    return new_dim;
}

void LLImageRaw::biasedScaleToPowerOfTwo(S32 max_dim)
{
	// Find new sizes
	S32 new_width  = biasedDimToPowerOfTwo(getWidth(),max_dim);
	S32 new_height = biasedDimToPowerOfTwo(getHeight(),max_dim);

	scale( new_width, new_height );
}

// Calculates (U8)(255*(a/255.f)*(b/255.f) + 0.5f).  Thanks, Jim Blinn!
inline U8 LLImageRaw::fastFractionalMult( U8 a, U8 b )
{
	U32 i = a * b + 128;
	return U8((i + (i>>8)) >> 8);
}


void LLImageRaw::composite( LLImageRaw* src )
{
	LLImageRaw* dst = this;  // Just for clarity.

	if (!validateSrcAndDst("LLImageRaw::composite", src, dst))
	{
		return;
	}

	llassert(3 == src->getComponents());
	llassert(3 == dst->getComponents());

	if( 3 == dst->getComponents() )
	{
		if( (src->getWidth() == dst->getWidth()) && (src->getHeight() == dst->getHeight()) )
		{
			// No scaling needed
			if( 3 == src->getComponents() )
			{
				copyUnscaled( src );  // alpha is one so just copy the data.
			}
			else
			{
				compositeUnscaled4onto3( src );
			}
		}
		else
		{
			if( 3 == src->getComponents() )
			{
				copyScaled( src );  // alpha is one so just copy the data.
			}
			else
			{
				compositeScaled4onto3( src );
			}
		}
	}
}


// Src and dst can be any size.  Src has 4 components.  Dst has 3 components.
void LLImageRaw::compositeScaled4onto3(LLImageRaw* src)
{
	LL_INFOS() << "compositeScaled4onto3" << LL_ENDL;

	LLImageRaw* dst = this;  // Just for clarity.

	llassert( (4 == src->getComponents()) && (3 == dst->getComponents()) );

	S32 temp_data_size = src->getWidth() * dst->getHeight() * src->getComponents();
	llassert_always(temp_data_size > 0);
	std::vector<U8> temp_buffer(temp_data_size);

	// Vertical: scale but no composite
	for( S32 col = 0; col < src->getWidth(); col++ )
	{
		copyLineScaled( src->getData() + (src->getComponents() * col), &temp_buffer[0] + (src->getComponents() * col), src->getHeight(), dst->getHeight(), src->getWidth(), src->getWidth() );
	}

	// Horizontal: scale and composite
	for( S32 row = 0; row < dst->getHeight(); row++ )
	{
		compositeRowScaled4onto3( &temp_buffer[0] + (src->getComponents() * src->getWidth() * row), dst->getData() + (dst->getComponents() * dst->getWidth() * row), src->getWidth(), dst->getWidth() );
	}
}


// Src and dst are same size.  Src has 4 components.  Dst has 3 components.
void LLImageRaw::compositeUnscaled4onto3( LLImageRaw* src )
{
	LLImageRaw* dst = this;  // Just for clarity.

	llassert( (3 == src->getComponents()) || (4 == src->getComponents()) );
	llassert( (src->getWidth() == dst->getWidth()) && (src->getHeight() == dst->getHeight()) );

	U8* src_data = src->getData();
	U8* dst_data = dst->getData();
	S32 pixels = getWidth() * getHeight();
	while( pixels-- )
	{
		U8 alpha = src_data[3];
		if( alpha )
		{
			if( 255 == alpha )
			{
				dst_data[0] = src_data[0];
				dst_data[1] = src_data[1];
				dst_data[2] = src_data[2];
			}
			else
			{

				U8 transparency = 255 - alpha;
				dst_data[0] = fastFractionalMult( dst_data[0], transparency ) + fastFractionalMult( src_data[0], alpha );
				dst_data[1] = fastFractionalMult( dst_data[1], transparency ) + fastFractionalMult( src_data[1], alpha );
				dst_data[2] = fastFractionalMult( dst_data[2], transparency ) + fastFractionalMult( src_data[2], alpha );
			}
		}

		src_data += 4;
		dst_data += 3;
	}
}


void LLImageRaw::copyUnscaledAlphaMask( LLImageRaw* src, const LLColor4U& fill)
{
	LLImageRaw* dst = this;  // Just for clarity.

	if (!validateSrcAndDst("LLImageRaw::copyUnscaledAlphaMask", src, dst))
	{
		return;
	}

	llassert( 1 == src->getComponents() );
	llassert( 4 == dst->getComponents() );
	llassert( (src->getWidth() == dst->getWidth()) && (src->getHeight() == dst->getHeight()) );

	S32 pixels = getWidth() * getHeight();
	U8* src_data = src->getData();
	U8* dst_data = dst->getData();
	for ( S32 i = 0; i < pixels; i++ )
	{
		dst_data[0] = fill.mV[0];
		dst_data[1] = fill.mV[1];
		dst_data[2] = fill.mV[2];
		dst_data[3] = src_data[0];
		src_data += 1;
		dst_data += 4;
	}
}


// Fill the buffer with a constant color
void LLImageRaw::fill( const LLColor4U& color )
{
	if (isBufferInvalid())
	{
		LL_WARNS() << "Invalid image buffer" << LL_ENDL;
		return;
	}

	S32 pixels = getWidth() * getHeight();
	if( 4 == getComponents() )
	{
		U32* data = (U32*) getData();
		U32 rgbaColor = color.asRGBA();
		for( S32 i = 0; i < pixels; i++ )
		{
			data[ i ] = rgbaColor;
		}
	}
	else
	if( 3 == getComponents() )
	{
		U8* data = getData();
		for( S32 i = 0; i < pixels; i++ )
		{
			data[0] = color.mV[0];
			data[1] = color.mV[1];
			data[2] = color.mV[2];
			data += 3;
		}
	}
}

LLPointer<LLImageRaw> LLImageRaw::duplicate()
{
	if(getNumRefs() < 2)
	{
		return this; //nobody else refences to this image, no need to duplicate.
	}

	//make a duplicate
	LLPointer<LLImageRaw> dup = new LLImageRaw(getData(), getWidth(), getHeight(), getComponents());
	return dup; 
}

// Src and dst can be any size.  Src and dst can each have 3 or 4 components.
void LLImageRaw::copy(LLImageRaw* src)
{
	LLImageRaw* dst = this;  // Just for clarity.

	if (!validateSrcAndDst("LLImageRaw::copy", src, dst))
	{
		return;
	}

	if( (src->getWidth() == dst->getWidth()) && (src->getHeight() == dst->getHeight()) )
	{
		// No scaling needed
		if( src->getComponents() == dst->getComponents() )
		{
			copyUnscaled( src );
		}
		else
		if( 3 == src->getComponents() )
		{
			copyUnscaled3onto4( src );
		}
		else
		{
			// 4 == src->getComponents()
			copyUnscaled4onto3( src );
		}
	}
	else
	{
		// Scaling needed
		// No scaling needed
		if( src->getComponents() == dst->getComponents() )
		{
			copyScaled( src );
		}
		else
		if( 3 == src->getComponents() )
		{
			copyScaled3onto4( src );
		}
		else
		{
			// 4 == src->getComponents()
			copyScaled4onto3( src );
		}
	}
}

// Src and dst are same size.  Src and dst have same number of components.
void LLImageRaw::copyUnscaled(LLImageRaw* src)
{
	LLImageRaw* dst = this;  // Just for clarity.

	llassert( (1 == src->getComponents()) || (3 == src->getComponents()) || (4 == src->getComponents()) );
	llassert( src->getComponents() == dst->getComponents() );
	llassert( (src->getWidth() == dst->getWidth()) && (src->getHeight() == dst->getHeight()) );

	memcpy( dst->getData(), src->getData(), getWidth() * getHeight() * getComponents() );	/* Flawfinder: ignore */
}


// Src and dst can be any size.  Src has 3 components.  Dst has 4 components.
void LLImageRaw::copyScaled3onto4(LLImageRaw* src)
{
	llassert( (3 == src->getComponents()) && (4 == getComponents()) );

	// Slow, but simple.  Optimize later if needed.
	LLImageRaw temp( src->getWidth(), src->getHeight(), 4);
	temp.copyUnscaled3onto4( src );
	copyScaled( &temp );
}


// Src and dst can be any size.  Src has 4 components.  Dst has 3 components.
void LLImageRaw::copyScaled4onto3(LLImageRaw* src)
{
	llassert( (4 == src->getComponents()) && (3 == getComponents()) );

	// Slow, but simple.  Optimize later if needed.
	LLImageRaw temp( src->getWidth(), src->getHeight(), 3);
	temp.copyUnscaled4onto3( src );
	copyScaled( &temp );
}


// Src and dst are same size.  Src has 4 components.  Dst has 3 components.
void LLImageRaw::copyUnscaled4onto3( LLImageRaw* src )
{
	LLImageRaw* dst = this;  // Just for clarity.

	llassert( (3 == dst->getComponents()) && (4 == src->getComponents()) );
	llassert( (src->getWidth() == dst->getWidth()) && (src->getHeight() == dst->getHeight()) );

	S32 pixels = getWidth() * getHeight();
	U8* src_data = src->getData();
	U8* dst_data = dst->getData();
	for( S32 i=0; i<pixels; i++ )
	{
		dst_data[0] = src_data[0];
		dst_data[1] = src_data[1];
		dst_data[2] = src_data[2];
		src_data += 4;
		dst_data += 3;
	}
}


// Src and dst are same size.  Src has 3 components.  Dst has 4 components.
void LLImageRaw::copyUnscaled3onto4( LLImageRaw* src )
{
	LLImageRaw* dst = this;  // Just for clarity.
	llassert( 3 == src->getComponents() );
	llassert( 4 == dst->getComponents() );
	llassert( (src->getWidth() == dst->getWidth()) && (src->getHeight() == dst->getHeight()) );

	S32 pixels = getWidth() * getHeight();
	U8* src_data = src->getData();
	U8* dst_data = dst->getData();
	for( S32 i=0; i<pixels; i++ )
	{
		dst_data[0] = src_data[0];
		dst_data[1] = src_data[1];
		dst_data[2] = src_data[2];
		dst_data[3] = 255;
		src_data += 3;
		dst_data += 4;
	}
}


// Src and dst can be any size.  Src and dst have same number of components.
void LLImageRaw::copyScaled( LLImageRaw* src )
{
	LLImageRaw* dst = this;  // Just for clarity.

	if (!validateSrcAndDst("LLImageRaw::copyScaled", src, dst))
	{
		return;
	}

	llassert_always( (1 == src->getComponents()) || (3 == src->getComponents()) || (4 == src->getComponents()) );
	llassert_always( src->getComponents() == dst->getComponents() );

	if( (src->getWidth() == dst->getWidth()) && (src->getHeight() == dst->getHeight()) )
	{
		memcpy( dst->getData(), src->getData(), getWidth() * getHeight() * getComponents() );	/* Flawfinder: ignore */
		return;
	}

	bilinear_scale(
			src->getData(), src->getWidth(), src->getHeight(), src->getComponents(), src->getWidth()*src->getComponents()
		,	dst->getData(), dst->getWidth(), dst->getHeight(), dst->getComponents(), dst->getWidth()*dst->getComponents()
	);

	/*
	S32 temp_data_size = src->getWidth() * dst->getHeight() * getComponents();
	llassert_always(temp_data_size > 0);
	std::vector<U8> temp_buffer(temp_data_size);

	// Vertical
	for( S32 col = 0; col < src->getWidth(); col++ )
	{
		copyLineScaled( src->getData() + (getComponents() * col), &temp_buffer[0] + (getComponents() * col), src->getHeight(), dst->getHeight(), src->getWidth(), src->getWidth() );
	}

	// Horizontal
	for( S32 row = 0; row < dst->getHeight(); row++ )
	{
		copyLineScaled( &temp_buffer[0] + (getComponents() * src->getWidth() * row), dst->getData() + (getComponents() * dst->getWidth() * row), src->getWidth(), dst->getWidth(), 1, 1 );
	}
	*/
}


bool LLImageRaw::scale( S32 new_width, S32 new_height, bool scale_image_data )
{
    S32 components = getComponents();
    if (components != 1 && components != 3 && components != 4)
    {
        LL_WARNS() << "Invalid getComponents value (" << components << ")" << LL_ENDL;
        return false;
    }

	if (isBufferInvalid())
	{
		LL_WARNS() << "Invalid image buffer" << LL_ENDL;
		return false;
	}

	S32 old_width = getWidth();
	S32 old_height = getHeight();
	
	if( (old_width == new_width) && (old_height == new_height) )
	{
		return true;  // Nothing to do.
	}

	// Reallocate the data buffer.

	if (scale_image_data)
	{
		S32 new_data_size = new_width * new_height * components;

		if (new_data_size > 0)
        {
            U8 *new_data = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), new_data_size); 
            if(NULL == new_data) 
            {
                return false; 
            }

            bilinear_scale(getData(), old_width, old_height, components, old_width*components, new_data, new_width, new_height, components, new_width*components);
            setDataAndSize(new_data, new_width, new_height, components); 
		}
	}
	else
	{
		// copy	out	existing image data
		S32	temp_data_size = old_width * old_height	* components;
		std::vector<U8> temp_buffer(temp_data_size);
		memcpy(&temp_buffer[0],	getData(), temp_data_size);

		// allocate	new	image data,	will delete	old	data
		U8*	new_buffer = allocateDataSize(new_width, new_height, components);

        if (!new_buffer)
        {
            LL_WARNS() << "Failed to allocate new image data buffer" << LL_ENDL;
            return false;
        }
        
        for( S32 row = 0; row <	new_height;	row++ )
        {
            if (row	< old_height)
            {
                memcpy(new_buffer +	(new_width * row * components), &temp_buffer[0] + (old_width *	row	* components),	components * llmin(old_width, new_width));
                if (old_width <	new_width)
                {
                    // pad out rest	of row with	black
                    memset(new_buffer +	(components * ((new_width * row) +	old_width)), 0,	components * (new_width - old_width));
                }
            }
            else
            {
                // pad remaining rows with black
                memset(new_buffer +	(new_width * row * components), 0,	new_width *	components);
            }
        }
	}

	return true ;
}

LLPointer<LLImageRaw> LLImageRaw::scaled(S32 new_width, S32 new_height)
{
    LLPointer<LLImageRaw> result;

    S32 components = getComponents();
    if (components != 1 && components != 3 && components != 4)
    {
        LL_WARNS() << "Invalid getComponents value (" << components << ")" << LL_ENDL;
        return result;
    }

    if (isBufferInvalid())
    {
        LL_WARNS() << "Invalid image buffer" << LL_ENDL;
        return result;
    }

    S32 old_width = getWidth();
    S32 old_height = getHeight();

    if ((old_width == new_width) && (old_height == new_height))
    {
        result = new LLImageRaw(old_width, old_height, components);
        if (!result)
        {
            LL_WARNS() << "Failed to allocate new image" << LL_ENDL;
            return result;
        }
        memcpy(result->getData(), getData(), getDataSize());
    }
    else
    {
        S32 new_data_size = new_width * new_height * components;

        if (new_data_size > 0)
        {
            result = new LLImageRaw(new_width, new_height, components);
            if (!result)
            {
                LL_WARNS() << "Failed to allocate new image" << LL_ENDL;
                return result;
            }
            bilinear_scale(getData(), old_width, old_height, components, old_width*components, result->getData(), new_width, new_height, components, new_width*components);
        }
    }

    return result;
}

void LLImageRaw::copyLineScaled( U8* in, U8* out, S32 in_pixel_len, S32 out_pixel_len, S32 in_pixel_step, S32 out_pixel_step )
{
	const S32 components = getComponents();
	llassert( components >= 1 && components <= 4 );

	const F32 ratio = F32(in_pixel_len) / out_pixel_len; // ratio of old to new
	const F32 norm_factor = 1.f / ratio;

	S32 goff = components >= 2 ? 1 : 0;
	S32 boff = components >= 3 ? 2 : 0;
	for( S32 x = 0; x < out_pixel_len; x++ )
	{
		// Sample input pixels in range from sample0 to sample1.
		// Avoid floating point accumulation error... don't just add ratio each time.  JC
		const F32 sample0 = x * ratio;
		const F32 sample1 = (x+1) * ratio;
		const S32 index0 = llfloor(sample0);			// left integer (floor)
		const S32 index1 = llfloor(sample1);			// right integer (floor)
		const F32 fract0 = 1.f - (sample0 - F32(index0));	// spill over on left
		const F32 fract1 = sample1 - F32(index1);			// spill-over on right

		if( index0 == index1 )
		{
			// Interval is embedded in one input pixel
			S32 t0 = x * out_pixel_step * components;
			S32 t1 = index0 * in_pixel_step * components;
			U8* outp = out + t0;
			U8* inp = in + t1;
			for (S32 i = 0; i < components; ++i)
			{
				*outp = *inp;
				++outp;
				++inp;
			}
		}
		else
		{
			// Left straddle
			S32 t1 = index0 * in_pixel_step * components;
			F32 r = in[t1 + 0] * fract0;
			F32 g = in[t1 + goff] * fract0;
			F32 b = in[t1 + boff] * fract0;
			F32 a = 0;
			if( components == 4)
			{
				a = in[t1 + 3] * fract0;
			}
		
			// Central interval
			if (components < 4)
			{
				for( S32 u = index0 + 1; u < index1; u++ )
				{
					S32 t2 = u * in_pixel_step * components;
					r += in[t2 + 0];
					g += in[t2 + goff];
					b += in[t2 + boff];
				}
			}
			else
			{
				for( S32 u = index0 + 1; u < index1; u++ )
				{
					S32 t2 = u * in_pixel_step * components;
					r += in[t2 + 0];
					g += in[t2 + 1];
					b += in[t2 + 2];
					a += in[t2 + 3];
				}
			}

			// right straddle
			// Watch out for reading off of end of input array.
			if( fract1 && index1 < in_pixel_len )
			{
				S32 t3 = index1 * in_pixel_step * components;
				if (components < 4)
				{
					U8 in0 = in[t3 + 0];
					U8 in1 = in[t3 + goff];
					U8 in2 = in[t3 + boff];
					r += in0 * fract1;
					g += in1 * fract1;
					b += in2 * fract1;
				}
				else
				{
					U8 in0 = in[t3 + 0];
					U8 in1 = in[t3 + 1];
					U8 in2 = in[t3 + 2];
					U8 in3 = in[t3 + 3];
					r += in0 * fract1;
					g += in1 * fract1;
					b += in2 * fract1;
					a += in3 * fract1;
				}
			}

			r *= norm_factor;
			g *= norm_factor;
			b *= norm_factor;
			a *= norm_factor;  // skip conditional

			S32 t4 = x * out_pixel_step * components;
			out[t4 + 0] = U8(ll_round(r));
			if (components >= 2)
				out[t4 + 1] = U8(ll_round(g));
			if (components >= 3)
				out[t4 + 2] = U8(ll_round(b));
			if( components == 4)
				out[t4 + 3] = U8(ll_round(a));
		}
	}
}

void LLImageRaw::compositeRowScaled4onto3( U8* in, U8* out, S32 in_pixel_len, S32 out_pixel_len )
{
	llassert( getComponents() == 3 );

	const S32 IN_COMPONENTS = 4;
	const S32 OUT_COMPONENTS = 3;

	const F32 ratio = F32(in_pixel_len) / out_pixel_len; // ratio of old to new
	const F32 norm_factor = 1.f / ratio;

	for( S32 x = 0; x < out_pixel_len; x++ )
	{
		// Sample input pixels in range from sample0 to sample1.
		// Avoid floating point accumulation error... don't just add ratio each time.  JC
		const F32 sample0 = x * ratio;
		const F32 sample1 = (x+1) * ratio;
		const S32 index0 = S32(sample0);			// left integer (floor)
		const S32 index1 = S32(sample1);			// right integer (floor)
		const F32 fract0 = 1.f - (sample0 - F32(index0));	// spill over on left
		const F32 fract1 = sample1 - F32(index1);			// spill-over on right

		U8 in_scaled_r;
		U8 in_scaled_g;
		U8 in_scaled_b;
		U8 in_scaled_a;

		if( index0 == index1 )
		{
			// Interval is embedded in one input pixel
			S32 t1 = index0 * IN_COMPONENTS;
			in_scaled_r = in[t1 + 0];
			in_scaled_g = in[t1 + 0];
			in_scaled_b = in[t1 + 0];
			in_scaled_a = in[t1 + 0];
		}
		else
		{
			// Left straddle
			S32 t1 = index0 * IN_COMPONENTS;
			F32 r = in[t1 + 0] * fract0;
			F32 g = in[t1 + 1] * fract0;
			F32 b = in[t1 + 2] * fract0;
			F32 a = in[t1 + 3] * fract0;
		
			// Central interval
			for( S32 u = index0 + 1; u < index1; u++ )
			{
				S32 t2 = u * IN_COMPONENTS;
				r += in[t2 + 0];
				g += in[t2 + 1];
				b += in[t2 + 2];
				a += in[t2 + 3];
			}

			// right straddle
			// Watch out for reading off of end of input array.
			if( fract1 && index1 < in_pixel_len )
			{
				S32 t3 = index1 * IN_COMPONENTS;
				r += in[t3 + 0] * fract1;
				g += in[t3 + 1] * fract1;
				b += in[t3 + 2] * fract1;
				a += in[t3 + 3] * fract1;
			}

			r *= norm_factor;
			g *= norm_factor;
			b *= norm_factor;
			a *= norm_factor;

			in_scaled_r = U8(ll_round(r));
			in_scaled_g = U8(ll_round(g));
			in_scaled_b = U8(ll_round(b));
			in_scaled_a = U8(ll_round(a));
		}

		if( in_scaled_a )
		{
			if( 255 == in_scaled_a )
			{
				out[0] = in_scaled_r;
				out[1] = in_scaled_g;
				out[2] = in_scaled_b;
			}
			else
			{
				U8 transparency = 255 - in_scaled_a;
				out[0] = fastFractionalMult( out[0], transparency ) + fastFractionalMult( in_scaled_r, in_scaled_a );
				out[1] = fastFractionalMult( out[1], transparency ) + fastFractionalMult( in_scaled_g, in_scaled_a );
				out[2] = fastFractionalMult( out[2], transparency ) + fastFractionalMult( in_scaled_b, in_scaled_a );
			}
		}
		out += OUT_COMPONENTS;
	}
}

bool LLImageRaw::validateSrcAndDst(std::string func, LLImageRaw* src, LLImageRaw* dst)
{
	if (!src || !dst || src->isBufferInvalid() || dst->isBufferInvalid())
	{
		LL_WARNS() << func << ": Source: ";
		if (!src) LL_CONT << "Null pointer";
		else if (src->isBufferInvalid()) LL_CONT << "Invalid buffer";
		else LL_CONT << "OK";

		LL_CONT << "; Destination: ";
		if (!dst) LL_CONT << "Null pointer";
		else if (dst->isBufferInvalid()) LL_CONT << "Invalid buffer";
		else LL_CONT << "OK";
		LL_CONT << "." << LL_ENDL;

		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

static struct
{
	const char* exten;
	EImageCodec codec;
}
file_extensions[] =
{
	{ "bmp", IMG_CODEC_BMP },
	{ "tga", IMG_CODEC_TGA },
	{ "j2c", IMG_CODEC_J2C },
	{ "jp2", IMG_CODEC_J2C },
	{ "texture", IMG_CODEC_J2C },
	{ "jpg", IMG_CODEC_JPEG },
	{ "jpeg", IMG_CODEC_JPEG },
	{ "mip", IMG_CODEC_DXT },
	{ "dxt", IMG_CODEC_DXT },
	{ "png", IMG_CODEC_PNG }
};
#define NUM_FILE_EXTENSIONS LL_ARRAY_SIZE(file_extensions)
#if 0
static std::string find_file(std::string &name, S8 *codec)
{
	std::string tname;
	for (int i=0; i<(int)(NUM_FILE_EXTENSIONS); i++)
	{
		tname = name + "." + std::string(file_extensions[i].exten);
		llifstream ifs(tname.c_str(), llifstream::binary);
		if (ifs.is_open())
		{
			ifs.close();
			if (codec)
				*codec = file_extensions[i].codec;
			return std::string(file_extensions[i].exten);
		}
	}
	return std::string("");
}
#endif
EImageCodec LLImageBase::getCodecFromExtension(const std::string& exten)
{
	if (!exten.empty())
	{
		for (int i = 0; i < (int)(NUM_FILE_EXTENSIONS); i++)
		{
			if (exten == file_extensions[i].exten)
				return file_extensions[i].codec;
		}
	}
	return IMG_CODEC_INVALID;
}
#if 0
bool LLImageRaw::createFromFile(const std::string &filename, bool j2c_lowest_mip_only)
{
	std::string name = filename;
	size_t dotidx = name.rfind('.');
	S8 codec = IMG_CODEC_INVALID;
	std::string exten;
	
	deleteData(); // delete any existing data

	if (dotidx != std::string::npos)
	{
		exten = name.substr(dotidx+1);
		LLStringUtil::toLower(exten);
		codec = getCodecFromExtension(exten);
	}
	else
	{
		exten = find_file(name, &codec);
		name = name + "." + exten;
	}
	if (codec == IMG_CODEC_INVALID)
	{
		return false; // format not recognized
	}

	llifstream ifs(name.c_str(), llifstream::binary);
	if (!ifs.is_open())
	{
		// SJB: changed from LL_INFOS() to LL_DEBUGS() to reduce spam
		LL_DEBUGS() << "Unable to open image file: " << name << LL_ENDL;
		return false;
	}
	
	ifs.seekg (0, std::ios::end);
	int length = ifs.tellg();
	if (j2c_lowest_mip_only && length > 2048)
	{
		length = 2048;
	}
	ifs.seekg (0, std::ios::beg);

	if (!length)
	{
		LL_INFOS() << "Zero length file file: " << name << LL_ENDL;
		return false;
	}
	
	LLPointer<LLImageFormatted> image = LLImageFormatted::createFromType(codec);
	llassert(image.notNull());

	U8 *buffer = image->allocateData(length);
	ifs.read ((char*)buffer, length);
	ifs.close();
	
	bool success;

	success = image->updateData();
	if (success)
	{
		if (j2c_lowest_mip_only && codec == IMG_CODEC_J2C)
		{
			S32 width = image->getWidth();
			S32 height = image->getHeight();
			S32 discard_level = 0;
			while (width > 1 && height > 1 && discard_level < MAX_DISCARD_LEVEL)
			{
				width >>= 1;
				height >>= 1;
				discard_level++;
			}
			((LLImageJ2C *)((LLImageFormatted*)image))->setDiscardLevel(discard_level);
		}
		success = image->decode(this, 100000.0f);
	}

	image = NULL; // deletes image
	if (!success)
	{
		deleteData();
		LL_WARNS() << "Unable to decode image" << name << LL_ENDL;
		return false;
	}

	return true;
}
#endif
//---------------------------------------------------------------------------
// LLImageFormatted
//---------------------------------------------------------------------------

//static
S32 LLImageFormatted::sGlobalFormattedMemory = 0;

LLImageFormatted::LLImageFormatted(S8 codec)
	: LLImageBase(),
	  mCodec(codec),
	  mDecoding(0),
	  mDecoded(0),
	  mDiscardLevel(-1),
	  mLevels(0)
{
}

// virtual
LLImageFormatted::~LLImageFormatted()
{
	// NOTE: ~LLimageBase() call to deleteData() calls LLImageBase::deleteData()
	//        NOT LLImageFormatted::deleteData()
	deleteData();
}

//----------------------------------------------------------------------------

//virtual
void LLImageFormatted::resetLastError()
{
	LLImage::setLastError("");
}

//virtual
void LLImageFormatted::setLastError(const std::string& message, const std::string& filename)
{
	std::string error = message;
	if (!filename.empty())
		error += std::string(" FILE: ") + filename;
	LLImage::setLastError(error);
}

//----------------------------------------------------------------------------

// static
LLImageFormatted* LLImageFormatted::createFromType(S8 codec)
{
	LLImageFormatted* image;
	switch(codec)
	{
	  case IMG_CODEC_BMP:
		image = new LLImageBMP();
		break;
	  case IMG_CODEC_TGA:
		image = new LLImageTGA();
		break;
	  case IMG_CODEC_JPEG:
		image = new LLImageJPEG();
		break;
	  case IMG_CODEC_PNG:
		image = new LLImagePNG();
		break;
	  case IMG_CODEC_J2C:
		image = new LLImageJ2C();
		break;
	  case IMG_CODEC_DXT:
		image = new LLImageDXT();
		break;
	  default:
		image = NULL;
		break;
	}
	return image;
}

// static
LLImageFormatted* LLImageFormatted::createFromExtension(const std::string& instring)
{
	std::string exten;
	size_t dotidx = instring.rfind('.');
	if (dotidx != std::string::npos)
	{
		exten = instring.substr(dotidx+1);
	}
	else
	{
		exten = instring;
	}
	S8 codec = getCodecFromExtension(exten);
	return createFromType(codec);
}
//----------------------------------------------------------------------------

// virtual
void LLImageFormatted::dump()
{
	LLImageBase::dump();

	LL_INFOS() << "LLImageFormatted"
			<< " mDecoding " << mDecoding
			<< " mCodec " << S32(mCodec)
			<< " mDecoded " << mDecoded
			<< LL_ENDL;
}

//----------------------------------------------------------------------------

S32 LLImageFormatted::calcDataSize(S32 discard_level)
{
	if (discard_level < 0)
	{
		discard_level = mDiscardLevel;
	}
	S32 w = getWidth() >> discard_level;
	S32 h = getHeight() >> discard_level;
	w = llmax(w, 1);
	h = llmax(h, 1);
	return w * h * getComponents();
}

S32 LLImageFormatted::calcDiscardLevelBytes(S32 bytes)
{
	llassert(bytes >= 0);
	S32 discard_level = 0;
	while (1)
	{
		S32 bytes_needed = calcDataSize(discard_level); // virtual
		if (bytes_needed <= bytes)
		{
			break;
		}
		discard_level++;
		if (discard_level > MAX_IMAGE_MIP)
		{
			return -1;
		}
	}
	return discard_level;
}


//----------------------------------------------------------------------------

// Subclasses that can handle more than 4 channels should override this function.
bool LLImageFormatted::decodeChannels(LLImageRaw* raw_image,F32  decode_time, S32 first_channel, S32 max_channel)
{
	llassert( (first_channel == 0) && (max_channel == 4) );
	return decode( raw_image, decode_time );  // Loads first 4 channels by default.
} 

//----------------------------------------------------------------------------

// virtual
U8* LLImageFormatted::allocateData(S32 size)
{
	U8* res = LLImageBase::allocateData(size); // calls deleteData()
	sGlobalFormattedMemory += getDataSize();
	return res;
}

// virtual
U8* LLImageFormatted::reallocateData(S32 size)
{
	sGlobalFormattedMemory -= getDataSize();
	U8* res = LLImageBase::reallocateData(size);
	sGlobalFormattedMemory += getDataSize();
	return res;
}

// virtual
void LLImageFormatted::deleteData()
{
	sGlobalFormattedMemory -= getDataSize();
	LLImageBase::deleteData();
}

//----------------------------------------------------------------------------

// virtual
void LLImageFormatted::sanityCheck()
{
	LLImageBase::sanityCheck();

	if (mCodec >= IMG_CODEC_EOF)
	{
		LL_ERRS() << "Failed LLImageFormatted::sanityCheck "
			   << "decoding " << S32(mDecoding)
			   << "decoded " << S32(mDecoded)
			   << "codec " << S32(mCodec)
			   << LL_ENDL;
	}
}

//----------------------------------------------------------------------------

bool LLImageFormatted::copyData(U8 *data, S32 size)
{
	if ( data && ((data != getData()) || (size != getDataSize())) )
	{
		deleteData();
		allocateData(size);
		memcpy(getData(), data, size);	/* Flawfinder: ignore */
	}
	return true;
}

// LLImageFormatted becomes the owner of data
void LLImageFormatted::setData(U8 *data, S32 size)
{
	if (data && data != getData())
	{
		deleteData();
		setDataAndSize(data, size); // Access private LLImageBase members

		sGlobalFormattedMemory += getDataSize();
	}
}

void LLImageFormatted::appendData(U8 *data, S32 size)
{
	if (data)
	{
		if (!getData())
		{
			setData(data, size);
		}
		else 
		{
			S32 cursize = getDataSize();
			S32 newsize = cursize + size;
			reallocateData(newsize);
			memcpy(getData() + cursize, data, size);
			FREE_MEM(LLImageBase::getPrivatePool(), data);
		}
	}
}

//----------------------------------------------------------------------------

bool LLImageFormatted::load(const std::string &filename, int load_size)
{
	resetLastError();

	S32 file_size = 0;
	LLAPRFile infile ;
	infile.open(filename, LL_APR_RB, NULL, &file_size);
	apr_file_t* apr_file = infile.getFileHandle();
	if (!apr_file)
	{
		setLastError("Unable to open file for reading", filename);
		return false;
	}
	if (file_size == 0)
	{
		setLastError("File is empty",filename);
		return false;
	}

	// Constrain the load size to acceptable values
	if ((load_size == 0) || (load_size > file_size))
	{
		load_size = file_size;
	}
	bool res;
	U8 *data = allocateData(load_size);
	apr_size_t bytes_read = load_size;
	apr_status_t s = apr_file_read(apr_file, data, &bytes_read); // modifies bytes_read
	if (s != APR_SUCCESS || (S32) bytes_read != load_size)
	{
		deleteData();
		setLastError("Unable to read file",filename);
		res = false;
	}
	else
	{
		res = updateData();
	}
	
	return res;
}

bool LLImageFormatted::save(const std::string &filename)
{
	resetLastError();

	LLAPRFile outfile ;
	outfile.open(filename, LL_APR_WB);
	if (!outfile.getFileHandle())
	{
		setLastError("Unable to open file for writing", filename);
		return false;
	}
	
	outfile.write(getData(), 	getDataSize());
	outfile.close() ;
	return true;
}

// bool LLImageFormatted::save(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type)
// Depricated to remove VFS dependency.
// Use:
// LLVFile::writeFile(image->getData(), image->getDataSize(), vfs, uuid, type);

//----------------------------------------------------------------------------

S8 LLImageFormatted::getCodec() const
{
	return mCodec;
}

//============================================================================

static void avg4_colors4(const U8* a, const U8* b, const U8* c, const U8* d, U8* dst)
{
	dst[0] = (U8)(((U32)(a[0]) + b[0] + c[0] + d[0])>>2);
	dst[1] = (U8)(((U32)(a[1]) + b[1] + c[1] + d[1])>>2);
	dst[2] = (U8)(((U32)(a[2]) + b[2] + c[2] + d[2])>>2);
	dst[3] = (U8)(((U32)(a[3]) + b[3] + c[3] + d[3])>>2);
}

static void avg4_colors3(const U8* a, const U8* b, const U8* c, const U8* d, U8* dst)
{
	dst[0] = (U8)(((U32)(a[0]) + b[0] + c[0] + d[0])>>2);
	dst[1] = (U8)(((U32)(a[1]) + b[1] + c[1] + d[1])>>2);
	dst[2] = (U8)(((U32)(a[2]) + b[2] + c[2] + d[2])>>2);
}

static void avg4_colors2(const U8* a, const U8* b, const U8* c, const U8* d, U8* dst)
{
	dst[0] = (U8)(((U32)(a[0]) + b[0] + c[0] + d[0])>>2);
	dst[1] = (U8)(((U32)(a[1]) + b[1] + c[1] + d[1])>>2);
}

void LLImageBase::setDataAndSize(U8 *data, S32 size)
{ 
	ll_assert_aligned(data, 16);
	mData = data; 
	disclaimMem(mDataSize); 
	mDataSize = size; 
	claimMem(mDataSize);
}	

//static
void LLImageBase::generateMip(const U8* indata, U8* mipdata, S32 width, S32 height, S32 nchannels)
{
	llassert(width > 0 && height > 0);
	U8* data = mipdata;
	S32 in_width = width*2;
	for (S32 h=0; h<height; h++)
	{
		for (S32 w=0; w<width; w++)
		{
			switch(nchannels)
			{
			  case 4:
				avg4_colors4(indata, indata+4, indata+4*in_width, indata+4*in_width+4, data);
				break;
			  case 3:
				avg4_colors3(indata, indata+3, indata+3*in_width, indata+3*in_width+3, data);
				break;
			  case 2:
				avg4_colors2(indata, indata+2, indata+2*in_width, indata+2*in_width+2, data);
				break;
			  case 1:
				*(U8*)data = (U8)(((U32)(indata[0]) + indata[1] + indata[in_width] + indata[in_width+1])>>2);
				break;
			  default:
				LL_ERRS() << "generateMmip called with bad num channels" << LL_ENDL;
			}
			indata += nchannels*2;
			data += nchannels;
		}
		indata += nchannels*in_width; // skip odd lines
	}
}


//============================================================================

//static
F32 LLImageBase::calc_download_priority(F32 virtual_size, F32 visible_pixels, S32 bytes_sent)
{
	F32 w_priority;

	F32 bytes_weight = 1.f;
	if (!bytes_sent)
	{
		bytes_weight = 20.f;
	}
	else if (bytes_sent < 1000)
	{
		bytes_weight = 1.f;
	}
	else if (bytes_sent < 2000)
	{
		bytes_weight = 1.f/1.5f;
	}
	else if (bytes_sent < 4000)
	{
		bytes_weight = 1.f/3.f;
	}
	else if (bytes_sent < 8000)
	{
		bytes_weight = 1.f/6.f;
	}
	else if (bytes_sent < 16000)
	{
		bytes_weight = 1.f/12.f;
	}
	else if (bytes_sent < 32000)
	{
		bytes_weight = 1.f/20.f;
	}
	else if (bytes_sent < 64000)
	{
		bytes_weight = 1.f/32.f;
	}
	else
	{
		bytes_weight = 1.f/64.f;
	}
	bytes_weight *= bytes_weight;


	//LL_INFOS() << "VS: " << virtual_size << LL_ENDL;
	F32 virtual_size_factor = virtual_size / (10.f*10.f);

	// The goal is for weighted priority to be <= 0 when we've reached a point where
	// we've sent enough data.
	//LL_INFOS() << "BytesSent: " << bytes_sent << LL_ENDL;
	//LL_INFOS() << "BytesWeight: " << bytes_weight << LL_ENDL;
	//LL_INFOS() << "PreLog: " << bytes_weight * virtual_size_factor << LL_ENDL;
	w_priority = (F32)log10(bytes_weight * virtual_size_factor);

	//LL_INFOS() << "PreScale: " << w_priority << LL_ENDL;

	// We don't want to affect how MANY bytes we send based on the visible pixels, but the order
	// in which they're sent.  We post-multiply so we don't change the zero point.
	if (w_priority > 0.f)
	{
		F32 pixel_weight = (F32)log10(visible_pixels + 1)*3.0f;
		w_priority *= pixel_weight;
	}

	return w_priority;
}

//============================================================================
