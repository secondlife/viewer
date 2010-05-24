# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  MESSAGE(FATAL_ERROR "*TODO standalone support for google breakad is unimplemented")
  # *TODO - implement this include(FindGoogleBreakpad)
else (STANDALONE)
  use_prebuilt_binary(google_breakpad)
  if (DARWIN)
    set(BREAKPAD_EXCEPTION_HANDLER_LIBRARIES exception_handler)
  endif (DARWIN)
  if (LINUX)
    set(BREAKPAD_EXCEPTION_HANDLER_LIBRARIES breakpad_client)
  endif (LINUX)
  if (WINDOWS)
    set(BREAKPAD_EXCEPTION_HANDLER_LIBRARIES exception_handler crash_generation_client common)
  endif (WINDOWS)
endif (STANDALONE)

