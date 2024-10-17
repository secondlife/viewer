/**
 * @file llviewereventrecorder.h
 * @brief Viewer event recording and playback support for mouse and keyboard events
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 *
 * Copyright (c) 2013, Linden Research, Inc.
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

#ifndef LL_VIEWER_EVENT_RECORDER
#define LL_VIEWER_EVENT_RECORDER


#include "linden_common.h"

#include "lldir.h"
#include "llsd.h"
#include "llfile.h"
#include "lldate.h"
#include "llsdserialize.h"
#include "llkeyboard.h"
#include "llstring.h"

#include <sstream>

#include "llsingleton.h" // includes llerror which we need here so we can skip the include here

class LLViewerEventRecorder : public LLSimpleton<LLViewerEventRecorder>
{
public:
    LLViewerEventRecorder();
    ~LLViewerEventRecorder();

  void updateMouseEventInfo(S32 local_x,S32 local_y, S32 global_x, S32 global_y,  std::string mName);
  void setMouseLocalCoords(S32 x,S32 y);
  void setMouseGlobalCoords(S32 x,S32 y);

  void logMouseEvent(std::string button_state, std::string button_name );
  void logKeyEvent(KEY key, MASK mask);
  void logKeyUnicodeEvent(llwchar uni_char);

  void logVisibilityChange(std::string xui, std::string name, bool visibility, std::string event_subtype);

  void clear_xui();
  std::string get_xui();
  void update_xui(std::string xui);

  bool getLoggingStatus() const { return logEvents; }
  void setEventLoggingOn();
  void setEventLoggingOff();

  void playbackRecording();

  bool displayViewerEventRecorderMenuItems();


 protected:
  // On if we wish to log events at the moment - toggle via Develop/Recorder submenu
  bool logEvents;

  std::string mLogFilename;
  llofstream  mLog;


 private:

  // Mouse event info
  S32 global_x;
  S32 global_y;
  S32 local_x;
  S32 local_y;

  // XUI path of UI element
  std::string xui;

  // Actually write the event out to llsd log file
  void recordEvent(LLSD event);

  void clear(S32 r);

  static const S32 UNDEFINED=-1;
};
#endif
