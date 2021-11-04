/**
 * @file llviewereventrecorder.cpp
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


#include "llviewereventrecorder.h"
#include "llui.h"
#include "llleap.h"

LLViewerEventRecorder::LLViewerEventRecorder() {

  clear(UNDEFINED);
  logEvents = false;
  // Remove any previous event log file
  std::string old_log_ui_events_to_llsd_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "SecondLife_Events_log.old");
  LLFile::remove(old_log_ui_events_to_llsd_file, ENOENT);
  

  mLogFilename = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "SecondLife_Events_log.llsd");
  LLFile::rename(mLogFilename, old_log_ui_events_to_llsd_file, ENOENT);

}


bool LLViewerEventRecorder::displayViewerEventRecorderMenuItems() {
  return LLUI::getInstance()->mSettingGroups["config"]->getBOOL("ShowEventRecorderMenuItems");
}


void LLViewerEventRecorder::setEventLoggingOn() {
  if (! mLog.is_open()) {
      mLog.open(mLogFilename.c_str(), std::ios_base::out);
  }
  logEvents=true; 
  LL_DEBUGS() << "LLViewerEventRecorder::setEventLoggingOn event logging turned on" << LL_ENDL;
}

void LLViewerEventRecorder::setEventLoggingOff() {
  logEvents=false;
  mLog.flush();
  mLog.close();
  LL_DEBUGS() << "LLViewerEventRecorder::setEventLoggingOff event logging turned off" << LL_ENDL;
}


 LLViewerEventRecorder::~LLViewerEventRecorder() {
  if (mLog.is_open()) {
      mLog.close();
    }
}

void LLViewerEventRecorder::clear_xui() {
  xui.clear();
}

void LLViewerEventRecorder::clear(S32 r) {

  xui.clear();

  local_x=r;
  local_y=r;

  global_x=r;
  global_y=r;
    

}

void LLViewerEventRecorder::setMouseLocalCoords(S32 x, S32 y) {
  local_x=x;
  local_y=y;
}

void LLViewerEventRecorder::setMouseGlobalCoords(S32 x, S32 y) {
  global_x=x;
  global_y=y;
}

void LLViewerEventRecorder::updateMouseEventInfo(S32 local_x, S32 local_y, S32 global_x, S32 global_y, std::string mName) {

  LLView * target_view = LLUI::getInstance()->resolvePath(LLUI::getInstance()->getRootView(), xui);
  if (! target_view) {
    LL_DEBUGS() << "LLViewerEventRecorder::updateMouseEventInfo - xui path on file at moment is NOT valid - so DO NOT record these local coords" << LL_ENDL;
    return;
  }
  LL_DEBUGS() << "LLViewerEventRecorder::updateMouseEventInfo b4 updatemouseeventinfo - local_x|global x   "<< this->local_x << " " << this->global_x  << "local/global y " << this->local_y << " " << this->global_y << " mname: " << mName << " xui: " << xui << LL_ENDL;


  if (this->local_x < 1 && this->local_y<1 && local_x && local_y) {
    this->local_x=local_x;
    this->local_y=local_y;
  }
  this->global_x=global_x;
  this->global_y=global_y;

  // ONLY record deepest xui path for hierarchy searches - or first/only xui for floaters/panels reached via mouse captor - and llmousehandler
  if (mName!="" &&  mName!="/" && xui=="") { 
    //	xui=std::string("/")+mName+xui; 
    //xui=mName+xui; 
    xui = mName; // TODO review confirm we never call with partial path - also cAN REMOVE CHECK FOR "" - ON OTHER HAND IT'S PRETTY HARMLESS
  }

  LL_DEBUGS() << "LLViewerEventRecorder::updateMouseEventInfo after updatemouseeventinfo - local_x|global x   "<< this->local_x << " " << this->global_x  << "local/global y " << this->local_y << " " << this->global_y << " mname: " << mName << " xui: " << xui << LL_ENDL;
}

void LLViewerEventRecorder::logVisibilityChange(std::string xui, std::string name, BOOL visibility, std::string event_subtype) {

  LLSD  event=LLSD::emptyMap();

  event.insert("event",LLSD(std::string("visibility")));

  if (visibility) {
    event.insert("visibility",LLSD(true));
  } else {
    event.insert("visibility",LLSD(false));
  }

  if (event_subtype!="") {
    event.insert("event_subtype", LLSD(event_subtype));
  }

  if(name!="") {
    event.insert("name",LLSD(name));
  }

  if (xui!="") {
    event.insert("path",LLSD(xui));
  }

  event.insert("timestamp",LLSD(LLDate::now().asString())); 
  recordEvent(event);
}


std::string LLViewerEventRecorder::get_xui() {
  return xui;
}
void LLViewerEventRecorder::update_xui(std::string xui) {
  if (xui!="" && this->xui=="" ) {
    LL_DEBUGS() << "LLViewerEventRecorder::update_xui to " << xui << LL_ENDL;
    this->xui=xui;
  } else {
    LL_DEBUGS() << "LLViewerEventRecorder::update_xui called with empty string" << LL_ENDL;
  }
}

void LLViewerEventRecorder::logKeyEvent(KEY key, MASK mask) {

  // NOTE: Event recording only logs keydown events - the viewer itself hides keyup events at a fairly low level in the code and does not appear to care about them anywhere

  LLSD event = LLSD::emptyMap();

  event.insert("event",LLSD("type"));

  // keysym ...or
  // keycode...or
  // char
  event.insert("keysym",LLSD(LLKeyboard::stringFromKey(key)));

  // path (optional) - for now we are not recording path for key events during record - should not be needed for full record and playback of recorded steps
  // as a vita script - it does become useful if you edit the resulting vita script and wish to remove some steps leading to a key event - that sort of edit might
  // break the test script and it would be useful to have more context to make these sorts of edits safer
 
  // TODO  replace this with a call which extracts to an array of names of masks (just like vita expects during playback)
  // This is looking more and more like an object is a good idea, for this part a handy method call to setMask(mask) would be nice :-)
  // call the func - llkeyboard::llsdStringarrayFromMask

  LLSD key_mask=LLSD::emptyArray();

  if (mask & MASK_CONTROL)     { key_mask.append(LLSD("CTL")); }  // Mac command key - has code of 0x1  in llcommon/indra_contstants
  if (mask & MASK_ALT)         { key_mask.append(LLSD("ALT")); }
  if (mask & MASK_SHIFT)       { key_mask.append(LLSD("SHIFT")); }
  if (mask & MASK_MAC_CONTROL) { key_mask.append(LLSD("MAC_CONTROL")); }

  event.insert("mask",key_mask); 
  event.insert("timestamp",LLSD(LLDate::now().asString())); 

  // Although vita has keyDown and keyUp requests it does not have type as a high-level concept 
  // (maybe it should) - instead it has a convenience method that generates the keydown and keyup events 
  // Here  we will use  "type" as  our event type

  LL_DEBUGS() << "LLVIewerEventRecorder::logKeyEvent Serialized LLSD for event " << event.asString() << "\n" << LL_ENDL;


  //LL_DEBUGS()  << "[VITA] key_name: "  << LLKeyboard::stringFromKey(key) << "mask: "<< mask  << "handled by " << getName() << LL_ENDL;
  LL_DEBUGS()  << "LLVIewerEventRecorder::logKeyEvent  key_name: "  << LLKeyboard::stringFromKey(key) << "mask: "<< mask  << LL_ENDL;


  recordEvent(event);

}

void LLViewerEventRecorder::playbackRecording() {

  LLSD LeapCommand;

  // ivita sets this on startup, it also sends commands to the viewer to make start, stop, and playback menu items visible in viewer
  LeapCommand =LLUI::getInstance()->mSettingGroups["config"]->getLLSD("LeapPlaybackEventsCommand");
  
  LL_DEBUGS() << "[VITA] launching playback - leap command is: " << LLSDXMLStreamer(LeapCommand) << LL_ENDL;
  LLLeap::create("", LeapCommand, false); // exception=false
  
}


void LLViewerEventRecorder::recordEvent(LLSD event) {
  LL_DEBUGS() << "LLViewerEventRecorder::recordEvent event written to log: " << LLSDXMLStreamer(event) << LL_ENDL;
  mLog << event << std::endl;
  
}
void LLViewerEventRecorder::logKeyUnicodeEvent(llwchar uni_char) {
  if (! logEvents) return;

  // Note: keyUp is not captured since the viewer seems to not care about keyUp events

  LLSD event=LLSD::emptyMap();

  event.insert("timestamp",LLSD(LLDate::now().asString()));

  
  // keysym ...or
  // keycode...or
  // char

  LL_DEBUGS() << "Wrapped in conversion to wstring " <<  wstring_to_utf8str(LLWString( 1, uni_char)) << "\n" << LL_ENDL;
  
  event.insert("char",
	       LLSD(  wstring_to_utf8str(LLWString( 1,uni_char))  )
	       ); 

  // path (optional) - for now we are not recording path for key events during record - should not be needed for full record and playback of recorded steps
  // as a vita script - it does become useful if you edit the resulting vita script and wish to remove some steps leading to a key event - that sort of edit might
  // break the test script and it would be useful to have more context to make these sorts of edits safer

  // TODO need to consider mask keys too? Doesn't seem possible - at least not easily at this point

  event.insert("event",LLSD("keyDown")); 

  LL_DEBUGS()  << "[VITA] unicode key: " << uni_char   << LL_ENDL;
  LL_DEBUGS()  << "[VITA] dumpxml " << LLSDXMLStreamer(event) << "\n" << LL_ENDL;


  recordEvent(event);

}

void LLViewerEventRecorder::logMouseEvent(std::string button_state,std::string button_name)
{
  if (! logEvents) return; 

  LLSD  event=LLSD::emptyMap();

  event.insert("event",LLSD(std::string("mouse"+ button_state)));
  event.insert("button",LLSD(button_name));
  if (xui!="") {
    event.insert("path",LLSD(xui));
  }

  if (local_x>0 && local_y>0) {
    event.insert("local_x",LLSD(local_x));
    event.insert("local_y",LLSD(local_y));
  }

  if (global_x>0 && global_y>0) {
    event.insert("global_x",LLSD(global_x));
    event.insert("global_y",LLSD(global_y));
  }
  event.insert("timestamp",LLSD(LLDate::now().asString())); 
  recordEvent(event);


  clear(UNDEFINED);
   

}
