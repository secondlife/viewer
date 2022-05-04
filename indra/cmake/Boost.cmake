# -*- cmake -*-
include(Prebuilt)

set(Boost_FIND_QUIETLY ON)
set(Boost_FIND_REQUIRED ON)

if (USESYSTEMLIBS)
  include(FindBoost)

  set(BOOST_CONTEXT_LIBRARY boost_context-mt)
  set(BOOST_FIBER_LIBRARY boost_fiber-mt)
  set(BOOST_FILESYSTEM_LIBRARY boost_filesystem-mt)
  set(BOOST_PROGRAM_OPTIONS_LIBRARY boost_program_options-mt)
  set(BOOST_REGEX_LIBRARY boost_regex-mt)
  set(BOOST_SIGNALS_LIBRARY boost_signals-mt)
  set(BOOST_SYSTEM_LIBRARY boost_system-mt)
  set(BOOST_THREAD_LIBRARY boost_thread-mt)
else (USESYSTEMLIBS)
  use_prebuilt_binary(boost)
  set(Boost_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)

  # As of sometime between Boost 1.67 and 1.72, Boost libraries are suffixed
  # with the address size.
  set(addrsfx "-x${ADDRESS_SIZE}")

  if (WINDOWS)
    set(BOOST_CONTEXT_LIBRARY
        optimized libboost_context-mt${addrsfx}
        debug libboost_context-mt${addrsfx}-gd)
    set(BOOST_FIBER_LIBRARY
        optimized libboost_fiber-mt${addrsfx}
        debug libboost_fiber-mt${addrsfx}-gd)
    set(BOOST_FILESYSTEM_LIBRARY
        optimized libboost_filesystem-mt${addrsfx}
        debug libboost_filesystem-mt${addrsfx}-gd)
    set(BOOST_PROGRAM_OPTIONS_LIBRARY
        optimized libboost_program_options-mt${addrsfx}
        debug libboost_program_options-mt${addrsfx}-gd)
    set(BOOST_REGEX_LIBRARY
        optimized libboost_regex-mt${addrsfx}
        debug libboost_regex-mt${addrsfx}-gd)
    set(BOOST_SIGNALS_LIBRARY
        optimized libboost_signals-mt${addrsfx}
        debug libboost_signals-mt${addrsfx}-gd)
    set(BOOST_SYSTEM_LIBRARY
        optimized libboost_system-mt${addrsfx}
        debug libboost_system-mt${addrsfx}-gd)
    set(BOOST_THREAD_LIBRARY
        optimized libboost_thread-mt${addrsfx}
        debug libboost_thread-mt${addrsfx}-gd)
  elseif (LINUX)
    set(BOOST_CONTEXT_LIBRARY
        optimized boost_context-mt${addrsfx}
        debug boost_context-mt${addrsfx}-d)
    set(BOOST_FIBER_LIBRARY
        optimized boost_fiber-mt${addrsfx}
        debug boost_fiber-mt${addrsfx}-d)
    set(BOOST_FILESYSTEM_LIBRARY
        optimized boost_filesystem-mt${addrsfx}
        debug boost_filesystem-mt${addrsfx}-d)
    set(BOOST_PROGRAM_OPTIONS_LIBRARY
        optimized boost_program_options-mt${addrsfx}
        debug boost_program_options-mt${addrsfx}-d)
    set(BOOST_REGEX_LIBRARY
        optimized boost_regex-mt${addrsfx}
        debug boost_regex-mt${addrsfx}-d)
    set(BOOST_SIGNALS_LIBRARY
        optimized boost_signals-mt${addrsfx}
        debug boost_signals-mt${addrsfx}-d)
    set(BOOST_SYSTEM_LIBRARY
        optimized boost_system-mt${addrsfx}
        debug boost_system-mt${addrsfx}-d)
    set(BOOST_THREAD_LIBRARY
        optimized boost_thread-mt${addrsfx}
        debug boost_thread-mt${addrsfx}-d)
  elseif (DARWIN)
    set(BOOST_CONTEXT_LIBRARY
        optimized boost_context-mt${addrsfx}
        debug boost_context-mt${addrsfx})
    set(BOOST_FIBER_LIBRARY
        optimized boost_fiber-mt${addrsfx}
        debug boost_fiber-mt${addrsfx})
    set(BOOST_FILESYSTEM_LIBRARY
        optimized boost_filesystem-mt${addrsfx}
        debug boost_filesystem-mt${addrsfx})
    set(BOOST_PROGRAM_OPTIONS_LIBRARY
        optimized boost_program_options-mt${addrsfx}
        debug boost_program_options-mt${addrsfx})
    set(BOOST_REGEX_LIBRARY
        optimized boost_regex-mt${addrsfx}
        debug boost_regex-mt${addrsfx})
    set(BOOST_SIGNALS_LIBRARY
        optimized boost_signals-mt${addrsfx}
        debug boost_signals-mt${addrsfx})
    set(BOOST_SYSTEM_LIBRARY
        optimized boost_system-mt${addrsfx}
        debug boost_system-mt${addrsfx})
    set(BOOST_THREAD_LIBRARY
        optimized boost_thread-mt${addrsfx}
        debug boost_thread-mt${addrsfx})
  endif (WINDOWS)
endif (USESYSTEMLIBS)

if (LINUX)
    set(BOOST_SYSTEM_LIBRARY ${BOOST_SYSTEM_LIBRARY} rt)
    set(BOOST_THREAD_LIBRARY ${BOOST_THREAD_LIBRARY} rt)
endif (LINUX)

