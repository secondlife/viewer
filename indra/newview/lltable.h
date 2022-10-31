/** 
 * @file lltable.h
 * @brief Description of LLTable template class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLTABLE_H
#define LL_LLTABLE_H

template<class T> class LLTable
{
private:
    T   *_tab;
    U32 _w;
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
