/**
 * @file   llappviewermacosx-for-objc.h
 * @author Nat Goodspeed
 * @date   2018-06-15
 * @brief  llappviewermacosx.h publishes the C++ API for
 *         llappviewermacosx.cpp, just as
 *         llappviewermacosx-objc.h publishes the Objective-C++ API for
 *         llappviewermacosx-objc.mm.
 *
 *         This header is intended to publish for Objective-C++ consumers a
 *         subset of the C++ API presented by llappviewermacosx.cpp. It's a
 *         subset because, if an Objective-C++ consumer were to #include
 *         the full llappviewermacosx.h, we would almost surely run into
 *         trouble due to the discrepancy between Objective-C++'s BOOL versus
 *         classic Microsoft/Linden BOOL.
 * 
 * $LicenseInfo:firstyear=2018&license=viewerlgpl$
 * Copyright (c) 2018, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLAPPVIEWERMACOSX_FOR_OBJC_H)
#define LL_LLAPPVIEWERMACOSX_FOR_OBJC_H

#include <string>

bool initViewer();
void handleUrl(const char* url_utf8);
bool pumpMainLoop();
void handleQuit();
void cleanupViewer();
std::string getOldLogFilePathname();
std::string getFatalMessage();
std::string getAgentFullname();
void infos(const std::string& message);

#endif /* ! defined(LL_LLAPPVIEWERMACOSX_FOR_OBJC_H) */
