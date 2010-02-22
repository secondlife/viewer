# -*- cmake -*-
include(Prebuilt)

set(Boost_FIND_QUIETLY ON)
set(Boost_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindBoost)

  set(BOOST_PROGRAM_OPTIONS_LIBRARY boost_program_options-mt)
  set(BOOST_REGEX_LIBRARY boost_regex-mt)
  set(BOOST_SIGNALS_LIBRARY boost_signals-mt)
else (STANDALONE)
  use_prebuilt_binary(boost)
  set(Boost_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)

  if (WINDOWS)
    set(BOOST_VERSION 1_39)
    if (MSVC71)
      set(BOOST_PROGRAM_OPTIONS_LIBRARY 
          optimized libboost_program_options-vc71-mt-s-${BOOST_VERSION}
          debug libboost_program_options-vc71-mt-sgd-${BOOST_VERSION})
      set(BOOST_REGEX_LIBRARY
          optimized libboost_regex-vc71-mt-s-${BOOST_VERSION}
          debug libboost_regex-vc71-mt-sgd-${BOOST_VERSION})
      set(BOOST_SIGNALS_LIBRARY 
          optimized libboost_signals-vc71-mt-s-${BOOST_VERSION}
          debug libboost_signals-vc71-mt-sgd-${BOOST_VERSION})
    else (MSVC71)
      set(BOOST_PROGRAM_OPTIONS_LIBRARY 
          optimized libboost_program_options-vc80-mt-${BOOST_VERSION}
          debug libboost_program_options-vc80-mt-gd-${BOOST_VERSION})
      set(BOOST_REGEX_LIBRARY
          optimized libboost_regex-vc80-mt-${BOOST_VERSION}
          debug libboost_regex-vc80-mt-gd-${BOOST_VERSION})
      set(BOOST_SIGNALS_LIBRARY 
          optimized libboost_signals-vc80-mt-${BOOST_VERSION}
          debug libboost_signals-vc80-mt-gd-${BOOST_VERSION})
    endif (MSVC71)
  elseif (DARWIN)
    set(BOOST_PROGRAM_OPTIONS_LIBRARY boost_program_options-xgcc40-mt)
    set(BOOST_REGEX_LIBRARY boost_regex-xgcc40-mt)
    set(BOOST_SIGNALS_LIBRARY boost_signals-xgcc40-mt)
  elseif (LINUX)
    set(BOOST_PROGRAM_OPTIONS_LIBRARY boost_program_options-gcc41-mt)
    set(BOOST_REGEX_LIBRARY boost_regex-gcc41-mt)
    set(BOOST_SIGNALS_LIBRARY boost_signals-gcc41-mt)
  endif (WINDOWS)
endif (STANDALONE)
