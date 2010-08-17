# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  set(BREAKPAD_EXCEPTION_HANDLER_FIND_REQUIRED ON)
  include(FindGoogleBreakpad)
else (STANDALONE)
  use_prebuilt_binary(google_breakpad)
  set(BREAKPAD_EXCEPTION_HANDLER_LIBRARIES exception_handler)
endif (STANDALONE)

