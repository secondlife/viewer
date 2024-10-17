# -*- cmake -*-
include(Prebuilt)

include_guard()

add_library( ll::boost INTERFACE IMPORTED )
if( USE_CONAN )
  target_link_libraries( ll::boost INTERFACE CONAN_PKG::boost )
  target_compile_definitions( ll::boost INTERFACE BOOST_ALLOW_DEPRECATED_HEADERS BOOST_BIND_GLOBAL_PLACEHOLDERS )
  return()
endif()

use_prebuilt_binary(boost)

# As of sometime between Boost 1.67 and 1.72, Boost libraries are suffixed
# with the address size.
set(addrsfx "-x${ADDRESS_SIZE}")

if (WINDOWS)
  target_link_libraries( ll::boost INTERFACE
          libboost_context-mt${addrsfx}
          libboost_fiber-mt${addrsfx}
          libboost_filesystem-mt${addrsfx}
          libboost_program_options-mt${addrsfx}
          libboost_regex-mt${addrsfx}
          libboost_system-mt${addrsfx}
          libboost_thread-mt${addrsfx}
          libboost_url-mt${addrsfx})
elseif (LINUX)
  target_link_libraries( ll::boost INTERFACE
          boost_fiber-mt${addrsfx}
          boost_context-mt${addrsfx}
          boost_filesystem-mt${addrsfx}
          boost_program_options-mt${addrsfx}
          boost_regex-mt${addrsfx}
          boost_system-mt${addrsfx}
          boost_thread-mt${addrsfx}
          boost_url-mt${addrsfx})
elseif (DARWIN)
  target_link_libraries( ll::boost INTERFACE
          boost_context-mt${addrsfx}
          boost_fiber-mt${addrsfx}
          boost_filesystem-mt${addrsfx}
          boost_program_options-mt${addrsfx}
          boost_regex-mt${addrsfx}
          boost_system-mt${addrsfx}
          boost_thread-mt${addrsfx}
          boost_url-mt${addrsfx})
endif (WINDOWS)

if (LINUX)
    set(BOOST_SYSTEM_LIBRARY ${BOOST_SYSTEM_LIBRARY} rt)
    set(BOOST_THREAD_LIBRARY ${BOOST_THREAD_LIBRARY} rt)
endif (LINUX)

