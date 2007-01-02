/** 
 * @file lltable.h
 * @brief Description of LLTable template class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
