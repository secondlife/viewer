# -*- cmake -*-
include(Prebuilt)

set(Boost_FIND_QUIETLY ON)
set(Boost_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindBoost)

  set(BOOST_PROGRAM_OPTIONS_LIBRARY boost_program_options-mt)
  set(BOOST_REGEX_LIBRARY boost_regex-mt)
  set(BOOST_SIGNALS_LIBRARY boost_signals-mt)
  set(BOOST_SYSTEM_LIBRARY boost_system-mt)
  set(BOOST_FILESYSTEM_LIBRARY boost_filesystem-mt)
  set(BOOST_THREAD_LIBRARY boost_thread-mt)
else (STANDALONE)
  use_prebuilt_binary(boost)
  set(Boost_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
  set(BOOST_VERSION "1.52")

  if (WINDOWS)
    if(MSVC80)
      set(BOOST_PROGRAM_OPTIONS_LIBRARY 
          optimized libboost_program_options-vc80-mt-${BOOST_VERSION}
          debug libboost_program_options-vc80-mt-gd-${BOOST_VERSION})
      set(BOOST_REGEX_LIBRARY
          optimized libboost_regex-vc80-mt-${BOOST_VERSION}
          debug libboost_regex-vc80-mt-gd-${BOOST_VERSION})
      set(BOOST_SIGNALS_LIBRARY 
          optimized libboost_signals-vc80-mt-${BOOST_VERSION}
          debug libboost_signals-vc80-mt-gd-${BOOST_VERSION})
      set(BOOST_SYSTEM_LIBRARY 
          optimized libboost_system-vc80-mt-${BOOST_VERSION}
          debug libboost_system-vc80-mt-gd-${BOOST_VERSION})
      set(BOOST_FILESYSTEM_LIBRARY 
          optimized libboost_filesystem-vc80-mt-${BOOST_VERSION}
          debug libboost_filesystem-vc80-mt-gd-${BOOST_VERSION})
    else(MSVC80)
      # MSVC 10.0 config
      set(BOOST_PROGRAM_OPTIONS_LIBRARY 
          optimized libboost_program_options-mt
          debug libboost_program_options-mt-gd)
      set(BOOST_REGEX_LIBRARY
          optimized libboost_regex-mt
          debug libboost_regex-mt-gd)
      set(BOOST_SYSTEM_LIBRARY 
          optimized libboost_system-mt
          debug libboost_system-mt-gd)
      set(BOOST_FILESYSTEM_LIBRARY 
          optimized libboost_filesystem-mt
          debug libboost_filesystem-mt-gd)
      set(BOOST_THREAD_LIBRARY 
          optimized libboost_thread-mt
          debug libboost_thread-mt-gd)
    endif (MSVC80)
  elseif (LINUX)
    set(BOOST_PROGRAM_OPTIONS_LIBRARY
        optimized boost_program_options-mt
        debug boost_program_options-mt-d)
    set(BOOST_REGEX_LIBRARY
        optimized boost_regex-mt
        debug boost_regex-mt-d)
    set(BOOST_SYSTEM_LIBRARY
        optimized boost_system-mt
        debug boost_system-mt-d)
    set(BOOST_FILESYSTEM_LIBRARY
        optimized boost_filesystem-mt
        debug boost_filesystem-mt-d)
    set(BOOST_THREAD_LIBRARY
        optimized boost_thread-mt
        debug boost_thread-mt-d)
  elseif (DARWIN)
    set(BOOST_PROGRAM_OPTIONS_LIBRARY
        optimized boost_program_options-mt
        debug boost_program_options-mt-d)
    set(BOOST_PROGRAM_OPTIONS_LIBRARY
        optimized boost_program_options-mt
        debug boost_program_options-mt-d)
    set(BOOST_REGEX_LIBRARY
        optimized boost_regex-mt
        debug boost_regex-mt-d)
    set(BOOST_SYSTEM_LIBRARY
        optimized boost_system-mt
        debug boost_system-mt-d)
    set(BOOST_FILESYSTEM_LIBRARY
        optimized boost_filesystem-mt
        debug boost_filesystem-mt-d)
    set(BOOST_THREAD_LIBRARY
        optimized boost_thread-mt
        debug boost_thread-mt-d)
  endif (WINDOWS)
endif (STANDALONE)
