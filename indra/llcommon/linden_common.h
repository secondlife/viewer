/** 
 * @file linden_common.h
 * @brief Includes common headers that are always safe to include
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LINDEN_COMMON_H
#define LL_LINDEN_COMMON_H

#include "llpreprocessor.h"

#include <cstring>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>

// Work around stupid Microsoft STL warning
#ifdef LL_WINDOWS
#pragma warning (disable : 4702) // warning C4702: unreachable code
#endif	//	LL_WINDOWS

#include <algorithm>
#include <list>
#include <map>
#include <vector>
#include <string>

#ifdef LL_WINDOWS
#pragma warning (3 : 4702) // we like level 3, not 4
#endif	//	LL_WINDOWS

// Linden only libs in alpha-order other than stdtypes.h
#include "stdtypes.h"
#include "lldefs.h"
#include "llerror.h"
#include "llextendedstatus.h"
#include "llfasttimer.h"
#include "llfile.h"
#include "llformat.h"
#include "llstring.h"
#include "llsys.h"
#include "lltimer.h"

#endif
