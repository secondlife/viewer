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
else (STANDALONE)
  use_prebuilt_binary(boost)
  set(Boost_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)

  if (WINDOWS)
    set(BOOST_VERSION 1_45)
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
          optimized libboost_program_options-vc100-mt-${BOOST_VERSION}
          debug libboost_program_options-vc100-mt-gd-${BOOST_VERSION})
      set(BOOST_REGEX_LIBRARY
          optimized libboost_regex-vc100-mt-${BOOST_VERSION}
          debug libboost_regex-vc100-mt-gd-${BOOST_VERSION})
      set(BOOST_SYSTEM_LIBRARY 
          optimized libboost_system-vc100-mt-${BOOST_VERSION}
          debug libboost_system-vc100-mt-gd-${BOOST_VERSION})
      set(BOOST_FILESYSTEM_LIBRARY 
          optimized libboost_filesystem-vc100-mt-${BOOST_VERSION}
          debug libboost_filesystem-vc100-mt-gd-${BOOST_VERSION})    
    endif (MSVC80)
  elseif (DARWIN OR LINUX)
    set(BOOST_PROGRAM_OPTIONS_LIBRARY boost_program_options)
    set(BOOST_REGEX_LIBRARY boost_regex)
    set(BOOST_SYSTEM_LIBRARY boost_system)
    set(BOOST_FILESYSTEM_LIBRARY boost_filesystem)
  endif (WINDOWS)
endif (STANDALONE)
