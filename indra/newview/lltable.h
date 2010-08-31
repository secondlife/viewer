/** 
 * @file lltable.h
 * @brief Description of LLTable template class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLTABLE_H
#define LL_LLTABLE_H

template<class T> class LLTable
{
private:
	T	*_tab;
	U32	_w;
	U32 _h;
	U32 _size;
public:
	LLTable(U32 w, U32 h = 0) : _tab(0), _w(w), _h(h)
	{
		if (_w < 0) _w = 0;
		if (_h < 0) _h = 0;
		if (0 == h)
			_h = _w;
		_size = _w * _h;
		if ((_w > 0) && (_h > 0))
			_tab = new T[_size];
	}

	~LLTable()
	{
		delete[] _tab;
		_tab = NULL;
	}

	void init(const T& t)
	{
		for (U32 i = 0; i < _size; ++i)
			_tab[i] = t;
	}
	const T& at(U32 w, U32 h) const { return _tab[h * _w + w]; }
	T& at(U32 w, U32 h) { return _tab[h * _w + w]; }
	U32 size() const { return _size; }
	U32 w() const { return _w; }
	U32 h() const { return _h; }
};
#endif // LL_LLTABLE_H
