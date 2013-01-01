//
//  NSObject_llappviewermacosx_objc.h
//  SecondLife
//
//  Created by Geenz on 12/16/12.
//
//

#include <boost/tr1/functional.hpp>
typedef std::tr1::function<void()> VoidCallback;
typedef void* ViewerAppRef;

int createNSApp(int argc, const char **argv);

bool initViewer();
void handleQuit();
bool runMainLoop();
void initMainLoop();
void cleanupViewer();