 /** 
 * @file llkdumem.cpp
 * @brief Helper class for kdu memory management
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llkdumem.h"

// Various image utility functions from kdu
#include "llerror.h"

#if defined(LL_WINDOWS)
# pragma warning(disable: 4702) // unreachable code
#endif

LLKDUMemIn::LLKDUMemIn(const U8 *data,
					   const U32 size,
					   const U16 width,
					   const U16 height,
					   const U8 in_num_components,
					   siz_params *siz)
{
	U8 n;

	first_comp_idx = 0;
	rows = height;
	cols = width;
	num_components = in_num_components;
	alignment_bytes = 0;

	for (n=0; n<3; ++n)
	{
		precision[n] = 0;
	}

	for (n=0; n < num_components; ++n)
	{
		siz->set(Sdims,n,0,rows);
		siz->set(Sdims,n,1,cols);
		siz->set(Ssigned,n,0,false);
		siz->set(Sprecision,n,0,8);
	}
	incomplete_lines = NULL;
	free_lines = NULL;
	num_unread_rows = rows;

	mData = data;
	mDataSize = size;
	mCurPos = 0;
}


LLKDUMemIn::~LLKDUMemIn()
{
	if ((num_unread_rows > 0) || (incomplete_lines != NULL))
	{ kdu_warning w;
		w << "Not all rows of image components "
			<< first_comp_idx << " through "
			<< first_comp_idx+num_components-1
			<< " were consumed!";
	}
	image_line_buf *tmp;
	while ((tmp=incomplete_lines) != NULL)
    {
		incomplete_lines = tmp->next;
		delete tmp; 
	}
	while ((tmp=free_lines) != NULL)
    {
		free_lines = tmp->next;
		delete tmp;
	}
}


bool LLKDUMemIn::get(int comp_idx, kdu_line_buf &line, int x_tnum)
{
	int idx = comp_idx - this->first_comp_idx;
	assert((idx >= 0) && (idx < num_components));
	x_tnum = x_tnum*num_components+idx;
	image_line_buf *scan, *prev=NULL;
	for (scan=incomplete_lines; scan != NULL; prev=scan, scan=scan->next)
    {
		assert(scan->next_x_tnum >= x_tnum);
		if (scan->next_x_tnum == x_tnum)
		{
			break;
		}
    }
	if (scan == NULL)
    { // Need to read a new image line.
		assert(x_tnum == 0); // Must consume in very specific order.
		if (num_unread_rows == 0)
		{
	        return false;
		}
		if ((scan = free_lines) == NULL)
		{
			scan = new image_line_buf(cols+3,num_components);
		}
		free_lines = scan->next;
		if (prev == NULL)
		{
	        incomplete_lines = scan;
		}
		else
		{
			prev->next = scan;
		}

		// Copy from image buffer into scan.
		memcpy(scan->buf, mData+mCurPos, cols*num_components);
		mCurPos += cols*num_components;

		num_unread_rows--;
		scan->accessed_samples = 0;
		scan->next_x_tnum = 0;
    }

	assert((cols-scan->accessed_samples) >= line.get_width());

	int comp_offset = idx;
	kdu_byte *sp = scan->buf+num_components*scan->accessed_samples + comp_offset;
	int n=line.get_width();

	if (line.get_buf32() != NULL)
	{
		kdu_sample32 *dp = line.get_buf32();
		if (line.is_absolute())
		{ // 32-bit absolute integers
			for (; n > 0; n--, sp+=num_components, dp++)
			{
				dp->ival = ((kdu_int32)(*sp)) - 128;
			}
		}
		else
		{ // true 32-bit floats
			for (; n > 0; n--, sp+=num_components, dp++)
			{
				dp->fval = (((float)(*sp)) / 256.0F) - 0.5F;
			}
		}
	}
	else
    {
		kdu_sample16 *dp = line.get_buf16();
		if (line.is_absolute())
		{ // 16-bit absolute integers
			for (; n > 0; n--, sp+=num_components, dp++)
			{
				dp->ival = ((kdu_int16)(*sp)) - 128;
			}
		}
		else
		{ // 16-bit normalized representation.
			for (; n > 0; n--, sp+=num_components, dp++)
			{
				dp->ival = (((kdu_int16)(*sp)) - 128) << (KDU_FIX_POINT-8);
			}
		}
    }

	scan->next_x_tnum++;
	if (idx == (num_components-1))
	{
		scan->accessed_samples += line.get_width();
	}
	if (scan->accessed_samples == cols)
	{ // Send empty line to free list.
		assert(scan == incomplete_lines);
		incomplete_lines = scan->next;
		scan->next = free_lines;
		free_lines = scan;
	}

  return true;
}


/*
LLKDUMemOut::LLKDUMemOut(U8 *data, siz_params *siz, U8 in_num_components)
{
	int is_signed = 0;
	int n;

	// Allocate memory segment

	first_comp_idx = 0;
	if (!(siz->get(Scomponents,0,0,num_components) &&
		siz->get(Sdims,first_comp_idx,0,rows) &&
		siz->get(Sdims,first_comp_idx,1,cols) &&
		siz->get(Ssigned,first_comp_idx,0,is_signed)))
    {
		kdu_error e; e << "Attempting to create output image files before "
	      "all information is available concerning the image component "
			"dimensions, bit-depth and signed/unsigned characteristics.";
	}
	num_components -= first_comp_idx;

	for (n=0; n < 3; n++)
	{
		precision[n] = 0;
	}

	for (n=0; n < num_components; n++)
	{
		int prec;

		if (!(siz->compare(Sdims,first_comp_idx+n,0,rows) &&
				siz->compare(Sdims,first_comp_idx+n,1,cols) &&
				siz->compare(Ssigned,first_comp_idx+n,0,is_signed)))
		{
			assert(n > 0);
			num_components = 1;
			break;
		}
		if (!siz->get(Sprecision,first_comp_idx+n,0,prec))
        {
			kdu_error e; e << "Attempting to create output image data before "
				"all information is available concerning the image component "
				"dimensions, bit-depth and signed/unsigned characteristics.";
		}
		llassert(n < 3);
		precision[n] = prec;
	}
	if (is_signed)
    {
		kdu_warning w;
		w << "Signed sample values will be written to the "
		   "BMP file as unsigned 8-bit quantities, centered about 128.";
    }

	mCurPos = 0;
	mData = data;
	mDataSize = rows*cols*num_components;

	incomplete_lines = NULL;
	free_lines = NULL;
	num_unwritten_rows = rows;
}

LLKDUMemOut::~LLKDUMemOut()
{
	if ((num_unwritten_rows > 0) || (incomplete_lines != NULL))
	{
		kdu_warning w;
		w << "Not all rows of image components "
			<< first_comp_idx << " through "
			<< first_comp_idx+num_components-1
			<< " were completed!";
	}
	image_line_buf *tmp;

	while ((tmp=incomplete_lines) != NULL)
    {
		incomplete_lines = tmp->next;
		delete tmp;
	}

	while ((tmp=free_lines) != NULL)
    {
		free_lines = tmp->next;
		delete tmp;
	}

	// Should either clean up or leave alone the data buffer...
//	cout << "Done Destroying" << endl;
}

void LLKDUMemOut::put(int comp_idx, kdu_line_buf &line, int x_tnum)
{
	int idx = 0;

	idx = comp_idx - this->first_comp_idx;

	assert((idx >= 0) && (idx < num_components));
	x_tnum = x_tnum*num_components+idx;
	image_line_buf *scan, *prev=NULL;
	for (scan=incomplete_lines; scan != NULL; prev=scan, scan=scan->next)
	{
		assert(scan->next_x_tnum >= x_tnum);
		if (scan->next_x_tnum == x_tnum)
		{
			break;
		}
    }
	if (scan == NULL)
    { // Need to open a new line buffer
		assert(x_tnum == 0); // Must consume in very specific order.
		if ((scan = free_lines) == NULL)
        {
			scan = new image_line_buf(cols+3,num_components);
        }
		free_lines = scan->next;
		if (prev == NULL)
		{
			incomplete_lines = scan;
		}
		else
        {
			prev->next = scan;
		}
		scan->accessed_samples = 0;
		scan->next_x_tnum = 0;
    }

	assert((cols-scan->accessed_samples) >= line.get_width());

	int comp_offset = idx;

	if (line.get_buf32() != NULL)
    {
		if (line.is_absolute())
		{
			convert_ints_to_bytes(line.get_buf32(),
					scan->buf+num_components*scan->accessed_samples+comp_offset,
					line.get_width(),precision[idx],num_components);
		}
		else
		{
			convert_floats_to_bytes(line.get_buf32(),
					scan->buf+num_components*scan->accessed_samples+comp_offset,
					line.get_width(),precision[idx],num_components);
		}
    }
	else
    {
		if (line.is_absolute())
        {
			convert_shorts_to_bytes(line.get_buf16(),
					scan->buf+num_components*scan->accessed_samples+comp_offset,
					line.get_width(),precision[idx],num_components);
		}
		else
        {
			convert_fixpoint_to_bytes(line.get_buf16(),
					scan->buf+num_components*scan->accessed_samples+comp_offset,
					line.get_width(),precision[idx],num_components);
		}
    }

	scan->next_x_tnum++;
	if (idx == (num_components-1))
    {
		scan->accessed_samples += line.get_width();
	}
	if (scan->accessed_samples == cols)
    {
		// Write completed line and send it to the free list.
		if (num_unwritten_rows == 0)
        {
			kdu_error e; e << "Attempting to write too many lines to image "
				"file for components " << first_comp_idx << " through "
				<< first_comp_idx+num_components-1 << ".";
		}
		if ((mCurPos + cols*num_components) > mDataSize)
		{
			llerrs << "LLKDUMemOut::put() too much data" << llendl;
		}
		// Write the data to the output buffer.
		memcpy(mData+mCurPos, scan->buf, cols*num_components);
		mCurPos += cols*num_components;

		num_unwritten_rows--;
		assert(scan == incomplete_lines);
		incomplete_lines = scan->next;
		scan->next = free_lines;
		free_lines = scan;
    }
}
*/
