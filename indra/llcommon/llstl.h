/** 
 * @file llstl.h
 * @brief helper object & functions for use with the stl.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLSTL_H
#define LL_LLSTL_H

#include <functional>
#include <algorithm>
#include <map>
#include <vector>
#include <set>
#include <deque>
#include <typeinfo>

// Use to compare the first element only of a pair
// e.g. typedef std::set<std::pair<int, Data*>, compare_pair<int, Data*> > some_pair_set_t; 
template <typename T1, typename T2>
struct compare_pair_first
{
	bool operator()(const std::pair<T1, T2>& a, const std::pair<T1, T2>& b) const
	{
		return a.first < b.first;
	}
};

template <typename T1, typename T2>
struct compare_pair_greater
{
	bool operator()(const std::pair<T1, T2>& a, const std::pair<T1, T2>& b) const
	{
		if (!(a.first < b.first))
			return true;
		else if (!(b.first < a.first))
			return false;
		else
			return !(a.second < b.second);
	}
};

// Use to compare the contents of two pointers (e.g. std::string*)
template <typename T>
struct compare_pointer_contents
{
	typedef const T* Tptr;
	bool operator()(const Tptr& a, const Tptr& b) const
	{
		return *a < *b;
	}
};

// DeletePointer is a simple helper for deleting all pointers in a container.
// The general form is:
//
//  std::for_each(cont.begin(), cont.end(), DeletePointer());
//  somemap.clear();
//
// Don't forget to clear()!

struct DeletePointer
{
	template<typename T> void operator()(T* ptr) const
	{
		delete ptr;
	}
};
struct DeletePointerArray
{
	template<typename T> void operator()(T* ptr) const
	{
		delete[] ptr;
	}
};

// DeletePairedPointer is a simple helper for deleting all pointers in a map.
// The general form is:
//
//  std::for_each(somemap.begin(), somemap.end(), DeletePairedPointer());

struct DeletePairedPointer
{
	template<typename T> void operator()(T &ptr) const
	{
		delete ptr.second;
		ptr.second = NULL;
	}
};
struct DeletePairedPointerArray
{
	template<typename T> void operator()(T &ptr) const
	{
		delete[] ptr.second;
		ptr.second = NULL;
	}
};


// Alternate version of the above so that has a more cumbersome
// syntax, but it can be used with compositional functors.
// NOTE: The functor retuns a bool because msdev bombs during the
// composition if you return void. Once we upgrade to a newer
// compiler, the second unary_function template parameter can be set
// to void.
//
// Here's a snippit showing how you use this object:
//
// typedef std::map<int, widget*> map_type;
// map_type widget_map;
// ... // add elements
// // delete them all
// for_each(widget_map.begin(),
//          widget_map.end(),
//          llcompose1(DeletePointerFunctor<widget>(),
//                     llselect2nd<map_type::value_type>()));

template<typename T>
struct DeletePointerFunctor : public std::unary_function<T*, bool>
{
	bool operator()(T* ptr) const
	{
		delete ptr;
		return true;
	}
};

// See notes about DeleteArray for why you should consider avoiding this.
template<typename T>
struct DeleteArrayFunctor : public std::unary_function<T*, bool>
{
	bool operator()(T* ptr) const
	{
		delete[] ptr;
		return true;
	}
};

// CopyNewPointer is a simple helper which accepts a pointer, and
// returns a new pointer built with the copy constructor. Example:
//
//  transform(in.begin(), in.end(), out.end(), CopyNewPointer());

struct CopyNewPointer
{
	template<typename T> T* operator()(const T* ptr) const
	{
		return new T(*ptr);
	}
};

// Simple function to help with finding pointers in maps.
// For example:
// 	typedef  map_t;
//  std::map<int, const char*> foo;
//	foo[18] = "there";
//	foo[2] = "hello";
// 	const char* bar = get_ptr_in_map(foo, 2); // bar -> "hello"
//  const char* baz = get_ptr_in_map(foo, 3); // baz == NULL
template <typename K, typename T>
inline T* get_ptr_in_map(const std::map<K,T*>& inmap, const K& key)
{
	// Typedef here avoids warnings because of new c++ naming rules.
	typedef typename std::map<K,T*>::const_iterator map_iter;
	map_iter iter = inmap.find(key);
	if(iter == inmap.end())
	{
		return NULL;
	}
	else
	{
		return iter->second;
	}
};

// helper function which returns true if key is in inmap.
template <typename K, typename T>
inline bool is_in_map(const std::map<K,T>& inmap, const K& key)
{
	typedef typename std::map<K,T>::const_iterator map_iter;
	if(inmap.find(key) == inmap.end())
	{
		return false;
	}
	else
	{
		return true;
	}
}

// Similar to get_ptr_in_map, but for any type with a valid T(0) constructor.
// To replace LLSkipMap getIfThere, use:
//   get_if_there(map, key, 0)
// WARNING: Make sure default_value (generally 0) is not a valid map entry!
template <typename K, typename T>
inline T get_if_there(const std::map<K,T>& inmap, const K& key, T default_value)
{
	// Typedef here avoids warnings because of new c++ naming rules.
	typedef typename std::map<K,T>::const_iterator map_iter;
	map_iter iter = inmap.find(key);
	if(iter == inmap.end())
	{
		return default_value;
	}
	else
	{
		return iter->second;
	}
};

// Useful for replacing the removeObj() functionality of LLDynamicArray
// Example:
//  for (std::vector<T>::iterator iter = mList.begin(); iter != mList.end(); )
//  {
//    if ((*iter)->isMarkedForRemoval())
//      iter = vector_replace_with_last(mList, iter);
//    else
//      ++iter;
//  }
template <typename T, typename Iter>
inline Iter vector_replace_with_last(std::vector<T>& invec, Iter iter)
{
	typename std::vector<T>::iterator last = invec.end(); --last;
	if (iter == invec.end())
	{
		return iter;
	}
	else if (iter == last)
	{
		invec.pop_back();
		return invec.end();
	}
	else
	{
		*iter = *last;
		invec.pop_back();
		return iter;
	}
};

// Useful for replacing the removeObj() functionality of LLDynamicArray
// Example:
//   vector_replace_with_last(mList, x);
template <typename T>
inline bool vector_replace_with_last(std::vector<T>& invec, const T& val)
{
	typename std::vector<T>::iterator iter = std::find(invec.begin(), invec.end(), val);
	if (iter != invec.end())
	{
		typename std::vector<T>::iterator last = invec.end(); --last;
		*iter = *last;
		invec.pop_back();
		return true;
	}
	return false;
}

// Append N elements to the vector and return a pointer to the first new element.
template <typename T>
inline T* vector_append(std::vector<T>& invec, S32 N)
{
	U32 sz = invec.size();
	invec.resize(sz+N);
	return &(invec[sz]);
}

// call function f to n members starting at first. similar to std::for_each
template <class InputIter, class Size, class Function>
Function ll_for_n(InputIter first, Size n, Function f)
{
	for ( ; n > 0; --n, ++first)
		f(*first);
	return f;
}

// copy first to result n times, incrementing each as we go
template <class InputIter, class Size, class OutputIter>
OutputIter ll_copy_n(InputIter first, Size n, OutputIter result)
{
	for ( ; n > 0; --n, ++result, ++first)
		*result = *first;
	return result;
}

// set  *result = op(*f) for n elements of f
template <class InputIter, class OutputIter, class Size, class UnaryOp>
OutputIter ll_transform_n(
	InputIter first,
	Size n,
	OutputIter result,
	UnaryOp op)
{
	for ( ; n > 0; --n, ++result, ++first)
		*result = op(*first);
	return result;
}



/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996-1998
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */


// helper to deal with the fact that MSDev does not package
// select... with the stl. Look up usage on the sgi website.

template <class _Pair>
struct _LLSelect1st : public std::unary_function<_Pair, typename _Pair::first_type> {
  const typename _Pair::first_type& operator()(const _Pair& __x) const {
    return __x.first;
  }
};

template <class _Pair>
struct _LLSelect2nd : public std::unary_function<_Pair, typename _Pair::second_type>
{
  const typename _Pair::second_type& operator()(const _Pair& __x) const {
    return __x.second;
  }
};

template <class _Pair> struct llselect1st : public _LLSelect1st<_Pair> {};
template <class _Pair> struct llselect2nd : public _LLSelect2nd<_Pair> {};

// helper to deal with the fact that MSDev does not package
// compose... with the stl. Look up usage on the sgi website.

template <class _Operation1, class _Operation2>
class ll_unary_compose :
	public std::unary_function<typename _Operation2::argument_type,
							   typename _Operation1::result_type>
{
protected:
  _Operation1 __op1;
  _Operation2 __op2;
public:
  ll_unary_compose(const _Operation1& __x, const _Operation2& __y)
    : __op1(__x), __op2(__y) {}
  typename _Operation1::result_type
  operator()(const typename _Operation2::argument_type& __x) const {
    return __op1(__op2(__x));
  }
};

template <class _Operation1, class _Operation2>
inline ll_unary_compose<_Operation1,_Operation2>
llcompose1(const _Operation1& __op1, const _Operation2& __op2)
{
  return ll_unary_compose<_Operation1,_Operation2>(__op1, __op2);
}

template <class _Operation1, class _Operation2, class _Operation3>
class ll_binary_compose
  : public std::unary_function<typename _Operation2::argument_type,
							   typename _Operation1::result_type> {
protected:
  _Operation1 _M_op1;
  _Operation2 _M_op2;
  _Operation3 _M_op3;
public:
  ll_binary_compose(const _Operation1& __x, const _Operation2& __y,
					const _Operation3& __z)
    : _M_op1(__x), _M_op2(__y), _M_op3(__z) { }
  typename _Operation1::result_type
  operator()(const typename _Operation2::argument_type& __x) const {
    return _M_op1(_M_op2(__x), _M_op3(__x));
  }
};

template <class _Operation1, class _Operation2, class _Operation3>
inline ll_binary_compose<_Operation1, _Operation2, _Operation3>
llcompose2(const _Operation1& __op1, const _Operation2& __op2,
         const _Operation3& __op3)
{
  return ll_binary_compose<_Operation1,_Operation2,_Operation3>
    (__op1, __op2, __op3);
}

// helpers to deal with the fact that MSDev does not package
// bind... with the stl. Again, this is from sgi.
template <class _Operation>
class llbinder1st :
	public std::unary_function<typename _Operation::second_argument_type,
							   typename _Operation::result_type> {
protected:
  _Operation op;
  typename _Operation::first_argument_type value;
public:
  llbinder1st(const _Operation& __x,
			  const typename _Operation::first_argument_type& __y)
      : op(__x), value(__y) {}
	typename _Operation::result_type
	operator()(const typename _Operation::second_argument_type& __x) const {
		return op(value, __x);
	}
};

template <class _Operation, class _Tp>
inline llbinder1st<_Operation>
llbind1st(const _Operation& __oper, const _Tp& __x)
{
  typedef typename _Operation::first_argument_type _Arg1_type;
  return llbinder1st<_Operation>(__oper, _Arg1_type(__x));
}

template <class _Operation>
class llbinder2nd
	: public std::unary_function<typename _Operation::first_argument_type,
								 typename _Operation::result_type> {
protected:
	_Operation op;
	typename _Operation::second_argument_type value;
public:
	llbinder2nd(const _Operation& __x,
				const typename _Operation::second_argument_type& __y)
		: op(__x), value(__y) {}
	typename _Operation::result_type
	operator()(const typename _Operation::first_argument_type& __x) const {
		return op(__x, value);
	}
};

template <class _Operation, class _Tp>
inline llbinder2nd<_Operation>
llbind2nd(const _Operation& __oper, const _Tp& __x)
{
  typedef typename _Operation::second_argument_type _Arg2_type;
  return llbinder2nd<_Operation>(__oper, _Arg2_type(__x));
}

/**
 * Compare std::type_info* pointers a la std::less. We break this out as a
 * separate function for use in two different std::less specializations.
 */
inline
bool before(const std::type_info* lhs, const std::type_info* rhs)
{
#if LL_LINUX && defined(__GNUC__) && ((__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 4))
    // If we're building on Linux with gcc, and it's either gcc 3.x or
    // 4.{0,1,2,3}, then we have to use a workaround. Note that we use gcc on
    // Mac too, and some people build with gcc on Windows (cygwin or mingw).
    // On Linux, different load modules may produce different type_info*
    // pointers for the same type. Have to compare name strings to get good
    // results.
    return strcmp(lhs->name(), rhs->name()) < 0;
#else  // not Linux, or gcc 4.4+
    // Just use before(), as we normally would
    return lhs->before(*rhs);
#endif
}

/**
 * Specialize std::less<std::type_info*> to use std::type_info::before().
 * See MAINT-1175. It is NEVER a good idea to directly compare std::type_info*
 * because, on Linux, you might get different std::type_info* pointers for the
 * same type (from different load modules)!
 */
namespace std
{
	template <>
	struct less<const std::type_info*>:
		public std::binary_function<const std::type_info*, const std::type_info*, bool>
	{
		bool operator()(const std::type_info* lhs, const std::type_info* rhs) const
		{
			return before(lhs, rhs);
		}
	};

	template <>
	struct less<std::type_info*>:
		public std::binary_function<std::type_info*, std::type_info*, bool>
	{
		bool operator()(std::type_info* lhs, std::type_info* rhs) const
		{
			return before(lhs, rhs);
		}
	};
} // std

#endif // LL_LLSTL_H
