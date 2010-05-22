# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  MESSAGE(FATAL_ERROR "*TODO standalone support for google breakad is unimplemented")
  # *TODO - implement this include(FindGoogleBreakpad)
else (STANDALONE)
  use_prebuilt_binary(google_breakpad)
  set(BREAKPAD_EXCEPTION_HANDLER_LIBRARIES exception_handler crash_generation_client common)
  if (LINUX)
    set(BREAKPAD_EXCEPTION_HANDLER_LIBRARIES breakpad_client)
  endif (LINUX)
endif (STANDALONE)

