/**
 * @file   llallocator_heap_profile_test.cpp
 * @author Brad Kittenbrink
 * @date   2008-02-
 * @brief  Test for llallocator_heap_profile.cpp.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2009-2009, Linden Research, Inc.
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

#include "../llallocator_heap_profile.h"
#include "../test/lltut.h"

namespace tut
{
    struct llallocator_heap_profile_data
    {
        LLAllocatorHeapProfile prof;

        static char const * const sample_win_profile;
        // *TODO - get test output from mac/linux tcmalloc
        static char const * const sample_mac_profile;
        static char const * const sample_lin_profile;

        static char const * const crash_testcase;
    };
    typedef test_group<llallocator_heap_profile_data> factory;
    typedef factory::object object;
}
namespace
{
        tut::factory llallocator_heap_profile_test_factory("LLAllocatorHeapProfile");
}

namespace tut
{
    template<> template<>
    void object::test<1>()
    {
        prof.parse(sample_win_profile);

        ensure_equals("count lines", prof.mLines.size() , 5);
        ensure_equals("alloc counts", prof.mLines[0].mLiveCount, 2131854U);
        ensure_equals("alloc counts", prof.mLines[0].mLiveSize, 2245710106ULL);
        ensure_equals("alloc counts", prof.mLines[0].mTotalCount, 14069198U);
        ensure_equals("alloc counts", prof.mLines[0].mTotalSize, 4295177308ULL);
        ensure_equals("count markers", prof.mLines[0].mTrace.size(), 0);
        ensure_equals("count markers", prof.mLines[1].mTrace.size(), 0);
        ensure_equals("count markers", prof.mLines[2].mTrace.size(), 4);
        ensure_equals("count markers", prof.mLines[3].mTrace.size(), 6);
        ensure_equals("count markers", prof.mLines[4].mTrace.size(), 7);

        //prof.dump(std::cout);
    }

    template<> template<>
    void object::test<2>()
    {
        prof.parse(crash_testcase);

        ensure_equals("count lines", prof.mLines.size(), 2);
        ensure_equals("alloc counts", prof.mLines[0].mLiveCount, 3U);
        ensure_equals("alloc counts", prof.mLines[0].mLiveSize, 1049652ULL);
        ensure_equals("alloc counts", prof.mLines[0].mTotalCount, 8U);
        ensure_equals("alloc counts", prof.mLines[0].mTotalSize, 1049748ULL);
        ensure_equals("count markers", prof.mLines[0].mTrace.size(), 0);
        ensure_equals("count markers", prof.mLines[1].mTrace.size(), 0);

        //prof.dump(std::cout);
    }

    template<> template<>
    void object::test<3>()
    {
        // test that we don't crash on edge case data
        prof.parse("");
        ensure("emtpy on error", prof.mLines.empty());

        prof.parse("heap profile:");
        ensure("emtpy on error", prof.mLines.empty());
    }

char const * const llallocator_heap_profile_data::sample_win_profile =
"heap profile: 2131854: 2245710106 [14069198: 4295177308] @\n"
"308592: 1073398388 [966564: 1280998739] @\n"
"462651: 375969538 [1177377: 753561247] @        2        3        6        1\n"
"314744: 206611283 [2008722: 570934755] @        2        3        3        7       21       32\n"
"277152: 82862770 [621961: 168503640] @        2        3        3        7       21       32       87\n"
"\n"
"MAPPED_LIBRARIES:\n"
"00400000-02681000 r-xp 00000000 00:00 0           c:\\proj\\tcmalloc-eval-9\\indra\\build-vc80\\newview\\RelWithDebInfo\\secondlife-bin.exe\n"
"77280000-773a7000 r-xp 00000000 00:00 0           C:\\Windows\\system32\\ntdll.dll\n"
"76df0000-76ecb000 r-xp 00000000 00:00 0           C:\\Windows\\system32\\kernel32.dll\n"
"76000000-76073000 r-xp 00000000 00:00 0           C:\\Windows\\system32\\comdlg32.dll\n"
"75ee0000-75f8a000 r-xp 00000000 00:00 0           C:\\Windows\\system32\\msvcrt.dll\n"
"76c30000-76c88000 r-xp 00000000 00:00 0           C:\\Windows\\system32\\SHLWAPI.dll\n"
"75f90000-75fdb000 r-xp 00000000 00:00 0           C:\\Windows\\system32\\GDI32.dll\n"
"77420000-774bd000 r-xp 00000000 00:00 0           C:\\Windows\\system32\\USER32.dll\n"
"75e10000-75ed6000 r-xp 00000000 00:00 0           C:\\Windows\\system32\\ADVAPI32.dll\n"
"75b00000-75bc2000 r-xp 00000000 00:00 0           C:\\Windows\\system32\\RPCRT4.dll\n"
"72ca0000-72d25000 r-xp 00000000 00:00 0           C:\\Windows\\WinSxS\\x86_microsoft.windows.common-controls_6595b64144ccf1df_5.82.6001.18000_none_886786f450a74a05\\COMCTL32.dll\n"
"76120000-76c30000 r-xp 00000000 00:00 0           C:\\Windows\\system32\\SHELL32.dll\n"
"71ce0000-71d13000 r-xp 00000000 00:00 0           C:\\Windows\\system32\\DINPUT8.dll\n";


char const * const llallocator_heap_profile_data::crash_testcase =
"heap profile:      3:  1049652 [     8:  1049748] @\n"
"     3:  1049652 [     8:  1049748] @\n"
"\n"
"MAPPED_LIBRARIES:\n"
"00400000-004d5000 r-xp 00000000 00:00 0           c:\\code\\linden\\tcmalloc\\indra\\build-vc80\\llcommon\\RelWithDebInfo\\llallocator_test.exe\n"
"7c900000-7c9af000 r-xp 00000000 00:00 0           C:\\WINDOWS\\system32\\ntdll.dll\n"
"7c800000-7c8f6000 r-xp 00000000 00:00 0           C:\\WINDOWS\\system32\\kernel32.dll\n"
"77dd0000-77e6b000 r-xp 00000000 00:00 0           C:\\WINDOWS\\system32\\ADVAPI32.dll\n"
"77e70000-77f02000 r-xp 00000000 00:00 0           C:\\WINDOWS\\system32\\RPCRT4.dll\n"
"77fe0000-77ff1000 r-xp 00000000 00:00 0           C:\\WINDOWS\\system32\\Secur32.dll\n"
"71ab0000-71ac7000 r-xp 00000000 00:00 0           C:\\WINDOWS\\system32\\WS2_32.dll\n"
"77c10000-77c68000 r-xp 00000000 00:00 0           C:\\WINDOWS\\system32\\msvcrt.dll\n"
"71aa0000-71aa8000 r-xp 00000000 00:00 0           C:\\WINDOWS\\system32\\WS2HELP.dll\n"
"76bf0000-76bfb000 r-xp 00000000 00:00 0           C:\\WINDOWS\\system32\\PSAPI.DLL\n"
"5b860000-5b8b5000 r-xp 00000000 00:00 0           C:\\WINDOWS\\system32\\NETAPI32.dll\n"
"10000000-10041000 r-xp 00000000 00:00 0           c:\\code\\linden\\tcmalloc\\indra\\build-vc80\\llcommon\\RelWithDebInfo\\libtcmalloc_minimal.dll\n"
"7c420000-7c4a7000 r-xp 00000000 00:00 0           C:\\WINDOWS\\WinSxS\\x86_Microsoft.VC80.CRT_1fc8b3b9a1e18e3b_8.0.50727.1433_x-ww_5cf844d2\\MSVCP80.dll\n"
"78130000-781cb000 r-xp 00000000 00:00 0           C:\\WINDOWS\\WinSxS\\x86_Microsoft.VC80.CRT_1fc8b3b9a1e18e3b_8.0.50727.1433_x-ww_5cf844d2\\MSVCR80.dll\n";

}
